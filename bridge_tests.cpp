// Exercises the production bridge inside this test process. It neither loads nor
// opens the game. Only a hidden, owned window and synthetic icall table are used.
#include "payload.cpp"
#include <iostream>
#include <cstring>

namespace {
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
int fake_frame=1,fake_override=2,ui_notifications{},forwarded_messages{};
int native_count(){return 1;}
void native_touch(int index,touch::UnityTouch* out){*out={};out->finger_id=900+index;}
bool native_supported(){return false;}
int frame_count(){return fake_frame;}
int screen_width(){return 1000;}
int screen_height(){return 800;}
int get_override(void*){return fake_override;}
int get_effective(void*){return fake_override?fake_override:2;}
void set_override(int value,void*,void*){fake_override=value;++ui_notifications;}
LRESULT CALLBACK host_proc(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam) {
    if(message==WM_APP+1){++forwarded_messages;return 1234;}
    return DefWindowProcW(hwnd,message,wparam,lparam);
}
void commit_page(uintptr_t rva) {
    check(VirtualAlloc(reinterpret_cast<void*>(assembly+(rva&~uintptr_t{4095})),4096,MEM_COMMIT,PAGE_READWRITE)!=nullptr,"commit test page");
}
void put_slot(uintptr_t rva,void* value) {commit_page(rva);*reinterpret_cast<void**>(assembly+rva)=value;}
void put_function(uintptr_t rva,void* target) {
    commit_page(rva);
    // mov rax, imm64; jmp rax, preserving all incoming ABI arguments.
    unsigned char code[12]={0x48,0xb8};std::memcpy(code+2,&target,8);code[10]=0xff;code[11]=0xe0;
    std::memcpy(reinterpret_cast<void*>(assembly+rva),code,sizeof(code));
    DWORD old{};check(VirtualProtect(reinterpret_cast<void*>(assembly+(rva&~uintptr_t{4095})),4096,PAGE_EXECUTE_READ,&old)!=FALSE,"protect test trampoline");
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(assembly+rva),sizeof(code));
}
void test() {
    assembly=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x15b50000,MEM_RESERVE,PAGE_NOACCESS));
    check(assembly!=0,"reserve test table address space");
    put_slot(profile::touch_count_slot,reinterpret_cast<void*>(native_count));
    put_slot(profile::get_touch_slot,reinterpret_cast<void*>(native_touch));
    put_slot(profile::touch_supported_slot,reinterpret_cast<void*>(native_supported));
    put_slot(profile::frame_count_slot,reinterpret_cast<void*>(frame_count));
    put_slot(profile::screen_width_slot,reinterpret_cast<void*>(screen_width));
    put_slot(profile::screen_height_slot,reinterpret_cast<void*>(screen_height));
    put_function(profile::get_layout_override,reinterpret_cast<void*>(get_override));
    put_function(profile::set_layout_override,reinterpret_cast<void*>(set_override));
    put_function(profile::get_effective_layout,reinterpret_cast<void*>(get_effective));
    std::array<unsigned char,256> klass{};klass[203]=1;
    uintptr_t property=reinterpret_cast<uintptr_t>(klass.data());
    std::array<uintptr_t,20> provider{};provider[16]=reinterpret_cast<uintptr_t>(&property);provider[17]=reinterpret_cast<uintptr_t>(&property);
    std::vector<uintptr_t> pool(profile::ui_state_offset/sizeof(uintptr_t)+1);
    pool.back()=reinterpret_cast<uintptr_t>(provider.data());
    put_slot(profile::ui_class_slot,klass.data());put_slot(profile::static_reference_pool,pool.data());
    check(ready_ui_provider()==reinterpret_cast<uintptr_t>(provider.data()),"UI provider readiness");
    enable_event=CreateEventW(nullptr,TRUE,TRUE,nullptr);ready_event=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    WNDCLASSW wc{};wc.lpfnWndProc=host_proc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"ZZZTouchOwnedTestWindow";
    check(RegisterClassW(&wc)!=0,"register owned test window");
    HWND hwnd=CreateWindowExW(0,wc.lpszClassName,L"Touch bridge test",WS_OVERLAPPEDWINDOW,0,0,800,600,nullptr,nullptr,wc.hInstance,nullptr);
    check(hwnd!=nullptr&&install_window(hwnd),"subclass owned hidden window");
    check(install_icalls(),"atomic icall installation");
    auto count=reinterpret_cast<IntGetter>(slot_value(profile::touch_count_slot));
    auto get=reinterpret_cast<TouchGetter>(slot_value(profile::get_touch_slot));
    auto supported=reinterpret_cast<BoolGetter>(slot_value(profile::touch_supported_slot));
    touch::UnityTouch value{};
    check(count()==1&&!supported(),"disabled passthrough to native input");get(0,&value);check(value.finger_id==900,"native GetTouch passthrough");
    enabled=true;last_ui_check=0;
    SendMessageW(hwnd,WM_TIMER,timer_id.load(),0);
    check(fake_override==1&&ui_notifications==1&&effective_layout.load()==1,"UI setter runs through main-thread notification path");
    check(supported(),"touch supported while enabled");
    check(SendMessageW(hwnd,WM_APP+1,0,0)==1234&&forwarded_messages==1,"unrelated message forwarded to original window procedure");
    record_input(1,5,{.25f,.75f},true,false,false);record_input(1,6,{.75f,.25f},true,false,false);
    record_input(2,99,{.5f,.5f},true,false,false);
    ++fake_frame;check(count()==2,"bridge replaces native count and suppresses duplicate event source");
    get(0,&value);check(value.phase==touch::Began&&value.position.x==250&&value.position.y==200,"GetTouch ABI and normalized coordinate conversion");
    record_input(1,5,{.5f,.5f},false,false,false);get(0,&value);check(value.position.x==250,"snapshot remains stable across GetTouch calls within a frame");
    ++fake_frame;get(0,&value);check(value.phase==touch::Moved&&value.delta_position.x==250,"movement delivered next frame");
    SendMessageW(hwnd,WM_CANCELMODE,0,0);++fake_frame;
    check(count()==2,"cancellation retains contacts for terminal frame");get(0,&value);check(value.phase==touch::Canceled,"window cancellation delivered to Unity");
    ++fake_frame;check(count()==0,"native echo suppressed after bridge ends");++fake_frame;check(count()==0,"second echo frame suppressed");++fake_frame;check(count()==1,"native fallback resumes");
    enabled=false;last_ui_check=0;SendMessageW(hwnd,WM_TIMER,timer_id.load(),0);
    check(fake_override==2&&ui_notifications==2&&!supported(),"disable restores owned layout and native support result");
    check(!IsTouchWindow(hwnd,nullptr),"disable restores owned touch registration");
    // A later external layout edit must survive disable.
    enabled=true;last_ui_check=0;maintain_ui();fake_override=3;enabled=false;last_ui_check=0;maintain_ui();
    check(fake_override==3,"do not overwrite another writer's layout during restore");
    check(!exchange_slot(profile::touch_count_slot,reinterpret_cast<void*>(native_count),reinterpret_cast<void*>(native_supported)),"compare/exchange rejects a stale expected pointer");
    check(slot_value(profile::touch_count_slot)==reinterpret_cast<void*>(hooked_count),"failed compare/exchange preserves hook");
    DWORD old_thread=window_thread.load();window_thread=old_thread+1;enabled=true;last_ui_check=0;maintain_ui();
    check(fake_override==3,"UI setter prohibited on non-window thread");window_thread=old_thread;enabled=false;
    DestroyWindow(hwnd);check(!game_window.load(),"owned window destruction disables bridge");
    CloseHandle(enable_event);CloseHandle(ready_event);VirtualFree(reinterpret_cast<void*>(assembly),0,MEM_RELEASE);
}
}
int main() {
    try {test();std::cout<<"Production icall bridge, ABI, frame consistency, event-source deduplication, cancellation, native fallback, UI notifications/restoration and window forwarding: PASS\n";return 0;}
    catch(const std::exception& ex){std::cerr<<"FAIL: "<<ex.what()<<"\n";return 1;}
}
