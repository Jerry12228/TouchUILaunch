#pragma once
#include "ef_runtime.hpp"
#include <MinHook.h>
#include <atomic>
#include <cstdio>

namespace touchui::ef {
struct PointerContract {
    void* behavior{};
    int32_t independent{};
    void* process{};
    void* route{};
};
inline std::optional<PointerContract> resolve_pointers(const Api& api) {
    require(api.field_get_value&&api.field_set_value,"EF: UI pointer instance-field exports unavailable");
    auto domain=api.domain_get();if(!domain)return {};
    size_t count{};auto assemblies=api.domain_get_assemblies(domain,&count);
    require(count<=4096,"EF: invalid UI pointer assembly count");
    if(!assemblies||!count)return {};
    void* image{};
    for(size_t index=0;index<count;++index) {
        auto candidate=api.assembly_get_image(assemblies[index]);
        const auto name=candidate?api.image_get_name(candidate):nullptr;
        if(text_equal(name,"Unity.InputSystem.dll")||text_equal(name,"Unity.InputSystem")) {
            require(!image||image==candidate,"EF: ambiguous Unity.InputSystem image");image=candidate;
        }
    }
    if(!image)return {};
    auto module=api.class_from_name(image,"UnityEngine.InputSystem.UI","InputSystemUIInputModule");
    auto behavior=api.class_from_name(image,"UnityEngine.InputSystem.UI","UIPointerBehavior");
    auto control=api.class_from_name(image,"UnityEngine.InputSystem","InputControl");
    require(module&&behavior&&control,"EF: UI pointer classes unavailable");
    PointerContract c;
    c.behavior=api.class_get_field_from_name(module,"m_PointerBehavior");
    require(c.behavior&&(api.field_get_flags(c.behavior)&0x10)==0&&
        api.class_from_type(api.field_get_type(c.behavior))==behavior,"EF: UI pointer behavior field contract changed");
    auto literal=static_field(api,behavior,"AllPointersAsIs");
    auto value=api.class_get_field_from_name(behavior,"value__");
    require((api.field_get_flags(literal)&0x40)!=0&&api.class_from_type(api.field_get_type(literal))==behavior&&
        value&&api.type_get_type(api.field_get_type(value))==0x08,"EF: UI pointer enum contract changed");
    api.field_static_get_value(literal,&c.independent);
    require(c.independent>=0,"EF: UI pointer enum value is invalid");
    const void* process{};const void* route{};void* iterator{};
    while(auto method=api.class_get_methods(module,&iterator)) {
        if(!api.method_is_instance(method)||api.method_is_generic(method))continue;
        const auto name=api.method_get_name(method);const auto parameters=api.method_get_param_count(method);
        const auto result=api.type_get_type(api.method_get_return_type(method));
        if(text_equal(name,"Process")&&parameters==0&&result==0x01) {
            require(!process,"EF: ambiguous UI pointer Process method");process=method;
        }
        if(text_equal(name,"GetPointerStateIndexFor")&&parameters==2&&result==0x08&&
            !api.type_is_byref(api.method_get_param(method,0))&&!api.type_is_byref(api.method_get_param(method,1))&&
            api.class_from_type(api.method_get_param(method,0))==control&&
            api.type_get_type(api.method_get_param(method,1))==0x02) {
            require(!route,"EF: ambiguous UI pointer routing method");route=method;
        }
    }
    require(process&&route,"EF: UI pointer Process/routing signatures unavailable");
    c.process=method_entry(api,process);c.route=method_entry(api,route);
    require(c.process!=c.route,"EF: UI pointer method entries unexpectedly alias");
    return c;
}

// Pointer ownership follows the normal game call stack. No Unity instance is
// retained across frames, so no raw object address, field offset or GC handle
// table is needed. The native field API writes the same scalar as Unity's
// public pointerBehavior setter, before the original routing/Process call.
class PointerHooks {
    using Process=void(*)(void*,const void*);
    using Route=int32_t(*)(void*,void*,bool,const void*);
    using Logger=void(*)(const char*);
    inline static const Api* api_{};
    inline static PointerContract contract_{};
    inline static Process original_process_{};
    inline static Route original_route_{};
    inline static std::atomic_bool enabled_{};
    inline static std::atomic_uint applied_reports_{};
    inline static Logger logger_{};
    bool owns_init_{},process_created_{},route_created_{},attempted_{};
    static void note(const char* text)noexcept {try {if(logger_)logger_(text);}catch(...){}}
    static bool apply(void* instance) {
        if(!enabled_.load(std::memory_order_acquire)||!instance||!api_->thread_current())return true;
        int32_t current{};api_->field_get_value(instance,contract_.behavior,&current);
        if(current==contract_.independent)return true;
        const auto before=current;
        auto target=contract_.independent;api_->field_set_value(instance,contract_.behavior,&target);
        api_->field_get_value(instance,contract_.behavior,&current);
        if(current!=target)return false;
        if(applied_reports_.fetch_add(1,std::memory_order_relaxed)<8) {
            char message[192]{};
            std::snprintf(message,sizeof(message),"UI pointer routing: AllPointersAsIs applied before input (was %d, now %d); independent mouse/touch records",before,current);
            note(message);
        }
        return true;
    }
    static void guarded_apply(void* instance) {
        bool ok{};
        __try {ok=apply(instance);}
        __except(EXCEPTION_EXECUTE_HANDLER){ok=false;}
        if(!ok) {
            enabled_.store(false,std::memory_order_release);
            note("UI pointer routing: field access/readback failed; hooks now forward original calls");
        }
    }
    static void process(void* instance,const void* method) {
        guarded_apply(instance);original_process_(instance,method);
    }
    static int32_t route(void* instance,void* control,bool create,const void* method) {
        guarded_apply(instance);return original_route_(instance,control,create,method);
    }
public:
    bool install(const Api& api,Logger logger) {
        require(!attempted_,"EF: duplicate UI pointer hook attempt");
        auto contract=resolve_pointers(api);if(!contract)return false;
        require(!original_process_&&!original_route_,"EF: UI pointer hooks already have an owner");
        attempted_=true;api_=&api;contract_=*contract;logger_=logger;
        enabled_.store(false,std::memory_order_release);applied_reports_.store(0);
        const auto initialized=MH_Initialize();owns_init_=initialized==MH_OK;
        require(initialized==MH_OK||initialized==MH_ERROR_ALREADY_INITIALIZED,"EF: UI pointer hook initialization failed");
        require(MH_CreateHook(contract_.process,reinterpret_cast<void*>(&process),reinterpret_cast<void**>(&original_process_))==MH_OK,
            "EF: cannot prepare UI pointer Process hook");process_created_=true;
        require(MH_CreateHook(contract_.route,reinterpret_cast<void*>(&route),reinterpret_cast<void**>(&original_route_))==MH_OK,
            "EF: cannot prepare UI pointer routing hook");route_created_=true;
        // Until both are enabled, any partially installed detour is a pure
        // pass-through and keeps its trampoline resident for process lifetime.
        require(MH_EnableHook(contract_.route)==MH_OK&&MH_EnableHook(contract_.process)==MH_OK,
            "EF: cannot enable UI pointer routing hooks");
        enabled_.store(true,std::memory_order_release);
        note("UI pointer routing: independent mouse/touch policy ready");return true;
    }
    void deactivate()noexcept {enabled_.store(false,std::memory_order_release);}
    // Test-only use after all fixture callers have stopped. Production does
    // not unload its callbacks or restore code over another writer.
    void remove_fixture() {
        deactivate();
        for(const auto entry:{process_created_?contract_.process:nullptr,route_created_?contract_.route:nullptr}) {
            if(!entry)continue;const auto disabled=MH_DisableHook(entry);
            require(disabled==MH_OK||disabled==MH_ERROR_DISABLED,"EF pointer fixture: disable failed");
            require(MH_RemoveHook(entry)==MH_OK,"EF pointer fixture: removal failed");
        }
        if(owns_init_)require(MH_Uninitialize()==MH_OK,"EF pointer fixture: MinHook cleanup failed");
        process_created_=route_created_=owns_init_=attempted_=false;
        original_process_=nullptr;original_route_=nullptr;api_=nullptr;logger_=nullptr;
    }
};
}
