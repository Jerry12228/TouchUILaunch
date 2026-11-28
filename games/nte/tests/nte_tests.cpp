#include "nte_fixture.hpp"
#include "nte_launch.hpp"
#include <fstream>
#include <iostream>
#include <thread>

namespace {
using namespace touchui::nte;
using nte_fixture::Fixture;
unsigned checks{};
void check(bool condition,const char* message){++checks;if(!condition)throw std::runtime_error(message);}
template<class F> void rejects(F action,const char* message) {
    bool rejected{};try {action();}catch(const std::exception&){rejected=true;}check(rejected,message);
}
void discovery_tests() {
    for(unsigned iteration=0;iteration<2;++iteration) {
        Fixture f;auto module=f.snapshot();const auto plan=discover(module);
        check(plan.mobile_predicate==Fixture::predicate&&plan.navigation_slot==Fixture::vtable+0x3a0&&
            plan.navigation_original==Fixture::nav_original,"registration and virtual setter relationships");
        check(plan.cloud.object==f.base+0xa000&&plan.util_mobile.object==f.base+0xa100&&
            plan.navigation_mobile.object==f.base+0xa200,"identical names resolve by owner and callback");
        check(plan.cloud.exec==0x1800&&plan.util_mobile.exec==0x1900&&plan.navigation_mobile.exec==0x1a00,"native table ordering");
        check(plan.native_patch==NativePatch::AtomicStack,"international stack entry retains atomic publication");
        check(module.function_start(0x2a05)==0x2a00,"bounded runtime function lookup");
    }
    {
        Fixture f(true);auto module=f.snapshot();const auto plan=discover(module);
        check(plan.native_patch==NativePatch::SavedRegisterDetour,"saved-register entry selects complete-instruction detour");
        f.emit(Fixture::predicate,{0x40,0x55});f.sync();
        rejects([&]{auto changed=f.snapshot();discover(changed);},"unobserved saved register rejected");
    }
    {
        Fixture f(true);f.put<uint8_t>(Fixture::predicate+5,0x28);f.sync();
        rejects([&]{auto module=f.snapshot();discover(module);},"saved-register entry with different frame rejected");
    }
    const auto malformed=[](auto change,const char* message) {
        Fixture f;change(f);f.sync();rejects([&]{auto module=f.snapshot();discover(module);},message);
    };
    malformed([](Fixture& f){f.put<uint32_t>(0x3c,0xffffffff);},"oversized PE header rejected");
    malformed([](Fixture& f){f.put<uint32_t>(0xc00c,0x2800);},"unordered unwind entries rejected");
    malformed([](Fixture& f){f.put<uint8_t>(0xd000,0x21);f.put<uint32_t>(0xd004,0x2800);f.put<uint32_t>(0xd008,0x280d);f.put<uint32_t>(0xd00c,0xd000);},"cyclic unwind rejected");
    malformed([](Fixture& f){f.pair(0x6200,0x5000,0x3000);},"reversed generated table rejected");
    malformed([](Fixture& f){f.put<uintptr_t>(0xa020,f.base+0xa900);},"wrong UFunction owner rejected");
    malformed([](Fixture& f){f.put<uint32_t>(0xa0b0,0);},"non-native UFunction rejected");
    malformed([](Fixture& f){f.put<uint8_t>(0xa1b4,2);},"parameterized function rejected");
    malformed([](Fixture& f){f.put<uint16_t>(0xa2b8,0xffff);},"missing bool return rejected");
    malformed([](Fixture& f){f.emit(Fixture::predicate,{0x90,0x90,0x90,0x90});},"unsupported prologue rejected");
    malformed([](Fixture& f){f.put<uint8_t>(0x2203,0x4c);},"different navigation state rejected");
    malformed([](Fixture& f){f.pair(0x6230,0x3300,0x5000);f.lazy(0x3300,0x9060);f.put<uintptr_t>(0x9060,f.base+0xa000);},"ambiguous live function caches rejected");
    {
        Fixture f;auto module=f.snapshot();auto plan=discover_static(module);
        f.put<uintptr_t>(0x9000,0);f.sync();
        rejects([&]{bind_metadata(plan,f.base);},"pending class metadata rejected without writes");
        f.put<uintptr_t>(0x9000,f.base+0xa800);f.sync();bind_metadata(plan,f.base);
        check(plan.cloud.owner==f.base+0xa800,"live metadata retry reuses immutable snapshot");
        rejects([&]{bind_metadata(plan,f.base,[](uintptr_t,std::span<uint8_t>){return false;});},"unreadable metadata rejected");
    }
    {
        Fixture f;f.emit(0x280c,{0xe9,0,0,0,0,0xf0,0xff});f.rel(0x280d,0x2811,0x3800);f.sync();
        auto module=f.snapshot();check(discover(module).cloud.exec==0x1800,"tail-jump registrar does not decode neighboring functions");
    }
    {
        Fixture f;
        f.emit(0x2200,{0x0f,0xb6,0x81,0x4b,0x01,0,0,0x84,0xc0,0x74,0x01,0xc3,0x88,0x91,0x4b,0x01,0,0,0xc3});f.sync();
        auto module=f.snapshot();check(discover(module).navigation_original==Fixture::nav_original,"setter write after early return is reached by its branch");
    }
    {
        Fixture f;
        // Child unwind region containing the LEA links back to the registrar.
        f.put<uint32_t>(0xc004,0x2805);f.put<uint32_t>(0xc00c,0x2805);f.put<uint32_t>(0xc010,0x280d);f.put<uint32_t>(0xc014,0xd020);
        f.put<uint32_t>(0xc018,0x2900);f.put<uint32_t>(0xc01c,0x290d);f.put<uint32_t>(0xc020,0xd000);
        f.put<uint32_t>(0xc024,0x2a00);f.put<uint32_t>(0xc028,0x2a0d);f.put<uint32_t>(0xc02c,0xd000);
        f.put<uint32_t>(0x98+116+3*8,48);f.put<uint8_t>(0xd020,0x21);
        f.put<uint32_t>(0xd024,0x2800);f.put<uint32_t>(0xd028,0x2805);f.put<uint32_t>(0xd02c,0xd000);f.sync();
        auto module=f.snapshot();check(module.function_start(0x2805)==0x2800,"chained unwind restores registrar start");
        check(discover(module).cloud.exec==0x1800,"registration LEA in chained function is accepted");
    }
}
void saved_register_hook_tests() {
    using Predicate=bool(*)();
    {
        Fixture f(true);EarlyHookBuffer reservation;
        check(reservation.reserve(reinterpret_cast<void*>(f.base)),"reserve ordinary MinHook buffer before later setup");
        void* original{};auto* target=reinterpret_cast<void*>(f.base+Fixture::predicate);
        check(MH_CreateHook(target,reinterpret_cast<void*>(&native_mobile),&original)==MH_OK,"create hook using reserved pool");
        MEMORY_BASIC_INFORMATION reserved{},trampoline{};
        VirtualQuery(reservation.slot(),&reserved,sizeof(reserved));VirtualQuery(original,&trampoline,sizeof(trampoline));
        check(reserved.AllocationBase==trampoline.AllocationBase,"later native hook reuses the held nearby page");
        reservation.release();
        check(MH_EnableHook(target)==MH_OK&&reinterpret_cast<Predicate>(target)()&&!reinterpret_cast<Predicate>(original)(),
            "releasing dummy slot preserves both active hook and original trampoline");
        check(MH_RemoveHook(target)==MH_OK,"quiescent reserved-pool hook cleanup");
    }
    {
        Fixture f(true);auto module=f.snapshot();const auto plan=discover(module);HookOperations operations;
        bool creation_called{},enable_called{};
        operations.create_detour=[&](void* at,void* callback) {
            creation_called=at==reinterpret_cast<void*>(f.base+Fixture::predicate)&&callback==reinterpret_cast<void*>(&native_mobile);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);return MH_ERROR_MEMORY_ALLOC;
        };
        operations.enable_detour=[&](void*){enable_called=true;return MH_OK;};
        Hooks hooks(operations);std::string failure;
        try {hooks.install(module,plan,prepare_callbacks(f.base,plan));}
        catch(const std::exception& error){failure=error.what();}
        check(creation_called&&!enable_called&&failure.find("MH_ERROR_MEMORY_ALLOC")!=std::string::npos,
            "failed detour creation is reported without enabling a hook");
        check(failure.find("target=0x")!=std::string::npos&&failure.find("last_win32=8")!=std::string::npos&&
            failure.find("near_free_regions=")!=std::string::npos,"allocation failure retains bounded address-space diagnostics");
        check(!hooks.code_published&&f.live<uint32_t>(Fixture::predicate)==plan.predicate_original&&
            f.live<uintptr_t>(0xa0d8)==f.base+0x1800&&f.live<uintptr_t>(Fixture::vtable+0x3a0)==f.base+Fixture::nav_original,
            "allocation failure publishes neither native code nor callback slots");
    }
    {
        Fixture f(true);auto module=f.snapshot();const auto plan=discover(module);Hooks hooks;
        const auto predicate=reinterpret_cast<Predicate>(f.base+Fixture::predicate);
        check(!predicate(),"saved-register bool initially false");
        std::atomic<unsigned> before{},after{};
        struct Caller {
            std::atomic<bool> stop{};std::thread worker;
            ~Caller(){finish();}
            void finish(){stop=true;if(worker.joinable())worker.join();}
        } caller;
        caller.worker=std::thread([&]{while(!caller.stop.load()) {if(predicate())++after;else ++before;}});
        auto deadline=GetTickCount64()+3000;
        while(before.load()<100&&GetTickCount64()<deadline)SwitchToThread();
        check(before>=100,"concurrent original predicate calls established");
        hooks.install(module,plan,prepare_callbacks(f.base,plan));
        deadline=GetTickCount64()+3000;
        while(after.load()<100&&GetTickCount64()<deadline)SwitchToThread();
        caller.finish();
        check(hooks.code_published&&predicate()&&after>=100,"saved-register detour works with concurrent callers");
        uint8_t result{};reinterpret_cast<Exec>(f.live<uintptr_t>(0xa0d8))(nullptr,nullptr,&result);
        check(result==1,"reflection hooks coexist with native detour");
        MEMORY_BASIC_INFORMATION code{};VirtualQuery(reinterpret_cast<void*>(predicate),&code,sizeof(code));
        check(code.Protect==PAGE_EXECUTE_READ,"MinHook restores native code page protection");
        check(hooks.rollback_pointers()&&predicate(),"pointer rollback does not live-remove native detour");
        // Tests have joined every caller. Production retains this hook until exit.
        check(MH_RemoveHook(reinterpret_cast<void*>(predicate))==MH_OK&&!predicate(),"quiescent synthetic detour cleanup");
    }
    {
        Fixture f(true);auto module=f.snapshot();const auto plan=discover(module);Hooks hooks;
        void* at=reinterpret_cast<void*>(f.base+Fixture::predicate);
        check(MH_CreateHook(at,reinterpret_cast<void*>(&native_mobile),nullptr)==MH_OK,"prepare competing hook fixture");
        rejects([&]{hooks.install(module,plan,prepare_callbacks(f.base,plan));},"existing native detour ownership rejected");
        check(!hooks.code_published&&f.live<uintptr_t>(0xa0d8)==f.base+0x1800,"detour preparation failure publishes no callbacks");
        check(MH_RemoveHook(at)==MH_OK,"competing fixture retained until explicit cleanup");
    }
    {
        Fixture f(true);auto module=f.snapshot();const auto plan=discover(module);HookOperations operations;
        operations.enable_detour=[](void*){return MH_ERROR_MEMORY_PROTECT;};Hooks hooks(operations);
        rejects([&]{hooks.install(module,plan,prepare_callbacks(f.base,plan));},"native enable failure reported");
        void* at=reinterpret_cast<void*>(f.base+Fixture::predicate);
        check(!hooks.code_published&&f.live<uintptr_t>(0xa0d8)==f.base+0x1800&&
            f.live<uintptr_t>(Fixture::vtable+0x3a0)==f.base+Fixture::nav_original,"failed native enable restores owned pointer slots");
        check(MH_CreateHook(at,reinterpret_cast<void*>(&native_mobile),nullptr)==MH_ERROR_ALREADY_CREATED,
            "failed enable retains trampoline until owned process exit");
        check(MH_RemoveHook(at)==MH_OK,"quiescent failed-enable fixture cleanup");
    }
    {
        Fixture f(true);auto module=f.snapshot();const auto plan=discover(module);HookOperations operations;unsigned calls{};
        operations.protect=[&](void* at,size_t size,DWORD mode,DWORD* old){
            return ++calls==3?false:VirtualProtect(at,size,mode,old)!=FALSE;
        };Hooks hooks(operations);
        rejects([&]{hooks.install(module,plan,prepare_callbacks(f.base,plan));},"pointer failure before native enable reported");
        void* at=reinterpret_cast<void*>(f.base+Fixture::predicate);
        check(!hooks.code_published&&f.live<uintptr_t>(0xa0d8)==f.base+0x1800,"pre-enable failure restores owned pointers");
        check(MH_CreateHook(at,reinterpret_cast<void*>(&native_mobile),nullptr)==MH_OK,"unpublished never-attempted trampoline removed");
        check(MH_RemoveHook(at)==MH_OK,"pre-enable fixture cleanup");
    }
    {
        Fixture f(true);auto module=f.snapshot();const auto plan=discover(module);HookOperations operations;
        operations.flush=[](void*,size_t){return false;};Hooks hooks(operations);
        rejects([&]{hooks.install(module,plan,prepare_callbacks(f.base,plan));},"post-enable cache failure reported");
        void* at=reinterpret_cast<void*>(f.base+Fixture::predicate);
        check(hooks.code_published&&reinterpret_cast<Predicate>(at)(),"published detour retained on later failure");
        hooks.rollback_pointers();check(MH_RemoveHook(at)==MH_OK,"post-publication fixture cleanup only after quiescence");
    }
}
unsigned exec_calls{},navigation_calls{};void* expected_context{};void* expected_frame{};bool callback_ok{};
void original_test_exec(void* context,void* frame,void* result) {
    ++exec_calls;callback_ok&=context==expected_context&&frame==expected_frame;
    if(result)*static_cast<uint8_t*>(result)=0;
}
void original_test_navigation(void* context,uint8_t type) {
    ++navigation_calls;callback_ok&=context==expected_context&&type==touch_input;
}
void callback_tests() {
    int context{},frame{};uint8_t result{};expected_context=&context;expected_frame=&frame;callback_ok=true;exec_calls=0;navigation_calls=0;
    for(auto& original:original_exec)original.store(original_test_exec);
    original_navigation.store(original_test_navigation);
    mobile_exec<0>(&context,&frame,&result);check(result==1,"cloud callback overrides result after original exec");
    mobile_exec<1>(&context,&frame,&result);mobile_exec<2>(&context,&frame,nullptr);
    for(uint8_t type=0;type<5;++type)navigation(&context,type);
    check(callback_ok&&exec_calls==3&&navigation_calls==5,"callbacks preserve context/frame and forward Touch to original setter");
}
void hook_tests() {
    {
        Fixture f;auto module=f.snapshot();auto plan=discover(module);Hooks hooks;
        using Predicate=bool(*)();auto predicate=reinterpret_cast<Predicate>(f.base+Fixture::predicate);
        check(!predicate(),"fixture native bool initially false");
        hooks.install(module,plan,prepare_callbacks(f.base,plan));
        check(hooks.code_published&&predicate(),"native direct calls become true");
        uint8_t result{};reinterpret_cast<Exec>(f.live<uintptr_t>(0xa0d8))(nullptr,nullptr,&result);
        check(result==1,"installed reflection callback returns true");
        std::array<uint8_t,0x400> object{};
        reinterpret_cast<Navigation>(f.live<uintptr_t>(Fixture::vtable+0x3a0))(object.data(),0);
        check(object[0x14b]==2,"installed navigation callback uses original game setter");
        MEMORY_BASIC_INFORMATION code{},table{};
        VirtualQuery(reinterpret_cast<void*>(f.base+Fixture::predicate),&code,sizeof(code));
        VirtualQuery(reinterpret_cast<void*>(f.base+Fixture::vtable),&table,sizeof(table));
        check(code.Protect==PAGE_EXECUTE_READ&&table.Protect==PAGE_READONLY,"code and vtable page protections restored");
        rejects([&]{hooks.install(module,plan,prepare_callbacks(f.base,plan));},"reinstallation rejected");
        check(hooks.rollback_pointers(),"quiescent fixture pointer cleanup");
        check(predicate(),"native entry never live-restored after publication");
    }
    for(const unsigned fail_at:{1u,2u,3u,9u}) {
        Fixture f;auto module=f.snapshot();auto plan=discover(module);unsigned calls{};
        HookOperations operations;
        operations.protect=[&](void* address,size_t length,DWORD mode,DWORD* old) {
            if(++calls==fail_at)return false;
            return VirtualProtect(address,length,mode,old)!=FALSE;
        };
        Hooks hooks(operations);
        rejects([&]{hooks.install(module,plan,prepare_callbacks(f.base,plan));},"protection failure rejected");
        check(!hooks.code_published&&f.live<uintptr_t>(0xa0d8)==f.base+0x1800&&
            f.live<uintptr_t>(0xa1d8)==f.base+0x1900&&f.live<uintptr_t>(Fixture::vtable+0x3a0)==f.base+Fixture::nav_original,
            "partial failure restores owned pointer slots");
        check(f.live<uint32_t>(Fixture::predicate)==plan.predicate_original,"partial failure preserves native entry");
    }
    {
        Fixture f;auto module=f.snapshot();auto plan=discover(module);bool changed{};HookOperations operations;
        operations.protect=[&](void* address,size_t length,DWORD mode,DWORD* old) {
            const bool ok=VirtualProtect(address,length,mode,old)!=FALSE;
            if(ok&&!changed){changed=true;*reinterpret_cast<uintptr_t*>(f.base+0xa0d8)=reinterpret_cast<uintptr_t>(&original_test_exec);}
            return ok;
        };
        Hooks hooks(operations);rejects([&]{hooks.install(module,plan,prepare_callbacks(f.base,plan));},"stale callback CAS rejected");
        check(f.live<uintptr_t>(0xa0d8)==reinterpret_cast<uintptr_t>(&original_test_exec)&&!hooks.code_published,"competing callback writer preserved");
    }
    {
        Fixture f;auto module=f.snapshot();auto plan=discover(module);unsigned calls{};HookOperations operations;
        operations.protect=[&](void* address,size_t length,DWORD mode,DWORD* old) {
            if(++calls==3) {
                *reinterpret_cast<uintptr_t*>(f.base+0xa0d8)=reinterpret_cast<uintptr_t>(&original_test_exec);
                return false;
            }
            return VirtualProtect(address,length,mode,old)!=FALSE;
        };
        Hooks hooks(operations);rejects([&]{hooks.install(module,plan,prepare_callbacks(f.base,plan));},"partial failure with competing owner rejected");
        check(f.live<uintptr_t>(0xa0d8)==reinterpret_cast<uintptr_t>(&original_test_exec)&&!hooks.code_published,"rollback preserves a later competing writer");
    }
    for(const bool flush_failure:{true,false}) {
        Fixture f;auto module=f.snapshot();auto plan=discover(module);HookOperations operations;unsigned calls{};
        if(flush_failure)operations.flush=[](void*,size_t){return false;};
        else operations.protect=[&](void* address,size_t length,DWORD mode,DWORD* old) {
            if(++calls==10)return false;
            return VirtualProtect(address,length,mode,old)!=FALSE;
        };
        Hooks hooks(operations);rejects([&]{hooks.install(module,plan,prepare_callbacks(f.base,plan));},"post-publication failure reported");
        check(hooks.code_published&&f.live<uint32_t>(Fixture::predicate)==return_true,"published native entry retained until owned child exit");
        hooks.rollback_pointers();
    }
    {
        Fixture f;auto module=f.snapshot();auto plan=discover(module);f.put<uint16_t>(0xa0b6,8);f.sync();Hooks hooks;
        rejects([&]{hooks.install(module,plan,prepare_callbacks(f.base,plan));},"changed live reflection rejected before writes");
        check(f.live<uint32_t>(Fixture::predicate)==plan.predicate_original,"stale live contract causes no native patch");
    }
}
void payload_failure_test() {
    const auto pid=GetCurrentProcessId();
    Handle ready(CreateEventW(nullptr,TRUE,FALSE,event_name(pid,L"Ready").c_str()));
    Handle failed(CreateEventW(nullptr,TRUE,FALSE,event_name(pid,L"Failed").c_str()));
    check(ready.value&&failed.value,"owned payload startup events");
    const auto path=std::filesystem::path(module_path()).parent_path()/payload_filename;
    // Only our project DLL is loaded. This test EXE has no valid UE registry,
    // so the real worker must fail without publishing Ready. Retain the DLL
    // until process exit because its worker may still be returning.
    const auto payload=LoadLibraryW(path.c_str());check(payload!=nullptr,"load built NTE payload in owned test process");
    const HANDLE states[]={ready.value,failed.value};
    check(WaitForMultipleObjects(2,states,FALSE,5000)==WAIT_OBJECT_0+1,"production worker reports unsupported host through Failed event");
    check(WaitForSingleObject(ready,0)==WAIT_TIMEOUT,"failed worker never claims successful installation");
}
void sample_test(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);require(file.good(),"cannot open RVA-layout sample");
    const auto size=file.tellg();require(size>0&&size<=0x40000000,"sample size invalid");
    std::vector<uint8_t> bytes(static_cast<size_t>(size));file.seekg(0);file.read(reinterpret_cast<char*>(bytes.data()),size);
    require(file.good(),"cannot read complete sample");
    ModuleImage module(std::move(bytes),0x140000000);const auto plan=discover_static(module);
    // Independent IDA oracles are test-only. Never included by production.
    check(plan.mobile_predicate==0x8ce29b0,"1.5.5 native cloud predicate oracle");
    check(plan.cloud.exec==0x75d6810&&plan.util_mobile.exec==0x682ee50&&plan.navigation_mobile.exec==0x44549a0,"1.5.5 owner/exec oracles");
    check(plan.navigation_slot==0xe878f18&&plan.navigation_original==0x83fd320,"1.5.5 navigation vtable oracle");
    check(std::find(plan.cloud.candidates.begin(),plan.cloud.candidates.end(),0x1063f150)!=plan.cloud.candidates.end(),"1.5.5 cloud function cache oracle");
    std::cout<<"PASS: NTE 1.5.5 captured-module static discovery; read only, heap binding/UI/input not tested\n";
}
}
int wmain(int argc,wchar_t** argv) {
    try {
        if(argc==3&&std::wstring_view(argv[1])==L"--sample"){sample_test(argv[2]);return 0;}
        require(argc==1,"usage: NteTests [--sample <RVA-layout HTGame.module.memory.bin>]");
        discovery_tests();callback_tests();hook_tests();saved_register_hook_tests();payload_failure_test();
        std::cout<<"PASS: "<<checks<<" NTE fixture checks (discovery, callbacks, ownership and failure cleanup)\n";return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
