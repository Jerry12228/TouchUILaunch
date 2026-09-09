#pragma once
#include <cstdint>
namespace profile {
inline constexpr char sha256[]="4cba5d52c5fbfd478d2a9ec217075f82216780d56ad1bd1e85e4f724dcce30b4";
// RVAs derived from this exact GameAssembly.dll. See analysis/touch/injection.md.
inline constexpr uintptr_t touch_count_slot=0x540dcb8;
inline constexpr uintptr_t get_touch_slot=0x540dcd8;
inline constexpr uintptr_t touch_supported_slot=0x540dcc0;
inline constexpr uintptr_t frame_count_slot=0x540ad20;
inline constexpr uintptr_t screen_width_slot=0x5407458;
inline constexpr uintptr_t screen_height_slot=0x5407460;
inline constexpr uintptr_t ui_class_slot=0x5450eb0;
inline constexpr uintptr_t static_reference_pool=0x5360730;
inline constexpr uintptr_t ui_state_offset=186816;
inline constexpr uintptr_t get_layout_override=0x15b462a0;
inline constexpr uintptr_t set_layout_override=0x15b46680;
inline constexpr uintptr_t get_effective_layout=0x15b43570;
}
