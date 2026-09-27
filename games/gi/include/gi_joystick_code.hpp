#pragma once
#include "gi_touch_plan.hpp"

namespace touchui::gi {
inline constexpr float joystick_width_scale = 1.0f; // Preserve the 0.2.0 diameter and useful travel.

// The joystick leaf preserves RSP, flags and nonvolatile registers. It only
// copies the joystick width; gesture producers are never redirected here.
inline std::vector<uint8_t> joystick_code(const Site& site) {
    check(site.hook == Hook::joystick, "not a joystick code site");
    const auto original = std::span(site.expected).subspan(site.offset, site.replacement.size());
    std::vector<uint8_t> code;
    auto emit = [&](std::initializer_list<uint8_t> bytes) { code.insert(code.end(), bytes); };
    auto copy = [&](std::span<const uint8_t> bytes) { code.insert(code.end(), bytes.begin(), bytes.end()); };
    emit({0xf3,0x0f,0x59,0x05}); // mulss xmm0,[rip+factor]
    const auto relative = code.size(); emit({0,0,0,0});
    copy(original); // movss [rsi+decoded width field],xmm0
    emit({0xc3});
    const float factor = joystick_width_scale;
    // Keep the scalar constant pool aligned and adjust its RIP target.
    while (code.size() % 16) code.push_back(0);
    const auto aligned_displacement = static_cast<int32_t>(code.size() - relative - 4);
    std::memcpy(code.data() + relative, &aligned_displacement, 4);
    const auto* bytes = reinterpret_cast<const uint8_t*>(&factor);
    code.insert(code.end(), bytes, bytes + sizeof(factor));
    return code;
}

inline std::vector<uint8_t> call_patch(uintptr_t source, uintptr_t target, size_t size) {
    check(size >= 5, "call replacement is too short");
    const auto delta = static_cast<int64_t>(target) - static_cast<int64_t>(source) - 5;
    check(delta >= INT32_MIN && delta <= INT32_MAX, "joystick helper is outside rel32 range");
    std::vector<uint8_t> result(size, 0x90); result[0] = 0xe8;
    const auto relative = static_cast<int32_t>(delta);
    std::memcpy(result.data() + 1, &relative, 4);
    return result;
}
}
