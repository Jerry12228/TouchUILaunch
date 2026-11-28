#pragma once
#include "win_util.hpp"
#include <cstring>
#include <optional>

namespace touchui::ef {
inline void require(bool condition,const char* message) {
    if(!condition)throw std::runtime_error(message);
}

// Only the public IL2CPP API and Unity's MethodInfo entry prefix are used.
// No client RVAs, static-area offsets, metadata files or platform overrides.
struct Api {
    HMODULE module{};
    void* (*domain_get)(){};
    const void** (*domain_get_assemblies)(void*,size_t*){};
    void* (*assembly_get_image)(const void*){};
    const char* (*image_get_name)(void*){};
    void* (*class_from_name)(void*,const char*,const char*){};
    void* (*class_get_nested_types)(void*,void**){};
    const char* (*class_get_name)(void*){};
    const void* (*class_get_methods)(void*,void**){};
    const char* (*method_get_name)(const void*){};
    uint32_t (*method_get_param_count)(const void*){};
    const void* (*method_get_param)(const void*,uint32_t){};
    const void* (*method_get_return_type)(const void*){};
    bool (*method_is_instance)(const void*){};
    bool (*method_is_generic)(const void*){};
    void* (*class_from_type)(const void*){};
    int (*type_get_type)(const void*){};
    bool (*type_is_byref)(const void*){};
    void* (*class_get_field_from_name)(void*,const char*){};
    const void* (*field_get_type)(void*){};
    int (*field_get_flags)(void*){};
    void (*field_static_get_value)(void*,void*){};
    void (*field_get_value)(void*,void*,void*){};
    void (*field_set_value)(void*,void*,void*){};
    void* (*runtime_invoke)(const void*,void*,void**,void**){};
    void* (*thread_current)(){};

    static Api bind(HMODULE module) {
        require(module!=nullptr,"EF: GameAssembly is unavailable");
        Api api;api.module=module;
#define EF_API(member) api.member=reinterpret_cast<decltype(api.member)>(GetProcAddress(module,"il2cpp_" #member)); \
        require(api.member!=nullptr,"EF: required IL2CPP export missing: il2cpp_" #member)
        EF_API(domain_get);EF_API(domain_get_assemblies);EF_API(assembly_get_image);EF_API(image_get_name);
        EF_API(class_from_name);EF_API(class_get_nested_types);EF_API(class_get_name);EF_API(class_get_methods);
        EF_API(method_get_name);EF_API(method_get_param_count);EF_API(method_get_param);
        EF_API(method_get_return_type);EF_API(method_is_instance);EF_API(method_is_generic);
        EF_API(class_from_type);EF_API(type_get_type);EF_API(type_is_byref);
        EF_API(class_get_field_from_name);EF_API(field_get_type);EF_API(field_get_flags);
        EF_API(field_static_get_value);EF_API(runtime_invoke);EF_API(thread_current);
#undef EF_API
        // Optional UI-routing capability; missing exports must not disable
        // the independently verified layout selection path.
        api.field_get_value=reinterpret_cast<decltype(api.field_get_value)>(GetProcAddress(module,"il2cpp_field_get_value"));
        api.field_set_value=reinterpret_cast<decltype(api.field_set_value)>(GetProcAddress(module,"il2cpp_field_set_value"));
        return api;
    }
};

inline bool text_equal(const char* a,const char* b) {return a&&b&&std::strcmp(a,b)==0;}
inline bool common_image(const char* name) {
    return text_equal(name,"Common.Beyond.dll")||text_equal(name,"Common.Beyond");
}
inline void* method_entry(const Api& api,const void* method) {
    require(method!=nullptr,"EF: missing MethodInfo");
    void* entry{};
    std::memcpy(&entry,method,sizeof(entry));
    MEMORY_BASIC_INFORMATION region{};
    require(executable_pointer(entry)&&VirtualQuery(entry,&region,sizeof(region))==sizeof(region)&&
        region.Type==MEM_IMAGE&&region.AllocationBase==api.module,
        "EF: method entry is not executable code in GameAssembly");
    return entry;
}

struct Contract {
    const void* change_method{};
    void* change_entry{};
    void* input_field{};
    void* provider_field{};
    void* notification_field{};
    int32_t touch{};
};

inline void* static_field(const Api& api,void* klass,const char* name) {
    auto field=api.class_get_field_from_name(klass,name);
    require(field&&(api.field_get_flags(field)&0x10)!=0,"EF: required static DeviceInfo/enum field missing");
    return field;
}

// Called only on the game's attached window thread, never on the loader worker.
// No image yet means initialization is pending. A populated, incompatible image
// is a contract failure; it must not silently select another signature/address.
inline std::optional<Contract> resolve(const Api& api) {
    auto domain=api.domain_get();
    if(!domain)return {};
    size_t count{};
    auto assemblies=api.domain_get_assemblies(domain,&count);
    require(count<=4096,"EF: invalid assembly count");
    if(!assemblies||!count)return {};
    void* image{};
    for(size_t index=0;index<count;++index) {
        auto candidate=api.assembly_get_image(assemblies[index]);
        if(candidate&&common_image(api.image_get_name(candidate))) {
            require(!image||image==candidate,"EF: ambiguous Common.Beyond image");image=candidate;
        }
    }
    if(!image)return {};
    auto device=api.class_from_name(image,"Beyond","DeviceInfo");
    require(device!=nullptr,"EF: Beyond.DeviceInfo was not found");
    void* input_type{};void* iterator{};
    while(auto nested=api.class_get_nested_types(device,&iterator)) {
        if(text_equal(api.class_get_name(nested),"InputType")) {
            require(input_type==nullptr,"EF: ambiguous DeviceInfo.InputType");input_type=nested;
        }
    }
    require(input_type!=nullptr,"EF: DeviceInfo.InputType was not found");
    auto touch_field=static_field(api,input_type,"Touch");
    require((api.field_get_flags(touch_field)&0x40)!=0&&
        api.class_from_type(api.field_get_type(touch_field))==input_type,
        "EF: Touch is not an InputType enum literal");
    auto value_field=api.class_get_field_from_name(input_type,"value__");
    require(value_field&&api.type_get_type(api.field_get_type(value_field))==0x08,
        "EF: InputType must have an Int32 representation");
    Contract contract;
    contract.input_field=static_field(api,device,"<inputType>k__BackingField");
    require(api.class_from_type(api.field_get_type(contract.input_field))==input_type,
        "EF: inputType backing field type changed");
    contract.provider_field=static_field(api,device,"s_provider");
    contract.notification_field=static_field(api,device,"onInputTypeChanged");
    require(api.type_get_type(api.field_get_type(contract.provider_field))==0x12&&
        api.type_get_type(api.field_get_type(contract.notification_field))==0x12,
        "EF: DeviceInfo provider/notification reference types changed");
    api.field_static_get_value(touch_field,&contract.touch);
    require(contract.touch>=0,"EF: invalid Touch enum value");
    iterator=nullptr;
    while(auto method=api.class_get_methods(device,&iterator)) {
        if(!text_equal(api.method_get_name(method),"ChangeInputType")||api.method_is_instance(method)||api.method_is_generic(method)||
            api.method_get_param_count(method)!=1||
            api.type_get_type(api.method_get_return_type(method))!=0x01||
            api.type_is_byref(api.method_get_param(method,0))||
            api.class_from_type(api.method_get_param(method,0))!=input_type)continue;
        require(!contract.change_method,"EF: ambiguous ChangeInputType contract");
        contract.change_method=method;
    }
    require(contract.change_method!=nullptr,"EF: static void ChangeInputType(InputType) was not found");
    contract.change_entry=method_entry(api,contract.change_method);
    return contract;
}

enum class Tick {WaitingThread,WaitingMetadata,WaitingDevice,Deferred,Retry,ManagedException,Applied};
struct Observation {int32_t current=-1;bool provider{},notification{};uint32_t attempts{};};
class Layout {
    const Api& api_;
    bool pumping_{};
    DWORD thread_{};
    uint64_t next_{};
    std::optional<Contract> contract_;
    Observation observation_;
public:
    explicit Layout(const Api& api):api_(api){}
    const std::optional<Contract>& contract()const{return contract_;}
    const Observation& observation()const{return observation_;}
    Tick prepare() {
        if(!api_.thread_current())return Tick::WaitingThread;
        if(thread_)require(thread_==GetCurrentThreadId(),"EF: layout pump moved away from the game window thread");
        else thread_=GetCurrentThreadId();
        if(!contract_)contract_=resolve(api_);
        return contract_?Tick::WaitingDevice:Tick::WaitingMetadata;
    }
    Tick pump(uint64_t now) {
        if(pumping_||now<next_)return Tick::Deferred;
        struct Guard {bool& flag;~Guard(){flag=false;}} guard{pumping_};pumping_=true;
        next_=now+250;
        const auto prepared=prepare();
        if(prepared==Tick::WaitingThread||!contract_)return prepared;
        const auto& c=*contract_;
        void* provider{};void* notification{};
        api_.field_static_get_value(c.provider_field,&provider);
        api_.field_static_get_value(c.notification_field,&notification);
        observation_.provider=provider!=nullptr;observation_.notification=notification!=nullptr;
        api_.field_static_get_value(c.input_field,&observation_.current);
        if(!provider||!notification)return Tick::WaitingDevice;
        int32_t current=observation_.current;
        if(current==c.touch)return Tick::Applied;
        int32_t target=c.touch;void* args[]={&target};void* exception{};
        ++observation_.attempts;
        api_.runtime_invoke(c.change_method,nullptr,args,&exception);
        api_.field_static_get_value(c.input_field,&current);
        observation_.current=current;
        if(exception) {
            // A notification can throw after the field was written. The next
            // call would otherwise see Touch and incorrectly publish Ready
            // without knowing whether the UI event completed.
            require(current!=target,"EF: input state changed but the UI switch raised a managed exception; restart the game");
            return Tick::ManagedException;
        }
        return current==target?Tick::Applied:Tick::Retry;
    }
};
}
