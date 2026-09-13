#pragma once
#include "pe_image.hpp"
#include "third_party/hde64/hde64.h"
#include <array>

namespace discovery::x64 {
constexpr int none=-1,rip=-2;
struct Ins {
    uintptr_t at{};
    hde64s h{};
    uintptr_t next()const{return at+h.len;}
    int reg()const{return h.modrm_reg+8*h.rex_r;}
    int rm()const{return h.modrm_rm+8*h.rex_b;}
    int base()const {
        if(!(h.flags&F_MODRM) || h.modrm_mod==3)return none;
        if(h.flags&F_SIB)return h.modrm_mod==0 && h.sib_base==5?none:h.sib_base+8*h.rex_b;
        return h.modrm_mod==0 && h.modrm_rm==5?rip:rm();
    }
    int index()const{return (h.flags&F_SIB) && (h.sib_index!=4 || h.rex_x)?h.sib_index+8*h.rex_x:none;}
    int scale()const{return (h.flags&F_SIB)?1<<h.sib_scale:1;}
    int32_t disp()const {
        if(h.flags&F_DISP8)return static_cast<int8_t>(h.disp.disp8);
        if(h.flags&F_DISP32)return static_cast<int32_t>(h.disp.disp32);
        return 0;
    }
    unsigned immediate_size()const{return (h.flags&F_IMM64)?8:(h.flags&F_IMM32)?4:(h.flags&F_IMM16)?2:(h.flags&F_IMM8)?1:0;}
    unsigned displacement_size()const{return (h.flags&F_DISP32)?4:(h.flags&F_DISP16)?2:(h.flags&F_DISP8)?1:0;}
    uintptr_t displacement_at()const{return next()-immediate_size()-displacement_size();}
    uint64_t imm()const{return (h.flags&F_IMM64)?h.imm.imm64:(h.flags&F_IMM32)?h.imm.imm32:(h.flags&F_IMM16)?h.imm.imm16:h.imm.imm8;}
    bool memory(int b,int32_t d,int idx=none,int s=1)const{return base()==b && disp()==d && index()==idx && scale()==s;}
    bool mov_load()const{return h.opcode==0x8b && h.modrm_mod!=3 && h.rex_w;}
    bool mov_store()const{return h.opcode==0x89 && h.modrm_mod!=3 && h.rex_w;}
    bool mov(int dst,int src,bool wide)const {
        return !h.p_66 && h.modrm_mod==3 && bool(h.rex_w)==wide &&
            ((h.opcode==0x89 && rm()==dst && reg()==src)||(h.opcode==0x8b && reg()==dst && rm()==src));
    }
    bool immediate(int dst,uint32_t value)const{return !h.p_66 && h.opcode>=0xb8 && h.opcode<=0xbf && int(h.opcode-0xb8+8*h.rex_b)==dst && imm()==value && !h.rex_w;}
    bool zero(int r)const{return !h.p_66 && h.modrm_mod==3 && (h.opcode==0x31 || h.opcode==0x33) && reg()==r && rm()==r;}
    bool test(int r,bool byte=false)const{return !h.p_66 && h.opcode==(byte?0x84:0x85) && h.modrm_mod==3 && reg()==r && rm()==r;}
    bool cmp(int r,uint8_t value)const{return !h.p_66 && h.opcode==0x83 && h.modrm_mod==3 && h.modrm_reg==7 && rm()==r && imm()==value && !h.rex_w;}
    bool cmp_zero_byte(int b)const{return h.opcode==0x80 && h.modrm_reg==7 && base()==b && index()==none && imm()==0;}
    bool condition(int code)const{return h.opcode==0x70+code || (h.opcode==0x0f && h.opcode2==0x80+code);}
    bool conditional()const{return (h.opcode>=0x70 && h.opcode<=0x7f) || (h.opcode==0x0f && h.opcode2>=0x80 && h.opcode2<=0x8f);}
    bool call()const{return h.opcode==0xe8;}
    bool jump()const{return h.opcode==0xe9 || h.opcode==0xeb;}
    bool indirect_call()const{return h.opcode==0xff && h.modrm_reg==2;}
    bool ret()const{return h.opcode==0xc3;}
    bool nop()const{return (h.opcode==0x90 && !h.rex_b) || (h.opcode==0x0f && h.opcode2==0x1f);}
    bool push()const{return h.opcode>=0x50 && h.opcode<=0x57;}
    bool pop()const{return h.opcode>=0x58 && h.opcode<=0x5f;}
    int stack_reg()const{return (h.opcode&7)+8*h.rex_b;}
    bool stack_adjust(bool subtract)const{return (h.opcode==0x81 || (h.opcode==0x83 && imm()<128)) && h.rex_w && h.modrm_mod==3 && rm()==4 && h.modrm_reg==(subtract?5:0);}
    uintptr_t relative(const Image& image)const {
        require(call() || jump() || conditional(),"not a relative control transfer");
        const int64_t delta=immediate_size()==1?static_cast<int8_t>(h.imm.imm8):static_cast<int32_t>(h.imm.imm32);
        const auto target=int64_t(next())+delta;
        require(target>=0 && target<image.image_size,"decoded branch outside image");
        return static_cast<uintptr_t>(target);
    }
    uintptr_t storage(const Image& image)const {
        require(base()==rip && !h.p_67,"expected RIP-relative storage");
        const auto target=int64_t(next())+disp();
        require(target>=0 && target<image.image_size,"decoded storage outside image");
        return static_cast<uintptr_t>(target);
    }
};
inline Ins decode(const Image& image,uintptr_t at) {
    image.code(at);
    const auto* section=image.section(at,1);
    const auto available=std::min<size_t>(15,image.backed_size(*section)-(at-section->rva));
    // HDE may look ahead while decoding malformed input. A padded local buffer
    // keeps those reads bounded; never accept a length beyond captured bytes.
    std::array<uint8_t,32> padded{};
    auto data=image.view(at,available);std::copy(data.begin(),data.end(),padded.begin());
    Ins result;result.at=at;hde64_disasm(padded.data(),&result.h);
    require(result.h.len && result.h.len<=available && !(result.h.flags&F_ERROR),"unsupported or truncated x64 instruction");
    require(!result.h.p_67 && !result.h.p_lock,"unsupported address/lock prefix in discovery evidence");
    require(!result.h.p_seg || (result.nop() && result.h.p_seg==0x2e),"unsupported segment prefix in discovery evidence");
    return result;
}
struct Cursor {
    const Image& image;uintptr_t at,end;
    Cursor(const Image& source,uintptr_t start,size_t length=512):image(source),at(start),end(start+length){}
    Ins take(){require(at<end,"instruction evidence exceeds bound");auto i=decode(image,at);require(i.next()<=end,"instruction evidence exceeds bound");at=i.next();return i;}
    Ins peek()const{return decode(image,at);}
    void nops(){while(at<end && peek().nop())take();}
};
// Short fixed anchors select candidates. Decode and target checks below are
// separate from uniqueness: a byte sequence alone never authorizes a hook.
inline std::vector<uintptr_t> calls_to(const Image& image,uintptr_t target,bool indirect=false) {
    std::vector<uintptr_t> hits;
    for(const auto& s:image.sections) {
        if(!s.code() || image.backed_size(s)<6)continue;
        auto data=image.view(s.rva,image.backed_size(s));
        const size_t length=indirect?6:5;
        for(size_t pos=0;pos+length<=data.size();++pos) {
            const bool match=indirect?(data[pos]==0xff && data[pos+1]==0x15):data[pos]==0xe8;
            if(!match)continue;
            int32_t displacement{};
            std::memcpy(&displacement,data.data()+pos+length-4,4);
            if(int64_t(s.rva+pos+length)+displacement==int64_t(target)){hits.push_back(s.rva+pos);require(hits.size()<=4096,"too many references to an anchor");}
        }
    }
    require(hits.size()<=4096,"too many references to an anchor");return hits;
}
inline std::vector<uintptr_t> loads_from(const Image& image,uintptr_t target) {
    std::vector<uintptr_t> hits;
    for(const auto& s:image.sections) {
        if(!s.code() || image.backed_size(s)<7)continue;
        auto data=image.view(s.rva,image.backed_size(s));
        for(size_t pos=0;pos+7<=data.size();++pos) {
            if((data[pos]&0xf8)!=0x48 || data[pos+1]!=0x8b || (data[pos+2]&0xc7)!=5)continue;
            int32_t displacement{};std::memcpy(&displacement,data.data()+pos+3,4);
            if(int64_t(s.rva+pos+7)+displacement==int64_t(target)){hits.push_back(s.rva+pos);require(hits.size()<=65536,"too many class references");}
        }
    }
    require(hits.size()<=65536,"too many class references: "+std::to_string(hits.size()));return hits;
}
}
