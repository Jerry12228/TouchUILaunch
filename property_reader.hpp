#pragma once
#include "dispatch_reader.hpp"
#include <set>

namespace discovery {
struct Property {
    uintptr_t klass{},pool{},pool_offset{},field{},initialized{},interface_slot{},lookup_helper{};
    uintptr_t field_at{},pool_offset_at{},initialized_at{},class_load{},pool_load{},interface_load{};
    unsigned field_size{};
    size_t main_size{};
    Dispatch invocation;
};
inline bool same_owner(const Property& a,const Property& b) {
    return a.klass==b.klass && a.pool==b.pool && a.pool_offset==b.pool_offset && a.initialized==b.initialized &&
        a.interface_slot==b.interface_slot && a.lookup_helper==b.lookup_helper &&
        a.invocation.method_word==b.invocation.method_word && a.invocation.context_word==b.invocation.context_word &&
        a.invocation.table_offset==b.invocation.table_offset && a.invocation.context_count_offset==b.invocation.context_count_offset;
}
inline Property property(const Image& image,uintptr_t fn,bool setter) {
    using namespace x64;
    Cursor c(image,fn,512);Property p;
    std::vector<int> saved;
    while(c.peek().push()) {saved.push_back(c.take().stack_reg());require(saved.size()<=8,"accessor prologue is too long");}
    require(!saved.empty(),"accessor has no register-save prologue");
    const auto allocate=c.take();require(allocate.stack_adjust(true) && allocate.imm()>=16 && allocate.imm()<=4096,"invalid accessor stack allocation");
    int argument=none;
    if(setter) {
        const auto save=c.take();argument=save.h.opcode==0x89?save.rm():save.reg();
        require(save.mov(argument,1,false) && std::find(saved.begin(),saved.end(),argument)!=saved.end(),"setter does not preserve its input value");
        require(argument==3 || argument==5 || argument==6 || argument==7 || argument>=12,"setter input must survive helper calls");
    }
    size_t guards{};
    while(c.peek().cmp_zero_byte(rip)) {
        c.take();const auto branch=c.take();require(branch.condition(4)||branch.condition(5),"invalid metadata guard");
        require(++guards<=4,"too many accessor guards");
    }
    require(guards>0,"missing accessor initialization guard");
    const auto klass=c.take();require(klass.mov_load() && klass.base()==rip,"missing property class load");
    p.class_load=klass.at;p.klass=klass.storage(image);
    const auto initialized=c.take();require(initialized.cmp_zero_byte(klass.reg()),"missing property initialization byte");
    p.initialized=initialized.disp();p.initialized_at=initialized.displacement_at();
    require(c.take().condition(4),"unexpected class initialization branch");
    const auto pool=c.take();require(pool.mov_load() && pool.base()==rip,"missing static reference pool load");
    p.pool_load=pool.at;p.pool=pool.storage(image);
    const auto state=c.take();require(state.mov_load() && state.base()==pool.reg() && state.index()==none,"invalid state object load");
    p.pool_offset=state.disp();p.pool_offset_at=state.displacement_at();
    require(c.take().test(state.reg()) && c.take().condition(4),"missing state object null guard");
    const auto field=c.take();require(field.mov_load() && field.base()==state.reg() && field.index()==none,"invalid property member load");
    p.field=field.disp();p.field_at=field.displacement_at();p.field_size=field.displacement_size();
    const int object=field.reg();
    require(c.take().test(object) && c.take().condition(4),"missing property object null guard");
    const auto iface=c.take();require(iface.mov_load() && iface.base()==rip && iface.reg()==8,"missing property interface argument");
    p.interface_slot=iface.storage(image);p.interface_load=iface.at;
    const auto vptr=c.take();require(vptr.mov_load() && vptr.memory(object,0),"missing property vtable load");
    const auto count=c.take();require(count.h.opcode==0x0f && count.h.opcode2==0xb7 && count.base()==vptr.reg() && count.index()==none,"missing interface count");
    require(c.take().test(count.reg()),"invalid interface count test");
    const auto empty=c.take();require(empty.condition(4),"missing interface fallback branch");
    const auto table=c.take();require(table.mov_load() && table.base()==vptr.reg() && table.index()==none,"missing interface table load");
    const auto shift=c.take();require(shift.h.opcode==0xc1 && shift.h.modrm_reg==4 && shift.rm()==count.reg() && shift.imm()==4,"interface entries no longer have 16-byte stride");
    const auto zero=c.take();const int index=zero.rm();require(zero.zero(index),"missing interface loop index");
    require(std::set<int>{object,vptr.reg(),count.reg(),table.reg(),index,8}.size()==6,"interface loop clobbers a live register");
    c.nops();const auto compare=c.take();
    require(compare.h.opcode==0x39 && compare.reg()==8 && compare.memory(table.reg(),0,index),"interface identity is not compared");
    const auto fast=c.take();require(fast.condition(4),"missing interface match branch");
    const auto increment=c.take();require(increment.h.opcode==0x83 && increment.h.modrm_reg==0 && increment.rm()==index && increment.imm()==16,"invalid interface iteration stride");
    const auto bound=c.take();require(bound.h.opcode==0x39 && bound.h.modrm_mod==3 &&
        ((bound.reg()==count.reg() && bound.rm()==index)||(bound.reg()==index && bound.rm()==count.reg())),"invalid interface loop bound");
    const auto again=c.take();require(again.condition(5) && again.relative(image)==compare.at,"invalid interface loop edge");
    require(empty.relative(image)==c.at,"empty interface table does not reach lookup");
    const auto output=c.take();require(output.h.opcode==0x8d && output.reg()==1 && output.base()==4 && output.index()==none,"invalid interface lookup output");
    const int32_t temporary=output.disp();
    require(temporary>=0 && uint64_t(temporary)+16<=allocate.imm(),"lookup output exceeds stack allocation");
    require(c.take().mov(2,object,true),"lookup does not receive the property object");
    const auto slot=c.take();require(setter?slot.immediate(9,1):slot.zero(9),"wrong getter/setter interface slot");
    const auto lookup=c.take();require(lookup.call(),"missing interface lookup call");p.lookup_helper=lookup.relative(image);
    if(setter)require(argument!=index && argument!=object && argument!=table.reg() &&
        argument!=vptr.reg() && argument!=count.reg(),"setter input is overwritten before dispatch");
    p.invocation=dispatch(image,c,object,argument,vptr.reg(),table.reg(),index,temporary,setter,fast.relative(image));
    c.nops();const auto release=c.take();require(release.stack_adjust(false) && release.imm()==allocate.imm(),"accessor stack cleanup differs");
    for(auto it=saved.rbegin();it!=saved.rend();++it) {auto pop=c.take();require(pop.pop() && pop.stack_reg()==*it,"accessor register restore differs");}
    require(c.take().ret(),"accessor does not return after invocation");p.main_size=c.at-fn;
    require(p.field>=16 && p.field<=4096 && p.field%8==0 && (p.field_size==1 || p.field_size==4),"invalid property offset");
    require(p.pool_offset>0 && p.pool_offset<0x1000000 && p.pool_offset%8==0,"invalid static state offset");
    require(p.initialized>=64 && p.initialized<1024,"invalid class initialized offset");
    require(table.disp()>=0 && table.disp()<1024 && table.disp()%8==0,"invalid interface table layout");
    require(p.klass!=p.pool && p.klass!=p.interface_slot && p.pool!=p.interface_slot,"property storage aliases");
    image.slot(p.klass);image.slot(p.pool);image.slot(p.interface_slot);
    return p;
}
}
