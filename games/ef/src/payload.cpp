#include "payload_entry.hpp"
#include "ef_launch.hpp"
#include "ef_hook.hpp"
#include "ef_pointer.hpp"
#include <fstream>
#include <memory>
#include <mutex>

namespace touchui::ef {
namespace {
enum class Phase {Waiting,Ready,Failed};
std::atomic<Phase> phase{Phase::Waiting};
std::atomic_bool prepared{};
std::atomic<HWND> window{};
std::atomic<DWORD> window_thread{};
std::atomic<UINT_PTR> timer{};
Handle ready,failed,logging;
std::unique_ptr<Api> api;
std::unique_ptr<Layout> layout;
ChangeHook change_hook;
PointerHooks pointer_hooks;
bool pointer_ready{},pointer_unavailable{};
bool hooked{}; // owned by the timer's game window thread
std::ofstream logfile;
std::mutex log_mutex;
uint64_t started_at{};

void log(std::string_view text) {
    std::lock_guard lock(log_mutex);
    if(logfile.is_open())logfile<<"[+"<<(GetTickCount64()-started_at)<<" ms] "<<text<<"\n"<<std::flush;
}
void fail(std::string_view reason)noexcept {
    change_hook.deactivate();
    pointer_hooks.deactivate();
    phase.store(Phase::Failed,std::memory_order_release);
    if(failed.value)SetEvent(failed);
    try {log(std::string("ERROR: ")+std::string(reason));}catch(...){}
}
void tick() {
    const auto status=layout->prepare();
    if(status==Tick::WaitingThread||!layout->contract()) {
        static auto previous=Tick::Retry;
        if(status!=previous) {
            previous=status;log(status==Tick::WaitingThread?
                "Waiting for attached IL2CPP game window thread":"Waiting for Common.Beyond metadata");
        }
        return;
    }
    if(!hooked) {
        change_hook.install(*layout->contract());hooked=true;
        if(phase.load(std::memory_order_acquire)==Phase::Failed){change_hook.deactivate();return;}
        log("Resolved DeviceInfo contract and installed input-switch hook; Touch="+
            std::to_string(layout->contract()->touch));
    }
    if(!pointer_ready&&!pointer_unavailable) {
        try {
            pointer_ready=pointer_hooks.install(*api,[](const char* text){log(text);});
        }catch(const std::exception& error) {
            pointer_unavailable=true;pointer_hooks.deactivate();
            log(std::string("UI pointer routing unavailable; layout selection continues: ")+error.what());
        }
    }
    const auto result=layout->pump(GetTickCount64());
    if(result==Tick::Deferred)return;
    if(result==Tick::Applied) {
        if(phase.load(std::memory_order_acquire)==Phase::Failed)return;
        change_hook.activate();
        if(phase.load(std::memory_order_acquire)==Phase::Failed){change_hook.deactivate();return;}
        auto expected=Phase::Waiting;
        if(phase.compare_exchange_strong(expected,Phase::Ready,std::memory_order_acq_rel)) {
            log("READY: DeviceInfo.inputType read back as Touch");
            require(SetEvent(ready)!=FALSE,"EF: cannot publish touch UI readiness");
        }
    } else {
        static auto previous=Tick::Applied;
        static uint64_t next_report{};
        const auto now=GetTickCount64();
        if(result!=previous||now>=next_report) {
            previous=result;
            next_report=now+5000;
            log(result==Tick::ManagedException?"ChangeInputType raised a managed exception; retrying":
                result==Tick::WaitingDevice?"Waiting for DeviceInfo provider and UI notification object":
                "Touch input state not confirmed; retrying");
            const auto& observed=layout->observation();
            log("Input state: current="+std::to_string(observed.current)+
                ", target="+std::to_string(layout->contract()->touch)+
                ", provider="+(observed.provider?"ready":"pending")+
                ", notification="+(observed.notification?"ready":"pending")+
                ", switch_attempts="+std::to_string(observed.attempts));
        }
    }
}
// Keep SEH outside functions with C++ automatic objects. No exception may cross
// a USER32 timer callback, and no managed work is attempted by our loader thread.
void guarded_tick() {
    __try {tick();}
    __except(EXCEPTION_EXECUTE_HANDLER){fail("native memory fault in EF UI initialization; restart the game");}
}
void CALLBACK on_timer(HWND hwnd,UINT,UINT_PTR id,DWORD) {
    if(!prepared.load(std::memory_order_acquire)||phase.load(std::memory_order_acquire)==Phase::Failed||
        hwnd!=window.load(std::memory_order_acquire)||id!=timer.load(std::memory_order_acquire)||
        GetCurrentThreadId()!=window_thread.load(std::memory_order_acquire))return;
    try {guarded_tick();}
    catch(const std::exception& error){fail(error.what());}
    catch(...){fail("unexpected EF UI initialization failure");}
}
BOOL CALLBACK find_window(HWND hwnd,LPARAM value) {
    DWORD pid{};GetWindowThreadProcessId(hwnd,&pid);
    if(pid!=GetCurrentProcessId()||GetWindow(hwnd,GW_OWNER))return TRUE;
    wchar_t name[128]{};GetClassNameW(hwnd,name,128);
    if(std::wcscmp(name,L"UnityWndClass")!=0)return TRUE;
    RECT rect{};
    if(!GetClientRect(hwnd,&rect)||rect.right<320||rect.bottom<200)return TRUE;
    *reinterpret_cast<HWND*>(value)=hwnd;return FALSE;
}
}

DWORD run_payload(HMODULE module) {
    try {
        started_at=GetTickCount64();
        const auto pid=GetCurrentProcessId();
        ready.value=OpenEventW(EVENT_MODIFY_STATE,FALSE,event_name(pid,L"Ready").c_str());
        failed.value=OpenEventW(EVENT_MODIFY_STATE,FALSE,event_name(pid,L"Failed").c_str());
        logging.value=OpenEventW(SYNCHRONIZE,FALSE,event_name(pid,L"Log").c_str());
        require(ready.value&&failed.value&&logging.value,"EF: incomplete owned launch session");
        require(_wcsicmp(std::filesystem::path(module_path()).filename().c_str(),L"Endfield.exe")==0,
            "EF: payload activation requires the owned Endfield.exe process");
        if(WaitForSingleObject(logging,0)==WAIT_OBJECT_0) {
            const auto root=std::filesystem::path(module_path(module)).parent_path()/L"logs";
            std::filesystem::create_directories(root);
            logfile.open(root/(L"ef-"+std::to_wstring(pid)+L".log"));
        }
        HMODULE pinned{};
        require(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&run_payload),&pinned)!=FALSE,"EF: cannot retain payload callbacks");
        log("EF touch UI only: game state switch on attached window thread; no input conversion or platform spoofing");
        const auto deadline=GetTickCount64()+120000;
        HMODULE assembly{};
        while(!assembly&&GetTickCount64()<deadline) {
            assembly=GetModuleHandleW(L"GameAssembly.dll");if(!assembly)Sleep(100);
        }
        require(assembly!=nullptr,"EF: GameAssembly did not load");
        log("GameAssembly observed; preparing runtime contracts");
        require(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(assembly),&pinned)!=FALSE,"EF: cannot retain GameAssembly");
        api=std::make_unique<Api>(Api::bind(assembly));
        layout=std::make_unique<Layout>(*api);
        prepared.store(true,std::memory_order_release);
        const auto ui_deadline=GetTickCount64()+120000;
        while(phase.load(std::memory_order_acquire)!=Phase::Failed) {
            const auto hwnd=window.load(std::memory_order_acquire);
            if(!hwnd||!IsWindow(hwnd)) {
                HWND candidate{};EnumWindows(find_window,reinterpret_cast<LPARAM>(&candidate));
                if(candidate) {
                    const auto thread=GetWindowThreadProcessId(candidate,nullptr);
                    window_thread.store(thread,std::memory_order_release);
                    window.store(candidate,std::memory_order_release);
                    const auto id=SetTimer(candidate,reinterpret_cast<UINT_PTR>(&on_timer),250,on_timer);
                    require(id!=0,"EF: cannot schedule the game window-thread layout pump");
                    timer.store(id,std::memory_order_release);
                    log("Game window-thread pump scheduled; thread="+std::to_string(thread));
                }
            }
            if(phase.load(std::memory_order_acquire)==Phase::Waiting&&GetTickCount64()>=ui_deadline) {
                fail("Touch UI initialization exceeded 120 s after GameAssembly loaded");break;
            }
            Sleep(100);
        }
        const auto hwnd=window.load();const auto id=timer.exchange(0);
        if(hwnd&&id)KillTimer(hwnd,id);
        return ERROR_DLL_INIT_FAILED;
    }catch(const std::exception& error){fail(error.what());}
    catch(...){fail("unexpected EF payload failure");}
    return ERROR_DLL_INIT_FAILED;
}
}
