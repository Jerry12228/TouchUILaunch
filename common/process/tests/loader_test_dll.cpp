#include "win_util.hpp"

BOOL WINAPI DllMain(HINSTANCE,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
#if defined(TOUCHUI_LOADER_REJECT)
        SetLastError(ERROR_BAD_ENVIRONMENT);
        return FALSE;
#elif defined(TOUCHUI_LOADER_SLOW)
        Sleep(250);
        const auto name=L"Local\\LoaderTests.SlowDone."+std::to_wstring(GetCurrentProcessId());
        Handle done(OpenEventW(EVENT_MODIFY_STATE,FALSE,name.c_str()));
        if(done.value)SetEvent(done);
#endif
    }
    return TRUE;
}
