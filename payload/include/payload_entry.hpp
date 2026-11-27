#pragma once
#include "win_util.hpp"

namespace touchui::payload {
inline constexpr auto filename=L"TouchUILaunch.dll";
enum class Kind {Inactive,ZZZ,NTE};
// Treat conflicting or incomplete activation as invalid, never start both
// workers. NTE's own worker validates its session/role before doing any work.
inline Kind select(bool zzz_logging,bool nte_ready,bool nte_failed,bool nte_config) {
    const bool nte_any=nte_ready||nte_failed||nte_config;
    if(zzz_logging)return nte_any?Kind::Inactive:Kind::ZZZ;
    return nte_ready&&nte_failed?Kind::NTE:Kind::Inactive;
}
}
namespace touchui::zzz {DWORD run_payload(HMODULE module);}
namespace touchui::nte {DWORD run_payload(HMODULE module);}
