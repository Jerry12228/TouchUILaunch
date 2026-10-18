#pragma once
#include "x64_reader.hpp"
#include <array>
#include <map>
#include <optional>
#include <set>

namespace discovery::button_layout {
struct Site {
    uintptr_t gate{}, compare{}, word{}, apply{};
    uint64_t expected{}, replacement{};
};
struct Plan {
    std::array<Site,2> sites{}; // Position, then size. Never a global device override.
    uintptr_t device_getter{}, device_cache{}, device_ready{};
};
struct Resolution {
    std::optional<Plan> plan;
    std::string error;
};
struct Gate {
    Site site;
    uintptr_t device_getter{}, device_class{};
    int object{}, busy=-1;
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
    require(equal.h.opcode==0x0f && equal.h.opcode2==0x94 && equal.h.modrm_mod==3 && equal.rm()==self && (self<4 || equal.h.rex),
        "button gate does not return device equality");
    const auto done=c.take();require(done.jump(),"button gate result does not join its return");
    boolean_return(image,done.relative(image),self,stack.imm());
    Cursor failure(image,reject.relative(image),32);
    require(failure.take().zero(self),"button gate rejection is not false");
    boolean_return(image,failure.at,self,stack.imm());
    Cursor initialize(image,slow.relative(image),24);
    require(initialize.take().call(),"button gate initialization call missing");
    const auto retry=initialize.take();
    require(retry.jump() && retry.relative(image)==argument.at,"button gate initialization does not rejoin device check");
    bool active{};int busy=-1;
    for(size_t i=0;i+4<before.size();++i) {
        const auto& move=before[i];
        if(move.mov(1,self,true) && before[i+1].indirect_call() && before[i+1].base()==rip &&
            before[i+2].test(0,true) && before[i+3].condition(4) &&
            before[i+3].relative(image)==reject.relative(image))active=true;
        if(move.cmp_zero_byte(self)) {
            require(busy==-1 && move.disp()>=0x10 && move.disp()<0x1000,"ambiguous button gate reentrancy flag");
            require(before[i+1].condition(4),"button gate reentrancy branch differs");
            busy=move.disp();
        }
    }
    require(active,"button gate active-component check missing");
    const auto word=compare.at&~uintptr_t{7};
    require(compare.at-word<=5,"button gate comparison crosses an atomic patch word");
    Site site{entry,compare.at,word};
    site.expected=image.read<uint64_t>(word);
    auto bytes=image.view(word,8);std::array<uint8_t,8> patched{};std::copy(bytes.begin(),bytes.end(),patched.begin());
    const auto offset=compare.at-word;
    // cmp eax,eax; nop: remove this local hardware condition, leaving Mobile,
    // active-component and reentrancy checks intact. The device getter is unchanged.
    patched[offset]=0x39;patched[offset+1]=0xc0;patched[offset+2]=0x90;
    std::memcpy(&site.replacement,patched.data(),8);
    return {site,device.relative(image),klass.storage(image),self,busy};
}
inline uintptr_t application(const Image& image,const Gate& current) {
    using namespace x64;
    std::set<uintptr_t> matches;
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
            matches.insert(call);
        }catch(const std::runtime_error&){}
    }
    require(matches.size()==1,"expected one validated button application caller, got "+std::to_string(matches.size()));
    return *matches.begin();
}
inline std::pair<uintptr_t,uintptr_t> cached_device(const Image& image,uintptr_t fn,uintptr_t owner,uintptr_t initialized) {
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
    require(position->site.word!=size->site.word,"button layout patch words alias");
    const auto [cache,flag]=cached_device(image,position->device_getter,position->device_class,initialized);
    return {{position->site,size->site},position->device_getter,cache,flag};
}
inline Resolution resolve(const Image& image,uintptr_t ui_getter,uintptr_t initialized,const std::vector<uintptr_t>& references) {
    try {return {discover(image,ui_getter,initialized,references),{}};}
    catch(const std::runtime_error& ex){return {{},ex.what()};}
}
}
