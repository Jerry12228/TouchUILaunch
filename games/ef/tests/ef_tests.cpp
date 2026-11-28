#include "ef_hook.hpp"
#include "ef_launch.hpp"
#include "ef_parent.hpp"
#include "game.hpp"
#include "load_library_diagnostic.hpp"
#include <functional>
#include <iostream>
#include <thread>

namespace {
using namespace touchui::ef;
unsigned checks{};
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F function,const char* message) {
    bool rejected{};try {function();}catch(const std::runtime_error&){rejected=true;}
    check(rejected,message);
}
struct Class;
struct Type {int kind;Class* klass{};bool byref{};};
struct Field {const char* name;Type* type;int flags;uint64_t value;};
struct Method {void* entry;const char* name;bool instance;Type* result;std::vector<Type*> args;bool generic{};};
struct Class {const char* name;std::vector<Class*> nested;std::vector<Field*> fields;std::vector<Method*> methods;};
struct Fixture;
Fixture* current{};
__declspec(noinline) void original_switch(int32_t,const void*);
struct Fixture {
    Class device{"DeviceInfo"},input{"InputType"},other{"Other"};
    Type enum_type{0x11,&input},reference{0x12,&other},i4{0x08},void_type{0x01};
    Field touch{"Touch",&enum_type,0x50,7}; // deliberately different from the game
    Field underlying{"value__",&i4,0,0};
    Field backing{"<inputType>k__BackingField",&enum_type,0x10,0};
    Field provider{"s_provider",&reference,0x10,0x1111};
    Field notification{"onInputTypeChanged",&reference,0x10,0x2222};
    Method change{reinterpret_cast<void*>(&original_switch),"ChangeInputType",false,&void_type,{&enum_type}};
    std::vector<const void*> assemblies{&device};
    bool attached=true,domain_ready=true,reject_switch{},managed_exception{},exception_after_write{};
    unsigned invokes{},events{},original_calls{};
    int32_t last_target=-1;const void* last_method{};
    std::function<void()> reenter;
    Fixture() {
        current=this;device.nested={&input};device.fields={&backing,&provider,&notification};
        input.fields={&touch,&underlying};device.methods={&change};
    }
    Api api() {
        Api a;a.module=GetModuleHandleW(nullptr);
        a.domain_get=[]()->void*{return current->domain_ready?current:nullptr;};
        a.domain_get_assemblies=[](void*,size_t* count)->const void** {
            *count=current->assemblies.size();return current->assemblies.data();
        };
        a.assembly_get_image=[](const void* value)->void*{return const_cast<void*>(value);};
        a.image_get_name=[](void*){return "Common.Beyond.dll";};
        a.class_from_name=[](void*,const char* ns,const char* name)->void* {
            return text_equal(ns,"Beyond")&&text_equal(name,"DeviceInfo")?&current->device:nullptr;
        };
        a.class_get_nested_types=[](void* value,void** iterator)->void* {
            const auto& entries=static_cast<Class*>(value)->nested;
            const auto index=reinterpret_cast<uintptr_t>(*iterator);
            if(index>=entries.size())return nullptr;
            *iterator=reinterpret_cast<void*>(index+1);return entries[index];
        };
        a.class_get_name=[](void* value){return static_cast<Class*>(value)->name;};
        a.class_get_methods=[](void* value,void** iterator)->const void* {
            const auto& entries=static_cast<Class*>(value)->methods;
            const auto index=reinterpret_cast<uintptr_t>(*iterator);
            if(index>=entries.size())return nullptr;
            *iterator=reinterpret_cast<void*>(index+1);return entries[index];
        };
        a.method_get_name=[](const void* value){return static_cast<const Method*>(value)->name;};
        a.method_get_param_count=[](const void* value){return static_cast<uint32_t>(static_cast<const Method*>(value)->args.size());};
        a.method_get_param=[](const void* value,uint32_t index)->const void*{return static_cast<const Method*>(value)->args.at(index);};
        a.method_get_return_type=[](const void* value)->const void*{return static_cast<const Method*>(value)->result;};
        a.method_is_instance=[](const void* value){return static_cast<const Method*>(value)->instance;};
        a.method_is_generic=[](const void* value){return static_cast<const Method*>(value)->generic;};
        a.class_from_type=[](const void* value)->void*{return value?static_cast<const Type*>(value)->klass:nullptr;};
        a.type_get_type=[](const void* value){return value?static_cast<const Type*>(value)->kind:0;};
        a.type_is_byref=[](const void* value){return static_cast<const Type*>(value)->byref;};
        a.class_get_field_from_name=[](void* value,const char* name)->void* {
            for(auto field:static_cast<Class*>(value)->fields)if(text_equal(field->name,name))return field;
            return nullptr;
        };
        a.field_get_type=[](void* value)->const void*{return static_cast<Field*>(value)->type;};
        a.field_get_flags=[](void* value){return static_cast<Field*>(value)->flags;};
        a.field_static_get_value=[](void* value,void* output) {
            const auto field=static_cast<Field*>(value);
            std::memcpy(output,&field->value,field->type->kind==0x12?sizeof(void*):sizeof(int32_t));
        };
        a.thread_current=[]()->void*{return current->attached?current:nullptr;};
        a.runtime_invoke=[](const void* method,void* instance,void** args,void** exception)->void* {
            check(method==&current->change&&!instance,"invoke exact static ChangeInputType MethodInfo");
            ++current->invokes;current->last_target=*static_cast<int32_t*>(args[0]);
            if(current->reenter)current->reenter();
            if(current->managed_exception) {
                if(current->exception_after_write)current->backing.value=static_cast<uint32_t>(current->last_target);
                *exception=current;return nullptr;
            }
            if(!current->reject_switch) {
                current->backing.value=static_cast<uint32_t>(current->last_target);++current->events;
            }
            return nullptr;
        };
        return a;
    }
};
__declspec(noinline) void original_switch(int32_t value,const void* method) {
    volatile int32_t forwarded=value;
    current->last_target=forwarded;current->last_method=method;++current->original_calls;
}

void contract_tests() {
    Fixture f;auto a=f.api();f.domain_ready=false;
    check(!resolve(a),"domain pending is retried without invoking game methods");
    f.domain_ready=true;f.assemblies.clear();check(!resolve(a),"assemblies pending");
    f.assemblies={&f.device};const auto c=resolve(a);
    check(c&&c->touch==7&&c->change_method==&f.change&&c->change_entry==f.change.entry,"dynamic enum and exact callable contract");
    auto overload=f.change;overload.args={&f.i4};f.device.methods.insert(f.device.methods.begin(),&overload);
    check(resolve(a)->change_method==&f.change,"incorrect overload ignored");
    auto byref_type=f.enum_type;byref_type.byref=true;overload.args={&byref_type};
    check(resolve(a)->change_method==&f.change,"byref enum overload cannot replace value parameter");
    overload.args={&f.enum_type};overload.generic=true;
    check(resolve(a)->change_method==&f.change,"open generic method cannot replace concrete switch");overload.generic=false;
    overload.args={&f.enum_type};rejects([&]{resolve(a);},"ambiguous matching methods rejected");
    f.device.methods={&f.change};f.change.instance=true;rejects([&]{resolve(a);},"instance method cannot replace static switch");
    f.change.instance=false;f.change.result=&f.i4;rejects([&]{resolve(a);},"wrong method return type rejected");
    f.change.result=&f.void_type;f.change.entry=&f.backing;rejects([&]{resolve(a);},"non-executable MethodInfo entry rejected");
    f.change.entry=reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"GetTickCount64"));
    rejects([&]{resolve(a);},"executable entry in a different module rejected");
    f.change.entry=reinterpret_cast<void*>(&original_switch);f.backing.flags=0;
    rejects([&]{resolve(a);},"instance backing field rejected");f.backing.flags=0x10;
    f.backing.type=&f.i4;rejects([&]{resolve(a);},"wrong input enum field type rejected");f.backing.type=&f.enum_type;
    f.touch.flags=0x10;rejects([&]{resolve(a);},"nonliteral Touch rejected");f.touch.flags=0x50;
    f.underlying.type=&f.reference;rejects([&]{resolve(a);},"non-Int32 enum representation rejected");f.underlying.type=&f.i4;
    f.provider.type=&f.i4;rejects([&]{resolve(a);},"nonreference provider rejected");f.provider.type=&f.reference;
    f.device.nested.clear();rejects([&]{resolve(a);},"missing nested InputType rejected");f.device.nested={&f.input};
    f.assemblies.push_back(&f.other);rejects([&]{resolve(a);},"conflicting Common.Beyond images rejected");
    rejects([]{Api::bind(GetModuleHandleW(nullptr));},"missing IL2CPP exports rejected");
}
void layout_tests() {
    Fixture f;auto a=f.api();Layout l(a);
    f.attached=false;check(l.pump(0)==Tick::WaitingThread&&!l.contract()&&!f.invokes,"unattached thread does no metadata work");
    f.attached=true;f.provider.value=0;
    check(l.pump(250)==Tick::WaitingDevice&&!f.invokes,"wait for initialized provider");
    f.provider.value=1;f.notification.value=0;
    check(l.pump(500)==Tick::WaitingDevice&&!f.invokes,"wait for notification object");f.notification.value=1;
    f.reject_switch=true;
    check(l.pump(750)==Tick::Retry&&f.invokes==1&&f.backing.value==0,"rejected switch cannot claim success");
    check(l.pump(800)==Tick::Deferred&&f.invokes==1,"retry attempts bounded to four per second");
    f.reject_switch=false;f.managed_exception=true;
    check(l.pump(1000)==Tick::ManagedException&&f.backing.value==0,"managed exceptions do not claim success");
    f.managed_exception=false;
    f.reenter=[&]{check(l.pump(1250)==Tick::Deferred,"synchronous UI notification cannot reenter switch");};
    check(l.pump(1250)==Tick::Applied&&f.backing.value==7&&f.events==1,"switch updates real backing state and raises fixture notification");
    f.reenter={};const auto calls=f.invokes;
    check(l.pump(1500)==Tick::Applied&&f.invokes==calls,"matching real state is not redundantly switched");
    f.backing.value=2;check(l.pump(1750)==Tick::Applied&&f.invokes==calls+1,"initialization drift repaired without new configuration");
    f.attached=false;f.backing.value=0;
    check(l.pump(2000)==Tick::WaitingThread&&f.backing.value==0,"lost runtime attachment never uses cached contract");f.attached=true;
    bool wrong_thread{};
    std::thread thread([&]{try {l.prepare();}catch(const std::runtime_error&){wrong_thread=true;}});thread.join();
    check(wrong_thread,"a replacement window on another thread cannot execute cached UI calls");
    Fixture partial;auto partial_api=partial.api();Layout partial_layout(partial_api);
    partial.managed_exception=true;partial.exception_after_write=true;
    rejects([&]{partial_layout.pump(0);},"exception after field write must fail instead of falsely confirming UI notifications");
}
void hook_tests() {
    Fixture f;auto a=f.api();const auto c=*resolve(a);ChangeHook hook;hook.install(c);
    using Function=void(*)(int32_t,const void*);
    Function volatile call=&original_switch;
    call(2,&f.change);
    check(f.original_calls==1&&f.last_target==2,"new hook preserves startup choices until Touch is confirmed");
    hook.activate();call(2,&f.change);
    check(f.original_calls==2&&f.last_target==7&&f.last_method==&f.change,"real detour forces Touch and preserves MethodInfo/original call");
    hook.deactivate();call(2,&f.change);
    check(f.original_calls==3&&f.last_target==2,"deactivated detour forwards game's original input choice");
    hook.remove_fixture();call(0,nullptr);
    check(f.original_calls==4&&f.last_target==0&&!f.last_method,"fixture hook cleanup restores original entry");
}
void coordination_tests() {
    Handle ready(CreateEventW(nullptr,TRUE,FALSE,nullptr)),failed(CreateEventW(nullptr,TRUE,FALSE,nullptr));
    Handle process(OpenProcess(SYNCHRONIZE,FALSE,GetCurrentProcessId()));
    check(ready.value&&failed.value&&process.value,"owned coordination fixture handles created");
    rejects([&]{wait_ready(process,ready,failed,1);},"unconfirmed state times out");
    SetEvent(failed);rejects([&]{wait_ready(process,ready,failed,1);},"payload failure propagated");
    SetEvent(ready);rejects([&]{wait_ready(process,ready,failed,1);},"failure takes precedence over ready");
    ResetEvent(failed);wait_ready(process,ready,failed,1);check(true,"confirmed readiness accepted");
    const auto purpose=L"Fixture."+std::to_wstring(GetTickCount64());
    Handle first(create_event(GetCurrentProcessId(),purpose));
    rejects([&]{Handle second(create_event(GetCurrentProcessId(),purpose));},"preexisting activation objects rejected");
    check(game::parse(L"--EF")==game::Kind::EF&&game::accepts(game::Kind::EF,L"ENDFIELD.EXE")&&
        !game::accepts(game::Kind::EF,L"EndfieldBase.dll"),"EF selector and executable family");
    rejects([&]{game::validate(game::Kind::EF,module_path(),false,false);},"EF requires an explicit path before launch");
    check(version_parts(L"1.10.0")>version_parts(L"1.9.2")&&version_parts(L"invalid").empty(),"official launcher versions compare numerically");
    rejects([&]{Handle invalid(verify_parent(module_path()));},"unrelated executable cannot become the official parent");
    rejects([&]{game::Child invalid;invalid.start(module_path(),true,L"",nullptr,ready.value);},"nonprocess parent handle rejected before child launch");
}
struct TimerFixture {
    Layout* layout;HANDLE created,finished;HWND window{};DWORD thread{};bool applied{},correct_thread{};
};
void CALLBACK fixture_timer(HWND window,UINT,UINT_PTR id,DWORD) {
    auto& state=*reinterpret_cast<TimerFixture*>(id);
    try {
        state.correct_thread=GetCurrentThreadId()==state.thread;
        state.applied=state.layout->pump(GetTickCount64())==Tick::Applied;
    }catch(...){}
    KillTimer(window,id);SetEvent(state.finished);PostQuitMessage(0);
}
void window_thread_test() {
    Fixture f;auto a=f.api();Layout layout(a);
    Handle created(CreateEventW(nullptr,TRUE,FALSE,nullptr)),finished(CreateEventW(nullptr,TRUE,FALSE,nullptr));
    TimerFixture state{&layout,created,finished};
    std::thread thread([&] {
        state.thread=GetCurrentThreadId();
        WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"TouchUIEFWindowFixture";
        RegisterClassW(&wc);
        state.window=CreateWindowExW(0,wc.lpszClassName,L"EF owned window",WS_OVERLAPPEDWINDOW,0,0,640,480,nullptr,nullptr,wc.hInstance,nullptr);
        SetEvent(created);
        MSG message{};
        while(GetMessageW(&message,nullptr,0,0)>0){TranslateMessage(&message);DispatchMessageW(&message);}
        if(state.window)DestroyWindow(state.window);
        UnregisterClassW(wc.lpszClassName,wc.hInstance);
    });
    const bool opened=WaitForSingleObject(created,5000)==WAIT_OBJECT_0&&state.window;
    UINT_PTR id{};
    if(opened)id=SetTimer(state.window,reinterpret_cast<UINT_PTR>(&state),25,fixture_timer);
    const bool completed=id&&WaitForSingleObject(finished,5000)==WAIT_OBJECT_0;
    if(!completed) {
        if(id)KillTimer(state.window,id);
        PostThreadMessageW(state.thread,WM_QUIT,0,0);
    }
    thread.join();
    check(opened&&completed,"worker-scheduled timer delivered to owned game window");
    check(state.correct_thread&&state.applied&&f.backing.value==7&&f.events==1,
        "actual USER32 timer invokes layout switch on the window thread");
}
void preload_test() {
    // The production payload must map and dispatch before the owned child's
    // main thread runs. Its wrong-process rejection is intentional here.
    game::Child parent;parent.start(module_path(),true);
    game::Child early;early.start(module_path(),true,L"",nullptr,parent.info.hProcess);
    const auto pid=early.info.dwProcessId;
    Handle processes(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));
    PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);bool correct_parent{};
    if(Process32FirstW(processes,&entry))do {
        if(entry.th32ProcessID==pid)correct_parent=entry.th32ParentProcessID==parent.info.dwProcessId;
    }while(Process32NextW(processes,&entry));
    check(correct_parent,"owned child receives its actual configured parent process");
    Handle logging(create_event(pid,L"Log")),ready(create_event(pid,L"Ready")),failed(create_event(pid,L"Failed"));
    const auto dll=std::filesystem::path(module_path()).parent_path()/L"TouchUILaunch.dll";
    const auto loaded=touchui::loader::load_direct(early.info.hProcess,dll);
    check(loaded.module.base!=0,"shared layout DLL maps before the game's main thread");
    check(SuspendThread(early.info.hThread)==1,"EF preload preserves initial main-thread suspension");
    check(ResumeThread(early.info.hThread)==2,"EF preload fixture restores only its observer suspension");
    check(WaitForSingleObject(failed,5000)==WAIT_OBJECT_0,"preloaded shared DLL dispatches EF and rejects fixture identity");
    check(WaitForSingleObject(ready,0)==WAIT_TIMEOUT,"preloaded wrong process cannot publish Touch readiness");
}
}
int wmain(int argc,wchar_t** argv) {
    try {
        if(argc==3&&std::wstring_view(argv[1])==L"--parent") {
            const auto path=parent_for(std::filesystem::absolute(argv[2]));
            std::wcout<<L"PASS: trusted x64 GRYPHLINK launcher: "<<path<<L" (read only)\n";return 0;
        }
        check(argc==1,"Endfield fixture arguments");
        contract_tests();layout_tests();hook_tests();coordination_tests();window_thread_test();preload_test();
        std::cout<<"PASS: "<<checks<<" Endfield metadata, state, ABI and coordination checks\n";return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
