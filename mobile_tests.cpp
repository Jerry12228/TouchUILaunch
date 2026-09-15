#include "mobile_runtime.hpp"
#include "test_module.hpp"
#include <fstream>
#include <iostream>

namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F> void rejects(F fn,const char* message) {
    bool failed{};try {fn();}catch(const std::exception&){failed=true;}check(failed,message);
}
struct Fixture {
    std::vector<uint8_t> bytes=std::vector<uint8_t>(0x2800);
    template<class T> void raw(size_t at,T value){std::memcpy(bytes.data()+at,&value,sizeof(value));}
    size_t file(uintptr_t rva)const{return rva<0x3000?rva-0x1000+0x400:rva-0x3000+0x2400;}
    template<class T> void put(uintptr_t at,T value){raw(file(at),value);}
    Fixture() {
        raw<uint16_t>(0,0x5a4d);raw<uint32_t>(0x3c,0x80);raw<uint32_t>(0x80,0x4550);
        raw<uint16_t>(0x84,0x8664);raw<uint16_t>(0x86,2);raw<uint16_t>(0x94,0xf0);
        raw<uint16_t>(0x98,0x20b);raw<uint32_t>(0x98+56,0x4000);raw<uint32_t>(0x98+60,0x400);
        const size_t table=0x188;
        std::memcpy(bytes.data()+table,"il2cpp",6);
        raw<uint32_t>(table+8,0x2000);raw<uint32_t>(table+12,0x1000);raw<uint32_t>(table+16,0x2000);raw<uint32_t>(table+20,0x400);raw<uint32_t>(table+36,0x60000020);
        std::memcpy(bytes.data()+table+40,".data",5);
        raw<uint32_t>(table+48,0x1000);raw<uint32_t>(table+52,0x3000);raw<uint32_t>(table+56,0x400);raw<uint32_t>(table+60,0x2400);raw<uint32_t>(table+76,0xc0000040);
        std::fill(bytes.begin()+0x400,bytes.begin()+0x2400,uint8_t{0x90});
    }
    void pattern(uintptr_t at,const mobile::Pattern& pattern){std::copy(pattern.bytes.begin(),pattern.bytes.end(),bytes.begin()+file(at));}
    void rel(uintptr_t displacement,uintptr_t next,uintptr_t target){put<int32_t>(displacement,static_cast<int32_t>(int64_t(target)-int64_t(next)));}
    void sr(size_t variant,uintptr_t at=0x1000,uintptr_t target=0x3000) {
        pattern(at,mobile::sr_patterns[variant]);constexpr uintptr_t offsets[]={13,9,7};
        const auto op=at+offsets[variant];rel(op+2,op+10,target);
    }
    void gi(size_t variant=0) {
        pattern(0x1000,mobile::gi_patterns[variant]);rel(0x1003,0x1007,0x3000);put<uint32_t>(0x100a,0x20);
        const uintptr_t ui=0x1000+(variant?28:31),input=0x1000+(variant?41:47);
        rel(ui+1,ui+5,0x1500);rel(input+1,input+5,0x1600);
        pattern(0x1100,mobile::gi_input);rel(0x1103,0x1107,0x3000);put<uint32_t>(0x1110,0x28);
        pattern(0x1200,mobile::gi_init);rel(0x1209,0x120d,0x1700);
    }
    mobile::GiResolution resolve_gi()const{return mobile::resolve_gi(discovery::Image(bytes));}
    uintptr_t resolve_sr()const{return mobile::resolve_sr(discovery::Image(bytes));}
};
void resolver_tests() {
    for(size_t variant=0;variant<3;++variant) {
        Fixture f;f.sr(variant);check(f.resolve_sr()==0x3000,"SR reference variant resolves to data");
        test_module::Mapping memory(f.bytes);
        check(mobile::resolve_sr(mobile::capture(GetCurrentProcess(),memory.base))==0x3000,"SR file/loaded RVA agreement");
        f.sr(variant,0x1100);rejects([&]{f.resolve_sr();},"duplicate SR signature rejected");
    }
    for(size_t variant=0;variant<2;++variant) {
        Fixture f;f.gi(variant);const auto r=f.resolve_gi();
        check(r.init==0x1700&&r.ui==0x1500&&r.input==0x1600&&r.klass==0x3000&&r.ui_offset==0x20&&r.input_offset==0x28,"GI variant fields");
        test_module::Mapping memory(f.bytes);memory.put<uintptr_t>(0x3000,0x12345678);
        const auto loaded=mobile::resolve_gi(mobile::capture(GetCurrentProcess(),memory.base));
        check(loaded.init==r.init&&loaded.klass==r.klass,"GI file/loaded agreement with nonzero class slot");
        f.pattern(0x1300,mobile::gi_init);rejects([&]{f.resolve_gi();},"duplicate GI initialization signature rejected");
    }
    Fixture empty;rejects([&]{empty.resolve_sr();},"missing SR signature rejected");rejects([&]{empty.resolve_gi();},"missing GI signature rejected");
    Fixture variants;variants.sr(0);variants.sr(1,0x1100);rejects([&]{variants.resolve_sr();},"conflicting SR variants rejected");
    for(auto target:{uintptr_t(0x1000),uintptr_t(0x3001),uintptr_t(0x4000)}) {
        Fixture f;f.sr(2,0x1000,target);rejects([&]{f.resolve_sr();},"invalid SR write target rejected");
    }
    Fixture gi;gi.gi();gi.put<uint32_t>(0x100a,3);rejects([&]{gi.resolve_gi();},"misaligned GI object offset rejected");
    gi.gi();gi.rel(0x1209,0x120d,0x4000);rejects([&]{gi.resolve_gi();},"out-of-image GI function rejected");
    gi.gi();gi.put<uint8_t>(0x1700,0xe9);rejects([&]{gi.resolve_gi();},"existing GI hook rejected");
    // Real GI 7.0 has repeated call/access sites with identical targets.
    Fixture repeated;repeated.gi();const auto expected=repeated.resolve_gi();
    repeated.pattern(0x1800,mobile::gi_patterns[0]);repeated.rel(0x1803,0x1807,0x3000);repeated.put<uint32_t>(0x180a,0x20);
    repeated.rel(0x1820,0x1824,0x1500);repeated.rel(0x1830,0x1834,0x1600);
    for(const auto at:{uintptr_t(0x1900),uintptr_t(0x1a00),uintptr_t(0x1b00)}) {
        repeated.pattern(at,mobile::gi_input);repeated.rel(at+3,at+7,0x3000);repeated.put<uint32_t>(at+16,0x28);
    }
    repeated.pattern(0x1c00,mobile::gi_init);repeated.rel(0x1c09,0x1c0d,0x1700);
    check(repeated.resolve_gi()==expected,"equivalent GI sites collapse to one verified target set");
    test_module::Mapping repeated_memory(repeated.bytes);
    check(mobile::resolve_gi(mobile::capture(GetCurrentProcess(),repeated_memory.base))==expected,"repeated GI sites resolve in loaded memory");
    repeated.rel(0x1820,0x1824,0x1550);rejects([&]{repeated.resolve_gi();},"different UI targets still rejected");
    repeated.rel(0x1820,0x1824,0x1500);repeated.rel(0x1903,0x1907,0x3008);
    rejects([&]{repeated.resolve_gi();},"different input class slot rejected");
    repeated.rel(0x1903,0x1907,0x3000);repeated.put<uint32_t>(0x1910,0x30);
    rejects([&]{repeated.resolve_gi();},"different input offset rejected");
    repeated.put<uint32_t>(0x1910,0x28);repeated.rel(0x1c09,0x1c0d,0x1750);
    rejects([&]{repeated.resolve_gi();},"different initializer targets still rejected");
    Fixture section;section.sr(0);std::memcpy(section.bytes.data()+0x188,".text\0",6);rejects([&]{section.resolve_sr();},"unrelated code section ignored");
    Fixture truncated;truncated.sr(0);truncated.bytes.resize(64);rejects([&]{truncated.resolve_sr();},"truncated image rejected");
}
std::vector<int> calls;
uintptr_t callback_target{};
std::array<uint8_t,16> expected_original{};
int ui_object{},input_object{};
bool callbacks_ok=true;
uintptr_t original(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d) {
    calls.push_back(1);
    callbacks_ok=callbacks_ok&&a==11&&b==22&&c==33&&d==44&&std::memcmp(reinterpret_cast<void*>(callback_target),expected_original.data(),16)==0;
    return 0x123456789abcdef0;
}
void ui_set(void* object,int type,uintptr_t notify) {
    calls.push_back(2);callbacks_ok=callbacks_ok&&object==&ui_object&&type==0&&notify==1;
}
void input_set(void* object,int type,uintptr_t extra) {
    calls.push_back(3);callbacks_ok=callbacks_ok&&object==&input_object&&type==0&&extra==0;
}
void thunk(test_module::Mapping& mapping,uintptr_t rva,uintptr_t target) {
    uint8_t code[16]={0x48,0xb8};std::memcpy(code+2,&target,8);code[10]=0xff;code[11]=0xe0;
    std::memcpy(reinterpret_cast<void*>(mapping.base+rva),code,sizeof(code));
}
void gi_runtime_test(bool missing) {
    Fixture f;f.gi();const auto resolved=f.resolve_gi();test_module::Mapping mapping(f.bytes);
    thunk(mapping,0x1700,reinterpret_cast<uintptr_t>(&original));thunk(mapping,0x1500,reinterpret_cast<uintptr_t>(&ui_set));thunk(mapping,0x1600,reinterpret_cast<uintptr_t>(&input_set));
    std::array<uintptr_t,6> klass{};klass[4]=reinterpret_cast<uintptr_t>(&ui_object);klass[5]=reinterpret_cast<uintptr_t>(&input_object);
    mapping.put<uintptr_t>(0x3000,missing?0:reinterpret_cast<uintptr_t>(klass.data()));
    callback_target=mapping.base+0x1700;std::memcpy(expected_original.data(),reinterpret_cast<void*>(callback_target),16);
    mapping.protect(0x1000,0x2000,PAGE_EXECUTE_READ);
    calls.clear();callbacks_ok=true;
    const auto context=mobile::install_gi(GetCurrentProcess(),mapping.base,resolved);
    using Fn=uintptr_t(*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);
    check(reinterpret_cast<Fn>(callback_target)(11,22,33,44)==0x123456789abcdef0,"GI preserves original return");
    check(callbacks_ok&&calls==(missing?std::vector<int>{1}:std::vector<int>{1,2,3}),"GI restores original, calls original then setters with reference arguments");
    const auto state=mobile::read<mobile::GiContext>(GetCurrentProcess(),context);
    check(state.state==2&&state.outcome==(missing?2:1),"GI status distinguishes applied from unready objects");
    MEMORY_BASIC_INFORMATION region{};VirtualQuery(reinterpret_cast<void*>(callback_target),&region,sizeof(region));
    check(region.Protect==PAGE_EXECUTE_READ,"GI target protection restored");
    calls.clear();reinterpret_cast<Fn>(callback_target)(11,22,33,44);check(calls==std::vector<int>{1},"GI hook runs only once");
    VirtualFree(reinterpret_cast<void*>(context-0x1000),0,MEM_RELEASE);
}
void sr_runtime_test() {
    Fixture f;f.sr(0);test_module::Mapping mapping(f.bytes);mapping.put<uint32_t>(0x3000,3);
    mapping.put<uint32_t>(0x3004,0xfeedbeef);
    const auto installed=mobile::install_sr(GetCurrentProcess(),mapping.base,0x3000);
    Handle thread(OpenThread(SYNCHRONIZE,FALSE,installed.thread_id));check(thread.value!=nullptr,"open owned SR task");
    auto* state=reinterpret_cast<volatile LONG*>(mapping.base+0x3000);
    const bool initial=*state==2;InterlockedExchange(state,3);
    for(int i=0;i<100&&*state!=2;++i)Sleep(10);
    const bool reapplied=*state==2;
    InterlockedExchange(reinterpret_cast<volatile LONG*>(installed.context+offsetof(mobile::SrContext,stop)),1);
    check(WaitForSingleObject(thread,2000)==WAIT_OBJECT_0,"owned SR task stops before freeing memory");
    check(initial&&reapplied&&mobile::read<uint32_t>(GetCurrentProcess(),mapping.base+0x3004)==0xfeedbeef,"SR periodically writes only the UI state");
    VirtualFree(reinterpret_cast<void*>(installed.context-0x1000),0,MEM_RELEASE);
}
void child_tests() {
    const auto path=std::filesystem::path(module_path()).parent_path()/L"MobileUITestChild.exe";
    HANDLE owned{};
    {
        game::Child child;child.start(path,true);
        check(DuplicateHandle(GetCurrentProcess(),child.info.hProcess,GetCurrentProcess(),&owned,0,FALSE,DUPLICATE_SAME_ACCESS),"retain owned child handle");
        mobile::bootstrap(child.info.hProcess);
        const auto name=L"Local\\MobileUITests.Main."+std::to_wstring(child.info.dwProcessId);
        Handle marker(OpenEventW(SYNCHRONIZE,FALSE,name.c_str()));check(!marker.value,"game main did not run during bootstrap");
        wchar_t system[MAX_PATH]{};GetSystemDirectoryW(system,MAX_PATH);
        check(mobile::load_library(child.info.hProcess,std::filesystem::path(system)/L"version.dll")!=0,"remote LoadLibrary returns full module address");
        rejects([&]{mobile::load_library(child.info.hProcess,path.parent_path()/L"missing-module.dll");},"missing module fails without resuming main");
    }
    Handle failed(owned);DWORD code{};check(WaitForSingleObject(failed,5000)==WAIT_OBJECT_0&&GetExitCodeProcess(failed,&code)&&code==2,"failure cleanup terminates only the owned suspended child");
    {
        game::Child child;child.start(path,true);child.resume();child.release();
        check(WaitForSingleObject(child.info.hProcess,5000)==WAIT_OBJECT_0&&GetExitCodeProcess(child.info.hProcess,&code)&&code==19,"successful child resumes and exits normally");
    }
    {
        game::Child child;child.start(path,false,L"-CloudGame -CloudGamePlatform=Android");child.release();
        check(WaitForSingleObject(child.info.hProcess,5000)==WAIT_OBJECT_0&&GetExitCodeProcess(child.info.hProcess,&code)&&code==0,
              "WW child receives exact cloud arguments and executable working directory, without suspended initialization");
        std::filesystem::remove(path.parent_path()/L"ww-launch-passed.txt");
    }
    for(auto kind:{game::Kind::GI,game::Kind::SR,game::Kind::ZZZ,game::Kind::WW}) {
        std::optional<game::Kind> selected;game::select(selected,kind);check(*selected==kind,"explicit selection");
        rejects([&]{game::select(selected,kind);},"duplicate selection rejected");
        rejects([&]{game::validate(selected,path,true,false);},"wrong executable rejected");
    }
    check(game::accepts(game::Kind::GI,L"YuanShen.exe")&&game::accepts(game::Kind::GI,L"genshinimpact.EXE")&&game::accepts(game::Kind::SR,L"StarRail.exe"),"supported filenames");
    rejects([]{game::require_stopped(true);},"running game rejected before creation");game::require_stopped(false);
}
}
int wmain(int argc,wchar_t** argv) {
    try {
        if(argc==2||(argc==3&&std::wstring_view(argv[1])==L"--gi-file")) {
            const bool gi=argc==3;
            std::ifstream file(std::filesystem::path(argv[gi?2:1]),std::ios::binary);check(file.good(),"cannot read game sample");
            const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)),{});
            if(gi) {
                const auto resolved=mobile::resolve_gi(discovery::Image(bytes));
                std::cout<<"GI file: init=0x"<<std::hex<<resolved.init<<" ui=0x"<<resolved.ui<<" input=0x"<<resolved.input
                    <<" class=0x"<<resolved.klass<<" ui_offset=0x"<<resolved.ui_offset<<" input_offset=0x"<<resolved.input_offset
                    <<" (read only; no game code executed)\n";return 0;
            }
            std::cout<<"SR sample UI state RVA=0x"<<std::hex<<mobile::resolve_sr(discovery::Image(bytes))<<" (read only; no game code executed)\n";return 0;
        }
        resolver_tests();gi_runtime_test(false);gi_runtime_test(true);sr_runtime_test();child_tests();
        std::cout<<"PASS: GI/SR resolver variants, UI-only execution, suspended startup and owned-child cleanup\n";return 0;
    } catch(const std::exception& ex){std::cerr<<"FAIL: "<<ex.what()<<"\n";return 1;}
}
