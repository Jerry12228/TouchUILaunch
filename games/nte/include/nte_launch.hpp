#pragma once
#include "win_util.hpp"
#include "payload_entry.hpp"
#include <string_view>

namespace touchui::nte {
inline constexpr auto payload_filename=payload::filename;
// International launcher command observed in its successful 1.5.5 run.
// Updater_P performs the normal startup flow; Global selects Saved_Global.
inline constexpr std::wstring_view direct_defaults=
    L"/Game/LoginAndCreate/Map/Updater/Updater_P --saveddirsuffix=Global -SAVEWINPOS=1";
inline std::wstring direct_arguments(std::wstring_view extra) {
    std::wstring result(direct_defaults);
    if(!extra.empty()){result+=L" ";result+=extra;}
    return result;
}
inline std::wstring event_name(DWORD pid,std::wstring_view state) {
    return L"Local\\TouchUILaunch.NTE."+std::wstring(state)+L"."+std::to_wstring(pid);
}
inline bool window_ready(DWORD pid) {
    struct Search {DWORD pid;bool found{};} search{pid};
    EnumWindows([](HWND window,LPARAM parameter)->BOOL {
        auto& result=*reinterpret_cast<Search*>(parameter);DWORD owner{};
        GetWindowThreadProcessId(window,&owner);
        if(owner!=result.pid||!IsWindowVisible(window))return TRUE;
        wchar_t name[128]{};
        if(GetClassNameW(window,name,128)&&std::wstring_view(name)==L"UnrealWindow")result.found=true;
        return !result.found;
    },reinterpret_cast<LPARAM>(&search));
    return search.found;
}
}
