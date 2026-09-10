// Read-only tests against supplied DLL bytes plus small, non-executable fixtures.
#include "profile_resolver.hpp"
#include <fstream>
#include <iostream>
#include <functional>

namespace {
constexpr profile::Build golden25{
    "2.5", "69142459d5559677ac7f4c38ae88f568dc62ea41e2017e266f23889f33074ceb",
    0x4f43f80,0x4f43fa0,0x4f43f88,0x4f41b40,0x4f3d8b0,0x4f3d8b8,
    0x4f87fa0,0x4e93b48,0x3d9a0,0x7862010,0x7862130,0x786fc80,0xcb,0x68,0x70
};
// Independent 2.6 IDA evidence: analysis/versions/2.6. Test-only golden data;
// this hash is deliberately absent from the production fixed profile table.
constexpr profile::Build golden26{
    "2.6", "8547fc8a2aaa6b509a4ac4e3b8ddf65997afd52f3f7c3652917d0effe2625e1a",
    0x4916a68,0x4916a88,0x4916a70,0x4913c38,0x4910500,0x4910508,
    0x4950e90,0x4885c60,0x295a8,0xb10d400,0xb109140,0xb103be0,0xcb,0x70,0x60
};
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
void clone_function(Fixture& f,const discovery::Image& source,uintptr_t from,size_t size,uintptr_t to) {
    auto data=source.view(from,size);std::copy(data.begin(),data.end(),f.bytes.begin()+f.offset(to));
    discovery::x64::Cursor cursor(source,from,size);
    while(cursor.at<cursor.end) {
        auto ins=cursor.take();const auto clone=to+(ins.at-from);
        if(ins.base()==discovery::x64::rip) {
            const auto delta=int64_t(ins.storage(source))-int64_t(clone+ins.h.len);
            check(delta>=INT32_MIN && delta<=INT32_MAX,"clone RIP displacement range");
            put<int32_t>(f.bytes,f.offset(clone+(ins.displacement_at()-ins.at)),static_cast<int32_t>(delta));
        }
        if(ins.call() || ins.jump()) {
            const auto target=ins.relative(source);
            if(target<from || target>=from+size) {
                check(ins.immediate_size()==4,"clone external branch displacement width");
                f.rel(clone,ins.h.len-4,target);
            }
        }
    }
}
Fixture compact(const discovery::Image& source,const discovery::Result& found) {
    std::map<uintptr_t,bool> pages;
    auto code=[&](uintptr_t address,size_t size) {
        for(uintptr_t page=address&~uintptr_t{4095};page<address+size;page+=4096)pages[page]=true;
    };
    for(const auto& check:found.code_checks)code(check.rva,check.bytes.size());
    for(const auto& field:profile::fields) {
        const std::string name=field.name;
        if(name.ends_with("_slot") || name=="static_reference_pool")pages[(found.build.*(field.value))&~uintptr_t{4095}]=false;
    }
    auto owner=discovery::property(source,found.build.get_layout_override,false);
    pages[owner.interface_slot&~uintptr_t{4095}]=false;
    const uintptr_t spare=0x100000;
    check(!pages.contains(spare),"spare page collision");pages[spare]=true;
    check(pages.size()<80,"fixture section count");
    struct Run {uintptr_t page;size_t count;bool executable;};std::vector<Run> runs;
    for(auto [page,executable]:pages) {
        if(!runs.empty() && runs.back().executable==executable && runs.back().page+runs.back().count*4096==page)++runs.back().count;
        else runs.push_back({page,1,executable});
    }
    Fixture f;f.bytes.resize(4096+pages.size()*4096);
    put<uint16_t>(f.bytes,0,0x5a4d);put<uint32_t>(f.bytes,0x3c,0x80);
    put<uint32_t>(f.bytes,0x80,0x4550);put<uint16_t>(f.bytes,0x84,0x8664);
    put<uint16_t>(f.bytes,0x86,static_cast<uint16_t>(runs.size()));put<uint16_t>(f.bytes,0x94,0xf0);
    put<uint16_t>(f.bytes,0x98,0x20b);put<uint32_t>(f.bytes,0x98+56,source.image_size);put<uint32_t>(f.bytes,0x98+60,4096);
    size_t index{},raw=4096;
    for(auto [page,count,executable]:runs) {
        const size_t header=0x188+index*40,length=count*4096;
        put<uint32_t>(f.bytes,header+8,static_cast<uint32_t>(length));put<uint32_t>(f.bytes,header+12,static_cast<uint32_t>(page));
        put<uint32_t>(f.bytes,header+16,static_cast<uint32_t>(length));put<uint32_t>(f.bytes,header+20,static_cast<uint32_t>(raw));
        put<uint32_t>(f.bytes,header+36,executable?0x60000020:0xc0000040);
        for(size_t offset=0;offset<length;offset+=4096)if(executable && page+offset!=spare) {
            auto bytes=source.view(page+offset,4096);std::copy(bytes.begin(),bytes.end(),f.bytes.begin()+raw+offset);
        }
        ++index;raw+=length;
    }
    return f;
}
std::vector<discovery::x64::Ins> dispatch_instructions(const discovery::Image& image,const discovery::Property& p) {
    discovery::x64::Cursor c(image,p.invocation.slow_start,p.invocation.invoke-p.invocation.slow_start);
    std::vector<discovery::x64::Ins> result;while(c.at<c.end)result.push_back(c.take());return result;
}
void swap_result_words(Fixture& f,const discovery::Property& p,bool reads=true,bool writes=true) {
    const discovery::Image image(f.bytes);const auto instructions=dispatch_instructions(image,p);
    int32_t temporary=INT32_MAX;size_t changed{};
    for(const auto& i:instructions)if(i.base()==4 && (i.mov_load() || i.mov_store()))temporary=std::min(temporary,i.disp());
    for(const auto& i:instructions)if(i.base()==4 && ((reads && i.mov_load())||(writes && i.mov_store()))) {
        check(i.displacement_size()==1 && (i.disp()==temporary || i.disp()==temporary+8),"fixture result pair");
        put<uint8_t>(f.bytes,f.offset(i.displacement_at()),static_cast<uint8_t>(temporary+8-(i.disp()-temporary)));++changed;
    }
    check(changed==size_t(reads?2:0)+size_t(writes?2:0),"result-word mutation covers each path");
}
void reorder_result_loads(Fixture& f,const discovery::Property& p) {
    const discovery::Image image(f.bytes);const auto a=discovery::x64::decode(image,p.invocation.slow_start);
    const auto b=discovery::x64::decode(image,a.next());
    check(a.mov_load() && b.mov_load() && a.base()==4 && b.base()==4 && a.reg()!=b.reg(),"independent result loads");
    const auto old=image.view(a.at,a.h.len+b.h.len);std::vector<uint8_t> replacement(old.begin()+a.h.len,old.end());
    replacement.insert(replacement.end(),old.begin(),old.begin()+a.h.len);
    std::copy(replacement.begin(),replacement.end(),f.bytes.begin()+f.offset(a.at));
}
void reorder_fast_calculation(Fixture& f,const discovery::Property& p) {
    const discovery::Image image(f.bytes);auto instructions=dispatch_instructions(image,p);
    auto at=std::find_if(instructions.begin(),instructions.end(),[&](auto& i){return i.at>=p.invocation.fast_start && i.mov_load() && i.index()>=0;});
    check(at!=instructions.end() && instructions.end()-at>=4,"fast calculation fixture");
    const auto a=at[0],b=at[1],c=at[2],d=at[3];
    check(b.mov_store() && c.h.opcode==0x0f && c.h.opcode2==0xb7 && d.h.opcode==0x01 && c.reg()!=a.reg() && c.reg()!=a.base() && c.reg()!=a.index(),"independent fast calculation");
    const auto first=image.view(a.at,c.at-a.at),second=image.view(c.at,d.next()-c.at);
    std::vector<uint8_t> replacement(second.begin(),second.end());replacement.insert(replacement.end(),first.begin(),first.end());
    std::copy(replacement.begin(),replacement.end(),f.bytes.begin()+f.offset(a.at));
}
void rename_call_register(Fixture& f,const discovery::Property& p) {
    const discovery::Image image(f.bytes);const auto call=discovery::x64::decode(image,p.invocation.invoke);
    const int previous=call.rm();constexpr int replacement=11;size_t changed{};
    for(const auto& i:dispatch_instructions(image,p))if((i.mov_load() || i.mov_store()) && i.reg()==previous) {
        check((f.bytes[f.offset(i.at)]&0xf8)==0x48,"64-bit move REX fixture");
        f.bytes[f.offset(i.at)]|=4;
        f.bytes[f.offset(i.at+2)]=static_cast<uint8_t>((i.h.modrm&~0x38)|((replacement&7)<<3));++changed;
    }
    check(changed==3,"rename function value's load/load/store uses only");
    check(call.h.len==2 || call.h.len==3,"register-call fixture encoding");
    if(call.h.len==2)check(image.view(call.next(),1)[0]==0x90,"consume post-call NOP for REX prefix");
    const std::array<uint8_t,3> bytes{0x41,0xff,0xd3};
    std::copy(bytes.begin(),bytes.end(),f.bytes.begin()+f.offset(call.at));
}
void dispatch_variations(const Fixture& original,const discovery::Result& found) {
    const discovery::Image base(original.bytes);
    const std::array properties{discovery::property(base,found.build.get_layout_override,false),
        discovery::property(base,found.default_getter,false),discovery::property(base,found.build.set_layout_override,true)};
    for(int variation=0;variation<4;++variation) {
        Fixture changed=original;
        for(const auto& p:properties) {
            if(variation==0)swap_result_words(changed,p);
            if(variation==1)reorder_result_loads(changed,p);
            if(variation==2)reorder_fast_calculation(changed,p);
            if(variation==3)rename_call_register(changed,p);
        }
        equal_profile(discovery::discover(discovery::Image(changed.bytes)).build,found.build);
    }
    std::cout<<"PASS: result member permutation, independent load order, independent calculation order and call-register renaming.\n";
}
void negatives(const Fixture& original,const discovery::Result& found) {
    const discovery::Image base(original.bytes);
    const auto fallback=discovery::property(base,found.default_getter,false);
    const auto override=discovery::property(base,found.build.get_layout_override,false);
    const auto setter=discovery::property(base,found.build.set_layout_override,true);
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
    rejected("different default provider",[&](Fixture& f){put<uint32_t>(f.bytes,f.offset(fallback.pool_offset_at),static_cast<uint32_t>(found.build.ui_state_offset+8));});
    rejected("wrong setter property",[&](Fixture& f){
        if(setter.field_size==1)put<uint8_t>(f.bytes,f.offset(setter.field_at),static_cast<uint8_t>(found.build.default_property_offset));
        else put<uint32_t>(f.bytes,f.offset(setter.field_at),static_cast<uint32_t>(found.build.default_property_offset));
    });
    rejected("ambiguous setter",[&](Fixture& f){
        clone_function(f,base,found.build.set_layout_override,setter.main_size,0x100000);
        // The clone must really validate as the same setter before exercising
        // ambiguity rejection; a broken clone would not test uniqueness.
        check(discovery::same_owner(setter,discovery::property(discovery::Image(f.bytes),0x100000,true)),"cloned setter owner");
    });
    rejected("wrong touch-loop count consumer",[&](Fixture& f){f.rel(found.loop_count_call,2,found.build.screen_width_slot);});
    rejected("changed Mobile enum",[&](Fixture& f){f.bytes[f.offset(found.mobile_value_at)]=5;});
    rejected("changed PC enum",[&](Fixture& f){f.bytes[f.offset(found.pc_value_at)]=5;});
    rejected("wrong setter interface slot",[&](Fixture& f){
        discovery::x64::Cursor cursor(base,found.build.set_layout_override,setter.main_size);bool changed{};
        while(cursor.at<cursor.end){const auto ins=cursor.take();if(ins.immediate(9,1)){put<uint32_t>(f.bytes,f.offset(ins.next()-4),0);changed=true;}}
        check(changed,"setter interface slot mutation found");
    });
    rejected("wrong property interface identity",[&](Fixture& f){f.rel(override.interface_load,3,override.pool);});
    rejected("GetTouch receives the wrong index",[&](Fixture& f){
        const auto argument=discovery::x64::decode(base,found.loop_get_touch_call-5);
        check(argument.h.opcode==0x89 && argument.h.len==2,"GetTouch argument fixture instruction");
        f.bytes[f.offset(argument.at+1)]^=8;
    });
    rejected("ambiguous validated choice",[&](Fixture& f){
        const auto original_choice=discovery::choice(base,found.build.get_effective_layout,found.build.get_effective_layout+0x26);
        clone_function(f,base,original_choice.fn,original_choice.size,0x100000);
        check(discovery::choice(discovery::Image(f.bytes),0x100000,0x100026).override_getter==original_choice.override_getter,"cloned layout choice");
    });
    rejected("aliased intrinsic slots",[&](Fixture& f){f.rel(found.screen_region+0x30,3,found.build.screen_width_slot);});
    rejected("nonzero icall storage",[&](Fixture& f){f.bytes[f.offset(found.build.get_touch_slot)]=1;});
    rejected("invalid class layout",[&](Fixture& f){put<uint32_t>(f.bytes,f.offset(override.initialized_at),0xfffffff0);});
    rejected("target in unmapped image gap",[&](Fixture& f){f.rel(found.frame_region,3,0x200000);});
    rejected("truncated raw section",[&](Fixture& f){f.bytes.resize(f.bytes.size()-1);});
    rejected("overlapping virtual sections",[&](Fixture& f){put<uint32_t>(f.bytes,0x188+40+12,*reinterpret_cast<const uint32_t*>(f.bytes.data()+0x188+12));});
    rejected("malformed PE section count",[&](Fixture& f){put<uint16_t>(f.bytes,0x86,0xffff);});
    rejected("no PE header",[&](Fixture& f){f.bytes.resize(20);});
    rejected("only slow result roles swapped",[&](Fixture& f){swap_result_words(f,override,true,false);});
    rejected("different default result layout",[&](Fixture& f){swap_result_words(f,fallback);});
    rejected("aliased direct result words",[&](Fixture& f){
        auto ins=dispatch_instructions(base,override);std::vector<discovery::x64::Ins> stores;
        for(auto& i:ins)if(i.mov_store() && i.base()==4)stores.push_back(i);
        check(stores.size()==2,"two result stores");put<uint8_t>(f.bytes,f.offset(stores[1].displacement_at()),static_cast<uint8_t>(stores[0].disp()));
    });
    rejected("call context as function",[&](Fixture& f){
        auto call=discovery::x64::decode(base,override.invocation.invoke);
        if(call.h.len==3)f.bytes[f.offset(call.at)]&=0xfe;
        f.bytes[f.offset(call.next()-1)]=0xd2;
    });
    rejected("read beyond lookup result",[&](Fixture& f){
        auto load=discovery::x64::decode(base,override.invocation.slow_start);
        put<uint8_t>(f.bytes,f.offset(load.displacement_at()),static_cast<uint8_t>(load.disp()+16));
    });
    rejected("lost setter input value",[&](Fixture& f){
        auto ins=dispatch_instructions(base,setter);bool changed{};
        for(auto& i:ins)if(i.at>=setter.invocation.join && i.h.opcode==0x89 && i.rm()==2) {
            f.bytes[f.offset(i.next()-1)]=0xca;changed=true;
        }
        check(changed,"setter argument mutation");
    });
    std::cout<<"Rejected "<<count<<" malformed, ambiguous, ABI-changing or inconsistent fixtures.\n";
}
void variations(const Fixture& original,const discovery::Result& found) {
    const discovery::Image base(original.bytes);
    Fixture unrelated=original;
    auto core=base.view(found.build.get_effective_layout+0x26,0x31);
    std::copy(core.begin(),core.end(),unrelated.bytes.begin()+unrelated.offset(0x100026));
    equal_profile(discovery::discover(discovery::Image(unrelated.bytes)).build,found.build);
    Fixture moved=original;
    const auto setter=discovery::property(base,found.build.set_layout_override,true);
    clone_function(moved,base,found.build.set_layout_override,setter.main_size,0x100000);
    moved.bytes[moved.offset(found.build.set_layout_override)]=0xcc;
    auto expected=found.build;expected.set_layout_override=0x100000;
    equal_profile(discovery::discover(discovery::Image(moved.bytes)).build,expected);
    Fixture truncated=original;truncated.bytes[truncated.offset(0x100fff)]=0xe8;
    bool rejected{};try{(void)discovery::x64::decode(discovery::Image(truncated.bytes),0x100fff);}catch(const std::runtime_error&){rejected=true;}
    check(rejected,"decoder must not consume padding outside the section");
    Fixture exchange=original;exchange.bytes[exchange.offset(0x100000)]=0x41;exchange.bytes[exchange.offset(0x100001)]=0x90;
    check(!discovery::x64::decode(discovery::Image(exchange.bytes),0x100000).nop(),"REX-prefixed register exchange is not a NOP");
    std::cout<<"PASS: unrelated short anchor ignored; relocated setter rediscovered; truncated instruction rejected.\n";
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
    const auto* older=std::string(fixed.build.sha256)==golden25.sha256?&golden25:
        std::string(fixed.build.sha256)==golden26.sha256?&golden26:nullptr;
    check(older?fixed.automatic:!fixed.automatic,"2.5/2.6 must use automatic discovery; known profiles must stay fixed");
    const auto& expected=older?*older:fixed.build;
    const auto automatic=discovery::resolve(path,true);
    check(automatic.automatic,"forced discovery must bypass the hash table");
    equal_profile(automatic.build,expected);
    Fixture fixture;
    {discovery::MappedFile file(path);fixture=compact(discovery::Image(file.bytes()),automatic);}
    const auto reduced=discovery::discover(discovery::Image(fixture.bytes));
    equal_profile(reduced.build,expected);
    negatives(fixture,reduced);
    variations(fixture,reduced);
    dispatch_variations(fixture,reduced);
    live_code(reduced,discovery::Image(fixture.bytes).image_size);
    const auto owned=std::filesystem::current_path()/("discovery-owned-"+std::to_string(GetCurrentProcessId())+".dll");
    check(!std::filesystem::exists(owned),"owned fixture path already exists");
    try {
        save(owned,fixture);
        const auto unknown=discovery::resolve(owned);
        check(unknown.automatic && !profile::find(unknown.build.sha256),"unknown file hash must use discovery");
        equal_profile(unknown.build,expected);
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
