#include "profile.hpp"
#include "touch_state.hpp"
#include "win_util.hpp"
#include <atomic>
#include <cstdio>
#include <cstdarg>
#include <mutex>
#include <unordered_set>
#include <windowsx.h>

namespace {
HMODULE self_module{};
uintptr_t assembly{};
std::atomic<bool> enabled{false}, hooks_ready{false}, ui_fault{false};
std::atomic<HWND> game_window{};
std::atomic<DWORD> window_thread{};
std::atomic<WNDPROC> previous_proc{};
std::atomic<UINT_PTR> timer_id{};
constexpr UINT_PTR requested_timer=0x5a5a7455;
HANDLE enable_event{};
HANDLE ready_event{};
bool touch_registration_added{};
FILE* logfile{};
std::mutex log_mutex;
void log(const char* format,...) {
    std::lock_guard lock(log_mutex);
    if(!logfile)return;
    SYSTEMTIME t{};GetLocalTime(&t);
    fprintf(logfile,"%02u:%02u:%02u.%03u ",t.wHour,t.wMinute,t.wSecond,t.wMilliseconds);
    va_list ap;va_start(ap,format);vfprintf(logfile,format,ap);va_end(ap);
    fputc('\n',logfile);fflush(logfile);
}
using IntGetter=int(*)();
using BoolGetter=bool(*)();
using TouchGetter=void(*)(int,touch::UnityTouch*);
IntGetter original_count{},get_frame{},get_width{},get_height{};
BoolGetter original_supported{};
TouchGetter original_touch{};
SRWLOCK input_lock=SRWLOCK_INIT;
struct InputGuard {
    InputGuard(){AcquireSRWLockExclusive(&input_lock);}
    ~InputGuard(){ReleaseSRWLockExclusive(&input_lock);}
};
touch::State input;
touch::Snapshot current_snapshot;
std::unordered_set<uint64_t> active_keys;
int event_source{};
ULONGLONG last_source_time{},last_frame_time{};
int64_t snapshot_frame=-1;
int echo_frames{};
bool snapshot_is_bridge{};
std::atomic<uint64_t> touch_messages{},pointer_messages{},count_calls{},bridge_frames{};
std::atomic<int> last_native_count{},last_bridge_count{},effective_layout{-1};

template<class T> bool read_at(uintptr_t address,T& result) {
    SIZE_T count{};
    return address&&ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),&result,sizeof(result),&count)&&count==sizeof(result);
}
void* slot_value(uintptr_t rva) {
    void* value{};read_at(assembly+rva,value);return value;
}
bool exchange_slot(uintptr_t rva,void* expected,void* replacement) {
    auto slot=reinterpret_cast<void* volatile*>(assembly+rva);
    DWORD old{};
    if(!VirtualProtect(const_cast<void**>(slot),sizeof(void*),PAGE_READWRITE,&old))return false;
    bool changed=InterlockedCompareExchangePointer(slot,replacement,expected)==expected;
    DWORD unused{};
    if(!VirtualProtect(const_cast<void**>(slot),sizeof(void*),old,&unused))log("ERROR: restoring slot protection failed: %lu",GetLastError());
    return changed;
}

// UI calls run on the game's window thread, after its static property exists.
// Readiness reads are non-throwing; SEH handles only native memory faults.
uintptr_t changed_provider{};
int saved_override{};
ULONGLONG last_ui_check{};
bool in_ui_call{};
void cancel_contacts();
uintptr_t ready_ui_provider() {
    uintptr_t klass{},pool{},provider{},property{},default_property{},property_class{};
    unsigned char initialized{};
    if(!read_at(assembly+profile::ui_class_slot,klass)||!read_at(klass+profile::class_initialized_offset,initialized)||!initialized)return 0;
    if(!read_at(assembly+profile::static_reference_pool,pool)||!read_at(pool+profile::ui_state_offset,provider))return 0;
    if(!read_at(provider+profile::override_property_offset,property)||!read_at(property,property_class)||!property_class)return 0;
    if(!read_at(provider+profile::default_property_offset,default_property)||!read_at(default_property,property_class)||!property_class)return 0;
    return provider;
}
int memory_fault_filter(DWORD code) {
    return code==EXCEPTION_ACCESS_VIOLATION||code==EXCEPTION_IN_PAGE_ERROR?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH;
}
bool call_layout_get(uintptr_t rva,int* value) {
    __try { *value=reinterpret_cast<int(*)(void*)>(assembly+rva)(nullptr);return true; }
    __except(memory_fault_filter(GetExceptionCode())) {return false;}
}
bool call_layout_set(int value) {
    __try {reinterpret_cast<void(*)(int,void*,void*)>(assembly+profile::set_layout_override)(value,nullptr,nullptr);return true;}
    __except(memory_fault_filter(GetExceptionCode())) {return false;}
}
void maintain_ui() {
    if(GetCurrentThreadId()!=window_thread.load()||!hooks_ready.load()||in_ui_call)return;
    const auto now=GetTickCount64();
    if(now-last_ui_check<500)return;
    last_ui_check=now;
    auto hwnd=game_window.load();
    if(hwnd) {
        if(enabled.load()&&!touch_registration_added&&!IsTouchWindow(hwnd,nullptr)) {
            touch_registration_added=RegisterTouchWindow(hwnd,0)!=FALSE;
            log("Native touch registration added=%d",touch_registration_added);
        } else if(!enabled.load()&&touch_registration_added) {
            if(UnregisterTouchWindow(hwnd))touch_registration_added=false;
        }
    }
    if(ui_fault.load())return;
    auto provider=ready_ui_provider();
    if(!provider)return;
    in_ui_call=true;
    int before{},after{};
    bool ok=call_layout_get(profile::get_layout_override,&before);
    if(ok&&enabled.load()) {
        if(provider!=changed_provider)changed_provider=0;
        if(before!=1) {
            saved_override=before;
            // Mark ownership before calling: synchronous property notifications can reenter input.
            changed_provider=provider;
            ok=call_layout_set(1);
            if(ok)log("UI override requested: %d -> Mobile(1), provider=%p",before,reinterpret_cast<void*>(provider));
        }
    } else if(ok&&changed_provider) {
        if(provider==changed_provider&&before==1) {
            ok=call_layout_set(saved_override);
            if(ok)log("UI override restored to %d",saved_override);
        }
        changed_provider=0;
    }
    if(ok)ok=call_layout_get(profile::get_effective_layout,&after);
    if(ok&&effective_layout.exchange(after)!=after)log("Effective UI layout=%d (1=Mobile, 2=PC)",after);
    if(!ok) {
        ui_fault=true;enabled=false;ResetEvent(enable_event);ResetEvent(ready_event);cancel_contacts();
        log("ERROR: native memory fault during UI call; UI calls disabled for this process. Restart game before retry.");
    }
    in_ui_call=false;
}

void prepare_snapshot() {
    const int64_t frame=get_frame();
    if(snapshot_frame==frame)return;
    const int width=get_width(),height=get_height();
    if(width<=0||height<=0)return;
    const auto now=GetTickCount64();
    const float dt=last_frame_time?std::clamp(static_cast<float>(now-last_frame_time)/1000.f,.000001f,.25f):1.f/60;
    last_frame_time=now;snapshot_frame=frame;
    current_snapshot=input.begin_frame(frame,width,height,dt);
    if(current_snapshot.count)echo_frames=2;
    snapshot_is_bridge=current_snapshot.count>0||echo_frames>0;
    if(!current_snapshot.count&&echo_frames>0)--echo_frames;
    if(snapshot_is_bridge)++bridge_frames;
    last_bridge_count=current_snapshot.count;
}
int hooked_count() {
    ++count_calls;
    // Never mix indices from the Windows snapshot and Unity's native touch array.
    {InputGuard guard;prepare_snapshot();
    if(snapshot_is_bridge)return current_snapshot.count;}
    int value=original_count();last_native_count=value;return value;
}
void hooked_touch(int index,touch::UnityTouch* result) {
    {
        InputGuard guard;prepare_snapshot();
        if(snapshot_is_bridge) {
            if(result){*result={};result->phase=touch::Canceled;result->finger_id=-1;
                if(index>=0&&index<current_snapshot.count)*result=current_snapshot.touches[index];}
            return;
        }
    }
    original_touch(index,result);
}
bool hooked_supported() {
    if(enabled.load())return true;
    {InputGuard guard;if(input.has_contacts())return true;}
    return original_supported();
}

touch::Vec2 normalize(HWND hwnd,POINT point) {
    ScreenToClient(hwnd,&point);RECT rect{};GetClientRect(hwnd,&rect);
    return {static_cast<float>(point.x)/std::max<LONG>(1,rect.right),static_cast<float>(point.y)/std::max<LONG>(1,rect.bottom)};
}
void record_input(int source,uint32_t id,touch::Vec2 position,bool down,bool up,bool canceled) {
    if(!enabled.load())return;
    InputGuard guard;
    const auto now=GetTickCount64();
    if(source!=event_source) {
        if(!down||!active_keys.empty()||now-last_source_time<250)return;
        event_source=source;
    }
    last_source_time=now;
    const uint64_t key=(static_cast<uint64_t>(source)<<32)|id;
    if(down){active_keys.insert(key);input.down(key,position);}
    else input.move(key,position);
    if(up||canceled){input.up(key,position,canceled);active_keys.erase(key);}
}
void cancel_contacts() {InputGuard guard;input.cancel_all();active_keys.clear();}
LRESULT CALLBACK window_proc(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam) {
    if(message==WM_TOUCH) {
        ++touch_messages;
        const UINT count=LOWORD(wparam);
        std::array<TOUCHINPUT,256> touches{};
        if(enabled.load()&&count&&count<=touches.size()&&GetTouchInputInfo(reinterpret_cast<HTOUCHINPUT>(lparam),count,touches.data(),sizeof(TOUCHINPUT))) {
            for(UINT i=0;i<count;++i){auto& t=touches[i];POINT p{TOUCH_COORD_TO_PIXEL(t.x),TOUCH_COORD_TO_PIXEL(t.y)};
                record_input(1,t.dwID,normalize(hwnd,p),(t.dwFlags&TOUCHEVENTF_DOWN)!=0,(t.dwFlags&TOUCHEVENTF_UP)!=0,false);}
        }
        // The original procedure retains ownership of HTOUCHINPUT and closes it.
    } else if(message==WM_POINTERDOWN||message==WM_POINTERUPDATE||message==WM_POINTERUP) {
        ++pointer_messages;
        POINTER_INFO info{};
        if(enabled.load()&&GetPointerInfo(GET_POINTERID_WPARAM(wparam),&info)&&info.pointerType==PT_TOUCH)
            record_input(2,info.pointerId,normalize(hwnd,info.ptPixelLocation),message==WM_POINTERDOWN,message==WM_POINTERUP,(info.pointerFlags&POINTER_FLAG_CANCELED)!=0);
    } else if(message==WM_KILLFOCUS||message==WM_CANCELMODE||message==WM_POINTERCAPTURECHANGED) {
        cancel_contacts();
    } else if(message==WM_TIMER&&wparam==timer_id.load()) {
        maintain_ui();return 0;
    } else if(message==WM_NCDESTROY) {
        cancel_contacts();enabled=false;ResetEvent(enable_event);ResetEvent(ready_event);
        KillTimer(hwnd,timer_id.load());game_window=nullptr;
        log("Game window destroyed; bridge disabled. Restart game to attach again.");
    }
    return CallWindowProcW(previous_proc.load(),hwnd,message,wparam,lparam);
}
struct WindowSearch {HWND result{};long long score{};};
BOOL CALLBACK find_window(HWND hwnd,LPARAM data) {
    DWORD pid{};GetWindowThreadProcessId(hwnd,&pid);
    if(pid!=GetCurrentProcessId()||!IsWindowVisible(hwnd)||GetWindow(hwnd,GW_OWNER))return TRUE;
    RECT r{};GetClientRect(hwnd,&r);long long score=static_cast<long long>(r.right)*r.bottom;
    wchar_t name[128]{};GetClassNameW(hwnd,name,128);
    if(wcscmp(name,L"UnityWndClass")==0)score+=1LL<<40;
    auto& search=*reinterpret_cast<WindowSearch*>(data);
    if(r.right>=320&&r.bottom>=200&&score>search.score){search.result=hwnd;search.score=score;}
    return TRUE;
}
bool install_window(HWND hwnd) {
    previous_proc=reinterpret_cast<WNDPROC>(GetWindowLongPtrW(hwnd,GWLP_WNDPROC));
    if(!previous_proc.load())return false;
    window_thread=GetWindowThreadProcessId(hwnd,nullptr);game_window=hwnd;
    SetLastError(0);
    auto old=SetWindowLongPtrW(hwnd,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(window_proc));
    if(!old&&GetLastError()){game_window=nullptr;log("ERROR: window subclass failed: %lu",GetLastError());return false;}
    previous_proc=reinterpret_cast<WNDPROC>(old);
    timer_id=SetTimer(hwnd,requested_timer,250,nullptr);
    if(!timer_id.load()) {
        log("ERROR: game-thread timer creation failed: %lu",GetLastError());
        SetWindowLongPtrW(hwnd,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(previous_proc.load()));
        game_window=nullptr;return false;
    }
    ULONG flags{};bool was_touch=IsTouchWindow(hwnd,&flags)!=FALSE;
    log("Window=%p thread=%lu timer=%llu previously_touch_registered=%d",hwnd,window_thread.load(),static_cast<unsigned long long>(timer_id.load()),was_touch);
    return true;
}
bool install_icalls() {
    original_count=reinterpret_cast<IntGetter>(slot_value(profile::touch_count_slot));
    original_touch=reinterpret_cast<TouchGetter>(slot_value(profile::get_touch_slot));
    original_supported=reinterpret_cast<BoolGetter>(slot_value(profile::touch_supported_slot));
    get_frame=reinterpret_cast<IntGetter>(slot_value(profile::frame_count_slot));
    get_width=reinterpret_cast<IntGetter>(slot_value(profile::screen_width_slot));
    get_height=reinterpret_cast<IntGetter>(slot_value(profile::screen_height_slot));
    for(auto p:{reinterpret_cast<void*>(original_count),reinterpret_cast<void*>(original_touch),reinterpret_cast<void*>(original_supported),reinterpret_cast<void*>(get_frame),reinterpret_cast<void*>(get_width),reinterpret_cast<void*>(get_height)})
        if(!executable_pointer(p))return false;
    if(!exchange_slot(profile::get_touch_slot,reinterpret_cast<void*>(original_touch),reinterpret_cast<void*>(hooked_touch)))return false;
    if(!exchange_slot(profile::touch_supported_slot,reinterpret_cast<void*>(original_supported),reinterpret_cast<void*>(hooked_supported))) {
        exchange_slot(profile::get_touch_slot,reinterpret_cast<void*>(hooked_touch),reinterpret_cast<void*>(original_touch));return false;
    }
    if(!exchange_slot(profile::touch_count_slot,reinterpret_cast<void*>(original_count),reinterpret_cast<void*>(hooked_count))) {
        exchange_slot(profile::touch_supported_slot,reinterpret_cast<void*>(hooked_supported),reinterpret_cast<void*>(original_supported));
        exchange_slot(profile::get_touch_slot,reinterpret_cast<void*>(hooked_touch),reinterpret_cast<void*>(original_touch));return false;
    }
    hooks_ready=true;
    log("ICall bridge installed: count=%p touch=%p supported=%p",reinterpret_cast<void*>(original_count),reinterpret_cast<void*>(original_touch),reinterpret_cast<void*>(original_supported));
    return true;
}
void worker() {
    auto dir=std::filesystem::path(module_path(self_module)).parent_path()/L"logs";
    std::filesystem::create_directories(dir);
    auto path=dir/(L"touch-"+std::to_wstring(GetCurrentProcessId())+L".log");
    _wfopen_s(&logfile,path.c_str(),L"a");
    log("ZZZTouchUI experimental build 1; pid=%lu",GetCurrentProcessId());
    if(!profile::configured()){log("ERROR: game offsets are not configured in profile.hpp; no hooks installed");return;}
    if(std::filesystem::path(module_path()).filename()!=L"ZenlessZoneZero.exe") {log("ERROR: unsupported process name");return;}
    HMODULE module{};
    for(int i=0;i<600&&!module;++i){module=GetModuleHandleW(L"GameAssembly.dll");if(!module)Sleep(200);}
    if(!module){log("ERROR: GameAssembly.dll not loaded after 120 s");return;}
    log("Verifying GameAssembly SHA-256...");
    if(file_sha256(module_path(module))!=profile::sha256){log("ERROR: unsupported GameAssembly version; no hooks installed");return;}
    assembly=reinterpret_cast<uintptr_t>(module);
    enable_event=CreateEventW(nullptr,TRUE,TRUE,enable_event_name(GetCurrentProcessId()).c_str());
    if(!enable_event){log("ERROR: enable event creation failed: %lu",GetLastError());return;}
    ready_event=CreateEventW(nullptr,TRUE,FALSE,ready_event_name(GetCurrentProcessId()).c_str());
    if(!ready_event){ResetEvent(enable_event);log("ERROR: ready event creation failed");return;}
    WindowSearch search{};
    for(int i=0;i<600&&!search.result;++i){EnumWindows(find_window,reinterpret_cast<LPARAM>(&search));if(!search.result)Sleep(200);}
    if(!search.result||!install_window(search.result)){ResetEvent(enable_event);log("ERROR: no usable game window after 120 s");return;}
    bool installed=false;
    for(int i=0;i<600&&!installed;++i){installed=install_icalls();if(!installed)Sleep(200);}
    if(!installed){ResetEvent(enable_event);log("ERROR: Unity icalls unresolved or table replacement failed after 120 s");return;}
    enabled=WaitForSingleObject(enable_event,0)==WAIT_OBJECT_0;
    SetEvent(ready_event);
    log("READY: native Windows touch bridge enabled=%d. Awaiting UI provider and input.",enabled.load());
    ULONGLONG last_status{};
    bool reported_conflict{};
    for(;;) {
        const bool requested=WaitForSingleObject(enable_event,0)==WAIT_OBJECT_0&&!ui_fault.load()&&game_window.load()!=nullptr&&!reported_conflict;
        if(enabled.exchange(requested)!=requested){if(!requested)cancel_contacts();log("Bridge enabled=%d; UI change will run on game thread",requested);}
        if(!reported_conflict&&(slot_value(profile::touch_count_slot)!=reinterpret_cast<void*>(hooked_count)||slot_value(profile::get_touch_slot)!=reinterpret_cast<void*>(hooked_touch)||slot_value(profile::touch_supported_slot)!=reinterpret_cast<void*>(hooked_supported))) {
            reported_conflict=true;ResetEvent(enable_event);ResetEvent(ready_event);enabled=false;cancel_contacts();
            log("ERROR: another writer changed the icall table. Bridge disabled; restart game to retry.");
            exchange_slot(profile::touch_count_slot,reinterpret_cast<void*>(hooked_count),reinterpret_cast<void*>(original_count));
            exchange_slot(profile::get_touch_slot,reinterpret_cast<void*>(hooked_touch),reinterpret_cast<void*>(original_touch));
            exchange_slot(profile::touch_supported_slot,reinterpret_cast<void*>(hooked_supported),reinterpret_cast<void*>(original_supported));
        }
        const auto now=GetTickCount64();
        if(now-last_status>=5000) {
            last_status=now;uint64_t accepted{},ignored{};
            {InputGuard guard;accepted=input.accepted_downs;ignored=input.ignored_downs;}
            log("status enabled=%d UI=%d WM_TOUCH=%llu WM_POINTER=%llu downs=%llu dropped=%llu count_calls=%llu bridge_frames=%llu bridge_count=%d native_count_last_fallback=%d",
                enabled.load(),effective_layout.load(),touch_messages.load(),pointer_messages.load(),accepted,ignored,count_calls.load(),bridge_frames.load(),last_bridge_count.load(),last_native_count.load());
        }
        Sleep(200);
    }
}
DWORD WINAPI worker_entry(void*) {
    try {worker();}catch(const std::exception& ex){enabled=false;ResetEvent(enable_event);ResetEvent(ready_event);cancel_contacts();log("ERROR: worker stopped: %s",ex.what());}
    return 0;
}
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
        self_module=module;
        HANDLE thread=CreateThread(nullptr,0,worker_entry,nullptr,0,nullptr);
        if(thread)CloseHandle(thread);
        else return FALSE;
    }
    // Hooks and window callback stay resident until process exit. Do not FreeLibrary.
    return TRUE;
}
