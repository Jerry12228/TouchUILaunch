// Read-only tests against supplied DLL bytes plus small, non-executable fixtures.
#include "profile_resolver.hpp"
#include <fstream>
#include <iostream>
#include <functional>

namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class T> void put(std::vector<uint8_t>& bytes,size_t at,T value) {
    check(at<=bytes.size() && sizeof(T)<=bytes.size()-at,"fixture write bounds");
    std::memcpy(bytes.data()+at,&value,sizeof(value));
}
void equal_profile(const profile::Build& actual,const profile::Build& expected) {
    for(const auto& field:profile::fields)if(actual.*(field.value)!=expected.*(field.value))throw std::runtime_error(std::string("Profile mismatch: ")+field.name);
}
struct Fixture {
    std::vector<uint8_t> bytes;
    size_t offset(uintptr_t rva)const {
        discovery::Image image(bytes);
        auto data=image.view(rva,1);return static_cast<size_t>(data.data()-bytes.data());
    }
    void rel(uintptr_t at,size_t opcode_size,uintptr_t target) {
        const auto delta=int64_t(target)-int64_t(at)-int64_t(opcode_size)-4;
        check(delta>=INT32_MIN && delta<=INT32_MAX,"fixture relative range");
        put<int32_t>(bytes,offset(at+opcode_size),static_cast<int32_t>(delta));
    }
};
Fixture compact(const discovery::Image& source,const discovery::Result& found) {
    std::map<uintptr_t,bool> pages;
    auto code=[&](uintptr_t address,size_t size) {
        for(uintptr_t page=address&~uintptr_t{4095};page<address+size;page+=4096)pages[page]=true;
    };
    code(found.input_region,scan_rules::input_region.bytes.size());
    code(found.screen_region,scan_rules::screen_region.bytes.size());
    code(found.frame_region,scan_rules::frame_region.bytes.size());
    code(found.touch_loop,0x400);
    code(found.device_selector,scan_rules::device_selector.bytes.size());
    code(found.build.get_effective_layout,scan_rules::effective_layout.bytes.size());
    code(found.build.get_layout_override,scan_rules::layout_getter.bytes.size());
    code(found.default_getter,scan_rules::layout_getter.bytes.size());
    for(auto setter:source.find(scan_rules::layout_setter))code(setter,scan_rules::layout_setter.bytes.size());
    for(const auto& field:profile::fields) {
        const std::string name=field.name;
        if(name.ends_with("_slot") || name=="static_reference_pool")pages[(found.build.*(field.value))&~uintptr_t{4095}]=false;
    }
    auto owner=discovery::property(source,found.build.get_layout_override,false);
    pages[owner.interface_slot&~uintptr_t{4095}]=false;
    const uintptr_t spare=0x100000;
    check(!pages.contains(spare),"spare page collision");pages[spare]=true;
    check(pages.size()<80,"fixture section count");
    Fixture f;f.bytes.resize(4096+pages.size()*4096);
    put<uint16_t>(f.bytes,0,0x5a4d);put<uint32_t>(f.bytes,0x3c,0x80);
    put<uint32_t>(f.bytes,0x80,0x4550);put<uint16_t>(f.bytes,0x84,0x8664);
    put<uint16_t>(f.bytes,0x86,static_cast<uint16_t>(pages.size()));put<uint16_t>(f.bytes,0x94,0xf0);
    put<uint16_t>(f.bytes,0x98,0x20b);put<uint32_t>(f.bytes,0x98+56,source.image_size);put<uint32_t>(f.bytes,0x98+60,4096);
    size_t index{};
    for(auto [page,executable]:pages) {
        const size_t header=0x188+index*40,raw=4096+index*4096;
        put<uint32_t>(f.bytes,header+8,4096);put<uint32_t>(f.bytes,header+12,static_cast<uint32_t>(page));
        put<uint32_t>(f.bytes,header+16,4096);put<uint32_t>(f.bytes,header+20,static_cast<uint32_t>(raw));
        put<uint32_t>(f.bytes,header+36,executable?0x60000020:0xc0000040);
        if(executable && page!=spare) {
            auto bytes=source.view(page,4096);std::copy(bytes.begin(),bytes.end(),f.bytes.begin()+raw);
        }
        ++index;
    }
    return f;
}
void negatives(const Fixture& original,const discovery::Result& found) {
    size_t count{};
    auto rejected=[&](const char* label,const std::function<void(Fixture&)>& mutate) {
        Fixture copy=original;mutate(copy);
        bool failed{};
        try {(void)discovery::discover(discovery::Image(copy.bytes));}catch(const std::runtime_error&){failed=true;}
        if(!failed)throw std::runtime_error(std::string("Invalid fixture accepted: ")+label);
        ++count;
    };
    rejected("missing Input anchor",[&](Fixture& f){f.bytes[f.offset(found.input_region)]^=1;});
    rejected("duplicate Input anchors",[&](Fixture& f){
        auto at=f.offset(found.input_region),to=f.offset(0x100000);
        std::copy_n(f.bytes.data()+at,scan_rules::input_region.bytes.size(),f.bytes.data()+to);
    });
    rejected("changed Touch ABI",[&](Fixture& f){
        // The first copy of the trailing four bytes uses [destination + 64].
        const auto start=f.offset(found.input_region);
        const std::array<uint8_t,3> copy{0x89,0x46,0x40};
        auto at=std::search(f.bytes.begin()+start,f.bytes.begin()+start+105,copy.begin(),copy.end());
        check(at!=f.bytes.begin()+start+105,"find Touch ABI fixture mutation");at[2]=0x44;
    });
    rejected("different default provider",[&](Fixture& f){put<uint32_t>(f.bytes,f.offset(found.default_getter+0x30),static_cast<uint32_t>(found.build.ui_state_offset+8));});
    rejected("wrong setter property",[&](Fixture& f){put<uint32_t>(f.bytes,f.offset(found.build.set_layout_override+0x50),static_cast<uint32_t>(found.build.default_property_offset));});
    rejected("ambiguous setter",[&](Fixture& f){
        auto from=f.offset(found.build.set_layout_override),to=f.offset(0x100000);
        std::copy_n(f.bytes.data()+from,scan_rules::layout_setter.bytes.size(),f.bytes.data()+to);
        auto prop=discovery::property(discovery::Image(original.bytes),found.build.get_layout_override,false);
        f.rel(0x100000+0x22,3,prop.klass);f.rel(0x100000+0x36,3,prop.pool);f.rel(0x100000+0x5d,3,prop.interface_slot);
    });
    rejected("wrong touch-loop count consumer",[&](Fixture& f){f.rel(found.touch_loop+0x2bf,2,found.build.screen_width_slot);});
    rejected("changed Mobile enum",[&](Fixture& f){f.bytes[f.offset(found.device_selector+0xd4)]=5;});
    rejected("aliased intrinsic slots",[&](Fixture& f){f.rel(found.screen_region+0x30,3,found.build.screen_width_slot);});
    rejected("nonzero icall storage",[&](Fixture& f){f.bytes[f.offset(found.build.get_touch_slot)]=1;});
    rejected("invalid class layout",[&](Fixture& f){put<uint32_t>(f.bytes,f.offset(found.build.get_layout_override+0x1b),0xfffffff0);});
    rejected("target in unmapped image gap",[&](Fixture& f){f.rel(found.frame_region,3,0x200000);});
    rejected("truncated raw section",[&](Fixture& f){f.bytes.resize(f.bytes.size()-1);});
    rejected("overlapping virtual sections",[&](Fixture& f){put<uint32_t>(f.bytes,0x188+40+12,*reinterpret_cast<const uint32_t*>(f.bytes.data()+0x188+12));});
    rejected("malformed PE section count",[&](Fixture& f){put<uint16_t>(f.bytes,0x86,0xffff);});
    rejected("no PE header",[&](Fixture& f){f.bytes.resize(20);});
    std::cout<<"Rejected "<<count<<" malformed, ambiguous, ABI-changing or inconsistent fixtures.\n";
}
void save(const std::filesystem::path& path,const Fixture& f) {
    std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(f.bytes.data()),static_cast<std::streamsize>(f.bytes.size()));
    check(bool(out),"write owned fixture");
}
void live_code(const discovery::Result& result,size_t image_size) {
    auto base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,image_size,MEM_RESERVE,PAGE_NOACCESS));
    check(base!=0,"reserve owned live-code fixture");
    try {
        for(const auto& entry:result.code_checks) {
            for(uintptr_t page=entry.rva&~uintptr_t{4095};page<entry.rva+entry.bytes.size();page+=4096)
                check(VirtualAlloc(reinterpret_cast<void*>(base+page),4096,MEM_COMMIT,PAGE_READWRITE)!=nullptr,"commit owned live-code page");
            std::memcpy(reinterpret_cast<void*>(base+entry.rva),entry.bytes.data(),entry.bytes.size());
        }
        // These pages are never executable or called; only ReadProcessMemory runs.
        discovery::validate_loaded_code(base,result);
        *reinterpret_cast<uint8_t*>(base+result.build.get_layout_override+0x40)^=8;
        bool rejected{};try{discovery::validate_loaded_code(base,result);}catch(const std::runtime_error&){rejected=true;}
        check(rejected,"live-code field changes must be rejected before hooks");
    } catch(...) {VirtualFree(reinterpret_cast<void*>(base),0,MEM_RELEASE);throw;}
    VirtualFree(reinterpret_cast<void*>(base),0,MEM_RELEASE);
}
void test(const std::filesystem::path& path) {
    const auto fixed=discovery::resolve(path);
    check(!fixed.automatic,"fixture baseline must be a known client");
    const auto automatic=discovery::resolve(path,true);
    check(automatic.automatic,"forced discovery must bypass the hash table");
    equal_profile(automatic.build,fixed.build);
    Fixture fixture;
    {discovery::MappedFile file(path);fixture=compact(discovery::Image(file.bytes()),automatic);}
    const auto reduced=discovery::discover(discovery::Image(fixture.bytes));
    equal_profile(reduced.build,fixed.build);
    negatives(fixture,reduced);
    live_code(reduced,discovery::Image(fixture.bytes).image_size);
    const auto owned=std::filesystem::current_path()/("discovery-owned-"+std::to_string(GetCurrentProcessId())+".dll");
    check(!std::filesystem::exists(owned),"owned fixture path already exists");
    try {
        save(owned,fixture);
        const auto unknown=discovery::resolve(owned);
        check(unknown.automatic && !profile::find(unknown.build.sha256),"unknown file hash must use discovery");
        equal_profile(unknown.build,fixed.build);
        fixture.bytes[fixture.offset(reduced.input_region)]^=1;save(owned,fixture);
        bool failed{};try{(void)discovery::resolve(owned);}catch(const std::runtime_error&){failed=true;}
        check(failed,"content changes must invalidate cached addresses");
    } catch(...) {std::filesystem::remove(owned);throw;}
    std::filesystem::remove(owned);
    discovery::print_json(std::cout,automatic);
    std::cout<<"PASS: known profile preserved; all 15 fields independently discovered; unknown hash, cache invalidation and loaded-code checks passed.\n";
}
}
int wmain(int argc,wchar_t** argv) {
    try {check(argc==2,"pass GameAssembly.dll path");test(argv[1]);return 0;}
    catch(const std::exception& ex){std::cerr<<"FAIL: "<<ex.what()<<"\n";return 1;}
}
