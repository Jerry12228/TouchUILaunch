#pragma once
#include "nte_launch.hpp"
#include "load_library_diagnostic.hpp"
#include <memory>
#include <cstring>
#include <algorithm>

namespace touchui::nte::relay {
inline constexpr DWORD protocol=0x4e544503;
inline constexpr size_t path_capacity=4096;
enum class Role:DWORD {Bridge=1,Game=2};
enum State:LONG {Waiting=0,Mapped=1,Ready=2,Failed=3};
struct Shared {
    DWORD magic{},controller_pid{},logging{};
    volatile LONG active{},state{},game_pid{},failure_writer{};
    alignas(8) volatile LONG64 game_handle{}; // Handle duplicated into controller, never a discovered PID.
    wchar_t game[path_capacity]{},root[path_capacity]{},payload[path_capacity]{},login_host[path_capacity]{};
    char error[1024]{};
};
struct Config {DWORD magic{};Role role{};wchar_t session[192]{};};
inline LONG read(volatile LONG& value){return InterlockedCompareExchange(&value,0,0);}
template<size_t N> void copy(wchar_t (&target)[N],std::wstring_view value) {
    loader::require(value.size()<N,"NTE relay path/name exceeds supported length");
    std::copy(value.begin(),value.end(),target);target[value.size()]=0;
}
template<class T> class Mapping {
    Handle handle_;T* view_{};
public:
    Mapping(const std::wstring& name,bool create):handle_(create?
        CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(T),name.c_str()):
        OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,name.c_str())) {
        const auto error=GetLastError();
        loader::require(handle_.value&&(!create||error!=ERROR_ALREADY_EXISTS),"Cannot open exclusive NTE relay mapping");
        view_=static_cast<T*>(MapViewOfFile(handle_,FILE_MAP_ALL_ACCESS,0,0,sizeof(T)));
        loader::require(view_!=nullptr,"Cannot map NTE relay configuration");
    }
    ~Mapping(){if(view_)UnmapViewOfFile(view_);}
    T* operator->(){return view_;}T& data(){return *view_;}
};
inline HANDLE exclusive_event(const std::wstring& name,bool signaled=false) {
    const auto event=CreateEventW(nullptr,TRUE,signaled,name.c_str());const auto error=GetLastError();
    if(!event||error==ERROR_ALREADY_EXISTS) {
        if(event)CloseHandle(event);throw std::runtime_error("Cannot create exclusive NTE relay event");
    }
    return event;
}
class Session {
public:
    const std::wstring name;
    Mapping<Shared> mapping;
    Handle changed,controller;
    Session(std::wstring key,bool create):name(std::move(key)),mapping(name,create),
        changed(create?exclusive_event(name+L".Changed"):OpenEventW(EVENT_MODIFY_STATE|SYNCHRONIZE,FALSE,(name+L".Changed").c_str())) {
        loader::require(changed.value!=nullptr,"NTE relay event unavailable");
        if(create){mapping->magic=protocol;mapping->controller_pid=GetCurrentProcessId();}
        loader::require(mapping->magic==protocol,"NTE relay protocol mismatch");
        controller.value=OpenProcess(SYNCHRONIZE,FALSE,mapping->controller_pid);
        loader::require(controller.value!=nullptr,"NTE relay controller unavailable");
    }
    bool alive(){return read(mapping->active)!=0&&WaitForSingleObject(controller,0)==WAIT_TIMEOUT;}
    void notify(){SetEvent(changed);}
    void fail(const std::string& message) {
        if(read(mapping->state)==Ready||InterlockedCompareExchange(&mapping->failure_writer,1,0)!=0)return;
        const auto size=std::min(message.size(),sizeof(mapping->error)-1);
        std::memcpy(mapping->error,message.data(),size);mapping->error[size]=0;
        InterlockedExchange(&mapping->active,0);InterlockedExchange(&mapping->state,Failed);notify();
    }
};
inline bool same_path(const std::filesystem::path& a,const std::filesystem::path& b) {
    return _wcsicmp(a.lexically_normal().c_str(),b.lexically_normal().c_str())==0;
}
inline bool within(const std::filesystem::path& file,const std::filesystem::path& root) {
    const auto left=file.lexically_normal(),right=root.lexically_normal();
    auto a=left.begin(),b=right.begin();
    for(;b!=right.end();++a,++b)if(a==left.end()||_wcsicmp(a->c_str(),b->c_str())!=0)return false;
    return a!=left.end();
}
inline bool launcher_name(const std::filesystem::path& path) {
    const auto name=path.filename().wstring();
    return _wcsicmp(name.c_str(),L"NTEGlobalLauncher.exe")==0||_wcsicmp(name.c_str(),L"NTEGlobalGame.exe")==0||
        _wcsicmp(name.c_str(),L"NTEGame.exe")==0;
}
inline std::filesystem::path image_path(HANDLE process) {
    wchar_t text[32768]{};DWORD size=32768;
    loader::require(QueryFullProcessImageNameW(process,0,text,&size)!=FALSE,"Cannot query newly created NTE child path");
    return std::wstring(text,size);
}
inline std::optional<Role> classify(const Shared& config,const std::filesystem::path& path) {
    if(same_path(path,config.game))return Role::Game;
    if(config.login_host[0]&&same_path(path,config.login_host))return Role::Bridge;
    if(launcher_name(path)&&within(path,config.root))return Role::Bridge;
    return {};
}
class Startup {
public:
    Mapping<Config> config;
    Handle boot_ready,boot_failed,ui_ready,ui_failed,logging;
    Startup(DWORD pid,const Session& session,Role role):config(event_name(pid,L"Config"),true),
        boot_ready(exclusive_event(event_name(pid,L"BootReady"))),boot_failed(exclusive_event(event_name(pid,L"BootFailed"))),
        ui_ready(exclusive_event(event_name(pid,L"Ready"))),ui_failed(exclusive_event(event_name(pid,L"Failed"))),
        logging(exclusive_event(event_name(pid,L"Log"))) {
        config->magic=protocol;config->role=role;copy(config->session,session.name);
    }
    void load(HANDLE process,const std::filesystem::path& payload) {
        const auto result=loader::load_direct(process,payload);
        loader::require(result.module.base!=0,"NTE relay DLL mapping unavailable");
        const HANDLE states[]={boot_ready.value,boot_failed.value,process};
        loader::require(WaitForMultipleObjects(3,states,FALSE,30000)==WAIT_OBJECT_0,
            "NTE relay DLL mapped but startup handshake failed");
    }
};
struct Activation {bool configured{},bridge{};std::unique_ptr<Session> session;};
// Defined only in the payload. The launcher uses the protocol/Startup classes.
Activation activate(HMODULE self);
}
