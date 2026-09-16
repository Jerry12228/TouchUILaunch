#include "test_module.hpp"
#include <iostream>

namespace {
void check(bool ok,const char* message){discovery::require(ok,message);}
template<class T> void put(std::vector<uint8_t>& bytes,size_t at,T value){std::memcpy(bytes.data()+at,&value,sizeof(value));}
std::vector<uint8_t> fixture() {
    std::vector<uint8_t> bytes(0x2200);
    put<uint16_t>(bytes,0,0x5a4d);put<uint32_t>(bytes,0x3c,0x80);
    put<uint32_t>(bytes,0x80,0x4550);put<uint16_t>(bytes,0x84,0x8664);put<uint16_t>(bytes,0x86,2);
    put<uint16_t>(bytes,0x94,0xf0);put<uint16_t>(bytes,0x98,0x20b);
    put<uint32_t>(bytes,0x98+56,0x5000);put<uint32_t>(bytes,0x98+60,0x200);
    // Raw .text starts at 0x200; its memory RVA is 0x1000 (two pages).
    put<uint32_t>(bytes,0x188+8,0x2000);put<uint32_t>(bytes,0x188+12,0x1000);
    put<uint32_t>(bytes,0x188+16,0x2000);put<uint32_t>(bytes,0x188+20,0x200);
    put<uint32_t>(bytes,0x188+36,0x60000020);
    // BSS slot storage has no file bytes.
    put<uint32_t>(bytes,0x1b0+8,0x1000);put<uint32_t>(bytes,0x1b0+12,0x4000);put<uint32_t>(bytes,0x1b0+36,0xc0000080);
    const uint8_t instruction[]{0x48,0x8b,0x05,0xfd,0x1f,0,0}; // [RIP + 0x1ffd] -> 0x4000
    std::copy(std::begin(instruction),std::end(instruction),bytes.begin()+0x11fc);
    return bytes;
}
void test() {
    const auto bytes=fixture();const discovery::Image file(bytes);
    test_module::Mapping loaded(bytes);
    loaded.put<uintptr_t>(0x4000,loaded.base+0x1000);
    loaded.protect(0x2000,4096,PAGE_READONLY); // Different VirtualQuery region across instruction.
    auto image=discovery::capture_module(loaded.base);
    const auto a=discovery::x64::decode(file,0x1ffc),b=discovery::x64::decode(image,0x1ffc);
    check(a.h.len==7 && b.h.len==7 && a.storage(file)==0x4000 && b.storage(image)==0x4000,"cross-page decoding in file and memory views");
    file.slot(0x4000);image.slot(0x4000);
    // Copies own the code bytes; a live write cannot silently alter the snapshot.
    loaded.put<uint8_t>(0x1ffc,0xcc);
    check(image.view(0x1ffc,1)[0]==0x48,"snapshot owns code");
    size_t rejected{};
    auto reject=[&](const char* label,auto mutate) {
        test_module::Mapping bad(bytes);mutate(bad);
        bool failed{};
        try {(void)discovery::capture_module(bad.base);}catch(const std::exception&){failed=true;}
        check(failed,label);++rejected;
    };
    reject("unreadable code page",[](auto& m){m.protect(0x2000,4096,PAGE_NOACCESS);});
    reject("guard code page",[](auto& m){m.protect(0x2000,4096,PAGE_READWRITE|PAGE_GUARD);});
    reject("execute-only page",[](auto& m){m.protect(0x2000,4096,PAGE_EXECUTE);});
    reject("truncated memory",[](auto& m){check(VirtualFree(reinterpret_cast<void*>(m.base+0x2000),4096,MEM_DECOMMIT)!=FALSE,"decommit fixture page");});
    reject("unreadable header",[](auto& m){m.protect(0,4096,PAGE_NOACCESS);});
    reject("PE offset overflow",[](auto& m){m.template put<uint32_t>(0x3c,0xfffffff0);});
    reject("wrong machine",[](auto& m){m.template put<uint16_t>(0x84,0x14c);});
    reject("wrong optional header",[](auto& m){m.template put<uint16_t>(0x98,0x10b);});
    reject("bad section count",[](auto& m){m.template put<uint16_t>(0x86,97);});
    reject("section table outside headers",[](auto& m){m.template put<uint32_t>(0x98+60,0x188);});
    reject("section outside image",[](auto& m){m.template put<uint32_t>(0x188+12,0x5000);});
    reject("section overlaps headers",[](auto& m){m.template put<uint32_t>(0x188+12,0x100);});
    reject("section overlaps section",[](auto& m){m.template put<uint32_t>(0x1b0+12,0x2000);});
    reject("image size overflow",[](auto& m){m.template put<uint32_t>(0x98+56,0xffffffff);});
    reject("uncommitted section tail",[](auto& m){m.template put<uint32_t>(0x188+8,0x3000);});
    for(auto address: {uintptr_t{0},~uintptr_t{}-1}) {
        bool failed{};try{(void)discovery::capture_module(address);}catch(const std::exception&){failed=true;}
        check(failed,"invalid module base");++rejected;
    }
    // Exercise the real retention/capture entry point using this test EXE only.
    // It contains no Unity input wrapper; rejection must occur at matching.
    static bool matched{};
    bool failed{};
    try {(void)discovery::resolve_module(GetModuleHandleW(nullptr),[](const char* stage){if(std::string_view(stage).starts_with("match anchors"))matched=true;});}
    catch(const std::exception&){failed=true;}
    check(matched && failed,"real module API reaches matcher without a client file");
    std::cout<<"PASS: cross-page instruction, raw/RVA separation, BSS slots, owned snapshot, real module retention; rejected "<<rejected<<" invalid memory/header cases.\n";
}
}
int main(){try{test();return 0;}catch(const std::exception& ex){std::cerr<<"FAIL: "<<ex.what()<<"\n";return 1;}}
