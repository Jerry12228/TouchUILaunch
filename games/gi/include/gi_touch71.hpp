#pragma once
#include "gi_touch71_plan.hpp"
#include "game.hpp"
#include "pe_image.hpp"
#include <bcrypt.h>
#include <winternl.h>
#include <memory>
#include <iostream>

namespace touchui::gi71 {
inline constexpr std::array<uint8_t,32> sample_sha256{
    0x08,0xa3,0x08,0x6d,0x5f,0x3f,0xe6,0x95,0xf0,0x1d,0xab,0x61,0xef,0xa4,0x2e,0x44,
    0x20,0x06,0xb1,0x8e,0x5e,0x47,0x5b,0x25,0x20,0xdf,0x35,0x6f,0x6a,0x07,0x3b,0x7d};
inline void check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}

inline std::array<uint8_t,32> sha256(std::span<const uint8_t> bytes) {
    struct Algorithm {BCRYPT_ALG_HANDLE value{};~Algorithm(){if(value)BCryptCloseAlgorithmProvider(value,0);}} algorithm;
    struct Hash {BCRYPT_HASH_HANDLE value{};~Hash(){if(value)BCryptDestroyHash(value);}} hash;
    check(BCryptOpenAlgorithmProvider(&algorithm.value,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0,"GI: SHA256 provider failed");
    check(BCryptCreateHash(algorithm.value,&hash.value,nullptr,0,nullptr,0,0)>=0,"GI: SHA256 initialization failed");
    while(!bytes.empty()) {
        const auto count=static_cast<ULONG>(std::min<size_t>(bytes.size(),1<<20));
        check(BCryptHashData(hash.value,const_cast<PUCHAR>(bytes.data()),count,0)>=0,"GI: SHA256 read failed");
        bytes=bytes.subspan(count);
    }
    std::array<uint8_t,32> result{};
    check(BCryptFinishHash(hash.value,result.data(),static_cast<ULONG>(result.size()),0)>=0,"GI: SHA256 finish failed");
    return result;
}

// Keep the verified EXE open without write/delete sharing across CreateProcess.
// Raw mapping is read-only and never passed to the executable loader.
class VerifiedImage {
    Handle file_,mapping_;
    struct View {const uint8_t* bytes{};~View(){if(bytes)UnmapViewOfFile(bytes);}} view_;
    std::unique_ptr<discovery::Image> image_;
public:
    explicit VerifiedImage(const std::filesystem::path& path)
        :file_(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr)) {
        check(file_.value!=INVALID_HANDLE_VALUE,"GI: cannot open executable for verification");
        LARGE_INTEGER size{};
        check(GetFileSizeEx(file_,&size)&&size.QuadPart==444260776,"GI: unsupported executable; this build requires the analyzed GI 7.1 sample");
        mapping_.value=CreateFileMappingW(file_,nullptr,PAGE_READONLY,0,0,nullptr);
        check(mapping_.value!=nullptr,"GI: cannot map executable");
        view_.bytes=static_cast<const uint8_t*>(MapViewOfFile(mapping_,FILE_MAP_READ,0,0,0));
        check(view_.bytes!=nullptr,"GI: cannot read executable mapping");
        const std::span bytes(view_.bytes,static_cast<size_t>(size.QuadPart));
        check(sha256(bytes)==sample_sha256,"GI: unsupported executable SHA256; no legacy fallback is used");
        image_=std::make_unique<discovery::Image>(bytes);
        validate_sites(sites);
        for(const auto& site:sites) {
            image_->code(site.rva,site.expected.size());
            const auto actual=image_->view(site.rva,site.expected.size());
            check(std::equal(actual.begin(),actual.end(),site.expected.begin(),site.expected.end()),"GI: analyzed code does not match the executable");
        }
    }
    uint32_t image_size()const{return image_->image_size;}
};

inline void read_process(HANDLE process,uintptr_t address,void* data,size_t size) {
    SIZE_T done{};
    check(ReadProcessMemory(process,reinterpret_cast<const void*>(address),data,size,&done)&&done==size,"GI: remote read failed");
}
inline uintptr_t child_image_base(const game::Child& child) {
    using Query=NTSTATUS(NTAPI*)(HANDLE,PROCESSINFOCLASS,PVOID,ULONG,PULONG);
    const auto query=reinterpret_cast<Query>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"NtQueryInformationProcess"));
    check(query!=nullptr,"GI: process information API unavailable");
    PROCESS_BASIC_INFORMATION basic{};
    check(query(child.info.hProcess,ProcessBasicInformation,&basic,sizeof(basic),nullptr)>=0&&basic.PebBaseAddress,
          "GI: cannot locate owned child PEB");
    // Windows x64 PEB.ImageBaseAddress. Toolhelp module enumeration is not ready
    // before the suspended primary thread has run the loader.
    uintptr_t base{};
    read_process(child.info.hProcess,reinterpret_cast<uintptr_t>(basic.PebBaseAddress)+0x10,&base,sizeof(base));
    check(base!=0,"GI: owned child image base unavailable");
    return base;
}
class RemoteCode {
    HANDLE process_;
    uintptr_t base_;
    uint32_t size_;
    uintptr_t address(uint32_t rva,size_t size)const {
        check(rva<=size_&&size<=size_-rva&&base_<=UINTPTR_MAX-size_,"GI: remote range outside image");
        return base_+rva;
    }
public:
    RemoteCode(HANDLE process,uintptr_t base,uint32_t size):process_(process),base_(base),size_(size){}
    std::vector<uint8_t> read(uint32_t rva,size_t size)const {
        std::vector<uint8_t> result(size);read_process(process_,address(rva,size),result.data(),size);return result;
    }
    void write_code(uint32_t rva,std::span<const uint8_t> bytes) {
        auto* target=reinterpret_cast<void*>(address(rva,bytes.size()));
        MEMORY_BASIC_INFORMATION info{};
        check(VirtualQueryEx(process_,target,&info,sizeof(info))==sizeof(info)&&info.State==MEM_COMMIT&&
              !(info.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&
              (info.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_WRITECOPY))&&
              reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(info.BaseAddress)+bytes.size()<=info.RegionSize,
              "GI: patch target is not a single executable region");
        DWORD previous{};
        check(VirtualProtectEx(process_,target,bytes.size(),PAGE_EXECUTE_READWRITE,&previous)!=0,"GI: cannot make UI code writable");
        SIZE_T done{};
        const bool written=WriteProcessMemory(process_,target,bytes.data(),bytes.size(),&done)&&done==bytes.size();
        const bool flushed=written&&FlushInstructionCache(process_,target,bytes.size());
        DWORD ignored{};
        const bool restored=VirtualProtectEx(process_,target,bytes.size(),previous,&ignored)!=0;
        check(written&&flushed&&restored,"GI: UI write/cache/protection failure; owned child will be terminated");
    }
};

template<class Progress>
void initialize(game::Child& child,const VerifiedImage& image,bool logging,Progress&& progress) {
    check(child.info.hProcess&&child.info.hThread,"GI: owned suspended child required");
    progress("GI 7.1: validate loaded UI and input instructions");
    const auto base=child_image_base(child);
    RemoteCode memory(child.info.hProcess,base,image.image_size());
    apply_plan(memory);
    progress("GI 7.1: Mobile layout and TouchScreen input selected");
    if(logging)std::cout<<"GI: "<<sites.size()<<" UI/input sites verified and patched before startup. "
        "TouchScreen input, 360 DPI touch-scale fallback and touchscreen settings caption selected; "
        "physical/streamed touch delivery still requires runtime verification.\n";
}
}
