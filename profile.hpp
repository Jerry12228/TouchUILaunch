#pragma once
#include <cstdint>
namespace profile {
inline constexpr char sha256[]="4cba5d52c5fbfd478d2a9ec217075f82216780d56ad1bd1e85e4f724dcce30b4";
// GameAssembly RVAs and structure offsets are intentionally omitted.
inline constexpr uintptr_t touch_count_slot=0; // TODO: supply the target build value.
inline constexpr uintptr_t get_touch_slot=0; // TODO: supply the target build value.
inline constexpr uintptr_t touch_supported_slot=0; // TODO: supply the target build value.
inline constexpr uintptr_t frame_count_slot=0; // TODO: supply the target build value.
inline constexpr uintptr_t screen_width_slot=0; // TODO: supply the target build value.
inline constexpr uintptr_t screen_height_slot=0; // TODO: supply the target build value.
inline constexpr uintptr_t ui_class_slot=0; // TODO: supply the target build value.
inline constexpr uintptr_t static_reference_pool=0; // TODO: supply the target build value.
inline constexpr uintptr_t ui_state_offset=0; // TODO: supply the target build value.
inline constexpr uintptr_t get_layout_override=0; // TODO: supply the target build value.
inline constexpr uintptr_t set_layout_override=0; // TODO: supply the target build value.
inline constexpr uintptr_t get_effective_layout=0; // TODO: supply the target build value.
inline constexpr uintptr_t class_initialized_offset=0; // TODO: supply the target build value.
inline constexpr uintptr_t override_property_offset=0; // TODO: supply the target build value.
inline constexpr uintptr_t default_property_offset=0; // TODO: supply the target build value.
inline constexpr bool configured() {
    return touch_count_slot &&
           get_touch_slot &&
           touch_supported_slot &&
           frame_count_slot &&
           screen_width_slot &&
           screen_height_slot &&
           ui_class_slot &&
           static_reference_pool &&
           ui_state_offset &&
           get_layout_override &&
           set_layout_override &&
           get_effective_layout &&
           class_initialized_offset &&
           override_property_offset &&
           default_property_offset;
}
}
