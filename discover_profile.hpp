#pragma once
#include "pe_image.hpp"
#include "profile.hpp"
#include <set>

namespace discovery {
struct Property {
    uintptr_t klass{},pool{},pool_offset{},field{},initialized{},interface_slot{};
};
inline Property property(const Image& image,uintptr_t fn,bool setter) {
    require(image.matches(fn,setter?scan_rules::layout_setter:scan_rules::layout_getter),"property accessor shape differs");
    const uintptr_t delta=setter?16:0;
    Property p;
    p.klass=image.relative(fn+0x12+delta,{0x48,0x8b,0x0d});
    image.expect(fn+0x19+delta,{0x80,0xb9});
    p.initialized=image.read<uint32_t>(fn+0x1b+delta);
    p.pool=image.relative(fn+0x26+delta,{0x48,0x8b,0x05});
    image.expect(fn+0x2d+delta,{0x48,0x8b,0x80});
    p.pool_offset=image.read<uint32_t>(fn+0x30+delta);
    image.expect(fn+0x3d+delta,{0x48,0x8b,static_cast<uint8_t>(setter?0xb8:0xb0)});
    p.field=image.read<uint32_t>(fn+0x40+delta);
    p.interface_slot=image.relative(fn+0x4d+delta,{0x4c,0x8b,0x05});
    require(p.field>=16 && p.field<=4096 && p.field%8==0,"invalid property offset");
    require(p.pool_offset>0 && p.pool_offset<0x1000000 && p.pool_offset%8==0,"invalid static state offset");
    require(p.initialized>=64 && p.initialized<1024,"invalid class initialized offset");
    image.zero_slot(p.klass);image.zero_slot(p.pool);image.zero_slot(p.interface_slot);
    return p;
}
inline bool same_owner(const Property& a,const Property& b) {
    return a.klass==b.klass && a.pool==b.pool && a.pool_offset==b.pool_offset && a.initialized==b.initialized && a.interface_slot==b.interface_slot;
}
struct Result {
    struct CodeCheck { uintptr_t rva; std::vector<uint8_t> bytes; };
    profile::Build build{};
    bool automatic{};
    uintptr_t input_region{},screen_region{},frame_region{},touch_loop{},device_selector{},default_getter{};
    size_t setter_candidates{};
    std::vector<CodeCheck> code_checks;
};
inline Result discover(const Image& image) {
    Result r;
    r.automatic=true;
    r.input_region=image.unique(scan_rules::input_region);
    r.screen_region=image.unique(scan_rules::screen_region);
    r.frame_region=image.unique(scan_rules::frame_region);
    r.touch_loop=image.unique(scan_rules::touch_loop);
    r.device_selector=image.unique(scan_rules::device_selector);
    auto& p=r.build;
    p.version="auto";
    p.get_effective_layout=image.unique(scan_rules::effective_layout);
    p.get_layout_override=image.relative(p.get_effective_layout+0x26,{0xe8});
    require(image.relative(p.get_effective_layout+0x45,{0xe9})==p.get_layout_override,"effective getter has conflicting override branches");
    r.default_getter=image.relative(p.get_effective_layout+0x52,{0xe9});
    require(p.get_layout_override!=r.default_getter,"override and default getters alias");
    require(image.relative(r.touch_loop+0x83,{0xe8})==p.get_effective_layout,"touch loop uses a different layout getter");
    // This compare is the native PC-vs-touch branch. The rule keeps PC == 2.
    image.expect(r.touch_loop+0x8b,{0x83,0xf8,0x02});
    // Bind the Mobile == 1 -> TouchScreen == 3 mapping to this same UI getter.
    require(image.relative(r.device_selector+0xc8,{0xe8})==p.get_effective_layout &&
            image.relative(r.device_selector+0x1dc,{0xe8})==p.get_effective_layout,"device selector uses a different layout getter");
    image.expect(r.device_selector+0xcd,{0xbf,3,0,0,0,0x83,0xf8,1});

    const auto override=property(image,p.get_layout_override,false),fallback=property(image,r.default_getter,false);
    require(same_owner(override,fallback) && override.field!=fallback.field,"layout getters do not share a provider");
    p.ui_class_slot=override.klass;p.static_reference_pool=override.pool;p.ui_state_offset=override.pool_offset;
    p.class_initialized_offset=override.initialized;p.override_property_offset=override.field;p.default_property_offset=fallback.field;
    // The effective accessor must use the same class and initialized-byte field.
    require(image.relative(p.get_effective_layout+0x16,{0x48,0x8b,0x0d})==p.ui_class_slot,"effective getter class differs");
    require(image.read<uint32_t>(p.get_effective_layout+0x1f)==p.class_initialized_offset,"effective getter initialized offset differs");

    auto setters=image.find(scan_rules::layout_setter);
    r.setter_candidates=setters.size();
    size_t valid{};
    for(auto candidate:setters) {
        const auto owner=property(image,candidate,true);
        if(same_owner(override,owner) && override.field==owner.field){p.set_layout_override=candidate;++valid;}
    }
    require(valid==1,"expected exactly one setter for the override property, got "+std::to_string(valid));
    require(std::set<uintptr_t>{p.get_layout_override,p.set_layout_override,p.get_effective_layout,r.default_getter}.size()==4,"UI function entries alias");

    p.get_touch_slot=image.relative(r.input_region+0x70,{0x48,0x8b,0x05});
    image.expect(r.input_region+0x77,{0x48,0xff,0xe0});
    p.touch_count_slot=image.relative(r.input_region+0x220,{0x48,0xff,0x25});
    p.touch_supported_slot=image.relative(r.input_region+0x230,{0x48,0xff,0x25});
    p.frame_count_slot=image.relative(r.frame_region,{0x48,0xff,0x25});
    p.screen_width_slot=image.relative(r.screen_region+0x20,{0x48,0xff,0x25});
    p.screen_height_slot=image.relative(r.screen_region+0x30,{0x48,0xff,0x25});
    // Independent consumers bind the touch loop to the detected Unity wrappers.
    require(image.relative(r.touch_loop+0x2bf,{0xff,0x15})==p.touch_count_slot,"touch loop count slot differs");
    require(image.relative(r.touch_loop+0x2fb,{0xff,0x15})==p.get_touch_slot,"touch loop GetTouch slot differs");
    const std::set<uintptr_t> slots{p.touch_count_slot,p.get_touch_slot,p.touch_supported_slot,p.frame_count_slot,p.screen_width_slot,p.screen_height_slot,p.ui_class_slot,p.static_reference_pool};
    require(slots.size()==8,"detected storage slots alias");
    for(auto slot:slots)image.zero_slot(slot);
    for(auto [rva,length]:{std::pair{r.input_region,scan_rules::input_region.bytes.size()},
            std::pair{r.screen_region,scan_rules::screen_region.bytes.size()},
            std::pair{r.frame_region,scan_rules::frame_region.bytes.size()},
            std::pair{r.touch_loop,size_t{0x310}},
            std::pair{r.device_selector,scan_rules::device_selector.bytes.size()},
            std::pair{p.get_layout_override,scan_rules::layout_getter.bytes.size()},
            std::pair{r.default_getter,scan_rules::layout_getter.bytes.size()},
            std::pair{p.set_layout_override,scan_rules::layout_setter.bytes.size()},
            std::pair{p.get_effective_layout,scan_rules::effective_layout.bytes.size()}}) {
        const auto data=image.view(rva,length);
        r.code_checks.push_back({rva,{data.begin(),data.end()}});
    }
    return r;
}
}
