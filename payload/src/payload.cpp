#include "payload_entry.hpp"
#include "nte_launch.hpp"
#include "ef_launch.hpp"

namespace {
DWORD WINAPI initialize(void* context) {
    try {
        const auto pid=GetCurrentProcessId();
        Handle zzz_logging(OpenEventW(SYNCHRONIZE,FALSE,log_event_name(pid).c_str()));
        Handle nte_ready(OpenEventW(SYNCHRONIZE,FALSE,touchui::nte::event_name(pid,L"Ready").c_str()));
        Handle nte_failed(OpenEventW(EVENT_MODIFY_STATE,FALSE,touchui::nte::event_name(pid,L"Failed").c_str()));
        Handle nte_config(OpenFileMappingW(FILE_MAP_READ,FALSE,touchui::nte::event_name(pid,L"Config").c_str()));
        Handle ef_logging(OpenEventW(SYNCHRONIZE,FALSE,touchui::ef::event_name(pid,L"Log").c_str()));
        Handle ef_ready(OpenEventW(SYNCHRONIZE,FALSE,touchui::ef::event_name(pid,L"Ready").c_str()));
        Handle ef_failed(OpenEventW(EVENT_MODIFY_STATE,FALSE,touchui::ef::event_name(pid,L"Failed").c_str()));
        const auto kind=touchui::payload::select(zzz_logging.value!=nullptr,nte_ready.value!=nullptr,
            nte_failed.value!=nullptr,nte_config.value!=nullptr,
            ef_logging.value!=nullptr,ef_ready.value!=nullptr,ef_failed.value!=nullptr);
        const auto module=static_cast<HMODULE>(context);
        if(kind==touchui::payload::Kind::ZZZ)return touchui::zzz::run_payload(module);
        if(kind==touchui::payload::Kind::NTE)return touchui::nte::run_payload(module);
        if(kind==touchui::payload::Kind::EF)return touchui::ef::run_payload(module);
        if(ef_failed.value)SetEvent(ef_failed);
        if(nte_failed.value)SetEvent(nte_failed);
        Handle boot_failed(OpenEventW(EVENT_MODIFY_STATE,FALSE,touchui::nte::event_name(pid,L"BootFailed").c_str()));
        if(boot_failed.value)SetEvent(boot_failed);
    } catch(...) {return ERROR_DLL_INIT_FAILED;}
    return ERROR_INVALID_DATA;
}
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
        // Keep thread notifications available for the static CRT/TLS used by
        // the ZZZ bridge. All selection and game initialization run off-lock.
        Handle worker(CreateThread(nullptr,0,initialize,module,0,nullptr));
        return worker.value!=nullptr;
    }
    // Installed game callbacks keep the module resident until process exit.
    return TRUE;
}
