#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>
#include "scan_patterns.hpp"

namespace discovery {
inline void require(bool condition,const std::string& message) {
    if(!condition)throw std::runtime_error("Automatic profile: "+message);
}
struct Section {
    uint32_t rva{},size{},raw{},raw_size{},flags{};
    bool code()const{return (flags&0x60000000)==0x60000000 && !(flags&0x80000000);}
    bool data()const{return (flags&0xc0000000)==0xc0000000 && !(flags&0x20000000);}
};

// One RVA interface for raw files and captured module code. Neither executes code.
class Image {
    std::span<const uint8_t> bytes_;
    using Reader=std::function<void(uintptr_t,std::span<uint8_t>)>;
    Reader memory_read_;
    std::vector<std::vector<uint8_t>> code_;
    template<class T> T raw(size_t offset)const {
        require(offset<=bytes_.size() && sizeof(T)<=bytes_.size()-offset,"truncated PE read");
        T value{};std::memcpy(&value,bytes_.data()+offset,sizeof(T));return value;
    }
public:
    uint32_t image_size{};
    std::vector<Section> sections;
    explicit Image(std::span<const uint8_t> bytes,Reader memory_read={}):bytes_(bytes),memory_read_(std::move(memory_read)) {
        require(raw<uint16_t>(0)==0x5a4d,"missing DOS signature");
        const size_t pe=raw<uint32_t>(0x3c);
        require(raw<uint32_t>(pe)==0x4550 && raw<uint16_t>(pe+4)==0x8664,"expected x64 PE");
        const auto count=raw<uint16_t>(pe+6),optional_size=raw<uint16_t>(pe+20);
        require(count>0 && count<=96 && optional_size>=112,"invalid PE headers");
        const size_t opt=pe+24;
        require(raw<uint16_t>(opt)==0x20b,"expected PE32+");
        image_size=raw<uint32_t>(opt+56);
        const auto headers=raw<uint32_t>(opt+60);
        require(image_size>0 && image_size<=0x80000000 && headers<=image_size && headers<=bytes_.size(),"invalid PE size");
        require(opt+optional_size+size_t(count)*40<=headers,"section table outside headers");
        for(size_t i=0;i<count;++i) {
            const size_t at=opt+optional_size+i*40;
            Section s{raw<uint32_t>(at+12),std::max(raw<uint32_t>(at+8),raw<uint32_t>(at+16)),raw<uint32_t>(at+20),raw<uint32_t>(at+16),raw<uint32_t>(at+36)};
            require(s.rva>=headers && uint64_t(s.rva)+s.size<=image_size,"section outside image");
            require(memory_read_ || !s.raw_size || (s.raw>=headers && uint64_t(s.raw)+s.raw_size<=bytes_.size()),"section outside file");
            for(const auto& prev:sections) {
                require(uint64_t(s.rva)+s.size<=prev.rva || uint64_t(prev.rva)+prev.size<=s.rva,"overlapping virtual sections");
                require(memory_read_ || !s.raw_size || !prev.raw_size || uint64_t(s.raw)+s.raw_size<=prev.raw || uint64_t(prev.raw)+prev.raw_size<=s.raw,"overlapping raw sections");
            }
            sections.push_back(s);
        }
        if(memory_read_) {
            code_.resize(sections.size());
            for(size_t i=0;i<sections.size();++i)if(sections[i].code()) {
                code_[i].resize(sections[i].size);
                memory_read_(sections[i].rva,code_[i]);
            }
            bytes_={}; // Header bytes belong to the caller; all metadata is now copied.
        }
    }
    size_t backed_size(const Section& s)const{return memory_read_?s.size:s.raw_size;}
    const Section* section(uintptr_t rva,size_t length)const {
        for(const auto& s:sections)if(rva>=s.rva && rva-s.rva<=s.size && length<=s.size-(rva-s.rva))return &s;
        return nullptr;
    }
    std::span<const uint8_t> view(uintptr_t rva,size_t length)const {
        auto s=section(rva,length);
        require(s && rva-s->rva<=backed_size(*s) && length<=backed_size(*s)-(rva-s->rva),"RVA is not captured/backed");
        if(memory_read_) {
            require(s->code(),"only code is available in the module snapshot");
            return std::span<const uint8_t>(code_[s-sections.data()]).subspan(rva-s->rva,length);
        }
        return bytes_.subspan(s->raw+rva-s->rva,length);
    }
    template<class T> T read(uintptr_t rva)const {
        auto data=view(rva,sizeof(T));T value{};std::memcpy(&value,data.data(),sizeof(T));return value;
    }
    void code(uintptr_t rva,size_t length=1)const {
        auto s=section(rva,length);require(s && s->code(),"target is not read-only executable code");
        (void)view(rva,length);
    }
    void slot(uintptr_t rva)const {
        const auto* s=section(rva,8);
        require(rva%8==0 && s && s->data(),"slot is not aligned writable data");
        if(memory_read_) {
            uint8_t bytes[8]{};memory_read_(rva,bytes);
            return; // Initialized pointers are checked for readiness by the hook/UI flow.
        }
        for(size_t i=0;i<8;++i) {
            const auto offset=rva-s->rva+i;
            require(offset>=s->raw_size || bytes_[s->raw+offset]==0,"slot is not initially zero");
        }
    }
    void expect(uintptr_t rva,std::initializer_list<uint8_t> bytes)const {
        auto data=view(rva,bytes.size());
        require(std::equal(data.begin(),data.end(),bytes.begin()),"unexpected instruction encoding");
    }
    uintptr_t relative(uintptr_t rva,std::initializer_list<uint8_t> opcode)const {
        expect(rva,opcode);
        const auto delta=read<int32_t>(rva+opcode.size());
        const int64_t result=int64_t(rva)+int64_t(opcode.size())+4+delta;
        require(result>=0 && result<image_size,"relative target outside image");
        return static_cast<uintptr_t>(result);
    }
    bool matches(uintptr_t rva,const scan_rules::Pattern& rule)const {
        auto s=section(rva,rule.bytes.size());
        if(!s || !s->code() || rva-s->rva>backed_size(*s) || rule.bytes.size()>backed_size(*s)-(rva-s->rva))return false;
        auto data=view(rva,rule.bytes.size());
        for(size_t i=0;i<data.size();++i)if((data[i]&rule.mask[i])!=rule.bytes[i])return false;
        return true;
    }
    std::vector<uintptr_t> find(const scan_rules::Pattern& rule,size_t limit=64,size_t minimum_fixed=64)const {
        require(rule.bytes.size()==rule.mask.size() && !rule.bytes.empty(),"invalid scan rule");
        size_t anchor{},length{},run{};
        for(size_t i=0;i<rule.mask.size();++i) {
            run=rule.mask[i]==255?run+1:0;
            if(run>length){length=run;anchor=i+1-run;}
        }
        // Short anchors are candidates for instruction/relationship checks.
        // Preserved Unity rules retain the default 64-byte requirement.
        require(minimum_fixed>=8 && length>=2 && size_t(std::count(rule.mask.begin(),rule.mask.end(),uint8_t{255}))>=minimum_fixed,"scan rule lacks enough fixed bytes");
        std::vector<uintptr_t> result;
        for(const auto& s:sections) {
            if(!s.code() || backed_size(s)<rule.bytes.size())continue;
            const auto* data=view(s.rva,backed_size(s)).data();
            size_t pos=anchor;
            const size_t last=backed_size(s)-rule.bytes.size()+anchor;
            while(pos<=last) {
                auto hit=static_cast<const uint8_t*>(std::memchr(data+pos,rule.bytes[anchor],last-pos+1));
                if(!hit)break;
                pos=static_cast<size_t>(hit-data);
                if(std::memcmp(hit,rule.bytes.data()+anchor,length)==0 && matches(s.rva+pos-anchor,rule)) {
                    result.push_back(s.rva+pos-anchor);
                    require(result.size()<=limit,std::string(rule.name)+": excessive matches");
                }
                ++pos;
            }
        }
        return result;
    }
    uintptr_t unique(const scan_rules::Pattern& rule)const {
        auto result=find(rule,2);
        require(result.size()==1,std::string(rule.name)+": expected exactly one match, got "+std::to_string(result.size()));
        return result.front();
    }
};
}
