#pragma once
#include "nte_input.hpp"
#include "nte_input_trace.hpp"
#include "MinHook.h"
#include <intrin.h>

namespace touchui::nte::pager {
// Preserve native event arguments and reply buffers. The paging correction
// changes one validated capture query and completes a missing start notification
// after native drag capture is requested. Optional observers record the result.
// These slots are accepted only with the derived ScrollBox constructor and
// touch-handler contracts below, rather than by a client version or RVA.
inline constexpr std::array<uintptr_t,8> slots={0x58,0x68,0x70,0x78,0x80,0xe8,0xf0,0xf8};
struct Plan {
    uintptr_t vtable{};std::array<uintptr_t,8> originals{};
    uintptr_t capture_query{},capture_return{},base_constructor{};
    uintptr_t weak_valid{},weak_resolve{},broadcast_start{};
    int32_t weak_offset{},start_delegate{};
    int32_t size{},enabled{},active{},pressed{},touch{},pointer{},delta{},drag_flags{};
};
inline std::vector<dx::Ins> body(const ModuleImage& module,uintptr_t at,size_t length) {
    std::vector<dx::Ins> result;const auto end=at+length;
    while(at<end) {
        auto ins=dx::decode(module.image(),at,true);result.push_back(ins);at=ins.next();
        if(ins.ret())break;
    }
    return result;
}
inline Plan discover(const ModuleImage& module) {
    const auto& image=module.image();std::set<uintptr_t> names;
    constexpr std::string_view name="SHTPageScrollBox";
    for(const auto& section:image.sections)if(!section.code()&&(section.flags&IMAGE_SCN_MEM_READ)) {
        const auto bytes=image.view(section.rva,section.size);auto next=bytes.begin();
        while(next!=bytes.end()) {
            const auto match=std::search(next,bytes.end(),name.begin(),name.end());if(match==bytes.end())break;
            const auto at=section.rva+static_cast<uintptr_t>(match-bytes.begin());
            if(module.ascii(at)==name)names.insert(at);next=match+1;
        }
    }
    const auto refs=input::references(module,input::one(names,"pager type name is ambiguous"));
    require(refs.size()==1,"pager construction reference is ambiguous");
    const auto rebuild=input::prefix(module,refs.front(),80);uintptr_t make_shared{};bool passed_name{};
    for(const auto& ins:rebuild)if(ins.at>=refs.front()) {
        if(ins.at==refs.front())passed_name=ins.reg()==2;
        else if(ins.call()){make_shared=ins.relative(image);break;}
    }
    require(passed_name&&module.code(make_shared),"pager type is not passed to a construction helper");
    const auto helper=body(module,make_shared,128);uintptr_t allocate{};
    for(const auto& ins:helper)if(ins.call()){allocate=ins.relative(image);break;}
    require(module.code(allocate),"pager allocator is unavailable");
    const auto constructor=body(module,allocate,256);Plan plan;std::set<uintptr_t> tables;
    for(size_t i=0;i<constructor.size();++i) {
        const auto& ins=constructor[i];
        if(ins.call())plan.base_constructor=ins.relative(image);
        if(ins.h.opcode==0xb9&&!ins.h.rex_w&&ins.imm()>32&&ins.imm()<0x4000)plan.size=static_cast<int32_t>(ins.imm())-16;
        if(ins.h.opcode==0x8d&&ins.base()==dx::rip&&i+1<constructor.size()) {
            const auto& store=constructor[i+1];
            if(store.mov_store()&&store.reg()==ins.reg()&&store.disp()==16&&store.index()==dx::none&&store.base()>=0) {
                const auto table=ins.storage(image);
                if(module.data(table,0x100)&&!(image.section(table,0x100)->flags&IMAGE_SCN_MEM_WRITE))tables.insert(table);
            }
        }
    }
    require(plan.size>32,"pager allocation size was not validated");
    plan.vtable=input::one(tables,"pager derived vtable is ambiguous");
    for(size_t i=0;i<slots.size();++i) {
        plan.originals[i]=module.rva(image.read<uintptr_t>(plan.vtable+slots[i]));
        require(module.code(plan.originals[i]),"pager input slot is not native code");
    }
    // OnMouseEnter(self, geometry, event) arms a TOptional pointer only for
    // enabled touch scrolling. Validate its two self tests and event test.
    const auto enter=body(module,plan.originals[3],256);std::vector<int32_t> members;bool saved_event{};
    for(const auto& ins:enter) {
        if(ins.cmp_zero_byte(1))members.push_back(ins.disp());
        if(ins.cmp_zero_byte(8))plan.touch=ins.disp();
        if(ins.mov(7,8,true))saved_event=true;
        if(ins.h.opcode==0x8b&&!ins.h.rex_w&&ins.reg()==1&&ins.base()==7)plan.pointer=ins.disp();
    }
    require(members.size()==2&&saved_event&&plan.touch>0&&plan.touch<256&&plan.pointer>0&&plan.pointer<256,
        "pager mouse-enter touch contract is invalid");
    plan.enabled=members[0];plan.active=members[1];
    const auto start=body(module,plan.originals[5],256);uintptr_t unhandled{};std::vector<size_t> start_calls;
    for(size_t i=0;i<start.size();++i) {
        const auto& ins=start[i];
        if(ins.h.opcode==0xc6&&ins.h.modrm_reg==0&&ins.base()==1&&ins.imm()==1)plan.pressed=ins.disp();
        if(ins.call()){unhandled=ins.relative(image);start_calls.push_back(i);}
    }
    require(unhandled==plan.originals[6],"pager touch-start/move fallback relationship is invalid");
    require(start_calls.size()==4&&start_calls[0]>0&&start_calls[1]>0&&start_calls[2]>0,"pager start notification contract is invalid");
    const auto& resolved=start[start_calls[1]-1];const auto& delegate=start[start_calls[2]-1];
    // Register saves may follow the ADD before the first call; locate the
    // unique owner-member adjustment in its bounded prefix instead.
    std::set<int32_t> weak_members;
    for(size_t i=0;i<start_calls[0];++i) {
        const auto& ins=start[i];
        if((ins.h.opcode==0x81||ins.h.opcode==0x83)&&ins.h.modrm_mod==3&&ins.h.modrm_reg==0&&ins.rm()==1&&ins.h.rex_w)
            weak_members.insert(static_cast<int32_t>(ins.imm()));
    }
    require(weak_members.size()==1,"pager weak-owner adjustment is ambiguous");plan.weak_offset=*weak_members.begin();
    require(resolved.h.opcode==0x8d&&resolved.reg()==1&&resolved.base()>=0&&resolved.disp()==plan.weak_offset&&resolved.index()==dx::none&&
        delegate.h.opcode==0x8d&&delegate.reg()==1&&delegate.base()==0&&delegate.index()==dx::none&&delegate.disp()>0&&delegate.disp()<0x4000,
        "pager start delegate owner relationship is invalid");
    plan.start_delegate=delegate.disp();plan.weak_valid=start[start_calls[0]].relative(image);
    plan.weak_resolve=start[start_calls[1]].relative(image);plan.broadcast_start=start[start_calls[2]].relative(image);
    require(module.code(plan.weak_valid)&&module.code(plan.weak_resolve)&&module.code(plan.broadcast_start)&&
        plan.weak_offset>plan.pressed&&plan.weak_offset+8<=plan.size,"pager notification functions/member exceed their contract");
    const auto move=body(module,plan.originals[2],512);bool touch_test{},active_test{};
    std::set<int> zero_registers,self_registers={1},event_registers={9};
    for(const auto& ins:move) {
        if(ins.mov_load()&&ins.reg()==0&&ins.base()==9&&ins.disp()>0&&ins.disp()<256)plan.delta=ins.disp();
        for(int reg=0;reg<16;++reg) {
            if(ins.zero(reg))zero_registers.insert(reg);
            for(const auto source:std::vector<int>(self_registers.begin(),self_registers.end()))if(ins.mov(reg,source,true))self_registers.insert(reg);
            for(const auto source:std::vector<int>(event_registers.begin(),event_registers.end()))if(ins.mov(reg,source,true))event_registers.insert(reg);
        }
        const bool zero_test=(ins.h.opcode==0x80&&ins.h.modrm_reg==7&&ins.imm()==0)||
            (ins.h.opcode==0x38&&zero_registers.contains(ins.reg()));
        if(zero_test&&event_registers.contains(ins.base())&&ins.disp()==plan.touch)touch_test=true;
        if(zero_test&&self_registers.contains(ins.base())&&ins.disp()==plan.active)active_test=true;
        if(ins.h.opcode==0x80&&ins.h.modrm_reg==1&&ins.imm()==1&&self_registers.contains(ins.base()))plan.drag_flags=ins.disp();
    }
    require(touch_test&&active_test&&plan.delta>0,"pager inherited movement contract is invalid");
    std::set<std::pair<uintptr_t,uintptr_t>> capture_calls;
    for(size_t i=1;i+2<move.size();++i)if(move[i].call()&&move[i+1].test(0,true)&&move[i+2].condition(5)) {
        for(const auto owner:self_registers)if(move[i-1].mov(1,owner,true))
            capture_calls.emplace(move[i].relative(image),move[i].next());
    }
    require(capture_calls.size()==1,"pager capture guard call is ambiguous");
    plan.capture_query=capture_calls.begin()->first;plan.capture_return=capture_calls.begin()->second;
    require(module.code(plan.capture_query)&&module.function_start(plan.capture_query)==plan.capture_query,
        "pager capture query is not a validated native function");
    bool shared_owner{},application{},virtual_query{};
    for(const auto& ins:body(module,plan.capture_query,512))if(ins.mov_load()) {
        shared_owner|=ins.base()==1&&ins.disp()==16;
        if(ins.base()==dx::rip)application|=input::writable(module,ins.storage(image),8);
        virtual_query|=ins.base()>=0&&ins.disp()==0x180;
    }
    require(shared_owner&&application&&virtual_query,"pager capture query contract is invalid");
    for(const auto field:{plan.enabled,plan.active,plan.pressed,plan.drag_flags})require(field>0&&field<plan.size,"pager member exceeds allocation");
    require(plan.enabled!=plan.active&&plan.pressed>plan.enabled&&plan.pressed>plan.active,
        "pager touch members overlap");
    return plan;
}
struct State {
    uintptr_t base{};Plan plan;std::array<bool,8> installed{};
    char kind{'S'};
    std::atomic<bool> capture_guard{},initialize_start{};std::atomic<void*> original_query{};
};
inline std::atomic<State*> active{};
inline thread_local uintptr_t starting_touch_widget{};
struct CaptureScope {
    uintptr_t previous{starting_touch_widget};bool starting{};
    CaptureScope(State& state,size_t index,uintptr_t widget,uintptr_t event) {
        if(index!=2||!state.capture_guard.load(std::memory_order_acquire))return;
        uint8_t touch{},drag{};
        if(read_value(event+state.plan.touch,touch)&&touch==1&&read_value(widget+state.plan.drag_flags,drag)&&!(drag&1))
            {starting_touch_widget=widget;starting=true;}
    }
    ~CaptureScope(){starting_touch_widget=previous;}
};
inline void complete_missing_start(State& state,uintptr_t widget) {
    if(!state.initialize_start.load(std::memory_order_acquire))return;
    uint8_t drag{},pressed{};
    if(!read_value(widget+state.plan.drag_flags,drag)||!(drag&1)||!read_value(widget+state.plan.pressed,pressed)||pressed)return;
    if(!input::writable_live(widget+state.plan.pressed,1))return;
    if(_InterlockedCompareExchange8(reinterpret_cast<volatile char*>(widget+state.plan.pressed),1,0)!=0)return;
    const auto owner=widget+state.plan.weak_offset;
    if(reinterpret_cast<bool(*)(uintptr_t)>(state.base+state.plan.weak_valid)(owner)) {
        const auto resolved=reinterpret_cast<uintptr_t(*)(uintptr_t)>(state.base+state.plan.weak_resolve)(owner);
        if(resolved)reinterpret_cast<void(*)(uintptr_t)>(state.base+state.plan.broadcast_start)(resolved+state.plan.start_delegate);
    }
}
inline bool bypass_capture(const State& state,uintptr_t widget,uintptr_t caller) {
    return state.capture_guard.load(std::memory_order_acquire)&&starting_touch_widget&&
        starting_touch_widget==widget&&caller==state.base+state.plan.capture_return;
}
// Only the verified pre-drag query in this pager's native touch movement is
// changed. Other callers, widgets, mouse input and already-started drags retain
// the original query. The native handler still owns capture/reply/release.
__declspec(noinline) inline bool capture_query_override(uintptr_t widget) {
    auto& state=*active.load(std::memory_order_acquire);
    if(bypass_capture(state,widget,reinterpret_cast<uintptr_t>(_ReturnAddress())))return false;
    return reinterpret_cast<bool(*)(uintptr_t)>(state.original_query.load(std::memory_order_acquire))(widget);
}
inline void enable_capture_guard(State& state) {
    const auto target=reinterpret_cast<void*>(state.base+state.plan.capture_query);
    const auto status=MH_Initialize();require(status==MH_OK||status==MH_ERROR_ALREADY_INITIALIZED,"cannot initialize pager capture guard");
    void* trampoline{};
    const auto created=MH_CreateHook(target,reinterpret_cast<void*>(&capture_query_override),&trampoline);
    require(created==MH_OK,"cannot prepare pager capture guard: "+std::string(MH_StatusToString(created)));
    state.original_query.store(trampoline,std::memory_order_release);
    const auto enabled=MH_EnableHook(target);
    require(enabled==MH_OK,"cannot enable pager capture guard: "+std::string(MH_StatusToString(enabled)));
    state.capture_guard.store(true,std::memory_order_release);
}
inline void observe(State& state,size_t index,char phase,uintptr_t widget,uintptr_t event,uintptr_t result=0)noexcept {
    try {
        auto* output=input_trace::active.load(std::memory_order_acquire);
        if(!output||output->stop.load())return;
        uint8_t enabled{},armed{},pressed{},touch{},drag{},handled{};DWORD pointer{};std::array<LONG,2> delta{};
        if(!read_value(widget+state.plan.enabled,enabled)||!read_value(widget+state.plan.active,armed)||
            !read_value(widget+state.plan.pressed,pressed)||!read_value(event+state.plan.touch,touch)||
            !read_value(event+state.plan.pointer,pointer)||!read_value(event+state.plan.delta,delta)||
            !read_value(widget+state.plan.drag_flags,drag))return;
        if(result&&!read_value(result,handled))return;
        const auto state_bits=(pressed&1)|((drag&1)<<1)|((handled&1)<<2);
        const auto flags=DWORD(enabled)|(DWORD(armed)<<8)|(DWORD(state_bits)<<16)|(DWORD(touch)<<24);
        input_trace::record(*output,{GetTickCount64(),static_cast<UINT>(slots[index]),pointer,flags,delta[0],delta[1],widget,
            reinterpret_cast<HWND>(static_cast<uintptr_t>(GetCurrentThreadId())),phase,state.kind});
    }catch(...){}
}
using Reply=uintptr_t(*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);
template<size_t Index> uintptr_t reply(uintptr_t widget,uintptr_t result,uintptr_t geometry,uintptr_t event) {
    auto& state=*active.load(std::memory_order_acquire);observe(state,Index,'B',widget,event);
    CaptureScope capture_scope(state,Index,widget,event);
    const auto returned=reinterpret_cast<Reply>(state.base+state.plan.originals[Index])(widget,result,geometry,event);
    if constexpr(Index==2)if(capture_scope.starting)complete_missing_start(state,widget);
    observe(state,Index,'A',widget,event,result);return returned;
}
inline void enter(uintptr_t widget,uintptr_t geometry,uintptr_t event) {
    auto& state=*active.load(std::memory_order_acquire);observe(state,3,'B',widget,event);
    reinterpret_cast<void(*)(uintptr_t,uintptr_t,uintptr_t)>(state.base+state.plan.originals[3])(widget,geometry,event);
    observe(state,3,'A',widget,event);
}
inline void leave(uintptr_t widget,uintptr_t event) {
    auto& state=*active.load(std::memory_order_acquire);observe(state,4,'B',widget,event);
    reinterpret_cast<void(*)(uintptr_t,uintptr_t)>(state.base+state.plan.originals[4])(widget,event);
    observe(state,4,'A',widget,event);
}
inline std::array<uintptr_t,8> callbacks() {
    return {reinterpret_cast<uintptr_t>(&reply<0>),reinterpret_cast<uintptr_t>(&reply<1>),reinterpret_cast<uintptr_t>(&reply<2>),
        reinterpret_cast<uintptr_t>(&enter),reinterpret_cast<uintptr_t>(&leave),reinterpret_cast<uintptr_t>(&reply<5>),
        reinterpret_cast<uintptr_t>(&reply<6>),reinterpret_cast<uintptr_t>(&reply<7>)};
}
inline void exchange(State& state,size_t index,bool restore,uintptr_t custom_callback=0) {
    auto* slot=reinterpret_cast<void**>(state.base+state.plan.vtable+slots[index]);DWORD old{};
    require(VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&old)!=FALSE,"cannot protect phone pager slot");
    const auto callback=custom_callback?custom_callback:callbacks()[index],original=state.base+state.plan.originals[index];
    const auto expected=restore?callback:original,replacement=restore?original:callback;
    const auto before=reinterpret_cast<uintptr_t>(InterlockedCompareExchangePointer(slot,
        reinterpret_cast<void*>(replacement),reinterpret_cast<void*>(expected)));
    if(!restore&&before==expected)state.installed[index]=true;
    if(restore&&before==expected)state.installed[index]=false;
    DWORD ignored{};const bool protected_again=VirtualProtect(slot,sizeof(void*),old,&ignored)!=FALSE;
    require(protected_again,"cannot restore phone pager page protection");
    if(!restore)require(before==expected,"phone pager slot changed before installation");
}
inline State* install(uintptr_t base,const Plan& plan) {
    require((base+plan.vtable)%8==0,"unaligned phone pager table");
    HMODULE module{};
    require(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&enter),&module)!=FALSE,"cannot pin phone pager callbacks");
    auto* state=new State;state->base=base;state->plan=plan;State* empty{};
    require(active.compare_exchange_strong(empty,state,std::memory_order_release),"phone pager already installed");
    try {for(size_t i=0;i<slots.size();++i)exchange(*state,i,false);}
    catch(...) {
        for(size_t i=slots.size();i-->0;)if(state->installed[i])try{exchange(*state,i,true);}catch(...){}
        throw; // Pinned context survives partial installation/in-flight calls.
    }
    return state;
}
struct ListPlan {Plan input;int32_t scroll_offset{},geometry_scale{};};
inline ListPlan discover_list(const ModuleImage& module,const Plan& pager) {
    const auto& image=module.image();std::set<uintptr_t> names;constexpr std::string_view name="SHTScrollBox";
    for(const auto& section:image.sections)if(!section.code()&&(section.flags&IMAGE_SCN_MEM_READ)) {
        const auto bytes=image.view(section.rva,section.size);auto next=bytes.begin();
        while(next!=bytes.end()) {
            const auto match=std::search(next,bytes.end(),name.begin(),name.end());if(match==bytes.end())break;
            const auto at=section.rva+static_cast<uintptr_t>(match-bytes.begin());
            if(module.ascii(at)==name)names.insert(at);next=match+1;
        }
    }
    const auto refs=input::references(module,input::one(names,"list type name is ambiguous"));
    require(refs.size()==1,"list construction reference is ambiguous");
    uintptr_t helper{};
    for(const auto& ins:input::prefix(module,refs.front(),80))if(ins.at>refs.front()&&ins.call()){helper=ins.relative(image);break;}
    require(module.code(helper)&&module.code(pager.base_constructor),"list/shared base construction is unavailable");
    ListPlan result;result.input=pager;auto& plan=result.input;plan.vtable=0;plan.size=0;
    std::set<uintptr_t> tables;
    for(const auto& ins:body(module,helper,512)) {
        if(ins.h.opcode==0xb9&&!ins.h.rex_w&&ins.imm()>32&&ins.imm()<0x4000)plan.size=static_cast<int32_t>(ins.imm())-16;
        if(!ins.call())continue;
        const auto callee=ins.relative(image);if(!module.code(callee))continue;
        std::vector<dx::Ins> code;try{code=body(module,callee,256);}catch(const std::exception&){continue;}bool base_called{};
        for(size_t p=0;p<code.size();++p) {
            const auto& store=code[p];
            if(store.call()&&store.relative(image)==pager.base_constructor)base_called=true;
            if(!base_called||!store.mov_store()||store.disp()!=0||store.base()<0)continue;
            for(size_t n=p;n>0&&p-n<4;--n) {
                const auto& source=code[n-1];
                if(source.h.opcode==0x8d&&source.base()==dx::rip&&source.reg()==store.reg()) {
                    const auto at=source.storage(image);
                    if(module.data(at,0x100)&&!(image.section(at,0x100)->flags&IMAGE_SCN_MEM_WRITE))tables.insert(at);
                    break;
                }
            }
        }
    }
    plan.vtable=input::one(tables,"list derived vtable is ambiguous");require(plan.size>plan.enabled,"list allocation size is invalid");
    for(size_t i=0;i<slots.size();++i) {
        plan.originals[i]=module.rva(image.read<uintptr_t>(plan.vtable+slots[i]));require(module.code(plan.originals[i]),"list callback is not native code");
    }
    require(plan.originals[2]==pager.originals[2]&&plan.originals[3]==pager.originals[3]&&plan.originals[4]==pager.originals[4]&&
        plan.originals[5]==pager.originals[6]&&plan.originals[6]==pager.originals[6],"list does not share the validated native touch movement ABI");
    std::set<int32_t> pressed,offsets;
    for(const auto& ins:body(module,plan.originals[0],128))if(ins.h.opcode==0xc6&&ins.h.modrm_reg==0&&ins.base()==1&&ins.imm()==1)pressed.insert(ins.disp());
    require(pressed.size()==1,"list pressed member is ambiguous");plan.pressed=*pressed.begin();
    const auto tick=module.rva(image.read<uintptr_t>(plan.vtable+0x10));
    require(module.code(tick),"list tick is not native code");
    for(const auto& ins:body(module,tick,512))if(ins.call()) {
        const auto callee=ins.relative(image);if(!module.code(callee))continue;
        std::vector<dx::Ins> getter;try{getter=body(module,callee,32);}catch(const std::exception&){continue;}
        if(getter.size()==2&&getter[1].ret()) {
            const auto& load=getter[0];
            if(load.h.opcode==0x0f&&load.h.opcode2==0x10&&load.h.p_rep==0xf3&&load.base()==1&&load.reg()==0&&load.index()==dx::none)
                offsets.insert(load.disp());
        }
    }
    require(offsets.size()==1,"list scroll-offset getter is ambiguous");result.scroll_offset=*offsets.begin();
    for(const auto& ins:body(module,plan.originals[2],512))if(ins.h.opcode==0x0f&&ins.h.opcode2==0x5e&&ins.h.p_rep==0xf3&&ins.base()==8)
        result.geometry_scale=ins.disp();
    require(plan.pressed>0&&plan.pressed<plan.size&&result.scroll_offset>0&&result.scroll_offset+4<=plan.size&&
        result.geometry_scale>0&&result.geometry_scale<256,"list metric field exceeds its contract");return result;
}
struct ListState {State input;int32_t offset{},scale{};};
inline std::atomic<ListState*> list_active{};
inline void list_metric(ListState& state,char phase,uintptr_t widget,uintptr_t geometry)noexcept {
    try {
        auto* output=input_trace::active.load(std::memory_order_acquire);if(!output||output->stop.load())return;
        LONG scale{},offset{};
        if(!read_value(geometry+state.scale,scale)||!read_value(widget+state.offset,offset))return;
        input_trace::record(*output,{GetTickCount64(),0x70,0,0,scale,offset,widget,nullptr,phase,'G'});
    }catch(...){}
}
inline uintptr_t list_move(uintptr_t widget,uintptr_t result,uintptr_t geometry,uintptr_t event) {
    auto& state=*list_active.load(std::memory_order_acquire);
    observe(state.input,2,'B',widget,event);list_metric(state,'B',widget,geometry);
    const auto returned=reinterpret_cast<Reply>(state.input.base+state.input.plan.originals[2])(widget,result,geometry,event);
    observe(state.input,2,'A',widget,event,result);list_metric(state,'A',widget,geometry);return returned;
}
inline ListState* install_list(uintptr_t base,const ListPlan& plan) {
    auto* state=new ListState;state->input.base=base;state->input.plan=plan.input;state->input.kind='L';state->offset=plan.scroll_offset;state->scale=plan.geometry_scale;
    ListState* empty{};require(list_active.compare_exchange_strong(empty,state,std::memory_order_release),"list observer already installed");
    try {exchange(state->input,2,false,reinterpret_cast<uintptr_t>(&list_move));}
    catch(...) {
        if(state->input.installed[2])try{exchange(state->input,2,true,reinterpret_cast<uintptr_t>(&list_move));}catch(...){}
        throw; // The pinned context remains valid if a callback was published.
    }
    return state;
}
}
