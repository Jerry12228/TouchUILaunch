#pragma once
#include "win_util.hpp"
#include <shellapi.h>
#include <objbase.h>
#include <iostream>
#include <string_view>

namespace elevation {
inline constexpr wchar_t relaunch_flag[]=L"--elevation-relaunch";
inline bool is_elevated() {
    HANDLE raw{};
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&raw))
        throw std::runtime_error("Cannot query elevation token, Windows error "+std::to_string(GetLastError()));
    Handle token(raw);TOKEN_ELEVATION value{};DWORD size{};
    if(!GetTokenInformation(token,TokenElevation,&value,sizeof(value),&size))
        throw std::runtime_error("Cannot read elevation state, Windows error "+std::to_string(GetLastError()));
    return value.TokenIsElevated!=0;
}
inline bool needs_relaunch(bool elevated,bool already_relaunched) {
    if(elevated)return false;
    if(already_relaunched)throw std::runtime_error("Elevation did not grant administrator privileges; not requesting UAC again.");
    return true;
}
// Encode argv for the Windows C runtime, including quotes and trailing backslashes.
// No command shell is involved in the elevated launch.
inline std::wstring quote_argument(std::wstring_view argument) {
    std::wstring result=L"\"";size_t slashes{};
    for(wchar_t c:argument) {
        if(c==L'\\'){++slashes;continue;}
        result.append(c==L'"'?slashes*2+1:slashes,L'\\');slashes=0;
        result+=c;
    }
    result.append(slashes*2,L'\\');result+=L'"';return result;
}
inline std::wstring parameters(const std::vector<std::wstring>& args) {
    std::wstring result;
    for(const auto& arg:args){if(!result.empty())result+=L' ';result+=quote_argument(arg);}
    return result;
}
using Execute=BOOL(WINAPI*)(SHELLEXECUTEINFOW*);
inline DWORD relaunch(const std::wstring& executable,const std::vector<std::wstring>& args,
                      const std::wstring& directory,Execute execute=ShellExecuteExW) {
    struct Apartment {
        HRESULT result=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE);
        ~Apartment(){if(SUCCEEDED(result))CoUninitialize();}
    } apartment;
    if(FAILED(apartment.result)&&apartment.result!=RPC_E_CHANGED_MODE)
        throw std::runtime_error("Cannot initialize the Windows shell apartment");
    const auto command=parameters(args);
    SHELLEXECUTEINFOW info{};info.cbSize=sizeof(info);
    info.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC|SEE_MASK_FLAG_NO_UI|SEE_MASK_NO_CONSOLE;
    info.lpVerb=L"runas";info.lpFile=executable.c_str();info.lpParameters=command.c_str();
    info.lpDirectory=directory.c_str();info.nShow=SW_SHOWNORMAL;
    if(!execute(&info)) {
        const auto error=GetLastError();
        if(error==ERROR_CANCELLED) {
            std::cerr<<"Administrator approval canceled. No game action was performed.\n";
            return ERROR_CANCELLED;
        }
        throw std::runtime_error("Administrator relaunch failed, Windows error "+std::to_string(error));
    }
    Handle child(info.hProcess);
    if(!child.value)throw std::runtime_error("Administrator relaunch returned no process handle");
    if(WaitForSingleObject(child,INFINITE)!=WAIT_OBJECT_0)
        throw std::runtime_error("Cannot wait for elevated launcher, Windows error "+std::to_string(GetLastError()));
    DWORD exit_code{};
    if(!GetExitCodeProcess(child,&exit_code))
        throw std::runtime_error("Cannot read elevated launcher exit code, Windows error "+std::to_string(GetLastError()));
    return exit_code;
}
}
