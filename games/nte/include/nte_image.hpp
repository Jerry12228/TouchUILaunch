#pragma once
#include "pe_image.hpp"
#include "win_util.hpp"
#include <memory>

namespace touchui::nte {
using MemoryReader=std::function<bool(uintptr_t,std::span<uint8_t>)>;
inline void require(bool value,const std::string& reason) {
    if(!value)throw std::runtime_error("NTE: "+reason);
}
inline bool read_memory(uintptr_t address,std::span<uint8_t> bytes) {
    SIZE_T count{};
    return address&&ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),
        bytes.data(),bytes.size(),&count)&&count==bytes.size();
}
template<class T> bool read_value(uintptr_t address,T& value) {
    return read_memory(address,{reinterpret_cast<uint8_t*>(&value),sizeof(value)});
}
template<class T> bool read_value(const MemoryReader& reader,uintptr_t address,T& value) {
    return reader(address,{reinterpret_cast<uint8_t*>(&value),sizeof(value)});
}
class MetadataPending:public std::runtime_error {
public:explicit MetadataPending(const std::string& reason):std::runtime_error("NTE: "+reason){}
};

// Own an immutable RVA-layout snapshot. Normalize only the private copy of PE
// section raw offsets so the shared bounded PE/x64 readers can read data too.
class ModuleImage {
    std::vector<uint8_t> bytes_;
    std::unique_ptr<discovery::Image> image_;
    template<class T> T get(size_t at)const {
        require(at<=bytes_.size()&&sizeof(T)<=bytes_.size()-at,"truncated image header");
        T value{};std::memcpy(&value,bytes_.data()+at,sizeof(value));return value;
    }
    void put32(size_t at,uint32_t value) {std::memcpy(bytes_.data()+at,&value,4);}
public:
    uintptr_t base{};
    uint32_t exception_rva{},exception_size{};
    ModuleImage(std::vector<uint8_t> bytes,uintptr_t address):bytes_(std::move(bytes)),base(address) {
        require(get<uint16_t>(0)==IMAGE_DOS_SIGNATURE,"missing DOS header");
        const size_t pe=get<uint32_t>(0x3c);
        require(get<uint32_t>(pe)==IMAGE_NT_SIGNATURE&&get<uint16_t>(pe+4)==IMAGE_FILE_MACHINE_AMD64,"expected x64 PE");
        const auto optional=pe+24;
        require(get<uint16_t>(optional)==IMAGE_NT_OPTIONAL_HDR64_MAGIC,"expected PE32+");
        const auto size=get<uint32_t>(optional+56);
        require(size==bytes_.size()&&size<=0x40000000,"expected complete module in RVA layout (maximum 1 GiB)");
        require(base&&base<=UINTPTR_MAX-size,"invalid module base");
        const auto count=get<uint16_t>(pe+6),optional_size=get<uint16_t>(pe+20);
        require(count&&count<=96&&optional_size>=144,"invalid section/directory header");
        const auto table=optional+optional_size;
        require(table<=bytes_.size()&&size_t(count)*40<=bytes_.size()-table,"truncated section table");
        exception_rva=get<uint32_t>(optional+112+3*8);
        exception_size=get<uint32_t>(optional+116+3*8);
        require(exception_size%12==0&&exception_rva<=size&&exception_size<=size-exception_rva,"invalid exception directory");
        for(unsigned i=0;i<count;++i) {
            const auto at=table+i*40;
            const auto va=get<uint32_t>(at+12);
            const auto length=std::max(get<uint32_t>(at+8),get<uint32_t>(at+16));
            require(va<=size&&length<=size-va,"section outside snapshot");
            put32(at+20,va);put32(at+16,length);
        }
        image_=std::make_unique<discovery::Image>(bytes_);
        uint32_t previous{};
        for(size_t offset=0;offset<exception_size;offset+=12) {
            const auto begin=image_->read<uint32_t>(exception_rva+offset);
            const auto end=image_->read<uint32_t>(exception_rva+offset+4);
            require((!offset||begin>previous)&&begin<end&&code(begin)&&code(end-1),"invalid or unordered runtime functions");
            previous=begin;
        }
    }
    const discovery::Image& image()const{return *image_;}
    uintptr_t rva(uint64_t pointer)const {
        require(pointer>=base&&pointer-base<image_->image_size,"pointer outside main module");
        return static_cast<uintptr_t>(pointer-base);
    }
    bool code(uintptr_t at,size_t length=1)const {
        const auto* s=image_->section(at,length);return s&&s->code();
    }
    bool data(uintptr_t at,size_t length=1,bool writable=false)const {
        const auto* s=image_->section(at,length);
        return s&&!s->code()&&(s->flags&IMAGE_SCN_MEM_READ)&&(!writable||s->data());
    }
    std::string ascii(uintptr_t at)const {
        const auto* s=image_->section(at,1);if(!s||s->code())return {};
        const auto n=std::min<size_t>(128,s->size-(at-s->rva));
        auto bytes=image_->view(at,n);std::string out;
        for(auto c:bytes) {
            if(!c)return out;
            if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'))return {};
            out.push_back(static_cast<char>(c));
        }
        return {};
    }
    bool wide_equals(uintptr_t at,std::wstring_view name)const {
        if(!data(at,(name.size()+1)*2))return false;
        auto text=image_->view(at,(name.size()+1)*2);
        return std::memcmp(text.data(),name.data(),name.size()*2)==0&&text[name.size()*2]==0&&text[name.size()*2+1]==0;
    }
    uintptr_t function_start(uintptr_t at)const {
        require(exception_size,"exception directory unavailable; wait for initialization");
        size_t low{},high=exception_size/12;
        while(low<high) {
            const auto mid=low+(high-low)/2;
            if(image_->read<uint32_t>(exception_rva+mid*12)<=at)low=mid+1;else high=mid;
        }
        require(low>0,"function has no unwind entry");
        auto entry=exception_rva+(low-1)*12;
        auto begin=image_->read<uint32_t>(entry),end=image_->read<uint32_t>(entry+4),unwind=image_->read<uint32_t>(entry+8);
        require(begin<=at&&at<end,"reference is outside its unwind entry");
        for(unsigned depth=0;depth<16;++depth) {
            require(code(begin)&&end>begin&&code(end-1),"invalid runtime function range");
            require(data(unwind,4),"unwind header outside data");
            const auto flags=image_->read<uint8_t>(unwind)>>3;
            if(!(flags&4))return begin;
            const auto count=image_->read<uint8_t>(unwind+2);
            entry=unwind+4+((count+1)&~1u)*2;
            require(data(entry,12),"chained unwind outside data");
            const auto next=image_->read<uint32_t>(entry);
            require(next<=begin,"invalid chained unwind parent");
            begin=next;end=image_->read<uint32_t>(entry+4);unwind=image_->read<uint32_t>(entry+8);
        }
        throw std::runtime_error("NTE: cyclic/deep unwind chain");
    }
};

inline std::unique_ptr<ModuleImage> capture_main_module() {
    const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    std::vector<uint8_t> header(4096);
    require(read_memory(base,header),"cannot read main module header");
    uint32_t pe{},size{};std::memcpy(&pe,header.data()+0x3c,4);
    require(pe<=header.size()-24-64,"main module header is not initialized");
    std::memcpy(&size,header.data()+pe+24+56,4);
    require(size>=4096&&size<=0x40000000,"invalid loaded image size");
    std::vector<uint8_t> bytes(size);
    require(read_memory(base,bytes),"main module is not fully readable");
    return std::make_unique<ModuleImage>(std::move(bytes),base);
}
}
