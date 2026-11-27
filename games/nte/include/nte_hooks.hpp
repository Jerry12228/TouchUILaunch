#pragma once
#include "nte_discovery.hpp"
#include "MinHook.h"

namespace touchui::nte {
using Exec=void(*)(void*,void*,void*);
using Navigation=void(*)(void*,uint8_t);
inline constexpr uint8_t touch_input=2;
inline constexpr uint32_t return_true=0x90c301b0; // mov al,1; ret; nop
inline bool native_mobile()noexcept {return true;}
struct Callbacks {std::array<uintptr_t,3> exec{};uintptr_t navigation{};};
struct HookOperations {
    std::function<bool(void*,size_t,DWORD,DWORD*)> protect=[](void* at,size_t size,DWORD mode,DWORD* previous) {
        return VirtualProtect(at,size,mode,previous)!=FALSE;
    };
    std::function<bool(void*,size_t)> flush=[](void* at,size_t size) {
        return FlushInstructionCache(GetCurrentProcess(),at,size)!=FALSE;
    };
    std::function<MH_STATUS(void*)> enable_detour=[](void* at){return MH_EnableHook(at);};
};
// Process-lifetime hooks. In particular, never restore the four-byte entry
// after publication: a concurrent caller can be at the replacement RET (+2).
// The launcher retains ownership of its child until installation succeeds.
class Hooks {
    struct Pointer {uintptr_t address{},original{},replacement{};bool published{};};
    std::array<Pointer,4> pointers_{};
    HookOperations operations_;
    bool attempted_{};
    void exchange(Pointer& p,bool restore) {
        auto* at=reinterpret_cast<void*>(p.address);DWORD old{};
        require(operations_.protect(at,8,PAGE_READWRITE,&old),"cannot protect callback slot");
        const auto expected=restore?p.replacement:p.original;
        const auto desired=restore?p.original:p.replacement;
        const auto previous=reinterpret_cast<uintptr_t>(InterlockedCompareExchangePointer(
            reinterpret_cast<void* volatile*>(at),reinterpret_cast<void*>(desired),reinterpret_cast<void*>(expected)));
        if(previous==expected)p.published=!restore;
        DWORD ignored{};const bool protected_again=operations_.protect(at,8,old,&ignored);
        require(protected_again,"cannot restore callback page protection");
        if(!restore)require(previous==expected,"callback slot changed before installation");
    }
public:
    bool code_published{};
    explicit Hooks(HookOperations operations={}):operations_(std::move(operations)){}
    // Used only before native-code publication (or by quiescent test fixtures).
    // A competing writer is never overwritten, even on partial failure.
    bool rollback_pointers()noexcept {
        bool ok=true;
        for(auto i=pointers_.rbegin();i!=pointers_.rend();++i)if(i->published) {
            try {exchange(*i,true);}catch(const std::exception&){ok=false;}
        }
        return ok;
    }
    void install(const ModuleImage& module,const Plan& plan,const Callbacks& callbacks) {
        require(!attempted_,"hook installation may only be attempted once");attempted_=true;
        const auto base=module.base;
        const std::array<const Function*,3> functions={&plan.cloud,&plan.util_mobile,&plan.navigation_mobile};
        const auto unchanged=[&](uintptr_t rva,size_t size) {
            std::vector<uint8_t> now(size);
            require(read_memory(base+rva,now)&&std::equal(now.begin(),now.end(),module.image().view(rva,size).begin()),"live code changed after discovery");
        };
        unchanged(plan.mobile_predicate,16);unchanged(plan.navigation_original,16);
        for(size_t i=0;i<functions.size();++i) {
            const auto& fn=*functions[i];uintptr_t object{},owner{},outer{},callback{};
            uint32_t flags{};uint8_t count{};uint16_t size{},result{};
            require(fn.object&&fn.object%8==0&&fn.owner&&read_value(base+fn.cache,object)&&object==fn.object&&
                read_value(base+fn.class_cache,owner)&&owner==fn.owner&&read_value(object+0x20,outer)&&outer==owner&&
                read_value(object+0xd8,callback)&&callback==base+fn.exec&&read_value(object+0xb0,flags)&&
                read_value(object+0xb4,count)&&read_value(object+0xb6,size)&&read_value(object+0xb8,result)&&
                (flags&0x400)&&count==1&&size==1&&result==0,"live reflection contract changed");
            require(executable_pointer(reinterpret_cast<void*>(callbacks.exec[i])),"invalid bool callback");
            unchanged(fn.exec,16);
            pointers_[i]={object+0xd8,callback,callbacks.exec[i],false};
        }
        require(executable_pointer(reinterpret_cast<void*>(callbacks.navigation)),"invalid navigation callback");
        require((base+plan.navigation_slot)%8==0&&
            (plan.native_patch!=NativePatch::AtomicStack||(base+plan.mobile_predicate)%4==0),"unaligned hook target");
        pointers_[3]={base+plan.navigation_slot,base+plan.navigation_original,callbacks.navigation,false};
        auto* at=reinterpret_cast<void*>(base+plan.mobile_predicate);
        bool detour_prepared{},detour_attempted{};MEMORY_BASIC_INFORMATION native_page{};
        try {
            if(plan.native_patch==NativePatch::SavedRegisterDetour) {
                require(VirtualQuery(at,&native_page,sizeof(native_page))==sizeof(native_page),"cannot query native bool page");
                const auto initialized=MH_Initialize();
                require(initialized==MH_OK||initialized==MH_ERROR_ALREADY_INITIALIZED,"cannot initialize native predicate detour");
                const auto created=MH_CreateHook(at,reinterpret_cast<void*>(&native_mobile),nullptr);
                require(created==MH_OK,std::string("cannot prepare native predicate detour: ")+MH_StatusToString(created));
                detour_prepared=true;
            }
            for(auto& p:pointers_)exchange(p,false);
            if(plan.native_patch==NativePatch::SavedRegisterDetour) {
                unchanged(plan.mobile_predicate,16);
                // MinHook relocates complete instructions and adjusts suspended
                // callers' RIPs. A two-byte PUSH cannot use our four-byte CAS.
                detour_attempted=true;
                const auto enabled=operations_.enable_detour(at);
                require(enabled==MH_OK,std::string("cannot enable native predicate detour: ")+MH_StatusToString(enabled));
                code_published=true;
                MEMORY_BASIC_INFORMATION after{};
                const bool restored=VirtualQuery(at,&after,sizeof(after))==sizeof(after)&&after.Protect==native_page.Protect;
                const bool flushed=operations_.flush(at,16);
                require(restored&&flushed,"native detour published but cache/protection restoration failed; owned child must exit");
                return; // The pinned callback and MinHook trampoline live until process exit.
            }
            DWORD old{};
            require(operations_.protect(at,4,PAGE_EXECUTE_READWRITE,&old),"cannot protect native bool entry");
            const auto previous=static_cast<uint32_t>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(at),
                static_cast<LONG>(return_true),static_cast<LONG>(plan.predicate_original)));
            code_published=previous==plan.predicate_original;
            const bool flushed=!code_published||operations_.flush(at,4);
            DWORD ignored{};const bool protected_again=operations_.protect(at,4,old,&ignored);
            require(code_published,"native bool entry changed before publication");
            require(flushed&&protected_again,"native entry published but cache/protection restoration failed; owned child must exit");
        } catch(...) {
            if(!code_published)rollback_pointers();
            // EnableHook can relocate a caller before reporting a protection
            // failure. Never free that trampoline while such a caller may run.
            if(detour_prepared&&!detour_attempted)MH_RemoveHook(at);
            throw;
        }
    }
};
}
