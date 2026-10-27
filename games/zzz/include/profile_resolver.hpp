#pragma once
#include <windows.h>
#include "discover_profile.hpp"

namespace discovery {
// A capture failure is fatal, unlike a rejected candidate. Keep it outside
// runtime_error so the matcher's candidate-rejection handlers cannot hide it.
class MemoryReadError final:public std::exception {
    std::string message_;
public:
    explicit MemoryReadError(std::string message):message_("Automatic profile: "+std::move(message)){}
    const char* what()const noexcept override{return message_.c_str();}
};
inline void require_memory(bool condition,const std::string& message) {
    if(!condition)throw MemoryReadError(message);
}
// No file/path/hash API: every byte comes from the current process.
struct ModuleReader {
    uintptr_t base;
    void operator()(uintptr_t rva,std::span<uint8_t> target)const {
        require_memory(base && rva<=UINTPTR_MAX-base && target.size()<=UINTPTR_MAX-(base+rva),"module address overflow");
        size_t done{};
        while(done<target.size()) {
            const auto address=base+rva+done;
            MEMORY_BASIC_INFORMATION region{};
            require_memory(VirtualQuery(reinterpret_cast<const void*>(address),&region,sizeof(region))==sizeof(region),"cannot query module memory at RVA "+std::to_string(rva+done));
            constexpr DWORD readable=PAGE_READONLY|PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY;
            // AllocationBase prevents a bad section from reading a neighbor.
            require_memory(reinterpret_cast<uintptr_t>(region.AllocationBase)==base && region.State==MEM_COMMIT &&
                !(region.Protect&(PAGE_GUARD|PAGE_NOACCESS)) && (region.Protect&readable),"unreadable or incomplete module range at RVA "+std::to_string(rva+done));
            const auto offset=address-reinterpret_cast<uintptr_t>(region.BaseAddress);
            require_memory(offset<region.RegionSize,"invalid module memory region");
            const auto length=std::min(target.size()-done,region.RegionSize-offset);
            SIZE_T count{};
            const auto success=ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),target.data()+done,length,&count);
            const auto error=success?ERROR_PARTIAL_COPY:GetLastError();
            require_memory(success && count==length,"module read failed at RVA "+std::to_string(rva+done)+" (Windows error "+std::to_string(error)+")");
            done+=length;
        }
    }
    template<class T> T read(uintptr_t rva)const {
        T value{};(*this)(rva,{reinterpret_cast<uint8_t*>(&value),sizeof(value)});return value;
    }
};
// Also used on owned, non-executable simulated mappings. Keep the allocation
// alive while matching: slot readability is checked against live memory.
inline Image capture_module(uintptr_t base) {
    const ModuleReader read{base};
    require(read.read<uint16_t>(0)==0x5a4d,"missing loaded DOS signature");
    const auto pe=read.read<uint32_t>(0x3c);
    require(pe>=64 && pe<=0x100000-24,"invalid loaded PE offset");
    require(read.read<uint32_t>(pe)==0x4550 && read.read<uint16_t>(pe+4)==0x8664,"expected loaded x64 PE");
    const auto optional_size=read.read<uint16_t>(pe+20);
    require(optional_size>=112,"truncated loaded optional header");
    const auto headers=read.read<uint32_t>(pe+24+60);
    require(headers>=pe+24+optional_size && headers<=0x100000,"invalid loaded header size");
    std::vector<uint8_t> bytes(headers);read(0,bytes);
    return Image(bytes,read);
}
using Progress=void(*)(const char*);
inline Result resolve_module(HMODULE module,Progress progress=nullptr) {
    require(module!=nullptr,"null GameAssembly module");
    if(progress)progress("retain module for process lifetime");
    HMODULE retained{};
    // Installed callbacks remain resident until process exit. Match that lifetime
    // even if worker initialization throws after installing one of the hooks.
    require(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(module),&retained) && retained==module,"cannot retain GameAssembly module");
    if(progress)progress("capture loaded PE headers and all readable code sections");
    const auto image=capture_module(reinterpret_cast<uintptr_t>(retained));
    if(progress)progress("match anchors, decode instructions and validate dataflow");
    auto result=discover(image);
    if(progress)progress("resolved all 15 fields from memory");
    return result;
}
}
