#pragma once
#include "button_layout.hpp"
#include <windows.h>

namespace touchui::zzz {
// The policy permits fault injection against owned fixtures. Production uses
// only these Windows calls; no process suspension, forced store or disk write.
struct ButtonLayoutMemory {
    static bool query(uintptr_t address,MEMORY_BASIC_INFORMATION& region) {
        return VirtualQuery(reinterpret_cast<const void*>(address),&region,sizeof(region))==sizeof(region);
    }
    static bool read(uintptr_t address,std::span<uint8_t> bytes) {
        SIZE_T count{};
        return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),bytes.data(),bytes.size(),&count) && count==bytes.size();
    }
    static bool protect(uintptr_t address,DWORD desired,DWORD& old) {
        return VirtualProtect(reinterpret_cast<void*>(address),8,desired,&old)!=FALSE;
    }
    static bool flush(uintptr_t address) {
        return FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<const void*>(address),8)!=FALSE;
    }
    static bool exchange(uintptr_t address,uint64_t expected,uint64_t desired,uint64_t& previous) {
        // A page can become inaccessible after VirtualProtect. Handle only
        // memory faults; never force access, suspend threads or use a raw store.
        __try {
            previous=static_cast<uint64_t>(InterlockedCompareExchange64(reinterpret_cast<volatile LONG64*>(address),
                static_cast<LONG64>(desired),static_cast<LONG64>(expected)));
            return true;
        } __except((GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION || GetExceptionCode()==EXCEPTION_IN_PAGE_ERROR)?
            EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {return false;}
    }
};
template<class Memory=ButtonLayoutMemory> class BasicButtonLayoutPatch {
    struct State {
        bool owned{},protection_pending{},flush_pending{},abandoned{};
        DWORD protection{};
    };
    discovery::button_layout::Plan plan_{};
    uintptr_t base_{};
    std::array<State,2> states_{};
    bool conflict_{};
    static bool executable(DWORD protection) {
        return !(protection&(PAGE_GUARD|PAGE_NOACCESS)) &&
            (protection==PAGE_EXECUTE_READ || protection==PAGE_EXECUTE_READWRITE || protection==PAGE_EXECUTE_WRITECOPY);
    }
    static bool region(uintptr_t base,uintptr_t rva,size_t size,MEMORY_BASIC_INFORMATION& result) {
        if(!base || !size || rva>UINTPTR_MAX-base || size>UINTPTR_MAX-(base+rva))return false;
        if(!Memory::query(base+rva,result) || result.AllocationBase!=reinterpret_cast<void*>(base) || result.State!=MEM_COMMIT ||
            !executable(result.Protect))return false;
        const auto start=reinterpret_cast<uintptr_t>(result.BaseAddress);
        return base+rva>=start && base+rva-start<result.RegionSize && size<=result.RegionSize-(base+rva-start);
    }
    static bool read(uintptr_t base,uintptr_t rva,std::span<uint8_t> bytes) {
        if(!base || rva>UINTPTR_MAX-base || bytes.size()>UINTPTR_MAX-(base+rva))return false;
        size_t done{};
        while(done<bytes.size()) {
            MEMORY_BASIC_INFORMATION part{};
            if(!region(base,rva+done,1,part))return false;
            const auto available=part.RegionSize-(base+rva+done-reinterpret_cast<uintptr_t>(part.BaseAddress));
            const auto length=std::min(available,bytes.size()-done);
            if(!Memory::read(base+rva+done,bytes.subspan(done,length)))return false;
            done+=length;
        }
        return true;
    }
    static bool word(uintptr_t base,uintptr_t rva,uint64_t& value) {
        return rva%8==0 && read(base,rva,{reinterpret_cast<uint8_t*>(&value),8});
    }
    bool finish(size_t index) {
        auto& state=states_[index];const auto at=base_+plan_.sites[index].word;
        if(state.flush_pending && Memory::flush(at))state.flush_pending=false;
        if(state.protection_pending) {
            MEMORY_BASIC_INFORMATION current{};
            // Only return the temporary RWX protection we installed. A later
            // owner's protection must never be forced back to our saved value.
            if(Memory::query(at,current) && current.AllocationBase==reinterpret_cast<void*>(base_) && current.State==MEM_COMMIT) {
                if(current.Protect==state.protection)state.protection_pending=false;
                else if(current.Protect==PAGE_EXECUTE_READWRITE) {
                    DWORD old{};
                    if(Memory::protect(at,state.protection,old))state.protection_pending=false;
                } else {state.protection_pending=false;state.owned=false;state.abandoned=true;conflict_=true;}
            }
        }
        return !state.flush_pending && !state.protection_pending;
    }
    bool exchange(size_t index,bool installing) {
        auto& state=states_[index];const auto& site=plan_.sites[index];
        if(!finish(index) || state.abandoned)return false;
        const auto expected=installing?site.expected:site.replacement;
        const auto desired=installing?site.replacement:site.expected;
        uint64_t value{};MEMORY_BASIC_INFORMATION current{};
        if(!word(base_,site.word,value))return false;
        if(value!=expected){state.owned=false;conflict_=true;return false;}
        if(!region(base_,site.word,8,current))return false;
        if(!installing && current.Protect!=state.protection) {
            state.owned=false;state.abandoned=true;conflict_=true;return false;
        }
        // Do not reopen pages after a competing writer revoked executability.
        DWORD old{};
        if(!Memory::protect(base_+site.word,PAGE_EXECUTE_READWRITE,old))return false;
        state.protection=old;state.protection_pending=true;
        if(!executable(old) || old!=current.Protect){finish(index);return false;}
        uint64_t previous{};
        const bool accessible=Memory::exchange(base_+site.word,expected,desired,previous);
        const bool changed=accessible && previous==expected;
        if(changed){state.owned=installing;state.flush_pending=true;}
        else if(accessible){state.owned=false;conflict_=true;}
        const bool complete=finish(index);
        return changed && complete;
    }
    static bool valid(const discovery::button_layout::Plan& plan) {
        if(plan.sites[0].word==plan.sites[1].word || plan.evidence.empty() || plan.evidence.size()>16)return false;
        for(const auto& site:plan.sites) {
            if(!site.gate || !site.apply || site.gate>=site.compare || site.compare-site.gate>1024 ||
                site.word!=(site.compare&~uintptr_t{7}) || site.compare-site.word>5)return false;
            std::array<uint8_t,8> original{},patched{};
            std::memcpy(original.data(),&site.expected,8);std::memcpy(patched.data(),&site.replacement,8);
            const auto offset=site.compare-site.word;
            if(original[offset]!=0x83 || original[offset+1]!=0xf8 || original[offset+2]!=1)return false;
            original[offset]=0x39;original[offset+1]=0xc0;original[offset+2]=0x90;
            if(original!=patched)return false;
            bool covered{};
            for(const auto& evidence:plan.evidence)if(evidence.rva<=site.word && site.word-evidence.rva<=evidence.bytes.size() &&
                8<=evidence.bytes.size()-(site.word-evidence.rva)) {
                uint64_t expected{};std::memcpy(&expected,evidence.bytes.data()+site.word-evidence.rva,8);
                if(expected==site.expected)covered=true;
            }
            if(!covered)return false;
        }
        return true;
    }
public:
    BasicButtonLayoutPatch()=default;
    BasicButtonLayoutPatch(const BasicButtonLayoutPatch&)=delete;
    BasicButtonLayoutPatch& operator=(const BasicButtonLayoutPatch&)=delete;
    ~BasicButtonLayoutPatch(){restore();}
    bool has_pending()const {
        for(const auto& state:states_)if(state.owned || state.protection_pending || state.flush_pending)return true;
        return false;
    }
    bool installed()const {
        for(const auto& state:states_)if(!state.owned || state.protection_pending || state.flush_pending)return false;
        return true;
    }
    bool intact()const {
        if(!installed())return false;
        for(size_t i=0;i<states_.size();++i) {
            const auto& site=plan_.sites[i];uint64_t value{};MEMORY_BASIC_INFORMATION current{};
            if(!region(base_,site.word,8,current) || current.Protect!=states_[i].protection ||
                !word(base_,site.word,value) || value!=site.replacement)return false;
        }
        return true;
    }
    bool install(uintptr_t base,const discovery::button_layout::Plan& plan) {
        if(has_pending() || !valid(plan))return false;
        // Validate all semantic evidence before the first mutation. The only
        // later writes remain conditional exchanges of the two exact words.
        for(const auto& evidence:plan.evidence) {
            if(evidence.bytes.empty() || evidence.bytes.size()>1024)return false;
            std::vector<uint8_t> live(evidence.bytes.size());
            if(!read(base,evidence.rva,live) || live!=evidence.bytes)return false;
        }
        for(const auto& site:plan.sites){uint64_t value{};if(!word(base,site.word,value) || value!=site.expected)return false;}
        base_=base;plan_=plan;states_={};conflict_=false;
        for(size_t i=0;i<states_.size();++i)if(!exchange(i,true)){restore();return false;}
        return true;
    }
    bool restore() {
        for(size_t i=states_.size();i-->0;) {
            if(states_[i].owned)exchange(i,false);
            else finish(i);
        }
        return !has_pending() && !conflict_;
    }
};
using ButtonLayoutPatch=BasicButtonLayoutPatch<>;
}
