#pragma once
#include "x64_reader.hpp"
#include <set>

namespace discovery {
struct Property {
    uintptr_t klass{},pool{},pool_offset{},field{},initialized{},interface_slot{},lookup_helper{};
    uintptr_t field_at{},pool_offset_at{},initialized_at{},class_load{},pool_load{},interface_load{};
    unsigned field_size{};
    size_t main_size{};
};
inline bool same_owner(const Property& a,const Property& b) {
    return a.klass==b.klass && a.pool==b.pool && a.pool_offset==b.pool_offset && a.initialized==b.initialized &&
        a.interface_slot==b.interface_slot && a.lookup_helper==b.lookup_helper;
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
    const auto method=c.take(),context=c.take();
    require(method.mov_load() && method.memory(4,temporary) && context.mov_load() && context.memory(4,temporary+8) && context.reg()==(setter?8:2),"invalid interface lookup result");
    const auto join=c.take();require(join.jump(),"missing interface invocation join");
    require(fast.relative(image)==c.at,"interface match does not reach direct dispatch");
    const auto offset=c.take();
    require(offset.memory(table.reg(),8,index) && (setter?offset.h.opcode==0x8b:offset.h.opcode==0x63),"invalid interface method index");
    int method_index=offset.reg();
    if(setter) {
        const auto next=c.take();require(next.h.opcode==0xff && next.h.modrm_reg==0 && next.h.modrm_mod==3 && next.rm()==method_index && !next.h.rex_w,"setter is not interface slot one");
        const auto sign=c.take();require(sign.h.opcode==0x63 && sign.h.modrm_mod==3 && sign.rm()==method_index && sign.h.rex_w,"missing signed method index");method_index=sign.reg();
    }
    const auto direct=c.take();require(direct.mov_load() && direct.base()==vptr.reg() && direct.index()==method_index && direct.scale()==8 && direct.reg()==method.reg(),"invalid direct method lookup");
    const auto save_method=c.take();require(save_method.mov_store() && save_method.reg()==method.reg() && save_method.memory(4,temporary),"direct method does not share lookup output");
    const auto context_count=c.take();require(context_count.h.opcode==0x0f && context_count.h.opcode2==0xb7 && context_count.base()==vptr.reg() && context_count.index()==none,"missing method context table count");
    const auto add=c.take();require(add.h.opcode==0x01 && add.h.modrm_mod==3 && add.reg()==method_index && add.rm()==context_count.reg(),"invalid method context index");
    const auto direct_context=c.take();require(direct_context.mov_load() && direct_context.memory(vptr.reg(),direct.disp(),context_count.reg(),8) && direct_context.reg()==context.reg(),"invalid method context lookup");
    const auto save_context=c.take();require(save_context.mov_store() && save_context.reg()==context.reg() && save_context.memory(4,temporary+8),"invalid method context output");
    require(join.relative(image)==c.at,"lookup result does not reach invocation");
    require(c.take().mov(1,object,true),"interface call does not receive the property object");
    if(setter)require(c.take().mov(2,argument,false),"setter does not pass its preserved input value");
    const auto invoke=c.take();require(invoke.indirect_call() && invoke.h.modrm_mod==3 && invoke.rm()==method.reg(),"wrong interface call target");
    c.nops();const auto release=c.take();require(release.stack_adjust(false) && release.imm()==allocate.imm(),"accessor stack cleanup differs");
    for(auto it=saved.rbegin();it!=saved.rend();++it) {auto pop=c.take();require(pop.pop() && pop.stack_reg()==*it,"accessor register restore differs");}
    require(c.take().ret(),"accessor does not return after invocation");p.main_size=c.at-fn;
    require(p.field>=16 && p.field<=4096 && p.field%8==0 && (p.field_size==1 || p.field_size==4),"invalid property offset");
    require(p.pool_offset>0 && p.pool_offset<0x1000000 && p.pool_offset%8==0,"invalid static state offset");
    require(p.initialized>=64 && p.initialized<1024,"invalid class initialized offset");
    require(table.disp()>=0 && table.disp()<1024 && table.disp()%8==0 && direct.disp()>0 && direct.disp()<4096 && direct.disp()%8==0,"invalid interface table layout");
    require(p.klass!=p.pool && p.klass!=p.interface_slot && p.pool!=p.interface_slot,"property storage aliases");
    require(object!=method_index && object!=method.reg() && object!=context_count.reg() && vptr.reg()!=method_index && vptr.reg()!=context_count.reg(),"interface dispatch clobbers its object or vtable");
    if(setter)require(argument!=index && argument!=object && argument!=method.reg() && argument!=context_count.reg() &&
        argument!=table.reg() && argument!=vptr.reg() && argument!=count.reg() && argument!=method_index,"setter input is overwritten before invocation");
    image.zero_slot(p.klass);image.zero_slot(p.pool);image.zero_slot(p.interface_slot);
    return p;
}
}
