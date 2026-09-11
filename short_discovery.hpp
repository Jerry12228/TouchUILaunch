#pragma once
#include "property_reader.hpp"
#include "profile.hpp"
#include <map>

namespace discovery {
struct Result {
    struct CodeEvidence { uintptr_t rva; std::vector<uint8_t> bytes; };
    profile::Build build{};
    uintptr_t input_region{},screen_region{},frame_region{},touch_loop{},device_selector{},default_getter{};
    uintptr_t loop_count_call{},loop_get_touch_call{},mobile_value_at{},pc_value_at{};
    size_t setter_candidates{};
    std::vector<CodeEvidence> code_evidence;
};
inline void evidence(const Image& image,Result& r,uintptr_t at,size_t size) {
    image.code(at,size);auto data=image.view(at,size);
    require(size>0 && size<=4096 && r.code_evidence.size()<64,"invalid code evidence size");
    r.code_evidence.push_back({at,{data.begin(),data.end()}});
}
struct Choice {
    uintptr_t fn{},override_getter{},default_getter{},klass{},initialized{};
    size_t size{};
};
inline Choice choice(const Image& image,uintptr_t fn,uintptr_t core) {
    using namespace x64;Cursor c(image,fn,256);Choice result;result.fn=fn;
    const auto stack=c.take();require(stack.stack_adjust(true),"layout choice prologue differs");
    for(int index=0;index<2;++index) {
        require(c.take().cmp_zero_byte(rip),"layout choice guard differs");
        require(c.take().condition(index?5:4),"layout choice guard branch differs");
    }
    const auto klass=c.take();require(klass.mov_load() && klass.base()==rip,"layout choice class load differs");result.klass=klass.storage(image);
    const auto init=c.take();require(init.cmp_zero_byte(klass.reg()),"layout choice initialization differs");result.initialized=init.disp();
    require(c.take().condition(4) && c.at==core,"layout choice core is not instruction-aligned");
    const auto call=c.take();require(call.call(),"layout choice does not call override");result.override_getter=call.relative(image);
    const auto repeated=c.take();require(repeated.mov_load() && repeated.base()==rip && repeated.storage(image)==result.klass,"layout choice classes differ");
    const auto ready=c.take();require(ready.h.opcode==0x0f && ready.h.opcode2==0xb6 && ready.memory(repeated.reg(),static_cast<int32_t>(result.initialized)),"layout choice ready byte differs");
    require(c.take().test(0),"layout choice does not test override value");
    const auto fallback=c.take();require(fallback.condition(4),"layout choice does not select default on zero");
    auto tail=[&]() {
        require(c.take().test(ready.reg(),true) && c.take().condition(4),"layout choice ready branch differs");
        const auto release=c.take();require(release.stack_adjust(false) && release.imm()==stack.imm(),"layout choice stack cleanup differs");
        const auto jump=c.take();require(jump.jump(),"layout choice is not a tail call");return jump.relative(image);
    };
    require(tail()==result.override_getter && fallback.relative(image)==c.at,"layout choice override edges differ");
    result.default_getter=tail();require(result.default_getter!=result.override_getter,"layout properties alias");
    result.size=c.at-fn;return result;
}
struct LayoutHelper {uintptr_t fn{},compare_at{};size_t size{};int value{};};
inline LayoutHelper layout_helper(const Image& image,uintptr_t fn,uintptr_t call,const Choice& current,int value) {
    using namespace x64;Cursor c(image,fn,256);
    const auto saved=c.take();require(saved.push(),"helper prologue differs");const int input=saved.stack_reg();
    const auto stack=c.take();require(stack.stack_adjust(true),"helper stack differs");
    require(c.take().mov(input,1,false),"helper does not preserve layout argument");
    for(int index=0;index<2;++index){require(c.take().cmp_zero_byte(rip),"helper guard differs");require(c.take().condition(index?5:4),"helper guard branch differs");}
    require(c.take().test(input),"helper does not test explicit layout argument");
    const auto explicit_value=c.take();require(explicit_value.condition(5),"helper explicit argument branch differs");
    const auto klass=c.take();require(klass.mov_load() && klass.base()==rip && klass.storage(image)==current.klass,"helper class differs");
    const auto initialized=c.take();require(initialized.cmp_zero_byte(klass.reg()) && initialized.disp()==int64_t(current.initialized),"helper initialization differs");
    require(c.take().condition(4) && c.at==call,"helper call position differs");
    const auto getter=c.take();require(getter.call() && getter.relative(image)==current.fn,"helper uses another layout getter");
    require(c.take().mov(input,0,false) && explicit_value.relative(image)==c.at,"helper explicit/current layouts do not join");
    const auto compare=c.take();require(compare.cmp(input,static_cast<uint8_t>(value)),"helper enum differs");
    const auto boolean=c.take();require(boolean.h.opcode==0x0f && boolean.h.opcode2==0x94 && boolean.h.modrm_mod==3 && boolean.rm()==0,"helper does not return equality");
    const auto release=c.take();require(release.stack_adjust(false) && release.imm()==stack.imm(),"helper stack cleanup differs");
    const auto pop=c.take();require(pop.pop() && pop.stack_reg()==input && c.take().ret(),"helper return differs");
    return {fn,compare.next()-1,c.at-fn,value};
}
inline std::vector<uintptr_t> preceding_entries(uintptr_t at,size_t distance) {
    std::vector<uintptr_t> entries;const uintptr_t lower=at>distance?at-distance:0;
    for(uintptr_t fn=(lower+15)&~uintptr_t{15};fn<at;fn+=16)entries.push_back(fn);
    return entries;
}
inline bool zero_argument(const Image& image,uintptr_t call) {
    for(size_t distance:{size_t{2},size_t{3},size_t{5}})if(call>=distance)try {
        auto i=x64::decode(image,call-distance);
        if(i.next()==call && (i.zero(1)||i.immediate(1,0)))return true;
    }catch(const std::runtime_error&){}
    return false;
}
struct PcBranch {uintptr_t call{},touch_target{};size_t size{};uintptr_t helper{},enum_at{};};
inline PcBranch pc_branch(const Image& image,uintptr_t call,bool helper) {
    using namespace x64;Cursor c(image,call,48);require(c.take().call(),"missing PC layout call");
    if(helper)require(zero_argument(image,call),"PC helper needs current layout argument zero");
    for(unsigned count=0;count<3;++count) {
        const auto i=c.peek();
        if((i.zero(i.rm()) && i.rm()!=0)||(i.h.opcode>=0xb8 && i.h.opcode<=0xbf && (i.h.opcode-0xb8+8*i.h.rex_b)!=0 && i.imm()==0))c.take();
        else break;
    }
    const auto compare=c.take();require(helper?compare.test(0,true):compare.cmp(0,2),"PC layout comparison differs");
    const auto branch=c.take();require(branch.condition(helper?4:5),"non-PC branch differs");
    return {call,branch.relative(image),c.at-call,helper?decode(image,call).relative(image):0,compare.next()-1};
}
inline bool reaches_count(const Image& image,uintptr_t from,uintptr_t count) {
    using namespace x64;
    if(from>count || count-from>128)return false;
    Cursor c(image,from,129);size_t steps{};
    bool guard{};
    while(c.at<count && ++steps<40) {
        const auto i=c.take();
        if(i.jump()) {const auto target=i.relative(image);if(target<=i.at || target>count)return false;c.at=target;continue;}
        if(i.conditional()) {if(!i.condition(5) || !guard)return false;guard=false;continue;}
        guard=i.cmp_zero_byte(rip);
        if(i.nop() || i.zero(i.rm()) || i.h.opcode==0x8d || (i.h.opcode==0x0f && i.h.opcode2==0x57) || i.cmp_zero_byte(rip))continue;
        if(i.h.opcode==0xff && i.h.modrm_reg==0 && i.h.modrm_mod==3)continue;
        return false;
    }
    return c.at==count;
}
inline void count_to_touch(const Image& image,uintptr_t count,uintptr_t touch) {
    using namespace x64;require(touch>count && touch-count<=128,"touch count/get consumers are too far apart");
    Cursor c(image,count,touch-count+6);const auto first=c.take();
    require(first.indirect_call() && first.base()==rip,"missing native touch count call");
    const auto compare=c.take();require(compare.h.opcode==0x39 && compare.h.modrm_mod==3 && compare.reg()==0 && compare.rm()!=0,"touch index is not bounded by count");const int index=compare.rm();
    require(c.take().condition(13),"touch loop does not stop at count");
    std::vector<Ins> setup;
    while(c.at<touch) {
        const auto i=c.take();require(!i.call() && !i.indirect_call() && !i.ret() && !i.jump(),"intervening transfer in touch retrieval");
        setup.push_back(i);
    }
    require(c.at==touch && setup.size()>=2,"touch call instruction boundary differs");
    const auto& argument=setup[setup.size()-2];const auto& output=setup.back();
    require(argument.mov(1,index,false) && ((output.h.opcode==0x89 && output.h.modrm_mod==3 && output.rm()==2 && output.h.rex_w)||
        (output.h.opcode==0x8d && output.reg()==2 && output.h.rex_w)),"touch call arguments differ");
    const auto last=c.take();require(last.indirect_call() && last.base()==rip,"missing native GetTouch call");
}
struct MobileBranch {uintptr_t call{},return_at{},enum_at{},helper{};size_t size{},return_size{};};
inline MobileBranch mobile_branch(const Image& image,uintptr_t call,bool helper,uintptr_t helper_enum=0) {
    using namespace x64;Cursor c(image,call,48);const auto target=c.take();require(target.call(),"missing mobile layout call");
    if(helper)require(zero_argument(image,call),"mobile helper needs current layout argument zero");
    const auto touch=c.take();const int reg=touch.h.opcode-0xb8+8*touch.h.rex_b;
    require(touch.immediate(reg,3) && reg!=0,"mobile branch does not select TouchScreen 3");
    const auto condition=c.take();require(helper?condition.test(0,true):condition.cmp(0,1),"mobile layout comparison differs");
    const auto branch=c.take();require(branch.condition(helper?5:4),"mobile selection edge differs");
    const auto destination=branch.relative(image);Cursor exit(image,destination,48);
    require(exit.take().mov(0,reg,false),"selected touch device is not returned");
    bool returned{};
    for(unsigned count=0;count<12;++count) {
        const auto i=exit.take();if(i.ret()){returned=true;break;}
        require(i.pop() || i.stack_adjust(false) || i.nop(),"touch return value is overwritten");
    }
    require(returned,"touch device branch has no bounded return");
    return {call,destination,helper?helper_enum:condition.next()-1,helper?target.relative(image):0,c.at-call,exit.at-destination};
}
inline Result discover(const Image& image) {
    Result r;auto& p=r.build;
    r.input_region=image.unique(scan_rules::input_region);r.screen_region=image.unique(scan_rules::screen_region);r.frame_region=image.unique(scan_rules::frame_region);
    p.get_touch_slot=image.relative(r.input_region+0x70,{0x48,0x8b,0x05});image.expect(r.input_region+0x77,{0x48,0xff,0xe0});
    p.touch_count_slot=image.relative(r.input_region+0x220,{0x48,0xff,0x25});p.touch_supported_slot=image.relative(r.input_region+0x230,{0x48,0xff,0x25});
    p.frame_count_slot=image.relative(r.frame_region,{0x48,0xff,0x25});p.screen_width_slot=image.relative(r.screen_region+0x20,{0x48,0xff,0x25});p.screen_height_slot=image.relative(r.screen_region+0x30,{0x48,0xff,0x25});
    // 49-byte normal choice core excludes metadata/hot-update dispatch bodies.
    const scan_rules::Pattern core{"layout_choice_core",scan_rules::effective_layout.bytes.subspan(0x26,0x31),scan_rules::effective_layout.mask.subspan(0x26,0x31)};
    std::map<uintptr_t,Choice> choices;
    for(auto hit:image.find(core,64,8))for(auto fn:preceding_entries(hit,96))try {auto found=choice(image,fn,hit);choices.emplace(fn,found);}catch(const std::runtime_error&){}
    require(choices.size()==1,"expected one validated layout choice, got "+std::to_string(choices.size()));
    const auto current=choices.begin()->second;p.get_effective_layout=current.fn;p.get_layout_override=current.override_getter;r.default_getter=current.default_getter;
    const auto override=property(image,p.get_layout_override,false),fallback=property(image,r.default_getter,false);
    require(same_owner(override,fallback) && override.field!=fallback.field,"layout getters do not share a provider");
    require(current.klass==override.klass && current.initialized==override.initialized,"choice and property class layouts differ");
    p.ui_class_slot=override.klass;p.static_reference_pool=override.pool;p.ui_state_offset=override.pool_offset;p.class_initialized_offset=override.initialized;p.override_property_offset=override.field;p.default_property_offset=fallback.field;
    std::map<uintptr_t,Property> setters;
    for(auto reference:x64::loads_from(image,p.ui_class_slot))for(auto entry:preceding_entries(reference,96))if(!setters.contains(entry))try {
        auto owner=property(image,entry,true);if(same_owner(override,owner))setters.emplace(entry,owner);
    }catch(const std::runtime_error&){}
    r.setter_candidates=setters.size();size_t valid{};Property setter;
    for(auto [entry,owner]:setters)if(owner.field==override.field){p.set_layout_override=entry;setter=owner;++valid;}
    require(valid==1,"expected one setter for the override property, got "+std::to_string(valid));
    require(std::set<uintptr_t>{p.get_layout_override,p.set_layout_override,p.get_effective_layout,r.default_getter}.size()==4,"UI function entries alias");
    const auto references=x64::calls_to(image,current.fn);
    std::map<uintptr_t,LayoutHelper> helpers;
    for(auto reference:references)for(auto fn:preceding_entries(reference,96))for(int value:{1,2})try {helpers.emplace(fn,layout_helper(image,fn,reference,current,value));}catch(const std::runtime_error&){}
    std::vector<PcBranch> pc;std::vector<MobileBranch> mobile;
    for(auto call:references) {
        try {pc.push_back(pc_branch(image,call,false));}catch(const std::runtime_error&){}
        try {mobile.push_back(mobile_branch(image,call,false));}catch(const std::runtime_error&){}
    }
    for(const auto& [fn,helper]:helpers)for(auto call:x64::calls_to(image,fn))try {
        if(helper.value==2)pc.push_back(pc_branch(image,call,true));else mobile.push_back(mobile_branch(image,call,true,helper.compare_at));
    }catch(const std::runtime_error&){}
    std::map<uintptr_t,std::vector<MobileBranch>> returns;
    for(const auto& branch:mobile)returns[branch.return_at].push_back(branch);
    require(returns.size()==1,"expected one Mobile-to-TouchScreen return, got "+std::to_string(returns.size()));
    const auto& selected=returns.begin()->second;r.device_selector=selected.front().call;r.mobile_value_at=selected.front().enum_at;
    require(selected.size()==2,"both device-selector branches must agree on Mobile and TouchScreen");
    const auto counts=x64::calls_to(image,p.touch_count_slot,true),touches=x64::calls_to(image,p.get_touch_slot,true);
    std::map<std::pair<uintptr_t,uintptr_t>,PcBranch> loops;
    for(auto count:counts)for(auto touch:touches)if(touch>count && touch-count<=128) {
        try {count_to_touch(image,count,touch);}catch(const std::runtime_error&){continue;}
        for(const auto& branch:pc)if(branch.call<count && count-branch.call<=1024) {
            bool reaches{};try {reaches=reaches_count(image,branch.touch_target,count);}catch(const std::runtime_error&){}
            if(!reaches)continue;
            auto [it,inserted]=loops.emplace(std::pair{count,touch},branch);
            require(inserted || it->second.call==branch.call,"ambiguous PC branch for touch consumers");
        }
    }
    require(loops.size()==1,"expected one layout-controlled native touch loop, got "+std::to_string(loops.size()));
    const auto [consumers,branch]=*loops.begin();r.touch_loop=branch.call;r.loop_count_call=consumers.first;r.loop_get_touch_call=consumers.second;
    r.pc_value_at=branch.helper?helpers.at(branch.helper).compare_at:branch.enum_at;
    const std::set<uintptr_t> slots{p.touch_count_slot,p.get_touch_slot,p.touch_supported_slot,p.frame_count_slot,p.screen_width_slot,p.screen_height_slot,p.ui_class_slot,p.static_reference_pool};
    require(slots.size()==8,"detected storage slots alias");for(auto slot:slots)image.slot(slot);
    evidence(image,r,r.input_region,scan_rules::input_region.bytes.size());evidence(image,r,r.screen_region,scan_rules::screen_region.bytes.size());evidence(image,r,r.frame_region,scan_rules::frame_region.bytes.size());
    evidence(image,r,current.fn,current.size);evidence(image,r,p.get_layout_override,override.main_size);evidence(image,r,r.default_getter,fallback.main_size);evidence(image,r,p.set_layout_override,setter.main_size);
    evidence(image,r,r.touch_loop-5,r.loop_get_touch_call+6-(r.touch_loop-5));
    for(const auto& site:selected){evidence(image,r,site.call-5,site.size+5);if(site.helper){auto helper=helpers.at(site.helper);evidence(image,r,helper.fn,helper.size);}}
    evidence(image,r,selected.front().return_at,selected.front().return_size);
    if(branch.helper){auto helper=helpers.at(branch.helper);evidence(image,r,helper.fn,helper.size);}
    return r;
}
}
