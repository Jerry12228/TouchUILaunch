#pragma once
#include "win_util.hpp"
#include <optional>
#include <string_view>

namespace game {
enum class Kind { GI, SR, ZZZ, WW };
inline const wchar_t* flag(Kind kind) {
    switch(kind){case Kind::GI:return L"--GI";case Kind::SR:return L"--SR";case Kind::WW:return L"--WW";default:return L"--ZZZ";}
}
inline std::optional<Kind> parse(std::wstring_view value) {
    for(auto kind:{Kind::GI,Kind::SR,Kind::ZZZ,Kind::WW})if(value==flag(kind))return kind;
    return {};
}
inline bool accepts(Kind kind,const std::filesystem::path& path) {
    const auto name=path.filename().wstring();
    switch(kind) {
    case Kind::GI:return _wcsicmp(name.c_str(),L"YuanShen.exe")==0||_wcsicmp(name.c_str(),L"GenshinImpact.exe")==0;
    case Kind::SR:return _wcsicmp(name.c_str(),L"StarRail.exe")==0;
    case Kind::WW:return _wcsicmp(name.c_str(),L"Client-Win64-Shipping.exe")==0;
    default:return _wcsicmp(name.c_str(),L"ZenlessZoneZero.exe")==0;
    }
}
inline void select(std::optional<Kind>& selected,Kind kind) {
    if(selected)throw std::runtime_error("Choose exactly one of --GI, --SR, --ZZZ, --WW (no duplicates)");
    selected=kind;
}
inline void validate(const std::optional<Kind>& kind,const std::filesystem::path& path,bool explicit_path,bool probe) {
    if(!kind)throw std::runtime_error("Choose exactly one of --GI, --SR, --ZZZ, --WW");
    if(probe&&*kind!=Kind::ZZZ)throw std::runtime_error("--probe and --probe-auto support --ZZZ only");
    if(*kind!=Kind::ZZZ&&!explicit_path)throw std::runtime_error("--GI, --SR and --WW require --game <exe>");
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
