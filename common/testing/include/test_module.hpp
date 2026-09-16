#pragma once
// Test-only manual PE mapping: never LoadLibrary, never executable pages.
#include "profile_resolver.hpp"
namespace test_module {
struct Mapping {
    uintptr_t base{};
    explicit Mapping(std::span<const uint8_t> bytes) {
        const discovery::Image file(bytes);
        base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,file.image_size,MEM_RESERVE,PAGE_NOACCESS));
        discovery::require(base!=0,"reserve simulated module");
        try {
            const auto pe=*reinterpret_cast<const uint32_t*>(bytes.data()+0x3c);
            const auto headers=*reinterpret_cast<const uint32_t*>(bytes.data()+pe+24+60);
            commit(0,headers);std::memcpy(reinterpret_cast<void*>(base),bytes.data(),headers);
            for(const auto& s:file.sections)if(s.size) {
                commit(s.rva,s.size);
                if(s.raw_size)std::memcpy(reinterpret_cast<void*>(base+s.rva),bytes.data()+s.raw,s.raw_size);
            }
        } catch(...) {VirtualFree(reinterpret_cast<void*>(base),0,MEM_RELEASE);throw;}
    }
    ~Mapping(){if(base)VirtualFree(reinterpret_cast<void*>(base),0,MEM_RELEASE);}
    Mapping(const Mapping&)=delete;Mapping& operator=(const Mapping&)=delete;
    void commit(uintptr_t rva,size_t size) {
        discovery::require(VirtualAlloc(reinterpret_cast<void*>(base+rva),size,MEM_COMMIT,PAGE_READWRITE)!=nullptr,"commit simulated module");
    }
    void protect(uintptr_t rva,size_t size,DWORD flags) {
        DWORD old{};discovery::require(VirtualProtect(reinterpret_cast<void*>(base+rva),size,flags,&old)!=FALSE,"protect simulated module");
    }
    template<class T> void put(uintptr_t rva,T value) {std::memcpy(reinterpret_cast<void*>(base+rva),&value,sizeof(value));}
};
}
