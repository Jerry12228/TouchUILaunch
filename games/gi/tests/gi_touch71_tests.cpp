#include "gi_touch71.hpp"
#include <cstring>
#include <map>
#include <limits>

using namespace touchui::gi71;
void require(bool condition,const char* message) {
    if(!condition)throw std::runtime_error(message);
}
template<class F> void rejects(F&& function,const char* message) {
    bool rejected=false;
    try {function();}catch(const std::runtime_error&){rejected=true;}
    require(rejected,message);
}

struct Fixture {
    std::map<uint32_t,uint8_t> bytes;
    size_t writes{};
    int fail_write{-1};
    bool corrupt_write{};
    Fixture() {
        for(const auto& site:sites)
            for(size_t i=0;i<site.expected.size();++i)bytes[site.rva+static_cast<uint32_t>(i)]=site.expected[i];
    }
    std::vector<uint8_t> read(uint32_t rva,size_t size) {
        std::vector<uint8_t> result;
        for(size_t i=0;i<size;++i)result.push_back(bytes.at(rva+static_cast<uint32_t>(i)));
        return result;
    }
    void write_code(uint32_t rva,std::span<const uint8_t> value) {
        if(static_cast<int>(writes++)==fail_write)throw std::runtime_error("simulated write failure");
        for(size_t i=0;i<value.size();++i)bytes.at(rva+static_cast<uint32_t>(i))=value[i];
        if(corrupt_write)bytes.at(rva)^=1;
    }
};

void plan_tests() {
    Fixture valid;const auto before=valid.bytes;
    apply_plan(valid);
    require(valid.writes==sites.size(),"not all UI/input paths patched");
    for(const auto& [rva,byte]:before) {
        auto expected=byte;
        for(const auto& site:sites)
            if(rva>=site.rva+site.offset&&rva-site.rva-site.offset<site.replacement.size())
                expected=site.replacement[rva-site.rva-site.offset];
        require(valid.bytes.at(rva)==expected,"modified unrelated code");
    }
    // A conflict at even the last site must reject before ANY mutation.
    for(const auto& site:sites) {
        Fixture conflict;conflict.bytes[site.rva+static_cast<uint32_t>(site.expected.size()-1)]^=1;
        rejects([&]{apply_plan(conflict);},"instruction conflict accepted");
        require(conflict.writes==0,"wrote before complete validation");
    }
    Fixture failed;failed.fail_write=1;
    rejects([&]{apply_plan(failed);},"write failure ignored");
    require(failed.writes==2,"continued after failed write");
    Fixture corrupt;corrupt.corrupt_write=true;
    rejects([&]{apply_plan(corrupt);},"failed readback accepted");
    require(corrupt.writes==1,"continued after bad readback");
    auto invalid=sites;invalid[0].offset=UINT32_MAX;
    Fixture bounds;rejects([&]{apply_plan(bounds,invalid);},"bad patch bounds accepted");
    invalid=sites;invalid[1].rva=invalid[0].rva;
    rejects([&]{apply_plan(bounds,invalid);},"overlapping sites accepted");
    require(bounds.writes==0,"invalid plan mutated memory");
    require(sha256({})==std::array<uint8_t,32>{
        0xe3,0xb0,0xc4,0x42,0x98,0xfc,0x1c,0x14,0x9a,0xfb,0xf4,0xc8,0x99,0x6f,0xb9,0x24,
        0x27,0xae,0x41,0xe4,0x64,0x9b,0x93,0x4c,0xa4,0x95,0x99,0x1b,0x78,0x52,0xb8,0x55},"SHA256 provider regression");
}

struct Allocation {
    HANDLE process;
    uint8_t* data;
    explicit Allocation(HANDLE value,size_t size=4096):process(value),data(static_cast<uint8_t*>(
        VirtualAllocEx(value,nullptr,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE))) {
        require(data!=nullptr,"fixture allocation failed");
    }
    ~Allocation(){if(data)VirtualFreeEx(process,data,0,MEM_RELEASE);}
};
auto seed(HANDLE process,Allocation& allocation) {
    auto plan=sites;
    for(size_t i=0;i<plan.size();++i) {
        plan[i].rva=static_cast<uint32_t>(i*128);
        SIZE_T written{};
        require(WriteProcessMemory(process,allocation.data+plan[i].rva,plan[i].expected.data(),plan[i].expected.size(),&written)&&
                written==plan[i].expected.size(),"fixture seed failed");
    }
    DWORD old{};
    require(VirtualProtectEx(process,allocation.data,4096,PAGE_EXECUTE_READ,&old)!=0,"fixture protection failed");
    return plan;
}
void execution_tests() {
    // seed protects only the first page as RX. The second page stays RW for
    // the DPI instruction fixture's actual store, separate from executable code.
    Allocation allocation(GetCurrentProcess(),8192);
    const auto plan=seed(GetCurrentProcess(),allocation);
    RemoteCode memory(GetCurrentProcess(),reinterpret_cast<uintptr_t>(allocation.data),4096);
    apply_plan(memory,plan);
    MEMORY_BASIC_INFORMATION info{};
    VirtualQuery(allocation.data,&info,sizeof(info));
    require(info.Protect==PAGE_EXECUTE_READ,"code protection not restored");
    // Test the replaced call window with a leaf continuation. Init's original
    // stack prologue/epilogue and existing refresh calls stay byte-identical.
    constexpr uint8_t ret[]{0xc3};
    using Default=int(*)();
    for(size_t index:{size_t{0},size_t{4}}) {
        const auto at=plan[index].rva+plan[index].offset;
        memory.write_code(at+5,ret);
        require(reinterpret_cast<Default>(allocation.data+at)()==0,"initial UI/input branch did not select touch");
    }
    struct Object {std::array<uint8_t,864> prefix;int mode;int guard;};
    Object object{};object.prefix.fill(0x7e);object.mode=2;object.guard=0x12345678;
    using Setter=void(*)(Object*,int);
    reinterpret_cast<Setter>(allocation.data+plan[3].rva)(&object,1);
    require(object.mode==0&&object.guard==0x12345678&&
            std::all_of(object.prefix.begin(),object.prefix.end(),[](auto b){return b==0x7e;}),"leaf setter corrupted object");
    // Execute each argument substitution with an independent continuation that
    // reports r8d; the game's original refresh/dispatch code is not executed.
    for(size_t index:{size_t{1},size_t{2},size_t{5}}) {
        const auto at=plan[index].rva+plan[index].offset;
        constexpr uint8_t continuation[]{0x44,0x89,0xc0,0xc3}; // mov eax,r8d; ret
        memory.write_code(at+3,continuation);
        using Argument=int(*)(void*,int,int);
        for(int mode:{0,1,2,3,-1,INT_MAX})
            require(reinterpret_cast<Argument>(allocation.data+at)(nullptr,mode,0x77)==0,"setter argument not clamped");
    }
    // Input transition stores edi, not r8d. Save its nonvolatile register in
    // our independent leaf harness and verify the notification flag survives.
    {
        const auto at=plan[6].rva+plan[6].offset;
        constexpr uint8_t save[]{0x57}; // push rdi
        constexpr uint8_t continuation[]{
            0x89,0x79,0x00,        // mov [rcx],edi
            0x44,0x89,0x41,0x04,   // mov [rcx+4],r8d
            0x5f,0xc3};            // pop rdi; ret
        memory.write_code(at-1,save);memory.write_code(at+2,continuation);
        using Transition=void(*)(int*,int,int);
        for(int mode:{0,1,2,3,-1,INT_MAX})for(int notify:{0,1}) {
            int result[]{-1,-1,0x12345678};
            reinterpret_cast<Transition>(allocation.data+at-1)(result,mode,notify);
            require(result[0]==0&&result[1]==notify&&result[2]==0x12345678,"input transition mode/flag regression");
        }
    }
    // The coordinator must clamp both UI (ebx) and input (r14d). Otherwise its
    // inlined UI store and notifications would still request keyboard mode.
    {
        const auto at=plan[7].rva+plan[7].offset;
        constexpr uint8_t save[]{0x53,0x41,0x56}; // push rbx; push r14
        constexpr uint8_t continuation[]{
            0x89,0x19,             // mov [rcx],ebx
            0x44,0x89,0x71,0x04,   // mov [rcx+4],r14d
            0x41,0x5e,0x5b,0xc3}; // pop r14; pop rbx; ret
        memory.write_code(at-3,save);memory.write_code(at+5,continuation);
        using Switch=void(*)(int*,int,int);
        for(int layout:{0,1,2})for(int device:{0,1,2,3}) {
            int result[]{-1,-1,0x12345678};
            reinterpret_cast<Switch>(allocation.data+at-3)(result,layout,device);
            require(result[0]==0&&result[1]==0&&result[2]==0x12345678,"UI and input switch became inconsistent");
        }
    }
    // Execute the actual DPI decision/store window, relocating only its data
    // references to owned memory. Class initialization is already complete in
    // this fixture; no engine call or game data is executed. Compare the
    // original positive-host-DPI branch with the patched built-in fallback.
    {
        const auto at=plan[8].rva;
        constexpr uint32_t dpi_at=4096,pointer_at=3088;
        std::array<uint8_t,208> initialized_class{};initialized_class[199]=1;
        const auto class_pointer=reinterpret_cast<uintptr_t>(initialized_class.data());
        auto put=[&](uint32_t where,const auto& value) {
            memory.write_code(where,{reinterpret_cast<const uint8_t*>(&value),sizeof(value)});
        };
        put(pointer_at,class_pointer);
        put(at+9,static_cast<int32_t>(dpi_at-at-13));
        put(at+24,static_cast<int32_t>(pointer_at-at-28));
        put(at+43,static_cast<int32_t>(dpi_at-at-51));
        constexpr uint8_t continuation[]{0xf3,0x0f,0x10,0x05,0,0,0,0,0xc3};
        memory.write_code(at+51,continuation);
        put(at+55,static_cast<int32_t>(dpi_at-at-59));
        using Dpi=float(*)(float);
        auto evaluate=reinterpret_cast<Dpi>(allocation.data+at+5);
        memory.write_code(at+19,sites[8].expected.subspan(19,2));
        for(float host:{96.0f,144.0f,360.0f,480.0f})
            require(evaluate(host)==host,"original DPI branch fixture is invalid");
        require(evaluate(0.0f)==360.0f,"game DPI fallback is not 360");
        memory.write_code(at+19,sites[8].replacement);
        for(float host:{-1.0f,0.0f,96.0f,144.0f,360.0f,480.0f,
                        std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})
            require(evaluate(host)==360.0f,"touch scale retained invalid or host-specific DPI");
    }
    // Keep the original caption-selection branches and mobile literal load.
    // Replace the Windows/cloud helper path with a keyboard-caption sentinel;
    // the mobile branch must load its own literal, regardless of host platform.
    {
        const auto at=plan[9].rva;
        constexpr uint32_t literal_at=3120;
        constexpr uint64_t touch_literal=42475;
        memory.write_code(literal_at,{reinterpret_cast<const uint8_t*>(&touch_literal),sizeof(touch_literal)});
        const int32_t displacement=static_cast<int32_t>(literal_at-at-102);
        memory.write_code(at+98,{reinterpret_cast<const uint8_t*>(&displacement),sizeof(displacement)});
        constexpr uint8_t save[]{0x56}; // push rsi
        constexpr uint8_t continuation[]{0x48,0x89,0xf0,0x5e,0xc3}; // mov rax,rsi; pop rsi; ret
        constexpr uint8_t desktop[]{0xb8,0xea,0xa5,0x00,0x00,0x5e,0xc3}; // keyboard literal 42474
        memory.write_code(at-1,save);memory.write_code(at+105,continuation);
        memory.write_code(at+62,desktop);
        using Caption=uint64_t(*)();
        auto evaluate=reinterpret_cast<Caption>(allocation.data+at-1);
        constexpr uint8_t windows_platform[]{0xb8,0x02,0x00,0x00,0x00};
        memory.write_code(at,windows_platform);
        require(evaluate()==42474,"original Windows caption fixture is invalid");
        memory.write_code(at,sites[9].replacement);
        require(evaluate()==touch_literal,"settings caption did not take touchscreen branch");
    }
    rejects([&]{memory.read(4095,2);},"out-of-image read accepted");
}
void child_tests() {
    // This EXE is the only process these tests start. A failed initialization
    // must keep the main thread suspended and let Child terminate its own PID.
    HANDLE observe{};
    {
        game::Child child;child.start(module_path(),true,L"--child");
        require(DuplicateHandle(GetCurrentProcess(),child.info.hProcess,GetCurrentProcess(),&observe,
                                SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,0)!=0,"child observer failed");
        const auto base=child_image_base(child);
        uint16_t magic{};read_process(child.info.hProcess,base,&magic,sizeof(magic));
        require(magic==0x5a4d,"suspended child base was not ASLR-rebased EXE");
        Allocation allocation(child.info.hProcess);
        auto plan=seed(child.info.hProcess,allocation);
        RemoteCode memory(child.info.hProcess,reinterpret_cast<uintptr_t>(allocation.data),4096);
        apply_plan(memory,plan);
        require(WaitForSingleObject(child.info.hProcess,0)==WAIT_TIMEOUT,"child ran before resume");
        // An already modified target is a conflict, not an implicit success.
        rejects([&]{apply_plan(memory,plan);},"duplicate initialization accepted");
    }
    Handle observer(observe);
    require(WaitForSingleObject(observer,5000)==WAIT_OBJECT_0,"failed child leaked");
    DWORD code{};GetExitCodeProcess(observer,&code);require(code==2,"failed child was resumed");
    game::Child success;success.start(module_path(),true,L"--child");
    success.resume();
    require(WaitForSingleObject(success.info.hProcess,5000)==WAIT_OBJECT_0,"owned child failed to resume");
    GetExitCodeProcess(success.info.hProcess,&code);require(code==71,"unexpected child exit");success.release();
}
int wmain(int argc,wchar_t** argv) {
    try {
        if(argc==2&&std::wstring_view(argv[1])==L"--child")return 71;
        if(argc==3&&std::wstring_view(argv[1])==L"--sample") {
            VerifiedImage image(argv[2]);
            std::cout<<"PASS: GI 7.1 file hash, PE ranges and "<<sites.size()<<" independent UI/input sites (read-only). No game launched.\n";
            return 0;
        }
        require(argc==1,"usage: GITouch71Tests [--sample <exe>]");
        plan_tests();execution_tests();child_tests();
        rejects([]{VerifiedImage unsupported(module_path());},"unsupported executable accepted");
        std::cout<<"PASS: GI UI plan, conflicts, failure propagation, x64 execution and owned suspended child.\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
