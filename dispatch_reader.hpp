#pragma once
#include "x64_reader.hpp"
#include <map>

namespace discovery {
// A bounded symbolic interpreter for the two interface-dispatch paths. Values
// name their purpose rather than a compiler-selected register or stack word.
// It reads instructions as data; no client instruction or helper is executed.
namespace dispatch_flow {
enum class Kind { unknown, object, input, vtable, interfaces, cursor,
    raw_index, method_index, context_count, context_index, function, context, lookup_word };
struct Value {
    Kind kind{};
    int32_t detail{};
    bool operator==(const Value&)const=default;
};
struct State {
    std::array<Value,16> registers{};
    std::map<int32_t,Value> stack;
    int32_t temporary{};
    int32_t context_count_offset{-1};
    Value get(int reg)const {require(reg>=0 && reg<16,"invalid dispatch register");return registers[reg];}
    void put(int reg,Value value) {require(reg>=0 && reg<16 && reg!=4,"invalid dispatch destination");registers[reg]=value;}
    bool word(int32_t offset)const {return offset==temporary || offset==temporary+8;}
    static bool preserved(int reg) {return reg==3 || reg==5 || reg==6 || reg==7 || reg>=12;}

    void step(const x64::Ins& i) {
        using enum Kind;
        if(i.nop())return;
        const auto& h=i.h;
        require(!h.p_66 && !h.p_rep,"unsupported dispatch operand prefix");
        if((h.opcode==0x89 || h.opcode==0x8b) && h.modrm_mod==3) {
            const int dst=h.opcode==0x89?i.rm():i.reg(),src=h.opcode==0x89?i.reg():i.rm();
            const auto value=get(src);
            require(value.kind!=unknown,"dispatch copies an unknown value");
            require(h.rex_w || value.kind==input || value.kind==raw_index,"dispatch truncates a live pointer or index");
            put(dst,value);return;
        }
        if(i.mov_load() && i.base()==4 && i.index()==x64::none) {
            const auto it=stack.find(i.disp());
            require(word(i.disp()) && it!=stack.end(),"dispatch reads an unproven result word");
            put(i.reg(),it->second);return;
        }
        if(i.mov_store() && i.base()==4 && i.index()==x64::none) {
            const auto value=get(i.reg());
            require(word(i.disp()) && value.kind!=unknown,"dispatch writes outside its proven result");
            stack[i.disp()]=value;return;
        }
        if((h.opcode==0x8b && !h.rex_w) || (h.opcode==0x63 && h.rex_w)) {
            if(h.modrm_mod!=3 && i.base()!=x64::none && i.base()!=x64::rip && i.index()!=x64::none &&
                get(i.base()).kind==interfaces && get(i.index()).kind==cursor && i.disp()==8 && i.scale()==1) {
                put(i.reg(),{h.opcode==0x63?method_index:raw_index,0});return;
            }
            if(h.opcode==0x63 && h.modrm_mod==3 && get(i.rm()).kind==raw_index) {
                put(i.reg(),{method_index,get(i.rm()).detail});return;
            }
        }
        if(h.opcode==0xff && h.modrm_mod==3 && h.modrm_reg==0 && !h.rex_w && get(i.rm()).kind==raw_index) {
            const auto value=get(i.rm());require(value.detail==0,"interface slot increment is not one");
            put(i.rm(),{raw_index,1});return;
        }
        if(h.opcode==0x0f && h.opcode2==0xb7 && h.modrm_mod!=3 && i.base()>=0 &&
            get(i.base()).kind==vtable && i.index()==x64::none) {
            require(i.disp()>=0 && i.disp()<1024 && i.disp()%2==0,"invalid context table count offset");
            require(context_count_offset<0 || context_count_offset==i.disp(),"inconsistent context table counts");
            context_count_offset=i.disp();put(i.reg(),{context_count,i.disp()});return;
        }
        if((h.opcode==0x01 || h.opcode==0x03) && h.modrm_mod==3 && h.rex_w) {
            const int dst=h.opcode==0x01?i.rm():i.reg(),src=h.opcode==0x01?i.reg():i.rm();
            const auto a=get(dst),b=get(src);
            require((a.kind==context_count && b.kind==method_index)||(a.kind==method_index && b.kind==context_count),"invalid context index data flow");
            put(dst,{context_index,a.kind==method_index?a.detail:b.detail});return;
        }
        if(i.mov_load() && i.base()>=0 && get(i.base()).kind==vtable && i.index()>=0 && i.scale()==8) {
            const auto index=get(i.index());
            require(index.kind==method_index || index.kind==context_index,"unproven dispatch table index");
            require(index.detail==expected_slot,"wrong getter/setter direct interface slot");
            require(i.disp()>0 && i.disp()<4096 && i.disp()%8==0,"invalid dispatch table base");
            put(i.reg(),{index.kind==method_index?function:context,i.disp()});return;
        }
        throw std::runtime_error("unsupported interface dispatch data flow");
    }
    int expected_slot{};
};
}
struct Dispatch {
    int32_t method_word{},context_word{},table_offset{},context_count_offset{};
    uintptr_t slow_start{},fast_start{},join{},invoke{};
};
inline Dispatch dispatch(const Image& image,x64::Cursor& c,int object_reg,int argument,int vptr,int table,int index,
                         int32_t temporary,bool setter,uintptr_t fast_start) {
    using namespace dispatch_flow;using enum Kind;
    Dispatch result;result.slow_start=c.at;result.fast_start=fast_start;
    State slow,fast;slow.temporary=fast.temporary=temporary;slow.expected_slot=fast.expected_slot=setter?1:0;
    require(State::preserved(object_reg) && (!setter || State::preserved(argument)),"dispatch inputs cannot survive lookup call");
    slow.put(object_reg,{Kind::object});fast.put(object_reg,{Kind::object});
    if(setter){slow.put(argument,{input});fast.put(argument,{input});}
    slow.stack={{temporary,{lookup_word,0}},{temporary+8,{lookup_word,8}}};
    fast.put(vptr,{vtable});fast.put(table,{interfaces});fast.put(index,{cursor});
    bool joined{};
    for(unsigned n=0;n<24;++n) {
        const auto i=c.take();
        if(i.jump()) {result.join=i.relative(image);joined=true;break;}
        slow.step(i);
    }
    require(joined && c.at==fast_start && result.join>fast_start && result.join-fast_start<=256,"invalid interface path join");
    unsigned steps{};
    while(c.at<result.join) {require(++steps<=48,"interface fast path exceeds bound");fast.step(c.take());}
    require(c.at==result.join,"interface join is not instruction-aligned");
    bool called{};
    for(unsigned n=0;n<24;++n) {
        const auto i=c.take();
        if(i.indirect_call() && i.h.modrm_mod==3) {
            const int context_register=setter?8:2;
            require(slow.get(1).kind==Kind::object && fast.get(1).kind==Kind::object,"interface invocation loses property object");
            if(setter)require(slow.get(2).kind==input && fast.get(2).kind==input,"setter invocation loses input value");
            const auto slow_fn=slow.get(i.rm()),slow_context=slow.get(context_register);
            require(slow_fn.kind==lookup_word && slow_context.kind==lookup_word && slow_fn.detail!=slow_context.detail,
                "interface result words do not independently supply target and context");
            const auto fast_fn=fast.get(i.rm()),fast_context=fast.get(context_register);
            require(fast_fn.kind==function && fast_context.kind==context && fast_fn.detail==fast_context.detail,
                "interface paths disagree on call target or context");
            require(fast.stack.contains(temporary+slow_fn.detail) && fast.stack.at(temporary+slow_fn.detail)==fast_fn &&
                fast.stack.contains(temporary+slow_context.detail) && fast.stack.at(temporary+slow_context.detail)==fast_context,
                "lookup and direct paths disagree on result member roles");
            result.method_word=slow_fn.detail;result.context_word=slow_context.detail;
            result.table_offset=fast_fn.detail;result.context_count_offset=fast.context_count_offset;
            result.invoke=i.at;called=true;break;
        }
        slow.step(i);fast.step(i);
    }
    require(called,"missing bounded interface invocation");return result;
}
}
