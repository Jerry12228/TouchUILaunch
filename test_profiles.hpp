// Generated manual oracles. Include in tests only.
#pragma once
#include "profile.hpp"
#include <string_view>
namespace test_profile {
struct Oracle { const char* version; const char* sha256; profile::Build build;
};
inline constexpr Oracle oracles[] = {
    {
        "2.5", "69142459d5559677ac7f4c38ae88f568dc62ea41e2017e266f23889f33074ceb", {
        0x4f43f80, // touch_count_slot
        0x4f43fa0, // get_touch_slot
        0x4f43f88, // touch_supported_slot
        0x4f41b40, // frame_count_slot
        0x4f3d8b0, // screen_width_slot
        0x4f3d8b8, // screen_height_slot
        0x4f87fa0, // ui_class_slot
        0x4e93b48, // static_reference_pool
        0x3d9a0, // ui_state_offset
        0x7862010, // get_layout_override
        0x7862130, // set_layout_override
        0x786fc80, // get_effective_layout
        0xcb, // class_initialized_offset
        0x68, // override_property_offset
        0x70, // default_property_offset
    }},
    {
        "2.6", "8547fc8a2aaa6b509a4ac4e3b8ddf65997afd52f3f7c3652917d0effe2625e1a", {
        0x4916a68, // touch_count_slot
        0x4916a88, // get_touch_slot
        0x4916a70, // touch_supported_slot
        0x4913c38, // frame_count_slot
        0x4910500, // screen_width_slot
        0x4910508, // screen_height_slot
        0x4950e90, // ui_class_slot
        0x4885c60, // static_reference_pool
        0x295a8, // ui_state_offset
        0xb10d400, // get_layout_override
        0xb109140, // set_layout_override
        0xb103be0, // get_effective_layout
        0xcb, // class_initialized_offset
        0x70, // override_property_offset
        0x60, // default_property_offset
    }},
    {
        "3.1", "4cba5d52c5fbfd478d2a9ec217075f82216780d56ad1bd1e85e4f724dcce30b4", {
        0x540dcb8, // touch_count_slot
        0x540dcd8, // get_touch_slot
        0x540dcc0, // touch_supported_slot
        0x540ad20, // frame_count_slot
        0x5407458, // screen_width_slot
        0x5407460, // screen_height_slot
        0x5450eb0, // ui_class_slot
        0x5360730, // static_reference_pool
        0x2d9c0, // ui_state_offset
        0x15b462a0, // get_layout_override
        0x15b46680, // set_layout_override
        0x15b43570, // get_effective_layout
        0xcb, // class_initialized_offset
        0x80, // override_property_offset
        0x88, // default_property_offset
    }},
    {
        "3.2", "2be366e9fca3b02d37e5285764590df6094b4ddc1dfb85ecd2320e326528f834", {
        0x4f7efa8, // touch_count_slot
        0x4f7efc8, // get_touch_slot
        0x4f7efb0, // touch_supported_slot
        0x4f7bef0, // frame_count_slot
        0x4f785f8, // screen_width_slot
        0x4f78600, // screen_height_slot
        0x4fc13e0, // ui_class_slot
        0x4ecd400, // static_reference_pool
        0x2e468, // ui_state_offset
        0x134c7d90, // get_layout_override
        0x134c7090, // set_layout_override
        0x134c8150, // get_effective_layout
        0xcb, // class_initialized_offset
        0x98, // override_property_offset
        0x88, // default_property_offset
    }},
};
}
