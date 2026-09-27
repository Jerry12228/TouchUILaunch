#include "gi_touch.hpp"
#include "gi_pattern_fixture.hpp"
#include <map>
#include <cmath>

using namespace touchui::gi;
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
template<class F> void rejects(F&& function, const char* message) {
    bool rejected = false;
    try { function(); } catch (const std::runtime_error&) { rejected = true; }
    require(rejected, message);
}

// A different PE layout and field layout from the game, without client code
// execution. Relative instruction operands point to independently seeded sections.
struct ImageFixture {
    std::vector<uint8_t> bytes = std::vector<uint8_t>(0x9000, 0xcc);
    std::vector<uint32_t> locations;
    template<class T> void put(size_t at, T value) { std::memcpy(bytes.data()+at, &value, sizeof(value)); }
    explicit ImageFixture(uint32_t slide = 0, uint32_t field_shift = 0) {
        std::fill(bytes.begin(), bytes.begin()+0x1000, uint8_t{0});
        put<uint16_t>(0,0x5a4d); put<uint32_t>(0x3c,0x80); put<uint32_t>(0x80,0x4550);
        put<uint16_t>(0x84,0x8664); put<uint16_t>(0x86,2); put<uint16_t>(0x94,0xf0);
        put<uint16_t>(0x98,0x20b); put<uint32_t>(0x98+56,0x9000); put<uint32_t>(0x98+60,0x1000);
        auto section = [&](size_t at,uint32_t rva,uint32_t size,uint32_t flags) {
            put<uint32_t>(at+8,size); put<uint32_t>(at+12,rva); put<uint32_t>(at+16,size);
            put<uint32_t>(at+20,rva); put<uint32_t>(at+36,flags);
        };
        section(0x188,0x1000,0x6000,0x60000020); section(0x1b0,0x7000,0x2000,0xc0000040);
        bytes[0x1100]=0xc3;
        for (size_t index=0; index<gi_fixture::windows.size(); ++index) {
            const auto at=index==13 ? locations[12]+static_cast<uint32_t>(gi_fixture::windows[12].size()) :
                static_cast<uint32_t>(0x1400+index*0x200+slide);
            locations.push_back(at);
            auto window=gi_fixture::windows[index];
            for (size_t offset=0; offset<window.size();) {
                std::array<uint8_t,32> padded{};
                std::copy_n(window.data()+offset,std::min<size_t>(15,window.size()-offset),padded.data());
                discovery::x64::Ins ins;ins.at=at+offset;hde64_disasm(padded.data(),&ins.h);
                require(ins.h.len && ins.h.len<=window.size()-offset,"fixture decode failed");
                if (ins.base()==discovery::x64::rip) {
                    const auto delta=static_cast<int32_t>(0x7200-ins.next());
                    std::memcpy(window.data()+offset+ins.h.len-ins.immediate_size()-4,&delta,4);
                } else if (field_shift && index<8 && ins.base()>=0 && ins.base()!=4 && ins.displacement_size()==4) {
                    const auto displacement=ins.disp();
                    if (displacement==864 || displacement==372) {
                        const auto changed=displacement+field_shift;
                        std::memcpy(window.data()+offset+ins.h.len-ins.immediate_size()-4,&changed,4);
                    }
                } else if (field_shift && index>=10 && ins.base()>=0 && ins.base()!=4 && ins.displacement_size()) {
                    // A second gesture/joystick object layout, independent of
                    // the UI/input fields and of the production generator.
                    const auto changed=ins.disp()+4;
                    if (ins.displacement_size()==1) window[offset+ins.h.len-ins.immediate_size()-1]=static_cast<uint8_t>(changed);
                    else std::memcpy(window.data()+offset+ins.h.len-ins.immediate_size()-4,&changed,4);
                }
                if (ins.call() || ins.jump() || ins.conditional()) {
                    const auto delta=static_cast<int32_t>(0x1100-ins.next());
                    // Preserve local short-branch topology; relocate external calls.
                    if (ins.immediate_size()==4) std::memcpy(window.data()+offset+ins.h.len-4,&delta,4);
                }
                offset+=ins.h.len;
            }
            std::copy(window.begin(),window.end(),bytes.begin()+at);
        }
    }
    discovery::Image image() const { return discovery::Image(bytes); }
};

struct MemoryFixture {
    std::map<uint32_t,uint8_t> bytes;
    size_t writes{};
    int fail_write{-1};
    bool corrupt_write{};
    explicit MemoryFixture(const Plan& plan) {
        for (const auto& site:plan) for (size_t i=0;i<site.expected.size();++i)
            bytes[site.rva+static_cast<uint32_t>(i)]=site.expected[i];
    }
    std::vector<uint8_t> read(uint32_t rva,size_t size) {
        std::vector<uint8_t> out;
        for (size_t i=0;i<size;++i) out.push_back(bytes.at(rva+static_cast<uint32_t>(i)));
        return out;
    }
    void write_code(uint32_t rva,std::span<const uint8_t> value) {
        if (static_cast<int>(writes++)==fail_write) throw std::runtime_error("simulated write failure");
        for (size_t i=0;i<value.size();++i) bytes.at(rva+static_cast<uint32_t>(i))=value[i];
        if (corrupt_write) bytes.at(rva)^=1;
    }
};

void discovery_tests() {
    for (auto slide:{0u,0x70u,0x120u}) for (auto shift:{0u,48u}) {
        ImageFixture fixture(slide,shift);
        auto plan=resolve(fixture.image());
        require(plan.size()==11,"unexpected UI/joystick patch count");
        for (size_t i=0;i<plan.size();++i) require(plan[i].rva==fixture.locations[i],"discovery used a fixed RVA");
        uint32_t stored{};std::memcpy(&stored,plan[3].replacement.data()+2,4);
        require(stored==864+shift,"leaf setter used a fixed field offset");
        uint32_t width_field{};std::memcpy(&width_field,plan[10].expected.data()+12,4);
        require(width_field==172u+(shift?4u:0u),"joystick field did not move");
        // Simulate the loaded PE at a different base. The image reader receives
        // RVAs and must derive exactly the same plan from its captured code.
        discovery::Image loaded(std::span(fixture.bytes).first(0x1000),[&](uintptr_t rva,std::span<uint8_t> out) {
            require(rva+out.size()<=fixture.bytes.size(),"snapshot escaped fixture");
            std::copy_n(fixture.bytes.data()+rva,out.size(),out.data());
        });
        const auto from_memory=resolve(loaded);
        for (size_t i=0;i<plan.size();++i)
            require(from_memory[i].rva==plan[i].rva && from_memory[i].expected==plan[i].expected,
                    "file/loaded discovery disagreed");
        for (auto& site:plan) if (site.hook!=Hook::none)
            site.replacement=call_patch(site.rva+site.offset,0x6800,site.replacement.size());
        MemoryFixture memory(plan);apply_plan(memory,plan);
        require(memory.writes==11,"not all discovered sites applied");
        for (const auto& site:plan) {
            MemoryFixture conflict(plan);conflict.bytes.at(site.rva+static_cast<uint32_t>(site.expected.size()-1))^=1;
            rejects([&]{apply_plan(conflict,plan);},"conflicting instructions accepted");
            require(conflict.writes==0,"wrote before validating entire plan");
        }
        MemoryFixture fail(plan);fail.fail_write=1;
        rejects([&]{apply_plan(fail,plan);},"write failure ignored");require(fail.writes==2,"continued writing after failure");
        MemoryFixture corrupt(plan);corrupt.corrupt_write=true;
        rejects([&]{apply_plan(corrupt,plan);},"readback failure ignored");
    }
    ImageFixture fixture;
    auto plan=resolve(fixture.image());
    MemoryFixture unprepared(plan);
    rejects([&]{apply_plan(unprepared,plan);},"unprepared hooks accepted");
    require(unprepared.writes==0,"wrote before validating helper readiness");
    for (size_t i=0;i<plan.size();++i) {
        auto missing=fixture;missing.bytes[missing.locations[i]]=0xcc;
        rejects([&]{resolve(missing.image());},"missing signature accepted");
        auto duplicate=fixture;
        std::copy(plan[i].expected.begin(),plan[i].expected.end(),duplicate.bytes.begin()+0x6400);
        // Use a duplicate without relative addressing to test candidate ambiguity.
        if (i==3) rejects([&]{resolve(duplicate.image());},"ambiguous target accepted");
    }
    // Gesture producers are not part of discovery or patching anymore.
    auto without_gestures=fixture;
    for (size_t i=11;i<gi_fixture::windows.size();++i)
        std::fill_n(without_gestures.bytes.begin()+without_gestures.locations[i],gi_fixture::windows[i].size(),uint8_t{0xcc});
    const auto without_plan=resolve(without_gestures.image());
    require(without_plan.size()==plan.size(),"removed gesture signatures still required");
    for (size_t i=0;i<plan.size();++i)
        require(without_plan[i].rva==plan[i].rva,"gesture producers affected UI/joystick discovery");
    auto disagreement=fixture;
    disagreement.put<uint32_t>(disagreement.locations[2]+46,868);
    rejects([&]{resolve(disagreement.image());},"disagreeing UI fields accepted");
    auto escaped=fixture;escaped.put<int32_t>(escaped.locations[0]+29,INT32_MAX);
    rejects([&]{resolve(escaped.image());},"out-of-image call accepted");
    auto wrong_dpi=fixture;wrong_dpi.put<uint32_t>(wrong_dpi.locations[8]+47,0x43f00000);
    rejects([&]{resolve(wrong_dpi.image());},"unknown DPI constant accepted");
    for (const auto [site, offset] : {std::pair{8u,20u},std::pair{9u,9u},std::pair{9u,44u},std::pair{9u,12u}}) {
        auto branch=fixture;branch.bytes[branch.locations[site]+offset]^=1;
        rejects([&]{resolve(branch.image());},"changed branch semantics accepted");
    }
    auto overlap=plan;overlap[1].rva=overlap[0].rva;
    rejects([&]{validate_sites(overlap);},"overlapping sites accepted");
    auto bad=plan;bad[0].offset=UINT32_MAX;
    rejects([&]{validate_sites(bad);},"invalid patch range accepted");
    rejects([]{call_patch(0x1000,0x100000000,8);},"out-of-range CALL accepted");
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
auto seed(HANDLE process,Allocation& allocation,bool with_joystick=false) {
    auto plan=resolve(ImageFixture{}.image());
    if (!with_joystick) plan.resize(10);
    for (size_t i=0;i<plan.size();++i) plan[i].expected=gi_fixture::windows[i];
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
        memory.write_code(at+19,std::span(plan[8].expected).subspan(19,2));
        for(float host:{96.0f,144.0f,360.0f,480.0f})
            require(evaluate(host)==host,"original DPI branch fixture is invalid");
        require(evaluate(0.0f)==360.0f,"game DPI fallback is not 360");
        memory.write_code(at+19,plan[8].replacement);
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
        memory.write_code(at,plan[9].replacement);
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
        auto plan=seed(child.info.hProcess,allocation,true);
        RemoteCode memory(child.info.hProcess,reinterpret_cast<uintptr_t>(allocation.data),4096);
        JoystickCode helpers(child.info.hProcess,reinterpret_cast<uintptr_t>(allocation.data));
        helpers.prepare(plan,reinterpret_cast<uintptr_t>(allocation.data));
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

void joystick_and_unmodified_gesture_tests() {
    Allocation allocation(GetCurrentProcess());
    auto plan=resolve(ImageFixture{}.image());
    for (size_t i=0;i<gi_fixture::windows.size();++i) {
        if (i<plan.size()) {
            plan[i].rva=static_cast<uint32_t>(i*128);
            plan[i].expected=gi_fixture::windows[i];
        }
        std::memcpy(allocation.data+i*128,gi_fixture::windows[i].data(),gi_fixture::windows[i].size());
    }
    DWORD previous{};
    require(VirtualProtect(allocation.data,4096,PAGE_EXECUTE_READ,&previous)!=0,"gesture fixture protection failed");
    const auto base=reinterpret_cast<uintptr_t>(allocation.data);
    RemoteCode memory(GetCurrentProcess(),base,4096);
    auto helpers=std::make_unique<JoystickCode>(GetCurrentProcess(),base);
    helpers->prepare(plan,base);apply_plan(memory,plan);
    for (size_t i=11;i<gi_fixture::windows.size();++i)
        require(memory.read(static_cast<uint32_t>(i*128),gi_fixture::windows[i].size())==gi_fixture::windows[i],
                "gesture producer was modified by the patch plan");
    int32_t relative{};std::memcpy(&relative,plan[10].replacement.data()+1,4);
    const auto helper_target=base+plan[10].rva+plan[10].offset+5+static_cast<intptr_t>(relative);
    MEMORY_BASIC_INFORMATION helper_info{};
    VirtualQuery(reinterpret_cast<void*>(helper_target),&helper_info,sizeof(helper_info));
    require(helper_info.Protect==PAGE_EXECUTE_READ,"helper code is not RX");
    constexpr uint8_t ret[]{0xc3};
    constexpr std::array<uint32_t,4> offsets{8,28,22,19}, lengths{8,8,9,6};
    const auto window_at=[&](size_t index) { return static_cast<uint32_t>(index*128)+offsets[index-10]; };
    for (size_t index=10;index<14;++index) {
        memory.write_code(window_at(index)+lengths[index-10],ret);
    }
    // Independent Win64 harnesses arrange the game's register inputs. Each
    // calls the patched joystick window or an UNMODIFIED gesture copy/store,
    // then restores its caller's registers. No game function is executed.
    auto wrapper=[&](uint32_t at,size_t index,std::initializer_list<uint8_t> before,
                     std::initializer_list<uint8_t> after) {
        std::vector<uint8_t> code(before);
        auto call=call_patch(base+at+code.size(),base+window_at(index),5);
        code.insert(code.end(),call.begin(),call.end());code.insert(code.end(),after);
        memory.write_code(at,code);
        return allocation.data+at;
    };
    using Joystick=void(*)(void*,float);
    auto joystick=reinterpret_cast<Joystick>(wrapper(2048,10,
        {0x56,0x48,0x89,0xce,0x0f,0x28,0xc1}, // save rsi; rsi=rcx; xmm0=xmm1
        {0x5e,0xc3}));
    using Finger=uint64_t(*)(void*,const void*);
    auto finger=reinterpret_cast<Finger>(wrapper(2176,11,
        {0x56,0x48,0x89,0xc8,0x48,0x89,0xd6}, // save rsi; rax=rcx; rsi=rdx
        {0x48,0x89,0xc8,0x5e,0xc3})); // return rcx from original copy
    using Two=uint64_t(*)(void*,uint64_t,const float*);
    auto two=reinterpret_cast<Two>(wrapper(2304,12,
        {0x53,0x41,0x54,0x48,0x83,0xec,0x18,0x0f,0x11,0x3c,0x24, // save rbx/r12/xmm7
         0x48,0x89,0xcb,0x49,0x89,0xd4,0xf3,0x41,0x0f,0x10,0x38}, // rbx=rcx;r12=rdx;xmm7=[r8]
        {0x4c,0x89,0xe0,0x0f,0x10,0x3c,0x24,0x48,0x83,0xc4,0x18,0x41,0x5c,0x5b,0xc3}));
    using Pinch=float(*)(void*,float);
    auto pinch=reinterpret_cast<Pinch>(wrapper(2432,13,
        {0x53,0x48,0x83,0xec,0x10,0x44,0x0f,0x11,0x0c,0x24, // save rbx/xmm9
         0x48,0x89,0xcb,0x44,0x0f,0x28,0xc9}, // rbx=rcx;xmm9=xmm1
        {0x41,0x0f,0x28,0xc1,0x44,0x0f,0x10,0x0c,0x24,0x48,0x83,0xc4,0x10,0x5b,0xc3}));
    using Object=std::array<uint8_t,256>;
    const auto put=[](Object& object,size_t offset,const auto& value) {
        std::memcpy(object.data()+offset,&value,sizeof(value));
    };
    for (float width:{0.5f,4.0f,10.0f}) {
        Object actual;actual.fill(0x7e);auto expected=actual;
        put(expected,172,width);joystick(actual.data(),width);
        require(actual==expected,"joystick baseline width or neighboring fields changed");
    }
    for (const auto delta: {std::array{0.0f,0.0f},std::array{16.0f,-32.0f},std::array{-4.0f,8.0f}}) {
        Object source;source.fill(0x6d);put(source,88,delta);const auto saved=source;
        uint64_t packed{};std::memcpy(&packed,delta.data(),8);
        for (int repeat=0;repeat<4;++repeat) {
            Object actual;actual.fill(0x7e);auto expected=actual;put(expected,88,delta);
            require(finger(actual.data(),source.data())==packed,"finger copy register changed");
            require(actual==expected && source==saved,"finger delta was scaled or absolute state changed");
            actual.fill(0x7e);expected=actual;
            const float time=3.5f;put(expected,88,delta);put(expected,104,time);
            require(two(actual.data(),packed,&time)==packed,"two-finger nonvolatile register changed");
            require(actual==expected,"two-finger delta/time or absolute state changed");
            actual.fill(0x7e);expected=actual;put(expected,116,delta[0]);
            require(pinch(actual.data(),delta[0])==delta[0],"pinch original xmm9 changed");
            require(actual==expected,"pinch sign, delta, distance or absolute state changed");
        }
    }
    MEMORY_BASIC_INFORMATION info{};VirtualQuery(allocation.data,&info,sizeof(info));
    require(info.Protect==PAGE_EXECUTE_READ,"gesture patch protection not restored");
    helpers.reset();
    VirtualQuery(reinterpret_cast<void*>(helper_target),&helper_info,sizeof(helper_info));
    require(helper_info.State==MEM_FREE,"unreleased helper allocation leaked");
}

int wmain(int argc,wchar_t** argv) {
    try {
        if (argc==2 && std::wstring_view(argv[1])==L"--child") return 71;
        if (argc==3 && (std::wstring_view(argv[1])==L"--sample" || std::wstring_view(argv[1])==L"--discover")) {
            PreparedImage image(argv[2]);
            for (size_t i=0;i<image.plan.size();++i) {
                if (std::wstring_view(argv[1])==L"--sample")
                    require(image.plan[i].rva==gi_fixture::sample_rvas[i],"sample discovery differs from independent IDA oracle");
                std::cout<<image.plan[i].name<<": 0x"<<std::hex<<image.plan[i].rva<<std::dec<<'\n';
            }
            std::cout<<"PASS: all eleven UI/joystick sample sites located by signatures (read-only). No game launched.\n";
            return 0;
        }
        require(argc==1,"usage: GITouchTests [--sample <7.1-exe> | --discover <exe>]");
        discovery_tests();execution_tests();child_tests();joystick_and_unmodified_gesture_tests();
        rejects([]{PreparedImage unsupported(module_path());},"unrelated executable accepted");
        std::cout<<"PASS: GI discovery, UI/joystick execution, unmodified gesture deltas and owned child lifecycle.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
