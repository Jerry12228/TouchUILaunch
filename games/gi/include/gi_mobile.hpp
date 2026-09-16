#pragma once

#include "mobile_runtime.hpp"

namespace touchui::gi {

// GI owns this adapter. The shared remote-process layer supplies allocation,
// memory protection and loader primitives; this boundary keeps the launcher
// unaware of GI signatures and initialization order.
inline void initialize(game::Child& child, const std::filesystem::path& executable,
                       bool logging, const mobile::Progress& progress = {}) {
    mobile::initialize(child, game::Kind::GI, executable, logging, progress);
}

} // namespace touchui::gi
