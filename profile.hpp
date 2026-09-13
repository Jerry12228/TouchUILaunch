// Generated field schema; no client addresses or fingerprints.
#pragma once
#include <cstdint>
namespace profile {
struct Build {
    uintptr_t touch_count_slot;
    uintptr_t get_touch_slot;
    uintptr_t touch_supported_slot;
    uintptr_t frame_count_slot;
    uintptr_t screen_width_slot;
    uintptr_t screen_height_slot;
    uintptr_t ui_class_slot;
    uintptr_t static_reference_pool;
    uintptr_t ui_state_offset;
    uintptr_t get_layout_override;
    uintptr_t set_layout_override;
    uintptr_t get_effective_layout;
    uintptr_t class_initialized_offset;
    uintptr_t override_property_offset;
    uintptr_t default_property_offset;
};
struct Field { const char* name; uintptr_t Build::* value; };
inline constexpr Field fields[] = {
    {"touch_count_slot", &Build::touch_count_slot},
    {"get_touch_slot", &Build::get_touch_slot},
    {"touch_supported_slot", &Build::touch_supported_slot},
    {"frame_count_slot", &Build::frame_count_slot},
    {"screen_width_slot", &Build::screen_width_slot},
    {"screen_height_slot", &Build::screen_height_slot},
    {"ui_class_slot", &Build::ui_class_slot},
    {"static_reference_pool", &Build::static_reference_pool},
    {"ui_state_offset", &Build::ui_state_offset},
    {"get_layout_override", &Build::get_layout_override},
    {"set_layout_override", &Build::set_layout_override},
    {"get_effective_layout", &Build::get_effective_layout},
    {"class_initialized_offset", &Build::class_initialized_offset},
    {"override_property_offset", &Build::override_property_offset},
    {"default_property_offset", &Build::default_property_offset},
};
}
