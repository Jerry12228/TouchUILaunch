#pragma once
#include "ef_runtime.hpp"
#include <MinHook.h>
#include <atomic>

namespace touchui::ef {
// One process-lifetime EF installation. Only immutable ABI arguments and an
// atomic gate are read by the detour; managed calls stay in the UI-thread pump.
class ChangeHook {
    using Function=void(*)(int32_t,const void*);
    inline static Function original_{};
    inline static std::atomic_bool enabled_{};
    inline static int32_t touch_{};
    void* entry_{};
    bool initialized_{};
    static void detour(int32_t value,const void* method) {
        original_(enabled_.load(std::memory_order_acquire)?touch_:value,method);
    }
public:
    void install(const Contract& c) {
        require(!entry_&&!original_,"EF: duplicate input-switch hook installation");
        const auto init=MH_Initialize();
        require(init==MH_OK,"EF: MinHook initialization failed");initialized_=true;
        const auto created=MH_CreateHook(c.change_entry,reinterpret_cast<void*>(&detour),
            reinterpret_cast<void**>(&original_));
        if(created!=MH_OK) {
            MH_Uninitialize();initialized_=false;original_=nullptr;
            throw std::runtime_error("EF: cannot hook ChangeInputType");
        }
        // Install as a pass-through. Do not force early initialization calls
        // before our guarded runtime_invoke has successfully applied Touch.
        entry_=c.change_entry;touch_=c.touch;enabled_.store(false,std::memory_order_release);
        if(MH_EnableHook(entry_)!=MH_OK) {
            enabled_.store(false,std::memory_order_release);
            MH_RemoveHook(entry_);entry_=nullptr;original_=nullptr;
            MH_Uninitialize();initialized_=false;
            throw std::runtime_error("EF: cannot enable ChangeInputType hook");
        }
    }
    void activate()noexcept {enabled_.store(true,std::memory_order_release);}
    void deactivate()noexcept {enabled_.store(false,std::memory_order_release);}
    // Fixtures call this only after all callers have stopped. Production keeps
    // the DLL, original trampoline and callbacks resident until process exit.
    void remove_fixture() {
        deactivate();
        if(entry_) {
            require(MH_DisableHook(entry_)==MH_OK,"EF fixture: cannot disable input hook");
            require(MH_RemoveHook(entry_)==MH_OK,"EF fixture: cannot remove input hook");entry_=nullptr;
        }
        original_=nullptr;
        if(initialized_){require(MH_Uninitialize()==MH_OK,"EF fixture: cannot release MinHook");initialized_=false;}
    }
};
}
