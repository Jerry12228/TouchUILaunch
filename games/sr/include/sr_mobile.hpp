#pragma once

#include "mobile_runtime.hpp"

namespace touchui::sr {

// SR owns the state-setting behavior. The worker is installed before the
// suspended main thread resumes, so a failed setup only affects the child
// process created by this launcher invocation.
inline void initialize(game::Child& child, const std::filesystem::path& executable,
                       bool logging, const mobile::Progress& progress = {}) {
    mobile::initialize(child, game::Kind::SR, executable, logging, progress);
}

} // namespace touchui::sr
