// Authored executable fixtures only. No game process or game code is loaded.
#include "button_layout_patch.hpp"
#include "test_module.hpp"
#include <iostream>
#include <map>

namespace {
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<class T> void put(std::vector<uint8_t>& bytes,uintptr_t at,T value) {
    check(at<=bytes.size() && sizeof(T)<=bytes.size()-at,"fixture bounds");
    std::memcpy(bytes.data()+at,&value,sizeof(value));
}
struct Emit {
    std::vector<uint8_t>& bytes;uintptr_t at;
    std::map<std::string,uintptr_t> labels;
    struct Fix {uintptr_t at;std::string label;};std::vector<Fix> fixups;
    void raw(std::initializer_list<uint8_t> value){for(auto b:value)put<uint8_t>(bytes,at++,b);}
    void dword(int32_t value){put<int32_t>(bytes,at,value);at+=4;}
    void relative(std::initializer_list<uint8_t> op,uintptr_t target) {
        raw(op);dword(static_cast<int32_t>(int64_t(target)-int64_t(at)-4));
    }
    void branch(std::initializer_list<uint8_t> op,const char* label) {
        raw(op);fixups.push_back({at,label});dword(0);
    }
    void label(const char* name){check(labels.emplace(name,at).second,"duplicate fixture label");}
    void finish(){for(const auto& f:fixups)put<int32_t>(bytes,f.at,static_cast<int32_t>(int64_t(labels.at(f.label))-int64_t(f.at)-4));}
};
struct Fixture {
    static constexpr uintptr_t owner=0x8000,active_slot=0x8010,klass=0x8200;
    static constexpr uintptr_t metadata=0x8100,hotfix=0x8101,enabled=0x8103,cache=0x8110,cached=0x8114,ui=0x8120;
    std::vector<uint8_t> bytes=std::vector<uint8_t>(0x9000);
    uintptr_t getter,device,position,size,position_apply,size_apply,active,initialize;
    uintptr_t initialized;
    explicit Fixture(uintptr_t delta=0,uintptr_t init=0x31):getter(0x1100+delta),device(0x1200+delta),
        position(0x1400+delta),size(0x1800+delta),position_apply(0x2000+delta),size_apply(0x2400+delta),
        active(0x2600+delta),initialize(0x2700+delta),initialized(init) {
        put<uint16_t>(bytes,0,0x5a4d);put<uint32_t>(bytes,0x3c,0x80);put<uint32_t>(bytes,0x80,0x4550);
        put<uint16_t>(bytes,0x84,0x8664);put<uint16_t>(bytes,0x86,2);put<uint16_t>(bytes,0x94,0xf0);
        put<uint16_t>(bytes,0x98,0x20b);put<uint32_t>(bytes,0x98+56,0x9000);put<uint32_t>(bytes,0x98+60,0x1000);
        for(int i=0;i<2;++i) {
            const auto h=0x188+i*40;const uint32_t rva=i?0x8000:0x1000,length=i?0x1000:0x6000;
            put<uint32_t>(bytes,h+8,length);put<uint32_t>(bytes,h+12,rva);
            put<uint32_t>(bytes,h+16,length);put<uint32_t>(bytes,h+20,rva);
            put<uint32_t>(bytes,h+36,i?0xc0000040:0x60000020);
        }
        std::fill(bytes.begin()+0x1000,bytes.begin()+0x7000,uint8_t{0xcc});
        bytes[metadata]=1;bytes[enabled]=1;bytes[cached]=1;bytes[klass+initialized]=1;
        put<int>(bytes,cache,3);put<int>(bytes,ui,1);
        Emit u{bytes,getter};u.relative({0x8b,0x05},ui);u.raw({0xc3});
        Emit a{bytes,active};a.raw({0x8a,0x41,0x18,0xc3});
        bytes[initialize]=0xc3;
        Emit d{bytes,device};d.raw({0x53,0x48,0x83,0xec,0x20,0x89,0xcb});
        d.relative({0x80,0x3d},metadata);d.raw({0});
        // RIP-relative byte comparisons have their immediate after displacement.
        put<int32_t>(bytes,d.at-5,static_cast<int32_t>(metadata-d.at));
        d.branch({0x0f,0x84},"cold");
        d.relative({0x80,0x3d},hotfix);d.raw({0});put<int32_t>(bytes,d.at-5,static_cast<int32_t>(hotfix-d.at));
        d.branch({0x0f,0x85},"cold");d.relative({0x48,0x8b,0x0d},owner);
        d.raw({0x0f,0xb6,0x81});d.dword(static_cast<int32_t>(initialized));d.raw({0x84,0xc0});d.branch({0x0f,0x84},"cold");
        d.relative({0x80,0x3d},cached);d.raw({0});put<int32_t>(bytes,d.at-5,static_cast<int32_t>(cached-d.at));
        d.branch({0x0f,0x84},"cold");d.raw({0x84,0xc0});d.branch({0x0f,0x84},"cold");
        d.relative({0x8b,0x05},cache);d.raw({0x48,0x83,0xc4,0x20,0x5b,0xc3});
        d.label("cold");d.raw({0xb8,3,0,0,0,0x48,0x83,0xc4,0x20,0x5b,0xc3});d.finish();
        make_gate(position,true);make_gate(size,false);
        make_apply(position_apply,position,true);make_apply(size_apply,size,false);
    }
    void make_gate(uintptr_t entry,bool is_position) {
        Emit e{bytes,entry};e.raw({0x56,0x48,0x83,0xec,0x20,0x48,0x89,0xce,0x48,0x89,0xf1});
        e.relative({0xff,0x15},active_slot);e.raw({0x84,0xc0});e.branch({0x0f,0x84},"false");
        e.relative({0x80,0x3d},enabled);e.raw({0});put<int32_t>(bytes,e.at-5,static_cast<int32_t>(enabled-e.at));
        e.branch({0x0f,0x84},"false");
        if(is_position) {
            e.raw({0x80,0xbe});e.dword(0x9b);e.raw({0});e.branch({0x0f,0x84},"mobile");
            e.label("false");e.raw({0x31,0xf6});e.label("result");e.raw({0x89,0xf0,0x48,0x83,0xc4,0x20,0x5e,0xc3});
        }
        e.label("mobile");
        // The cmp must fit in a single aligned word; vary relocation/field offsets.
        while((e.at+41)%8>5)e.raw({0x90});
        e.relative({0xe8},getter);e.raw({0x83,0xf8,1});e.branch({0x0f,0x85},"false");
        e.relative({0x48,0x8b,0x0d},owner);e.raw({0x80,0xb9});e.dword(static_cast<int32_t>(initialized));e.raw({0});
        e.branch({0x0f,0x84},"initialize");e.label("device");e.raw({0xb1,1});e.relative({0xe8},device);
        e.raw({0x83,0xf8,1,0x40,0x0f,0x94,0xc6});e.branch({0xe9},"result");
        if(!is_position){e.label("false");e.raw({0x31,0xf6});e.label("result");e.raw({0x89,0xf0,0x48,0x83,0xc4,0x20,0x5e,0xc3});}
        e.label("initialize");e.relative({0xe8},initialize);e.branch({0xe9},"device");e.finish();
    }
    void make_apply(uintptr_t entry,uintptr_t target,bool is_position) {
        Emit e{bytes,entry};e.raw({0x80,0xbb});e.dword(is_position?0x98:0x38);e.raw({0});e.branch({0x0f,0x84},"end");
        e.raw({0x48,0x89,0xd9});e.relative({0xe8},target);e.raw({0x84,0xc0});e.branch({0x0f,0x85},"apply");
        e.branch({0xe9},"end");e.label("apply");e.raw({0xc6,0x83});e.dword(is_position?0x9b:0x3a);e.raw({1});
        e.label("end");e.raw({0xc3});e.finish();
    }
    discovery::button_layout::Resolution resolve()const {
        const discovery::Image image(bytes);
        return discovery::button_layout::resolve(image,getter,initialized,discovery::x64::calls_to(image,getter));
    }
};
void negatives() {
    Fixture f;auto good=f.resolve();check(good.plan.has_value(),good.error.c_str());
    check(good.plan->device_cache==Fixture::cache && good.plan->device_ready==Fixture::cached,"derive shared device cache");
    f.bytes[f.size]=0xcc;check(!f.resolve().plan,"missing size gate must reject the entire pair");
    f=Fixture{};f.make_gate(0x3000,true);f.make_apply(0x3400,0x3000,true);
    check(!f.resolve().plan,"duplicate eligible handler must be rejected");
    f=Fixture{};good=f.resolve();
    f.bytes[good.plan->sites[0].compare+2]=2;
    check(!f.resolve().plan,"unexpected device enum comparison must be rejected");
    f=Fixture{};put<int32_t>(f.bytes,f.position_apply+2,0x99);
    check(!f.resolve().plan,"inconsistent application fields must be rejected");
    f=Fixture{};f.bytes[f.device+14]=0x90;
    check(!f.resolve().plan,"malformed device cache wrapper must be rejected");
    Fixture moved(0x1000,0x45);auto found=moved.resolve();
    check(found.plan.has_value(),found.error.c_str());
    check(found.plan->sites[0].gate==moved.position && found.plan->sites[1].gate==moved.size,"relocated role discovery");
}
void write_word(uintptr_t address,uint64_t value) {
    DWORD old{},ignored{};check(VirtualProtect(reinterpret_cast<void*>(address),8,PAGE_EXECUTE_READWRITE,&old)!=FALSE,"fixture writable");
    std::memcpy(reinterpret_cast<void*>(address),&value,8);
    check(FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(address),8)!=FALSE,"fixture flush");
    check(VirtualProtect(reinterpret_cast<void*>(address),8,old,&ignored)!=FALSE,"fixture protection restore");
}
void execution_and_ownership() {
    Fixture f;const auto result=f.resolve();check(result.plan.has_value(),result.error.c_str());
    const auto& plan=*result.plan;
    test_module::Mapping mapping(f.bytes);
    mapping.put<uintptr_t>(Fixture::owner,mapping.base+Fixture::klass);
    mapping.put<uintptr_t>(Fixture::active_slot,mapping.base+f.active);
    mapping.protect(0x1000,0x6000,PAGE_EXECUTE_READ);
    const auto loaded=discovery::capture_module(mapping.base);
    const auto memory_plan=discovery::button_layout::resolve(loaded,f.getter,f.initialized,discovery::x64::calls_to(loaded,f.getter));
    check(memory_plan.plan.has_value(),memory_plan.error.c_str());
    check(memory_plan.plan->sites[0].word==plan.sites[0].word,"loaded initialized slots and file view agree");
    alignas(8) std::array<uint8_t,256> receiver{};receiver[0x18]=1;
    auto position=reinterpret_cast<bool(*)(void*)>(mapping.base+f.position);
    auto size=reinterpret_cast<bool(*)(void*)>(mapping.base+f.size);
    check(!position(receiver.data()) && !size(receiver.data()),"reproduce Desktop rejecting Mobile custom layout");
    mapping.put<int>(Fixture::cache,1);check(position(receiver.data()) && size(receiver.data()),"original Handheld acceptance");
    mapping.put<int>(Fixture::cache,3);
    touchui::zzz::ButtonLayoutPatch patch;
    const auto changed=plan.sites[1].expected^uint64_t{0x100};
    write_word(mapping.base+plan.sites[1].word,changed);
    check(!patch.install(mapping.base,plan),"stale second word must prevent all mutation");
    check(*reinterpret_cast<uint64_t*>(mapping.base+plan.sites[0].word)==plan.sites[0].expected,"first word remains original on preflight failure");
    write_word(mapping.base+plan.sites[1].word,plan.sites[1].expected);
    check(patch.install(mapping.base,plan) && patch.installed(),"install both atomic words");
    check(position(receiver.data()) && size(receiver.data()),"patched Desktop custom layout eligibility");
    check(*reinterpret_cast<int*>(mapping.base+Fixture::cache)==3,"global hardware identity stays Desktop");
    mapping.put<int>(Fixture::ui,2);check(!position(receiver.data()) && !size(receiver.data()),"PC UI stays ineligible");
    mapping.put<int>(Fixture::ui,1);receiver[0x18]=0;check(!position(receiver.data()) && !size(receiver.data()),"inactive components stay ineligible");
    receiver[0x18]=1;receiver[0x9b]=1;check(!position(receiver.data()) && size(receiver.data()),"position reentrancy guard retained");
    receiver[0x9b]=0;mapping.put<uint8_t>(Fixture::enabled,0);check(!position(receiver.data()) && !size(receiver.data()),"shared enable flag retained");
    mapping.put<uint8_t>(Fixture::enabled,1);
    MEMORY_BASIC_INFORMATION region{};VirtualQuery(reinterpret_cast<void*>(mapping.base+plan.sites[0].word),&region,sizeof(region));
    check(region.Protect==PAGE_EXECUTE_READ,"code protection restored");
    check(patch.restore() && !patch.installed(),"owned restoration");
    check(!position(receiver.data()) && !size(receiver.data()),"original Desktop gate restored");
    check(patch.install(mapping.base,plan),"reinstall after owned restoration");
    const auto competing=plan.sites[1].replacement^uint64_t{0x100000000000000};
    write_word(mapping.base+plan.sites[1].word,competing);
    check(!patch.restore(),"report competing writer on restore");
    check(*reinterpret_cast<uint64_t*>(mapping.base+plan.sites[1].word)==competing,"preserve competing writer");
    check(*reinterpret_cast<uint64_t*>(mapping.base+plan.sites[0].word)==plan.sites[0].expected,"restore other owned word despite conflict");
}
}
int main() {
    try {negatives();execution_and_ownership();std::cout<<"PASS: button layout semantic discovery, local eligibility and atomic ownership\n";return 0;}
    catch(const std::exception& ex){std::cerr<<"FAIL: "<<ex.what()<<"\n";return 1;}
}
