#pragma once
#include "win_util.hpp"
#include <string_view>

namespace touchui::ef {
inline std::wstring event_name(DWORD pid,std::wstring_view purpose) {
    return L"Local\\TouchUILaunch.EF."+std::wstring(purpose)+L"."+std::to_wstring(pid);
}
inline HANDLE create_event(DWORD pid,std::wstring_view purpose,bool signaled=false) {
    SetLastError(ERROR_SUCCESS);
    HANDLE event=CreateEventW(nullptr,TRUE,signaled,event_name(pid,purpose).c_str());
    if(!event)throw std::runtime_error("EF: cannot create launch coordination event");
    if(GetLastError()==ERROR_ALREADY_EXISTS) {
        CloseHandle(event);throw std::runtime_error("EF: launch coordination event already exists");
    }
    return event;
}
inline void wait_ready(HANDLE process,HANDLE ready,HANDLE failed,DWORD timeout=240000) {
    const HANDLE handles[]={failed,process,ready};
    const auto result=WaitForMultipleObjects(3,handles,FALSE,timeout);
    if(result==WAIT_OBJECT_0+2)return;
    if(result==WAIT_OBJECT_0)throw std::runtime_error("EF: touch UI initialization failed; use --log for details");
    if(result==WAIT_OBJECT_0+1)throw std::runtime_error("EF: game exited before touch UI was applied");
    if(result==WAIT_TIMEOUT)throw std::runtime_error("EF: touch UI was not confirmed within "+std::to_string(timeout/1000)+" s; use --log for details");
    throw std::runtime_error("EF: cannot wait for touch UI initialization, Windows error "+std::to_string(GetLastError()));
}
}
