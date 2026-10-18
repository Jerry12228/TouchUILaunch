#pragma once
#include "button_layout.hpp"
#include <windows.h>

namespace touchui::zzz {
class ButtonLayoutPatch {
    discovery::button_layout::Plan plan_{};
    uintptr_t base_{};
    std::array<bool,2> owned_{};
    static bool readable_word(uintptr_t base,uintptr_t rva,uint64_t& value) {
        if(!base || rva>UINTPTR_MAX-base || base+rva>UINTPTR_MAX-8 || (base+rva)%8)return false;
        MEMORY_BASIC_INFORMATION region{};
        const auto* address=reinterpret_cast<const void*>(base+rva);
        if(!VirtualQuery(address,&region,sizeof(region)) || region.AllocationBase!=reinterpret_cast<void*>(base) ||
            region.State!=MEM_COMMIT || (region.Protect&(PAGE_GUARD|PAGE_NOACCESS)) ||
            !(region.Protect&(PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))return false;
        SIZE_T count{};
        return ReadProcessMemory(GetCurrentProcess(),address,&value,8,&count) && count==8;
    }
    static bool exchange(uintptr_t base,const discovery::button_layout::Site& site,uint64_t expected,uint64_t desired,bool& changed) {
        changed=false;uint64_t value{};
        if(!readable_word(base,site.word,value) || value!=expected)return false;
        auto* address=reinterpret_cast<volatile LONG64*>(base+site.word);
        DWORD old{};
        if(!VirtualProtect(const_cast<LONG64*>(address),8,PAGE_EXECUTE_READWRITE,&old))return false;
        changed=static_cast<uint64_t>(InterlockedCompareExchange64(address,static_cast<LONG64>(desired),static_cast<LONG64>(expected)))==expected;
        const bool flushed=!changed || FlushInstructionCache(GetCurrentProcess(),const_cast<LONG64*>(address),8)!=FALSE;
        DWORD ignored{};const bool restored=VirtualProtect(const_cast<LONG64*>(address),8,old,&ignored)!=FALSE;
        return changed && flushed && restored;
    }
public:
    ButtonLayoutPatch()=default;
    ButtonLayoutPatch(const ButtonLayoutPatch&)=delete;
    ButtonLayoutPatch& operator=(const ButtonLayoutPatch&)=delete;
    ~ButtonLayoutPatch(){restore();}
    bool installed()const{return owned_[0] && owned_[1];}
    bool install(uintptr_t base,const discovery::button_layout::Plan& plan) {
        if(owned_[0] || owned_[1])return false;
        if(plan.sites[0].word==plan.sites[1].word)return false;
        // Validate both words before the first mutation. Compare/exchange also
        // revalidates each word atomically at the point of mutation.
        for(const auto& site:plan.sites) {
            uint64_t value{};
            if(!readable_word(base,site.word,value) || value!=site.expected)return false;
        }
        base_=base;plan_=plan;
        for(size_t i=0;i<owned_.size();++i) {
            const auto& site=plan_.sites[i];bool changed{};
            const bool ok=exchange(base_,site,site.expected,site.replacement,changed);owned_[i]=changed;
            if(!ok){restore();return false;}
        }
        return true;
    }
    bool restore() {
        bool ok=true;
        for(size_t i=owned_.size();i-->0;)if(owned_[i]) {
            const auto& site=plan_.sites[i];bool changed{};
            ok=exchange(base_,site,site.replacement,site.expected,changed) && ok;
            // A competing writer invalidates ownership. Never overwrite it or
            // attempt a later restoration after its bytes happen to change back.
            owned_[i]=false;
        }
        return ok;
    }
};
}
