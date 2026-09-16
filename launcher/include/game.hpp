#pragma once
#include "win_util.hpp"
#include <optional>
#include <string_view>

namespace game {
enum class Kind { GI, SR, ZZZ, WW };
// The launcher owns the static registry. Game-specific initialization remains
// in its module, while this table defines only CLI identity and executable
// validation shared by process discovery and argument parsing.
struct Descriptor {
    Kind kind;
    const wchar_t* flag;
    const wchar_t* primary_executable;
    const wchar_t* alternate_executable{};
    bool requires_explicit_path;
};
inline constexpr Descriptor registry[] = {
    {Kind::GI, L"--GI", L"YuanShen.exe", L"GenshinImpact.exe", true},
    {Kind::SR, L"--SR", L"StarRail.exe", nullptr, true},
    {Kind::ZZZ, L"--ZZZ", L"ZenlessZoneZero.exe", nullptr, false},
    {Kind::WW, L"--WW", L"Client-Win64-Shipping.exe", nullptr, true},
};
inline const Descriptor& descriptor(Kind kind) {
    for(const auto& value:registry)if(value.kind==kind)return value;
    throw std::runtime_error("Unknown game kind");
}
inline const wchar_t* flag(Kind kind) { return descriptor(kind).flag; }
inline std::optional<Kind> parse(std::wstring_view value) {
    for(const auto& value_descriptor:registry)if(value==value_descriptor.flag)return value_descriptor.kind;
    return {};
}
inline bool accepts(Kind kind,const std::filesystem::path& path) {
    const auto name=path.filename().wstring();
    const auto& value=descriptor(kind);
    return _wcsicmp(name.c_str(),value.primary_executable)==0 ||
        (value.alternate_executable&&_wcsicmp(name.c_str(),value.alternate_executable)==0);
}
inline void select(std::optional<Kind>& selected,Kind kind) {
    if(selected)throw std::runtime_error("Choose exactly one of --GI, --SR, --ZZZ, --WW (no duplicates)");
    selected=kind;
}
inline void validate(const std::optional<Kind>& kind,const std::filesystem::path& path,bool explicit_path,bool probe) {
    if(!kind)throw std::runtime_error("Choose exactly one of --GI, --SR, --ZZZ, --WW");
    if(probe&&*kind!=Kind::ZZZ)throw std::runtime_error("--probe and --probe-auto support --ZZZ only");
    if(descriptor(*kind).requires_explicit_path&&!explicit_path)throw std::runtime_error("--GI, --SR and --WW require --game <exe>");
    if(!accepts(*kind,path))throw std::runtime_error("--game executable does not match the selected game");
    if(!std::filesystem::is_regular_file(path))throw std::runtime_error("Game executable not found; specify --game");
}
inline void require_stopped(bool running) {
    if(running)throw std::runtime_error("Selected game is already running. Exit it first; attaching is not supported.");
}
// Own only processes created by this launcher. Never construct from a discovered PID.
class Child {
    bool released_{};
public:
    PROCESS_INFORMATION info{};
    Child()=default;
    Child(const Child&)=delete;Child& operator=(const Child&)=delete;
    ~Child() {
        if(info.hProcess&&!released_){TerminateProcess(info.hProcess,2);WaitForSingleObject(info.hProcess,5000);}
        if(info.hThread)CloseHandle(info.hThread);
        if(info.hProcess)CloseHandle(info.hProcess);
    }
    void start(const std::filesystem::path& path,bool suspended,std::wstring_view arguments={}) {
        if(info.hProcess)throw std::runtime_error("Child already started");
        std::wstring command=L"\""+path.wstring()+L"\"";
        if(!arguments.empty()){command+=L" ";command+=arguments;}
        STARTUPINFOW startup{};startup.cb=sizeof(startup);
        if(!CreateProcessW(path.c_str(),command.data(),nullptr,nullptr,FALSE,suspended?CREATE_SUSPENDED:0,nullptr,
                           path.parent_path().c_str(),&startup,&info))
            throw std::runtime_error("CreateProcess failed, Windows error "+std::to_string(GetLastError()));
    }
    void resume() {
        if(ResumeThread(info.hThread)!=1)throw std::runtime_error("Cannot resume the newly created game main thread");
    }
    void release(){released_=true;}
};
}
