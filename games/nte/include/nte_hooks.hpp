#pragma once
#include "nte_discovery.hpp"
#include "MinHook.h"
#include <sstream>
extern "C" {
#include "third_party/minhook/src/buffer.h"
}

namespace touchui::nte {
using Exec=void(*)(void*,void*,void*);
using Navigation=void(*)(void*,uint8_t);
inline constexpr uint8_t touch_input=2;
inline constexpr uint32_t return_true=0x90c301b0; // mov al,1; ret; nop
inline bool native_mobile()noexcept {return true;}
// One worker owns this pinned MinHook v1.3.4 pool before the game main thread
// resumes. Holding a single unused slot keeps a normal nearby page available
// for later hook preparation; no game address or instruction is patched here.
class EarlyHookBuffer {
    void* slot_{};
public:
    EarlyHookBuffer()=default;
    EarlyHookBuffer(const EarlyHookBuffer&)=delete;EarlyHookBuffer& operator=(const EarlyHookBuffer&)=delete;
    ~EarlyHookBuffer(){release();}
    bool reserve(void* origin) {
        if(slot_)return true;
        const auto status=MH_Initialize();
        if(status!=MH_OK&&status!=MH_ERROR_ALREADY_INITIALIZED)return false;
        slot_=AllocateBuffer(origin);return slot_!=nullptr;
    }
    void release(){if(slot_){FreeBuffer(slot_);slot_=nullptr;}}
    void* slot()const{return slot_;}
};
struct Callbacks {std::array<uintptr_t,3> exec{};uintptr_t navigation{};};
inline std::string detour_preparation_failure(MH_STATUS status,void* target,DWORD last_error) {
    // MinHook can fail either its nearby executable buffer or private heap.
    // Observe the address space without allocating, changing protections or
    // treating a captured last-error value as a proven allocation cause.
    const auto origin=reinterpret_cast<uintptr_t>(target);
    constexpr uintptr_t range=0x40000000;
    const auto low=origin>range?origin-range:uintptr_t{};
    const auto high=origin<=UINTPTR_MAX-range?origin+range:UINTPTR_MAX;
    size_t free_regions{},largest{},aligned_regions{};SYSTEM_INFO system{};GetSystemInfo(&system);
    for(auto cursor=low;cursor<high;) {
        MEMORY_BASIC_INFORMATION region{};
        if(VirtualQuery(reinterpret_cast<void*>(cursor),&region,sizeof(region))!=sizeof(region))break;
        const auto begin=reinterpret_cast<uintptr_t>(region.BaseAddress);
        if(region.RegionSize>UINTPTR_MAX-begin)break;
        const auto next=begin+region.RegionSize;
        if(next<=cursor)break;
        if(region.State==MEM_FREE) {
            ++free_regions;largest=std::max(largest,static_cast<size_t>(std::min(next,high)-cursor));
            const auto granularity=static_cast<uintptr_t>(system.dwAllocationGranularity);
            const auto aligned=(cursor+granularity-1)&~(granularity-1);
            if(aligned>=cursor&&aligned<std::min(next,high)&&std::min(next,high)-aligned>=0x1000)++aligned_regions;
        }
        cursor=next;
    }
    MEMORYSTATUSEX memory{sizeof(memory)};
    const bool have_memory=GlobalMemoryStatusEx(&memory)!=FALSE;
    std::ostringstream result;
    result<<"cannot prepare native predicate detour: "<<MH_StatusToString(status)
        <<"; target=0x"<<std::hex<<origin<<std::dec<<"; last_win32="<<last_error
        <<"; near_free_regions="<<free_regions<<"; near_largest_bytes="<<largest<<"; near_aligned_regions="<<aligned_regions;
    if(have_memory)result<<"; available_commit_MiB="<<(memory.ullAvailPageFile>>20);
    return result.str();
}
struct HookOperations {
    std::function<bool(void*,size_t,DWORD,DWORD*)> protect=[](void* at,size_t size,DWORD mode,DWORD* previous) {
        return VirtualProtect(at,size,mode,previous)!=FALSE;
    };
    std::function<bool(void*,size_t)> flush=[](void* at,size_t size) {
        return FlushInstructionCache(GetCurrentProcess(),at,size)!=FALSE;
    };
    std::function<MH_STATUS(void*)> enable_detour=[](void* at){return MH_EnableHook(at);};
    std::function<MH_STATUS(void*,void*)> create_detour=[](void* at,void* callback) {
        return MH_CreateHook(at,callback,nullptr);
    };
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
                SetLastError(ERROR_SUCCESS);
                const auto created=operations_.create_detour(at,reinterpret_cast<void*>(&native_mobile));
                const auto last_error=GetLastError();
                if(created!=MH_OK)throw std::runtime_error(detour_preparation_failure(created,at,last_error));
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
