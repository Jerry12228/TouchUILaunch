#pragma once
#include "x64_reader.hpp"
#include <array>
#include <string_view>

namespace touchui::gi {
inline void check(bool ok, const std::string& message) {
    if (!ok) throw std::runtime_error("GI: " + message);
}

enum class Hook { none, joystick };
struct Site {
    std::string name;
    uint32_t rva{};
    std::vector<uint8_t> expected;
    uint32_t offset{};
    std::vector<uint8_t> replacement;
    Hook hook{};
};
using Plan = std::vector<Site>;

// Like the retained GI resolver, use masked candidates followed by decoded
// targets/field agreement. No sample addresses or file fingerprints select a plan.
struct Rule {
    const char* name;
    std::vector<uint8_t> bytes, mask;
    Rule(const char* label, std::string_view text) : name(label) {
        const auto digit = [](char c) {
            check((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'), "invalid rule digit");
            return c <= '9' ? c - '0' : c - 'A' + 10;
        };
        while (!text.empty()) {
            if (text.front() == ' ') { text.remove_prefix(1); continue; }
            check(text.size() >= 2, "truncated rule");
            const bool any = text.substr(0, 2) == "??";
            bytes.push_back(any ? 0 : static_cast<uint8_t>(digit(text[0]) * 16 + digit(text[1])));
            mask.push_back(any ? 0 : 255);
            text.remove_prefix(2);
        }
    }
    binary::Pattern pattern() const { return {name, bytes, mask}; }
};

inline const std::array<Rule,11> rules{{
    {"initial UI mode", "41 56 56 57 53 48 83 EC 28 49 89 CE 80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 4C 89 F1 E8 ?? ?? ?? ?? 41 89 86 ?? ?? ?? ?? 4C 89 F1 E8 ?? ?? ?? ??"},
    {"silent UI setter", "48 83 EC 28 41 89 D0 48 89 CA 80 3D ?? ?? ?? ?? 00 75 ?? 44 39 82 ?? ?? ?? ?? 74 ?? 44 89 82 ?? ?? ?? ?? 48 83 C4 28 C3"},
    {"refreshing UI setter", "55 56 57 53 48 83 EC 58 48 8D 6C 24 ?? 48 C7 45 ?? FE FF FF FF 44 89 C3 41 89 D0 48 89 CE 80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 44 39 86 ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? 44 89 86 ?? ?? ?? ??"},
    {"UI property setter", "89 91 ?? ?? ?? ?? C3 66 0F 1F 84 00 ?? ?? 00 00"},
    {"initial touch input branch", "56 48 83 EC 30 48 89 CE 80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 F8 0B 7E ??"},
    {"input mode setter and refresh", "48 83 EC 28 41 89 D0 48 89 CA 80 3D ?? ?? ?? ?? 00 75 ?? 44 89 82 ?? ?? ?? ?? 48 89 D1 31 D2 48 83 C4 28 E9 ?? ?? ?? ??"},
    {"input mode transition", "56 57 53 48 83 EC 70 44 89 C3 89 D7 48 89 CE 80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 8B 86 ?? ?? ?? ?? 39 F8 75 ?? 48 83 C4 70 5B 5F 5E C3"},
    {"coordinated UI and input switch", "55 41 56 56 57 53 48 81 EC C0 00 00 00 48 8D AC 24 ?? ?? ?? ?? 0F 29 75 ?? 48 C7 45 ?? FE FF FF FF 45 89 C6 89 D3 48 89 CF 80 3D ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ??"},
    {"physical touch scale fallback", "E8 ?? ?? ?? ?? F3 0F 11 05 ?? ?? ?? ?? 0F 57 C9 0F 2E C8 72 ?? 48 8B 0D ?? ?? ?? ?? 80 B9 ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ?? C7 05 ?? ?? ?? ?? 00 00 B4 43"},
    {"touchscreen settings caption", "E8 ?? ?? ?? ?? 83 F8 0B 7E ?? 8D 48 ?? 83 F9 FD 73 ?? 83 F8 17 74 ?? 83 F8 23 74 ?? EB ?? 48 83 C4 28 5B 5F 5E 41 5E C3 83 F8 08 74 ?? 83 F8 0B 74 ?? EB ?? E8 ?? ?? ?? ?? 83 F8 01 74 ?? 48 8B 0D ?? ?? ?? ?? 80 B9 ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 0F 84 ?? ?? ?? ?? 48 8D 35 ?? ?? ?? ?? 48 8B 36"},
    {"joystick physical width", "F3 0F 10 05 ?? ?? ?? ?? F3 0F 11 86 ?? ?? ?? ?? 48 89 F1 E8 ?? ?? ?? ?? 48 85 C0 0F 84 ?? ?? ?? ?? 48 89 C1"},
}};

inline uint32_t field(const discovery::Image& image, uintptr_t at) {
    const auto ins = discovery::x64::decode(image, at);
    check(ins.base() >= 0 && ins.index() == discovery::x64::none,
          "expected an object field access");
    const auto value = ins.disp();
    check(value >= 16 && value < 0x10000 && value % 4 == 0, "invalid object field");
    return static_cast<uint32_t>(value);
}

inline void validate_window(const discovery::Image& image, uintptr_t at, size_t size) {
    discovery::x64::Cursor cursor(image, at, size);
    while (cursor.at < cursor.end) {
        const auto ins = cursor.take();
        if (ins.call() || ins.jump() || ins.conditional()) image.code(ins.relative(image));
        if (ins.base() == discovery::x64::rip) {
            const auto* section = image.section(ins.storage(image), 1);
            check(section && !section->code(), "instruction references code as data");
        }
    }
}

template<class Accept>
uintptr_t locate(const discovery::Image& image, const Rule& rule, Accept accept) {
    const auto candidates = image.find(rule.pattern(), 4096, 8);
    std::vector<uintptr_t> accepted;
    for (auto at : candidates) {
        if (!accept(at)) continue;
        validate_window(image, at, rule.bytes.size());
        accepted.push_back(at);
    }
    check(accepted.size() == 1, std::string(rule.name) + ": expected one validated match, got " + std::to_string(accepted.size()));
    return accepted.front();
}
inline uintptr_t locate(const discovery::Image& image, const Rule& rule) {
    return locate(image, rule, [](uintptr_t) { return true; });
}

inline void validate_sites(std::span<const Site> plan) {
    check(!plan.empty(), "empty patch plan");
    for (size_t i = 0; i < plan.size(); ++i) {
        const auto& site = plan[i];
        check(!site.expected.empty() && !site.replacement.empty() && site.offset <= site.expected.size() &&
              site.replacement.size() <= site.expected.size() - site.offset &&
              uint64_t(site.rva) + site.expected.size() <= UINT32_MAX, "invalid patch bounds");
        for (size_t j = 0; j < i; ++j)
            check(uint64_t(site.rva) >= uint64_t(plan[j].rva) + plan[j].expected.size() ||
                  uint64_t(plan[j].rva) >= uint64_t(site.rva) + site.expected.size(), "overlapping patch sites");
    }
}

inline Plan resolve(const discovery::Image& image) {
    Plan plan;
    const auto add = [&](size_t rule, uintptr_t at, uint32_t offset, std::vector<uint8_t> replacement, Hook hook = Hook::none) {
        auto bytes = image.view(at, rules[rule].bytes.size());
        plan.push_back({rules[rule].name, static_cast<uint32_t>(at), {bytes.begin(), bytes.end()}, offset, std::move(replacement), hook});
    };
    const auto ui_init = locate(image, rules[0]);
    const auto ui_field = field(image, ui_init + 33);
    const auto ui_refresh = locate(image, rules[2]);
    check(field(image, ui_refresh + 43) == ui_field && field(image, ui_refresh + 56) == ui_field,
          "UI initializer/refresh fields disagree");
    const auto ui_silent = locate(image, rules[1], [&](uintptr_t at) { return field(image, at + 19) == ui_field; });
    check(field(image, ui_silent + 28) == ui_field, "UI silent setter fields disagree");
    const auto ui_leaf = locate(image, rules[3], [&](uintptr_t at) { return field(image, at) == ui_field; });
    const auto input_init = locate(image, rules[4]);
    const auto transition = locate(image, rules[6]);
    const auto input_field = field(image, transition + 28);
    check(ui_field != input_field, "UI/input fields unexpectedly overlap");
    const auto input_setter = locate(image, rules[5], [&](uintptr_t at) { return field(image, at + 19) == input_field; });
    const auto coordinated = locate(image, rules[7]);
    const auto dpi = locate(image, rules[8]);
    const auto label = locate(image, rules[9]);
    // These are offsets inside matched instruction windows, never module RVAs.
    // Mask branch encodings for discovery, then verify the paths we will force.
    check(discovery::x64::decode(image, dpi + 19).relative(image) == dpi + 51,
          "DPI branch no longer skips the fallback store");
    check(discovery::x64::decode(image, label + 8).relative(image) == label + 40 &&
          discovery::x64::decode(image, label + 43).relative(image) == label + 95 &&
          discovery::x64::decode(image, label + 10).disp() == -21,
          "touchscreen caption branch changed");
    const std::vector<uint8_t> zero_result{0x31,0xc0,0x90,0x90,0x90};
    const std::vector<uint8_t> zero_argument{0x45,0x31,0xc0};
    add(0, ui_init, 28, zero_result);
    add(1, ui_silent, 4, zero_argument);
    add(2, ui_refresh, 24, zero_argument);
    std::vector<uint8_t> leaf{0x83,0xa1,0,0,0,0,0,0xc3};
    std::memcpy(leaf.data() + 2, &ui_field, 4);
    add(3, ui_leaf, 0, leaf);
    add(4, input_init, 21, zero_result);
    add(5, input_setter, 4, zero_argument);
    add(6, transition, 10, {0x31,0xff});
    add(7, coordinated, 33, {0x45,0x31,0xf6,0x31,0xdb});
    // Keep the established 360 baseline. Joystick size is changed independently.
    const auto dpi_store = discovery::x64::decode(image, dpi + 41);
    check(dpi_store.imm() == 0x43b40000 &&
          dpi_store.storage(image) == discovery::x64::decode(image, dpi + 5).storage(image),
          "DPI fallback and cached DPI do not agree");
    add(8, dpi, 19, {0x90,0x90});
    add(9, label, 0, {0xb8,0x08,0,0,0});

    const auto joystick = locate(image, rules[10]);
    (void)field(image, joystick + 8);
    add(10, joystick, 8, std::vector<uint8_t>(8,0x90), Hook::joystick);
    validate_sites(plan);
    return plan;
}

template<class Memory>
void validate_plan(Memory& memory, std::span<const Site> plan) {
    validate_sites(plan);
    for (const auto& site : plan) {
        const auto actual = memory.read(site.rva, site.expected.size());
        check(std::equal(actual.begin(), actual.end(), site.expected.begin(), site.expected.end()),
              "loaded instruction mismatch at " + site.name);
    }
}
template<class Memory>
void apply_plan(Memory& memory, std::span<const Site> plan) {
    validate_plan(memory, plan);
    for (const auto& site : plan)
        check(site.hook == Hook::none || site.replacement.front() == 0xe8, "joystick hook is not prepared");
    for (const auto& site : plan) {
        memory.write_code(site.rva + site.offset, site.replacement);
        const auto actual = memory.read(site.rva + site.offset, site.replacement.size());
        check(std::equal(actual.begin(), actual.end(), site.replacement.begin(), site.replacement.end()),
              "patch verification failed at " + site.name);
    }
}
}
