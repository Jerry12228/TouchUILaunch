#pragma once

#include <cstdint>
#include <span>

namespace touchui::binary {

// A masked byte sequence. Game modules own their rule data; this common type
// only defines how PE readers consume a rule.
struct Pattern {
    const char* name;
    std::span<const uint8_t> bytes;
    std::span<const uint8_t> mask;
};

} // namespace touchui::binary
