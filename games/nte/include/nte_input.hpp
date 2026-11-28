#pragma once
#include "nte_discovery.hpp"
#include <atomic>

namespace touchui::nte::input {
struct Plan {uintptr_t singleton{},vtable{},duplicate_cvar{};int32_t fallback_offset{};};
inline uintptr_t one(const std::set<uintptr_t>& values,const char* reason) {
    require(values.size()==1,reason);return *values.begin();
}
inline bool writable(const ModuleImage& module,uintptr_t at,size_t size) {
    const auto* section=module.image().section(at,size);
    return section&&!section->code()&&(section->flags&IMAGE_SCN_MEM_WRITE)&&module.data(at,size);
}
// Decode from an unwind-validated start to a specific reference. Unlike the
// short leaf helper, constructors may contain branches before the reference.
inline std::vector<dx::Ins> prefix(const ModuleImage& module,uintptr_t reference,size_t after=64) {
    auto at=module.function_start(reference);
    require(reference-at<=0x4000&&after<=512,"input reference exceeds bounded function prefix");
    std::vector<dx::Ins> result;bool found{};
    while(at<reference+after) {
        dx::Ins ins;
        try {ins=dx::decode(module.image(),at,true);}
        catch(const std::exception& error){throw std::runtime_error("NTE: input instruction RVA="+std::to_string(at)+": "+error.what());}
        found|=at==reference;result.push_back(ins);at=ins.next();
    }
    require(found,"input reference is not an instruction boundary");return result;
}
inline uintptr_t string(const ModuleImage& module,std::wstring_view text) {
    std::vector<uint8_t> wanted((text.size()+1)*2);
    std::memcpy(wanted.data(),text.data(),text.size()*2);std::set<uintptr_t> matches;
    for(const auto& section:module.image().sections)if(!section.code()&&(section.flags&IMAGE_SCN_MEM_READ)) {
        const auto bytes=module.image().view(section.rva,section.size);auto next=bytes.begin();
        while(next!=bytes.end()) {
            const auto match=std::search(next,bytes.end(),wanted.begin(),wanted.end());if(match==bytes.end())break;
            matches.insert(section.rva+static_cast<uintptr_t>(match-bytes.begin()));next=match+1;
        }
    }
    return one(matches,"input configuration name is missing or ambiguous");
}
inline std::vector<uintptr_t> references(const ModuleImage& module,uintptr_t target,bool call=false) {
    std::set<uintptr_t> matches;const auto& image=module.image();
    for(const auto& section:image.sections)if(section.code()) {
        const auto bytes=image.view(section.rva,section.size);
        for(size_t p=0;p+7<=bytes.size();++p) {
            const bool candidate=call?bytes[p]==0xe8:
                (bytes[p]&0xf8)==0x48&&bytes[p+1]==0x8d&&(bytes[p+2]&0xc7)==5;
            if(!candidate)continue;
            const auto length=call?5u:7u;int32_t displacement{};
            std::memcpy(&displacement,bytes.data()+p+length-4,4);
            if(int64_t(section.rva+p+length)+displacement!=int64_t(target))continue;
            try {
                const auto at=section.rva+p;const auto code=prefix(module,at,1);
                const auto ins=std::find_if(code.begin(),code.end(),[&](const auto& i){return i.at==at;});
                if(ins!=code.end()&&(call?ins->call():ins->h.opcode==0x8d&&ins->base()==dx::rip))matches.insert(at);
            } catch(const std::exception&) {}
        }
    }
    return {matches.begin(),matches.end()};
}
inline Plan discover(const ModuleImage& module) {
    const auto key=string(module,L"bTouchFallbackToMouse"),section_name=string(module,L"MobileSlateUI");
    const auto key_refs=references(module,key);require(key_refs.size()==1,"Slate fallback constructor is ambiguous");
    const auto code=prefix(module,key_refs.front());const auto& image=module.image();
    size_t index{};while(index<code.size()&&code[index].at!=key_refs.front())++index;
    require(index>=1&&index+3<code.size()&&code[index].reg()==8,"invalid Slate fallback key argument");
    size_t field_index=index;
    for(size_t n=1;n<=4&&n<=index;++n) {
        const auto& before=code[index-n];
        if(before.h.opcode==0x8d&&before.reg()==9){field_index=index-n;break;}
        require(before.mov_store()&&before.memory(4,0x20),"unvalidated instruction before Slate fallback key");
    }
    require(field_index<index,"Slate fallback member argument is missing");const auto& field=code[field_index];
    require(field.base()>=0&&field.base()!=4&&field.index()==dx::none&&
        field.disp()>0&&field.disp()<0x4000,"invalid Slate fallback member argument");
    require(code[index+1].h.opcode==0x8d&&code[index+1].reg()==2&&code[index+1].base()==dx::rip&&
        code[index+1].storage(image)==section_name&&code[index+2].call(),"Slate fallback config read is not validated");
    const auto owner=field.base();bool initialized{};std::set<uintptr_t> vtables;
    for(size_t p=0;p<index;++p) {
        const auto& ins=code[p];
        if(ins.h.opcode==0xc7&&ins.h.modrm_reg==0&&ins.base()==owner&&ins.disp()==field.disp()&&ins.imm()==1)
            initialized=true;
        if(ins.mov_store()&&ins.base()==owner&&ins.disp()==0&&ins.index()==dx::none) {
            for(size_t n=p;n>0&&p-n<5;--n) {
                const auto& source=code[n-1];
                if(source.h.opcode==0x8d&&source.reg()==ins.reg()&&source.base()==dx::rip) {
                    const auto at=source.storage(image);
                    if(module.data(at,16)&&!(image.section(at,16)->flags&IMAGE_SCN_MEM_WRITE)) {
                        const auto first=image.read<uintptr_t>(at);
                        if(first>=module.base&&module.code(first-module.base))vtables.insert(at);
                    }
                    break;
                }
            }
        }
    }
    require(initialized,"Slate fallback default is not validated");
    Plan plan;plan.fallback_offset=field.disp();plan.vtable=one(vtables,"Slate primary vtable is ambiguous");
    const auto constructor=module.function_start(key_refs.front());const auto calls=references(module,constructor,true);
    require(calls.size()==1,"Slate singleton factory is ambiguous");
    const auto factory=prefix(module,calls.front(),512);size_t call_index{};
    while(call_index<factory.size()&&factory[call_index].at!=calls.front())++call_index;
    require(call_index+1<factory.size(),"Slate factory result is missing");
    const auto& saved=factory[call_index+1];int saved_register=-1;
    for(const int reg:{3,6,7,12,13,14,15})if(saved.mov(reg,0,true))saved_register=reg;
    require(saved_register>=0,"Slate factory does not preserve its constructor result");
    std::set<uintptr_t> singletons;
    for(size_t p=call_index+2;p<std::min(factory.size(),call_index+39);++p) {
        const auto& ins=factory[p];
        if(ins.mov_store()&&ins.reg()==saved_register&&ins.base()==dx::rip) {
            const auto at=ins.storage(image);if(at%8==0&&writable(module,at,8))singletons.insert(at);
        }
    }
    plan.singleton=one(singletons,"Slate singleton storage is ambiguous");
    const auto duplicate_name=string(module,L"Slate.PreventDuplicateMouseEventsForTouchForWindows7");
    const auto duplicate_refs=references(module,duplicate_name);
    require(duplicate_refs.size()==1,"duplicate-mouse CVar registration is ambiguous");
    const auto registration=prefix(module,duplicate_refs.front());std::set<uintptr_t> storages;bool name_argument{},register_call{};
    for(const auto& ins:registration) {
        if(ins.h.opcode==0x8d&&ins.base()==dx::rip) {
            const auto at=ins.storage(image);
            if(ins.reg()==2&&at==duplicate_name)name_argument=true;
            if(ins.reg()==8&&at%4==0&&writable(module,at,4))storages.insert(at);
        }
        if(ins.indirect_call()&&ins.memory(0,0x40))register_call=true;
    }
    require(name_argument&&register_call,"duplicate-mouse CVar registration contract is invalid");
    plan.duplicate_cvar=one(storages,"duplicate-mouse CVar storage is ambiguous");return plan;
}
enum class Mode {Observe,FilterPromotedMouse};
struct Result {uint8_t fallback{};LONG duplicate_before{},duplicate_after{};DWORD thread{};};
inline bool writable_live(uintptr_t at,size_t size) {
    MEMORY_BASIC_INFORMATION page{};
    if(VirtualQuery(reinterpret_cast<void*>(at),&page,sizeof(page))!=sizeof(page)||page.State!=MEM_COMMIT||
        (page.Protect&PAGE_GUARD))return false;
    const auto protection=page.Protect&0xff;
    const auto begin=reinterpret_cast<uintptr_t>(page.BaseAddress);
    return (protection==PAGE_READWRITE||protection==PAGE_WRITECOPY)&&at>=begin&&
        at-begin<=page.RegionSize&&size<=page.RegionSize-(at-begin);
}
inline Result configure(uintptr_t base,const Plan& plan,Mode mode=Mode::Observe) {
    uintptr_t object{},vtable{};uint8_t fallback{};LONG duplicate{};
    require(read_value(base+plan.singleton,object)&&object&&read_value(object,vtable)&&vtable==base+plan.vtable,
        "live Slate singleton does not match the discovered vtable");
    require(read_value(object+plan.fallback_offset,fallback)&&fallback<=1&&
        read_value(base+plan.duplicate_cvar,duplicate)&&(duplicate==0||duplicate==1),"live touch configuration is invalid");
    if(mode==Mode::FilterPromotedMouse) {
        require(writable_live(base+plan.duplicate_cvar,4),"live duplicate-mouse CVar is not writable; no protection changes attempted");
        require(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(base+plan.duplicate_cvar),1,duplicate)==duplicate,
            "duplicate-mouse CVar changed before configuration");
    }
    return {fallback,duplicate,mode==Mode::FilterPromotedMouse?1:duplicate,GetCurrentThreadId()};
}
struct Configuration {
    uintptr_t base{};Plan plan;HWND window{};UINT message{};DWORD thread{};
    Mode mode{Mode::Observe};
    std::atomic<bool> entered{},finished{};Result result;std::string error;
};
// The context and pinned callback outlive a possible SendMessageTimeout. No
// stack pointer is passed to the UI thread and no procedure is subclassed.
inline std::atomic<Configuration*> configuration{};
inline LRESULT CALLBACK configure_hook(int code,WPARAM wparam,LPARAM lparam) {
    auto* state=configuration.load(std::memory_order_acquire);
    if(code>=0&&state) {
        const auto* message=reinterpret_cast<const CWPSTRUCT*>(lparam);
        if(message->hwnd==state->window&&message->message==state->message&&!state->entered.exchange(true)) {
            try {
                require(GetCurrentThreadId()==state->thread,"touch configuration ran outside the window thread");
                state->result=configure(state->base,state->plan,state->mode);
            }catch(const std::exception& error){state->error=error.what();}
            state->finished.store(true,std::memory_order_release);
        }
    }
    return CallNextHookEx(nullptr,code,wparam,lparam);
}
inline Result install_on_window_thread(uintptr_t base,const Plan& plan,HWND window,Mode mode=Mode::Observe) {
    require(window&&configuration.load()==nullptr,"touch configuration may only be installed once");
    DWORD pid{};const auto thread=GetWindowThreadProcessId(window,&pid);
    require(thread&&pid==GetCurrentProcessId(),"touch configuration requires this process's window");
    if(thread==GetCurrentThreadId())return configure(base,plan,mode);
    auto* state=new Configuration;state->base=base;state->plan=plan;state->window=window;state->thread=thread;
    state->mode=mode;
    state->message=RegisterWindowMessageW(L"TouchUILaunch.NTE.ConfigureNativeTouch");
    require(state->message!=0,"cannot register touch configuration message");
    Configuration* empty{};
    require(configuration.compare_exchange_strong(empty,state,std::memory_order_release),"concurrent touch configuration installation");
    HMODULE module{};
    require(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&configure_hook),&module)!=FALSE,"cannot pin touch configuration callback");
    const auto hook=SetWindowsHookExW(WH_CALLWNDPROC,configure_hook,module,thread);
    require(hook!=nullptr,"cannot install window-thread touch configuration hook");
    DWORD_PTR ignored{};
    const bool sent=SendMessageTimeoutW(window,state->message,0,0,SMTO_ABORTIFHUNG|SMTO_BLOCK,5000,&ignored)!=0;
    const bool removed=UnhookWindowsHookEx(hook)!=FALSE;
    require(sent&&state->finished.load(std::memory_order_acquire),"window-thread touch configuration did not complete");
    require(removed,"cannot remove window-thread touch configuration hook");
    require(state->error.empty(),state->error);return state->result;
}
}
