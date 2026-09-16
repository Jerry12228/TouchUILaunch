#pragma once

#include <string_view>

namespace touchui::ww {

// These are game arguments rather than launcher options. Keep them owned by
// the WW module so the generic launcher never needs game-specific literals.
inline constexpr std::wstring_view cloud_arguments =
    L"-CloudGame -CloudGamePlatform=Android";

} // namespace touchui::ww
