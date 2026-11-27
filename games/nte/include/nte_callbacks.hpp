#pragma once
#include "nte_hooks.hpp"
#include <atomic>

namespace touchui::nte {
inline std::array<std::atomic<Exec>,3> original_exec{};
inline std::atomic<Navigation> original_navigation{};
template<size_t Index> void mobile_exec(void* context,void* frame,void* result) {
    // Preserve FFrame consumption and any original wrapper bookkeeping.
    original_exec[Index].load(std::memory_order_acquire)(context,frame,result);
    if(result)*static_cast<uint8_t*>(result)=1;
}
inline void navigation(void* context,uint8_t type) {
    (void)type;
    original_navigation.load(std::memory_order_acquire)(context,touch_input);
}
inline Callbacks prepare_callbacks(uintptr_t base,const Plan& plan) {
    const std::array<const Function*,3> functions={&plan.cloud,&plan.util_mobile,&plan.navigation_mobile};
    for(size_t i=0;i<functions.size();++i)original_exec[i].store(reinterpret_cast<Exec>(base+functions[i]->exec),std::memory_order_release);
    original_navigation.store(reinterpret_cast<Navigation>(base+plan.navigation_original),std::memory_order_release);
    return {{reinterpret_cast<uintptr_t>(&mobile_exec<0>),reinterpret_cast<uintptr_t>(&mobile_exec<1>),
        reinterpret_cast<uintptr_t>(&mobile_exec<2>)},reinterpret_cast<uintptr_t>(&navigation)};
}
}
