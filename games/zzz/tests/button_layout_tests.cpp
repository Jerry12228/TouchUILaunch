// Authored executable fixtures only. No game process or game code is loaded.
#include "button_layout_patch.hpp"
#include "test_module.hpp"
#include "short_discovery.hpp"
#include <fstream>
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
    static constexpr uintptr_t metadata=0x8100,hotfix=0x8101,enabled=0x8103,shortcut=0x8104,cache=0x8110,cached=0x8114,ui=0x8120;
    std::vector<uint8_t> bytes=std::vector<uint8_t>(0x9000);
    uintptr_t getter,device,position,size,position_apply,size_apply,active,initialize;
    uintptr_t initialized;
    bool branched;
    explicit Fixture(uintptr_t delta=0,uintptr_t init=0x31,bool branch_form=false):getter(0x1100+delta),device(0x1200+delta),
        position(0x1400+delta),size(0x1800+delta),position_apply(0x2000+delta),size_apply(0x2400+delta),
        active(0x2600+delta),initialize(0x2700+delta),initialized(init),branched(branch_form) {
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
            e.raw({0x80,0xbe});e.dword(branched?0xa3:0x9b);e.raw({0});e.branch({0x0f,0x84},"mobile");
            e.label("false");e.raw({0x31,0xf6});e.label("result");e.raw({0x89,0xf0,0x48,0x83,0xc4,0x20,0x5e,0xc3});
        }
        e.label("mobile");
        // The cmp must fit in a single aligned word; vary relocation/field offsets.
        while((e.at+(branched?54:41))%8>5)e.raw({0x90});
        e.relative({0xe8},getter);e.raw({0x83,0xf8,1});e.branch({0x0f,0x85},"false");
        if(branched) {
            e.relative({0x80,0x3d},shortcut);e.raw({0});put<int32_t>(bytes,e.at-5,static_cast<int32_t>(shortcut-e.at));
            e.branch({0x0f,0x85},"accepted");
        }
        e.relative({0x48,0x8b,0x0d},owner);e.raw({0x80,0xb9});e.dword(static_cast<int32_t>(initialized));e.raw({0});
        e.branch({0x0f,0x84},"initialize");e.label("device");e.raw({0xb1,1});e.relative({0xe8},device);
        e.raw({0x83,0xf8,1});
        if(branched){e.branch({0x0f,0x85},"false");e.label("accepted");e.raw({0x40,0xb6,1});}
        else e.raw({0x40,0x0f,0x94,0xc6});
        e.branch({0xe9},"result");
        if(!is_position){e.label("false");e.raw({0x31,0xf6});e.label("result");e.raw({0x89,0xf0,0x48,0x83,0xc4,0x20,0x5e,0xc3});}
        if(branched)while((e.at+12)%8!=(is_position?2u:7u))e.raw({0x90});
        e.label("initialize");e.relative({0xe8},initialize);
        if(branched) {
            e.raw({0xb1,1});e.relative({0xe8},device);e.raw({0x83,0xf8,1});
            e.branch({0x0f,0x84},"accepted");e.branch({0xe9},"false");
        } else e.branch({0xe9},"device");
        e.finish();
    }
    void make_apply(uintptr_t entry,uintptr_t target,bool is_position) {
        Emit e{bytes,entry};e.raw({0x80,0xbb});e.dword(branched?(is_position?0xa0:0x40):(is_position?0x98:0x38));e.raw({0});e.branch({0x0f,0x84},"end");
        e.raw({0x48,0x89,0xd9});e.relative({0xe8},target);e.raw({0x84,0xc0});e.branch({0x0f,0x85},"apply");
        e.branch({0xe9},"end");e.label("apply");e.raw({0xc6,0x83});e.dword(branched?(is_position?0xa3:0x42):(is_position?0x9b:0x3a));e.raw({1});
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
namespace {
struct FaultMemory:touchui::zzz::ButtonLayoutMemory {
    inline static unsigned protects{},flushes{},exchanges{};
    inline static std::set<unsigned> fail_protect,fail_flush,fail_exchange;
    inline static uintptr_t competing_at{};
    inline static uint64_t competing_word{};
    static void reset(){protects=flushes=exchanges=0;fail_protect.clear();fail_flush.clear();fail_exchange.clear();competing_at=0;}
    static bool protect(uintptr_t at,DWORD desired,DWORD& old) {
        if(fail_protect.contains(++protects))return false;
        return ButtonLayoutMemory::protect(at,desired,old);
    }
    static bool flush(uintptr_t at) {
        if(fail_flush.contains(++flushes))return false;
        return ButtonLayoutMemory::flush(at);
    }
    static bool exchange(uintptr_t at,uint64_t expected,uint64_t desired,uint64_t& previous) {
        if(fail_exchange.contains(++exchanges))return false;
        if(exchanges==2 && competing_at)write_word(competing_at,competing_word);
        return ButtonLayoutMemory::exchange(at,expected,desired,previous);
    }
};
using FaultPatch=touchui::zzz::BasicButtonLayoutPatch<FaultMemory>;
struct OwnedLayout {
    Fixture fixture;
    discovery::button_layout::Plan plan;
    test_module::Mapping mapping;
    explicit OwnedLayout(bool branched=false):fixture(0,0x31,branched),
        plan(*fixture.resolve().plan),mapping(fixture.bytes) {
        mapping.protect(0x1000,0x6000,PAGE_EXECUTE_READ);FaultMemory::reset();
    }
    uint64_t word(size_t index)const{return *reinterpret_cast<const uint64_t*>(mapping.base+plan.sites[index].word);}
    void original()const {
        for(size_t i=0;i<plan.sites.size();++i) {
            check(word(i)==plan.sites[i].expected,"all owned bytes restored after injected failure");
            MEMORY_BASIC_INFORMATION region{};
            check(VirtualQuery(reinterpret_cast<void*>(mapping.base+plan.sites[i].word),&region,sizeof(region))==sizeof(region) &&
                region.Protect==PAGE_EXECUTE_READ,"original RX protection restored after injected failure");
        }
    }
};
void review_safety() {
    {
        Fixture f;unsigned slot_reads{};
        const discovery::Image image(f.bytes,[&](uintptr_t at,std::span<uint8_t> target) {
            if(at>=0x8000){++slot_reads;throw discovery::MemoryReadError("injected optional slot failure");}
            std::copy_n(f.bytes.data()+at,target.size(),target.data());
        });
        const auto result=discovery::button_layout::resolve(image,f.getter,f.initialized,discovery::x64::calls_to(image,f.getter));
        check(!result.plan && slot_reads==1 && result.error.find("injected optional slot failure")!=std::string::npos,
            "unreadable optional evidence rejects feature without hiding failed reads as candidates");
    }
    {
        OwnedLayout f;FaultPatch patch;f.mapping.protect(0x1000,0x1000,PAGE_NOACCESS);
        check(!patch.install(f.mapping.base,f.plan) && FaultMemory::protects==0,"inaccessible code is never reopened");
    }
    {
        OwnedLayout f;FaultPatch patch;FaultMemory::fail_exchange={2};
        check(!patch.install(f.mapping.base,f.plan) && !patch.has_pending(),"atomic access fault rolls back first edit");f.original();
    }
    {
        OwnedLayout f;FaultPatch patch;check(patch.install(f.mapping.base,f.plan),"atomic restore fault fixture install");
        FaultMemory::fail_exchange={3};
        check(!patch.restore() && patch.has_pending(),"atomic restore access fault retains ownership");
        check(patch.restore() && !patch.has_pending(),"atomic restore fault can be retried conditionally");f.original();
    }
    // Changes outside the patch words must invalidate the entire install.
    for(int kind=0;kind<3;++kind) {
        OwnedLayout f;
        const auto at=kind==0?f.fixture.position:kind==1?f.fixture.position_apply:f.fixture.device;
        const auto aligned=at&~uintptr_t{7};
        const auto original=*reinterpret_cast<uint64_t*>(f.mapping.base+aligned);
        write_word(f.mapping.base+aligned,original^uint64_t{0x100});
        FaultPatch patch;
        check(!patch.install(f.mapping.base,f.plan) && FaultMemory::protects==0,"stale semantic evidence prevents all writes");
        check(f.word(0)==f.plan.sites[0].expected && f.word(1)==f.plan.sites[1].expected,"stale evidence keeps both gate words original");
    }
    {
        OwnedLayout f;FaultPatch patch;
        f.plan.sites[0].replacement^=uint64_t{1};
        check(!patch.install(f.mapping.base,f.plan) && FaultMemory::protects==0,"arbitrary or neighbor edit rejected before permissions");
        f.original();
    }
    {
        OwnedLayout f;FaultPatch patch;f.plan.evidence.clear();
        check(!patch.install(f.mapping.base,f.plan) && FaultMemory::protects==0,"evidence-free plan rejected");
    }
    {
        // The busy=false branch must skip the common false-return block.
        Fixture f;const discovery::Image image(f.bytes);
        discovery::x64::Cursor cursor(image,f.position,512);
        while(cursor.at<f.position+512) {
            const auto i=cursor.take();
            if(i.cmp_zero_byte(6) && i.disp()==0x9b) {
                const auto branch=cursor.take();
                put<int32_t>(f.bytes,branch.next()-4,0);break;
            }
        }
        check(!f.resolve().plan,"incorrect reentrancy continuation rejected");
    }
    {
        OwnedLayout f;FaultPatch patch;FaultMemory::fail_protect={3};
        check(!patch.install(f.mapping.base,f.plan) && !patch.has_pending(),"second write refusal rolls back first write");f.original();
    }
    {
        OwnedLayout f;FaultPatch patch;FaultMemory::fail_protect={2,3};
        check(!patch.install(f.mapping.base,f.plan) && patch.has_pending(),"failed RX cleanup retains owned state");
        check(f.word(0)==f.plan.sites[0].replacement,"failed cleanup leaves only tracked owned edit");
        check(patch.restore() && !patch.has_pending(),"retry uses originally saved RX protection");f.original();
    }
    {
        OwnedLayout f;FaultPatch patch;FaultMemory::fail_protect={3,4};
        check(!patch.install(f.mapping.base,f.plan) && patch.has_pending(),"failed partial rollback retains ownership");
        check(patch.restore() && !patch.has_pending(),"partial rollback can be completed later");f.original();
    }
    {
        OwnedLayout f;FaultPatch patch;FaultMemory::fail_flush={1,2};
        check(!patch.install(f.mapping.base,f.plan) && patch.has_pending(),"failed cache flush retains cleanup state");
        check(patch.restore() && !patch.has_pending(),"flush recovery completes owned rollback");f.original();
    }
    {
        OwnedLayout f;FaultPatch patch;
        check(patch.install(f.mapping.base,f.plan),"restore failure fixture install");
        FaultMemory::fail_protect={5};
        check(!patch.restore() && patch.has_pending(),"restore permission refusal retains one site");
        check(patch.restore() && !patch.has_pending(),"restore retry finishes remaining site");f.original();
    }
    {
        OwnedLayout f;FaultPatch patch;
        FaultMemory::competing_at=f.mapping.base+f.plan.sites[1].word;
        FaultMemory::competing_word=f.plan.sites[1].expected^uint64_t{0x100};
        check(!patch.install(f.mapping.base,f.plan) && !patch.has_pending(),"CAS conflict rolls back remaining owned edit");
        check(f.word(0)==f.plan.sites[0].expected && f.word(1)==FaultMemory::competing_word,"competing CAS writer preserved");
    }
    {
        OwnedLayout f;FaultPatch patch;
        check(patch.install(f.mapping.base,f.plan) && patch.intact(),"intact optional patch");
        const auto competing=f.plan.sites[1].replacement^uint64_t{0x100000000000000};
        write_word(f.mapping.base+f.plan.sites[1].word,competing);
        check(!patch.intact() && !patch.restore() && !patch.has_pending(),"changed installed words stop optional feature");
        check(f.word(1)==competing && f.word(0)==f.plan.sites[0].expected,"runtime conflict preserves competing word");
    }
    {
        OwnedLayout f;FaultPatch patch;
        check(patch.install(f.mapping.base,f.plan),"protection ownership fixture install");
        // Both fixture gates share this page. Preserve a later owner's RWX.
        f.mapping.protect(0x1000,0x1000,PAGE_EXECUTE_READWRITE);
        const auto before=FaultMemory::protects;
        check(!patch.restore() && !patch.has_pending() && FaultMemory::protects==before,"changed page permissions are preserved");
        MEMORY_BASIC_INFORMATION region{};VirtualQuery(reinterpret_cast<void*>(f.mapping.base+0x1000),&region,sizeof(region));
        check(region.Protect==PAGE_EXECUTE_READWRITE,"no forced permission restoration");
    }
}
void branched_discovery() {
    for(uintptr_t delta:{uintptr_t{0},uintptr_t{0x1000}}) {
        Fixture f(delta,0x45,true);auto result=f.resolve();check(result.plan.has_value(),result.error.c_str());
        const auto& sites=result.plan->sites;
        check(sites.size()==4,"both copied initialization checks must be planned");
        check(sites[3].compare%8==7 && sites[3].edit==discovery::button_layout::Edit::equal_branch,
            "straddling cold comparison uses its aligned proven success edge");
        check(sites[0].gate==f.position && sites[1].gate==f.size && sites[2].gate==f.position && sites[3].gate==f.size,
            "warm and cold checks retain their handler roles after relocation");
        f.bytes[sites[2].compare+2]=2;check(!f.resolve().plan,"unknown cold device enum rejects the entire feature");
    }
    Fixture f(0,0x31,true);auto good=f.resolve();check(good.plan.has_value(),good.error.c_str());
    const auto original=f.bytes;
    const discovery::Image image(f.bytes);
    // Mutate the new flag reference/edge, both cold-path call relationships,
    // and the return joins independently; none may become a first-match patch.
    discovery::x64::Cursor c(image,f.size,512);
    uintptr_t shortcut_compare{};
    while(c.at<f.size+512) {
        const auto i=c.take();
        if(i.cmp_zero_byte(discovery::x64::rip) && i.storage(image)==Fixture::shortcut){shortcut_compare=i.at;break;}
    }
    check(shortcut_compare!=0,"fixture shortcut comparison found");
    put<int32_t>(f.bytes,shortcut_compare+2,static_cast<int32_t>(Fixture::shortcut+1-shortcut_compare-7));
    check(!f.resolve().plan,"handlers using different shortcut flags rejected");
    f.bytes=original;f.bytes[shortcut_compare+8]=0x84;
    check(!f.resolve().plan,"inverted shortcut acceptance rejected");
    for(size_t site_index:{size_t{2},size_t{3}}) {
        const auto compare=good.plan->sites[site_index].compare;
        f.bytes=original;f.bytes[compare-7]=0;
        check(!f.resolve().plan,"changed cold device argument rejected");
        f.bytes=original;put<int32_t>(f.bytes,compare-4,0);
        check(!f.resolve().plan,"changed cold device callee rejected");
        f.bytes=original;f.bytes[compare+4]=0x85;
        check(!f.resolve().plan,"inverted cold success edge rejected");
        f.bytes=original;put<int32_t>(f.bytes,compare+5,0);
        check(!f.resolve().plan,"cold success edge outside shared true block rejected");
    }
    f.bytes=original;f.make_gate(0x3000,true);f.make_apply(0x3400,0x3000,true);
    check(!f.resolve().plan,"duplicate branch-form handler rejected");
}
void branched_execution_and_ownership() {
    OwnedLayout f(true);
    f.mapping.put<uintptr_t>(Fixture::owner,f.mapping.base+Fixture::klass);
    f.mapping.put<uintptr_t>(Fixture::active_slot,f.mapping.base+f.fixture.active);
    alignas(8) std::array<uint8_t,256> receiver{};receiver[0x18]=1;
    auto position=reinterpret_cast<bool(*)(void*)>(f.mapping.base+f.fixture.position);
    auto size=reinterpret_cast<bool(*)(void*)>(f.mapping.base+f.fixture.size);
    FaultPatch patch;check(!patch.installed(),"empty patch is not installed");
    for(bool initialized:{false,true}) {
        f.mapping.put<uint8_t>(Fixture::klass+f.fixture.initialized,initialized?1:0);
        check(!position(receiver.data()) && !size(receiver.data()),"unpatched Desktop rejection on warm and cold paths");
    }
    check(patch.install(f.mapping.base,f.plan) && patch.installed(),"install complete four-site plan");
    for(bool initialized:{false,true})for(uint8_t shortcut:{uint8_t{0},uint8_t{1}}) {
        f.mapping.put<uint8_t>(Fixture::klass+f.fixture.initialized,initialized?1:0);
        f.mapping.put<uint8_t>(Fixture::shortcut,shortcut);
        check(position(receiver.data()) && size(receiver.data()),"both warm and cold Desktop paths become eligible");
        check(*reinterpret_cast<uint8_t*>(f.mapping.base+Fixture::shortcut)==shortcut &&
            *reinterpret_cast<int*>(f.mapping.base+Fixture::cache)==3,"global shortcut and hardware values preserved");
        f.mapping.put<int>(Fixture::ui,2);check(!position(receiver.data()) && !size(receiver.data()),"PC UI rejected on all branch forms");
        f.mapping.put<int>(Fixture::ui,1);receiver[0x18]=0;
        check(!position(receiver.data()) && !size(receiver.data()),"inactive components rejected on all branch forms");
        receiver[0x18]=1;receiver[0xa3]=1;
        check(!position(receiver.data()) && size(receiver.data()),"branch-form position reentrancy remains enforced");
        receiver[0xa3]=0;f.mapping.put<uint8_t>(Fixture::enabled,0);
        check(!position(receiver.data()) && !size(receiver.data()),"branch-form enable condition remains enforced");
        f.mapping.put<uint8_t>(Fixture::enabled,1);
    }
    check(patch.restore(),"restore complete four-site plan");f.original();
    for(unsigned failure=1;failure<=4;++failure) {
        FaultMemory::reset();FaultMemory::fail_exchange={failure};
        check(!patch.install(f.mapping.base,f.plan) && !patch.has_pending(),"failure at any new site rolls back all owned edits");f.original();
    }
    FaultMemory::reset();auto incomplete=f.plan;incomplete.sites.pop_back();
    check(!patch.install(f.mapping.base,incomplete) && FaultMemory::protects==0,"incomplete copied-check plan never writes");
    auto wrong=f.plan;wrong.sites[3].accepted++;
    check(!patch.install(f.mapping.base,wrong) && FaultMemory::protects==0,"redirected success target rejected before writing");
    FaultMemory::reset();check(patch.install(f.mapping.base,f.plan),"four-site restore retry install");
    FaultMemory::fail_exchange={5};
    check(!patch.restore() && patch.has_pending(),"cold-site restore failure retains ownership");
    check(patch.restore() && !patch.has_pending(),"cold-site restore can be retried");f.original();
    FaultMemory::reset();check(patch.install(f.mapping.base,f.plan),"four-site conflict install");
    const auto competing=f.plan.sites[3].replacement^uint64_t{0x100000000000000};
    write_word(f.mapping.base+f.plan.sites[3].word,competing);
    check(!patch.restore() && !patch.has_pending(),"cold-site competing writer preserved while other owned edits restore");
    check(f.word(3)==competing,"cold-site conflicting bytes are not overwritten");
    for(size_t i=0;i<3;++i)check(f.word(i)==f.plan.sites[i].expected,"other owned sites restored after cold-site conflict");
}
void static_sample(const char* path) {
    std::ifstream stream(path,std::ios::binary|std::ios::ate);check(bool(stream),"cannot open read-only sample");
    const auto length=stream.tellg();check(length>0 && uint64_t(length)<=UINT32_MAX,"invalid sample size");
    std::vector<uint8_t> bytes(static_cast<size_t>(length));stream.seekg(0);
    check(bool(stream.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()))),"incomplete sample read");
    const discovery::Image image(bytes);const auto result=discovery::discover(image,true);
    check(result.button_layout.plan.has_value(),result.button_layout.error.c_str());
    const auto& plan=*result.button_layout.plan;
    for(const auto& site:plan.sites) {
        check(image.read<uint64_t>(site.word)==site.expected,"sample edit expected word differs");
        const auto expected=discovery::button_layout::replacement_word(site);
        check(expected && *expected==site.replacement,"sample edit is not an allowed local replacement");
        std::cout<<"site compare=0x"<<std::hex<<site.compare<<" patch=0x"<<site.patch<<" word=0x"<<site.word
            <<" kind="<<(site.edit==discovery::button_layout::Edit::comparison?"comparison":"equal_branch")<<std::dec<<"\n";
    }
    std::cout<<"PASS: read-only sample discovery; sites="<<plan.sites.size()<<"; no game code loaded or executed\n";
}
}

int main(int argc,char** argv) {
    try {
        if(argc==3 && std::string(argv[1])=="--sample"){static_sample(argv[2]);return 0;}
        check(argc==1,"usage: ButtonLayoutTests [--sample GameAssembly.dll]");
        negatives();execution_and_ownership();review_safety();branched_discovery();branched_execution_and_ownership();
        std::cout<<"PASS: button layout semantic discovery, warm/cold eligibility and atomic ownership\n";return 0;
    }
    catch(const std::exception& ex){std::cerr<<"FAIL: "<<ex.what()<<"\n";return 1;}
}
