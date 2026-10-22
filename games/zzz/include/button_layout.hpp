#pragma once
#include "x64_reader.hpp"
#include <array>
#include <map>
#include <optional>
#include <set>

namespace discovery::button_layout {
enum class Edit { comparison, equal_branch };
struct Site {
    uintptr_t gate{}, compare{}, word{}, apply{};
    uint64_t expected{}, replacement{};
    Edit edit{Edit::comparison};
    uintptr_t patch{}, accepted{};
};
inline std::optional<uint64_t> replacement_word(const Site& site) {
    if(site.word%8 || site.patch>UINT32_MAX-6 || site.accepted>UINT32_MAX ||
        site.patch<site.word || site.patch-site.word>=8)return {};
    std::array<uint8_t,8> bytes{};std::memcpy(bytes.data(),&site.expected,8);
    const auto offset=site.patch-site.word;
    if(site.edit==Edit::comparison) {
        if(site.patch!=site.compare || offset>5 || bytes[offset]!=0x83 || bytes[offset+1]!=0xf8 || bytes[offset+2]!=1)return {};
        bytes[offset]=0x39;bytes[offset+1]=0xc0;bytes[offset+2]=0x90;
    } else if(site.edit==Edit::equal_branch) {
        if(site.compare>UINTPTR_MAX-3 || site.patch!=site.compare+3 || offset>2 ||
            bytes[offset]!=0x0f || bytes[offset+1]!=0x84)return {};
        int32_t old{};std::memcpy(&old,bytes.data()+offset+2,4);
        if(int64_t(site.patch)+6+old!=int64_t(site.accepted))return {};
        const auto displacement=int64_t(site.accepted)-int64_t(site.patch)-5;
        if(displacement<INT32_MIN || displacement>INT32_MAX)return {};
        const auto delta=static_cast<int32_t>(displacement);
        bytes[offset]=0xe9;std::memcpy(bytes.data()+offset+1,&delta,4);bytes[offset+5]=0x90;
    } else return {};
    uint64_t result{};std::memcpy(&result,bytes.data(),8);return result;
}
struct CodeEvidence {
    uintptr_t rva{};
    std::vector<uint8_t> bytes;
};
inline void remember(const Image& image,std::vector<CodeEvidence>& evidence,uintptr_t at,size_t length) {
    require(length>0 && length<=1024,"button layout evidence exceeds bound");
    image.code(at,length);
    const auto bytes=image.view(at,length);
    evidence.push_back({at,{bytes.begin(),bytes.end()}});
}
struct Plan {
    // Primary position/size checks first; duplicated initialization checks follow.
    std::vector<Site> sites;
    uintptr_t device_getter{}, device_cache{}, device_ready{};
    std::vector<CodeEvidence> evidence;
};
struct Resolution {
    std::optional<Plan> plan;
    std::string error;
};
struct Gate {
    Site site;
    uintptr_t device_getter{}, device_class{};
    int object{}, busy=-1;
    std::vector<CodeEvidence> evidence;
    std::optional<Site> initialization_site;
    uintptr_t shortcut{};
};
inline void writable(const Image& image,uintptr_t at,size_t size) {
    const auto* section=image.section(at,size);
    require(section && section->data(),"button layout storage is not writable module data");
}
inline bool byte_store(const x64::Ins& i,int base,int offset) {
    return i.h.opcode==0xc6 && i.h.modrm_reg==0 && i.memory(base,offset) && i.imm()==1;
}
inline uintptr_t boolean_return(const Image& image,uintptr_t at,int saved,uint64_t stack) {
    x64::Cursor c(image,at,32);
    require(c.take().mov(0,saved,false),"button gate does not return its saved boolean");
    const auto release=c.take();
    require(release.stack_adjust(false) && release.imm()==stack,"button gate stack cleanup differs");
    const auto pop=c.take();
    require(pop.pop() && pop.stack_reg()==saved && c.take().ret(),"button gate return differs");
    return c.at;
}
inline Site comparison_site(const Image& image,uintptr_t entry,const x64::Ins& compare,
                            const x64::Ins* equal=nullptr) {
    require(compare.cmp(0,1) && compare.h.len==3,"button hardware comparison differs");
    Site site{entry,compare.at,compare.at&~uintptr_t{7}};site.patch=compare.at;
    if(compare.at-site.word>5) {
        // A copied cold check can straddle an atomic word. Preserve its getter
        // and comparison; only make the proven equality-to-success edge taken.
        require(equal && equal->condition(4) && equal->h.len==6 && equal->at==compare.next(),
            "button hardware comparison has no aligned supported edit");
        site.edit=Edit::equal_branch;site.patch=equal->at;site.word=equal->at&~uintptr_t{7};
        site.accepted=equal->relative(image);
    }
    site.expected=image.read<uint64_t>(site.word);
    const auto replacement=replacement_word(site);
    require(replacement.has_value(),"button hardware edit crosses an atomic word");site.replacement=*replacement;
    return site;
}
inline Gate gate(const Image& image,uintptr_t entry,uintptr_t call,uintptr_t ui_getter,uintptr_t initialized) {
    using namespace x64;
    Cursor prefix(image,entry,512);
    const auto push=prefix.take(), stack=prefix.take();
    require(push.push() && stack.stack_adjust(true),"button gate prologue differs");
    const int self=push.stack_reg();
    require(self==3 || self==6 || self==7,"button gate saved receiver differs");
    require(prefix.take().mov(self,1,true),"button gate does not preserve its receiver");
    std::vector<Ins> before;
    while(prefix.at<call)before.push_back(prefix.take());
    require(prefix.at==call,"button gate layout call is not instruction aligned");
    Cursor c(image,call,128);
    auto ui=c.take();
    require(ui.call() && ui.relative(image)==ui_getter && c.take().cmp(0,1),"button gate Mobile condition differs");
    const auto reject=c.take();require(reject.condition(5),"button gate must reject non-Mobile UI");
    uintptr_t shortcut{},accepted{};
    if(c.peek().cmp_zero_byte(rip)) {
        shortcut=c.take().storage(image);writable(image,shortcut,1);
        const auto branch=c.take();require(branch.condition(5),"button shortcut must accept only a nonzero flag");
        accepted=branch.relative(image);
    }
    const auto klass=c.take();require(klass.mov_load() && klass.reg()==1 && klass.base()==rip,"button gate device class differs");
    image.slot(klass.storage(image));
    const auto ready=c.take();
    require(ready.cmp_zero_byte(1) && ready.disp()==int64_t(initialized),"button gate class initialization differs");
    const auto slow=c.take();require(slow.condition(4),"button gate initialization branch differs");
    const auto argument=c.take();
    require(argument.h.opcode==0xb1 && argument.h.len==2 && argument.imm()==1,"button gate device argument differs");
    const auto device=c.take();require(device.call(),"button gate device getter missing");
    const auto compare=c.take();require(compare.cmp(0,1) && compare.h.len==3,"button gate Handheld comparison differs");
    const auto equal=c.take();
    if(shortcut) {
        require(equal.condition(5) && equal.relative(image)==reject.relative(image) && c.at==accepted,
            "button hardware rejection/shortcut edges differ");
        const auto truth=c.take();
        require(truth.h.opcode==0xb0+self && truth.imm()==1 && !truth.h.rex_b && (self<4 || truth.h.rex),
            "button gate success is not a saved true boolean");
    } else require(equal.h.opcode==0x0f && equal.h.opcode2==0x94 && equal.h.modrm_mod==3 && equal.rm()==self && (self<4 || equal.h.rex),
        "button gate does not return device equality");
    const auto done=c.take();require(done.jump(),"button gate result does not join its return");
    const auto success_end=boolean_return(image,done.relative(image),self,stack.imm());
    Cursor failure(image,reject.relative(image),32);
    require(failure.take().zero(self),"button gate rejection is not false");
    require(done.relative(image)==failure.at,"button true/false paths do not share their return");
    const auto failure_end=boolean_return(image,failure.at,self,stack.imm());
    Cursor initialize(image,slow.relative(image),32);
    require(initialize.take().call(),"button gate initialization call missing");
    std::optional<Site> initialization_site;
    if(shortcut) {
        const auto argument_again=initialize.take();
        require(argument_again.h.opcode==0xb1 && argument_again.h.len==2 && argument_again.imm()==1,
            "button initialization device argument differs");
        const auto device_again=initialize.take();
        require(device_again.call() && device_again.relative(image)==device.relative(image),
            "button initialization uses another device getter");
        const auto compare_again=initialize.take(), success=initialize.take();
        require(success.condition(4) && success.relative(image)==accepted,"button initialization equality must join success");
        const auto rejected=initialize.take();
        require(rejected.jump() && rejected.relative(image)==reject.relative(image),"button initialization failure differs");
        initialization_site=comparison_site(image,entry,compare_again,&success);
    } else {
        const auto retry=initialize.take();
        require(retry.jump() && retry.relative(image)==argument.at,"button gate initialization does not rejoin device check");
    }
    size_t active{};int busy=-1;
    for(size_t i=0;i+3<before.size();++i) {
        const auto& move=before[i];
        if(move.mov(1,self,true) && before[i+1].indirect_call() && before[i+1].base()==rip &&
            before[i+2].test(0,true) && before[i+3].condition(4) &&
            before[i+3].relative(image)==reject.relative(image)) {
            image.slot(before[i+1].storage(image));++active;
        }
        if(move.cmp_zero_byte(self)) {
            require(busy==-1 && move.disp()>=0x10 && move.disp()<0x1000,"ambiguous button gate reentrancy flag");
            const auto& branch=before[i+1];
            require(branch.condition(4) && branch.next()==reject.relative(image),"button gate reentrancy rejection differs");
            const auto target=branch.relative(image);
            require(target>=failure_end && target<=call &&
                (target==call || std::any_of(before.begin(),before.end(),[&](const auto& ins){return ins.at==target;})),
                "button gate reentrancy continuation differs");
            busy=move.disp();
        }
    }
    require(active==1,"button gate active-component check missing or ambiguous");
    Gate result{comparison_site(image,entry,compare),device.relative(image),klass.storage(image),self,busy};
    result.initialization_site=initialization_site;result.shortcut=shortcut;
    const auto end=std::max({c.at,success_end,failure_end,initialize.at});
    require(end>entry && end-entry<=1024,"button gate evidence is reversed or too large");
    size_t device_calls{};Cursor all(image,entry,end-entry);
    while(all.at<end) {const auto instruction=all.take();if(instruction.call() && instruction.relative(image)==result.device_getter)++device_calls;}
    require(device_calls==(shortcut?2u:1u),"button gate has unproven additional hardware calls");
    remember(image,result.evidence,entry,end-entry);
    return result;
}
inline uintptr_t application(const Image& image,Gate& current) {
    using namespace x64;
    std::map<uintptr_t,std::vector<CodeEvidence>> matches;
    for(auto call:calls_to(image,current.site.gate)) {
        for(uintptr_t start=call>32?call-32:0;start<call;++start)try {
            Cursor c(image,start,64);
            const auto ready=c.take();
            require(ready.h.opcode==0x80 && ready.h.modrm_reg==7 && ready.imm()==0 && ready.base()>=0 &&
                ready.index()==none && ready.disp()>=0x10 && ready.disp()<0x1000,"button apply readiness differs");
            const int self=ready.base();
            require(c.take().condition(4) && c.take().mov(1,self,true) && c.at==call,"button apply receiver differs");
            require(c.take().call() && c.take().test(0,true),"button apply gate result missing");
            const auto accepted=c.take();require(accepted.condition(5),"button apply success branch differs");
            require(c.take().jump(),"button apply failure does not exit");
            const auto marker=decode(image,accepted.relative(image));
            const int expected=current.busy<0?ready.disp()+2:current.busy;
            require(byte_store(marker,self,expected),"button apply marker does not match its gate");
            if(current.busy>=0)require(current.busy==ready.disp()+3,"position readiness and reentrancy flags differ");
            std::vector<CodeEvidence> evidence;
            remember(image,evidence,start,c.at-start);
            remember(image,evidence,marker.at,marker.h.len);
            matches.emplace(call,std::move(evidence));
        }catch(const std::runtime_error&){}
    }
    require(matches.size()==1,"expected one validated button application caller, got "+std::to_string(matches.size()));
    const auto& selected=*matches.begin();
    current.evidence.insert(current.evidence.end(),selected.second.begin(),selected.second.end());
    return selected.first;
}
inline std::pair<uintptr_t,uintptr_t> cached_device(const Image& image,uintptr_t fn,uintptr_t owner,uintptr_t initialized,std::vector<CodeEvidence>& evidence) {
    using namespace x64;Cursor c(image,fn,128);
    const auto saved=c.take(),stack=c.take();
    require(saved.push() && stack.stack_adjust(true) && c.take().mov(saved.stack_reg(),1,false),"device cache prologue differs");
    for(int i=0;i<2;++i)require(c.take().cmp_zero_byte(rip) && c.take().condition(i?5:4),"device cache dispatch guards differ");
    const auto klass=c.take();require(klass.mov_load() && klass.reg()==1 && klass.base()==rip && klass.storage(image)==owner,"device cache owner differs");
    const auto ready=c.take();
    require(ready.h.opcode==0x0f && ready.h.opcode2==0xb6 && ready.reg()==0 && ready.memory(1,static_cast<int32_t>(initialized)),"device cache initialization differs");
    require(c.take().test(0,true) && c.take().condition(4),"device cache initialization branch differs");
    const auto cached=c.take();require(cached.cmp_zero_byte(rip),"device cache ready flag missing");
    require(c.take().condition(4) && c.take().test(0,true) && c.take().condition(4),"device cache warm path differs");
    const auto value=c.take();
    require(value.h.opcode==0x8b && !value.h.rex_w && value.reg()==0 && value.base()==rip,"device cache integer load missing");
    const auto release=c.take(),pop=c.take();
    require(release.stack_adjust(false) && release.imm()==stack.imm() && pop.pop() && pop.stack_reg()==saved.stack_reg() && c.take().ret(),"device cache return differs");
    const auto cache=value.storage(image),flag=cached.storage(image);
    writable(image,cache,4);writable(image,flag,1);
    require(cache%4==0 && (flag<cache || flag>=cache+4),"device cache and flag overlap");
    remember(image,evidence,fn,c.at-fn);
    return {cache,flag};
}
inline Plan discover(const Image& image,uintptr_t ui_getter,uintptr_t initialized,const std::vector<uintptr_t>& references) {
    std::map<uintptr_t,Gate> gates;
    for(auto call:references) {
        const auto lower=call>512?call-512:0;
        for(uintptr_t entry=(lower+15)&~uintptr_t{15};entry<call;entry+=16)try {
            auto found=gate(image,entry,call,ui_getter,initialized);
            found.site.apply=application(image,found);
            gates.emplace(entry,found);
        }catch(const std::runtime_error&){}
    }
    require(gates.size()==2,"expected exactly two validated button layout gates, got "+std::to_string(gates.size()));
    std::optional<Gate> position,size;
    for(const auto& [entry,current]:gates) {
        auto& selected=current.busy<0?size:position;
        require(!selected,"button layout handler roles are ambiguous");selected=current;
    }
    require(position && size && position->device_getter==size->device_getter && position->device_class==size->device_class,
        "button layout handlers do not share a device source");
    require(position->shortcut==size->shortcut && position->initialization_site.has_value()==size->initialization_site.has_value(),
        "button layout handlers disagree on shortcut/initialization shape");
    require(position->site.word!=size->site.word,"button layout patch words alias");
    Plan result{{position->site,size->site},position->device_getter};
    if(position->initialization_site) {
        auto first=*position->initialization_site,second=*size->initialization_site;
        first.apply=position->site.apply;second.apply=size->site.apply;
        result.sites.push_back(first);result.sites.push_back(second);
    }
    std::set<uintptr_t> words;
    for(const auto& site:result.sites)require(words.insert(site.word).second,"button layout patch words alias");
    result.evidence=std::move(position->evidence);
    result.evidence.insert(result.evidence.end(),size->evidence.begin(),size->evidence.end());
    const auto [cache,flag]=cached_device(image,position->device_getter,position->device_class,initialized,result.evidence);
    result.device_cache=cache;result.device_ready=flag;
    return result;
}
inline Resolution resolve(const Image& image,uintptr_t ui_getter,uintptr_t initialized,const std::vector<uintptr_t>& references) {
    try {return {discover(image,ui_getter,initialized,references),{}};}
    // A read failure in this optional matcher rejects the entire feature,
    // never just a candidate. Core capture/discovery must succeed separately.
    catch(const std::exception& ex){return {{},ex.what()};}
}
}
