#include "ef_pointer.hpp"
#include <iostream>
#include <unordered_map>

namespace {
using namespace touchui::ef;
unsigned checks{};
void check(bool condition,const char* message){++checks;if(!condition)throw std::runtime_error(message);}
template<class F> void rejects(F function,const char* message) {
    bool rejected{};try {function();}catch(const std::runtime_error&){rejected=true;}
    check(rejected,message);
}
struct Class;
struct Type {int kind;Class* klass{};bool byref{};};
struct Field {const char* name;Type* type;int flags;int32_t literal{};};
struct Method {void* entry;const char* name;Type* result;std::vector<Type*> parameters;bool instance=true,generic{};};
struct Class {const char* name;std::vector<Field*> fields;std::vector<Method*> methods;};
struct Instance {void* header[2]{};int32_t mode=1;std::unordered_map<void*,bool> pressed;};
struct Fixture;
Fixture* current{};
__declspec(noinline) void process_original(void*,const void*);
__declspec(noinline) int32_t route_original(void*,void*,bool,const void*);
struct Fixture {
    Class module{"InputSystemUIInputModule"},behavior{"UIPointerBehavior"},control{"InputControl"};
    Type enum_type{0x11,&behavior},control_type{0x12,&control},i4{0x08},boolean{0x02},nothing{0x01};
    Field mode{"m_PointerBehavior",&enum_type,0};
    Field independent{"AllPointersAsIs",&enum_type,0x50,17},underlying{"value__",&i4,0};
    Method process{reinterpret_cast<void*>(&process_original),"Process",&nothing,{}};
    Method route{reinterpret_cast<void*>(&route_original),"GetPointerStateIndexFor",&i4,{&control_type,&boolean}};
    bool attached=true,registered=true,reject_write{};
    unsigned reads{},writes{},routes{},processes{},reports{};
    void* last_control{};bool last_create{};const void* last_method{};
    Fixture() {current=this;module.fields={&mode};module.methods={&process,&route};behavior.fields={&independent,&underlying};}
    static void log(const char*){++current->reports;}
    Api api() {
        Api a;a.module=GetModuleHandleW(nullptr);
        a.domain_get=[]()->void*{return current;};
        a.domain_get_assemblies=[](void*,size_t* count)->const void** {
            static const void* image;image=&current->module;*count=current->registered?1:0;return &image;
        };
        a.assembly_get_image=[](const void* value)->void*{return const_cast<void*>(value);};
        a.image_get_name=[](void*){return "Unity.InputSystem.dll";};
        a.class_from_name=[](void*,const char* ns,const char* name)->void* {
            if(text_equal(ns,"UnityEngine.InputSystem.UI")) {
                if(text_equal(name,"InputSystemUIInputModule"))return &current->module;
                if(text_equal(name,"UIPointerBehavior"))return &current->behavior;
            }
            return text_equal(ns,"UnityEngine.InputSystem")&&text_equal(name,"InputControl")?&current->control:nullptr;
        };
        a.class_get_field_from_name=[](void* value,const char* name)->void* {
            for(auto field:static_cast<Class*>(value)->fields)if(text_equal(name,field->name))return field;
            return nullptr;
        };
        a.field_get_flags=[](void* field){return static_cast<Field*>(field)->flags;};
        a.field_get_type=[](void* field)->const void*{return static_cast<Field*>(field)->type;};
        a.class_from_type=[](const void* type)->void*{return static_cast<const Type*>(type)->klass;};
        a.type_get_type=[](const void* type){return static_cast<const Type*>(type)->kind;};
        a.type_is_byref=[](const void* type){return static_cast<const Type*>(type)->byref;};
        a.field_static_get_value=[](void* field,void* value){*static_cast<int32_t*>(value)=static_cast<Field*>(field)->literal;};
        a.class_get_methods=[](void* klass,void** iterator)->const void* {
            const auto& methods=static_cast<Class*>(klass)->methods;const auto index=reinterpret_cast<uintptr_t>(*iterator);
            if(index>=methods.size())return nullptr;*iterator=reinterpret_cast<void*>(index+1);return methods[index];
        };
        a.method_get_name=[](const void* value){return static_cast<const Method*>(value)->name;};
        a.method_get_param_count=[](const void* value){return static_cast<uint32_t>(static_cast<const Method*>(value)->parameters.size());};
        a.method_get_param=[](const void* value,uint32_t index)->const void*{return static_cast<const Method*>(value)->parameters.at(index);};
        a.method_get_return_type=[](const void* value)->const void*{return static_cast<const Method*>(value)->result;};
        a.method_is_instance=[](const void* value){return static_cast<const Method*>(value)->instance;};
        a.method_is_generic=[](const void* value){return static_cast<const Method*>(value)->generic;};
        a.field_get_value=[](void* instance,void* field,void* result) {
            check(field==&current->mode,"read exact dynamically resolved instance field");++current->reads;
            *static_cast<int32_t*>(result)=static_cast<Instance*>(instance)->mode;
        };
        a.field_set_value=[](void* instance,void* field,void* value) {
            check(field==&current->mode,"write exact dynamically resolved instance field");++current->writes;
            if(!current->reject_write)static_cast<Instance*>(instance)->mode=*static_cast<int32_t*>(value);
        };
        a.thread_current=[]()->void*{return current->attached?current:nullptr;};
        return a;
    }
};
__declspec(noinline) void process_original(void* instance,const void* method) {
    volatile int32_t observed=static_cast<Instance*>(instance)->mode;
    current->last_method=method;++current->processes;
    check(observed==current->independent.literal||observed==1,"original Process receives an unchanged valid instance");
}
__declspec(noinline) int32_t route_original(void* instance,void* control,bool create,const void* method) {
    auto& object=*static_cast<Instance*>(instance);
    // A deliberately small model of the reported state-reuse hazard, not a
    // Unity implementation: unified input replaces all prior pointer records.
    if(object.mode!=current->independent.literal)object.pressed.clear();
    object.pressed[control]=create;current->last_control=control;current->last_create=create;
    current->last_method=method;++current->routes;return -37;
}
void contract_tests() {
    Fixture f;auto a=f.api();f.registered=false;
    check(!resolve_pointers(a),"unregistered InputSystem is pending");f.registered=true;
    const auto c=resolve_pointers(a);
    check(c&&c->independent==17&&c->behavior==&f.mode,"enum/field resolved without fixed values or offsets");
    auto overload=f.route;overload.parameters={&f.i4};f.module.methods.push_back(&overload);
    check(resolve_pointers(a)->route==f.route.entry,"other routing overload ignored");
    overload.parameters=f.route.parameters;rejects([&]{resolve_pointers(a);},"duplicate routing signature rejected");
    f.module.methods={&f.process,&f.route};f.route.instance=false;
    rejects([&]{resolve_pointers(a);},"static route cannot replace instance method");f.route.instance=true;
    f.process.result=&f.i4;rejects([&]{resolve_pointers(a);},"wrong Process return type rejected");f.process.result=&f.nothing;
    auto byref=f.control_type;byref.byref=true;f.route.parameters[0]=&byref;
    rejects([&]{resolve_pointers(a);},"byref control parameter rejected");f.route.parameters[0]=&f.control_type;
    f.mode.flags=0x10;rejects([&]{resolve_pointers(a);},"static behavior field rejected");f.mode.flags=0;
    f.mode.type=&f.i4;rejects([&]{resolve_pointers(a);},"wrong behavior enum field rejected");f.mode.type=&f.enum_type;
    f.independent.flags=0x10;rejects([&]{resolve_pointers(a);},"nonliteral independent mode rejected");f.independent.flags=0x50;
    f.route.entry=&f.mode;rejects([&]{resolve_pointers(a);},"non-executable pointer method rejected");
    f.route.entry=reinterpret_cast<void*>(&route_original);a.field_set_value=nullptr;
    rejects([&]{resolve_pointers(a);},"missing optional field API disables only pointer capability");
}
void routing_tests() {
    Fixture f;auto a=f.api();PointerHooks hooks;
    check(hooks.install(a,Fixture::log),"real pointer hooks install");
    PointerHooks duplicate;rejects([&]{duplicate.install(a,Fixture::log);},"second hook object cannot replace active pointer owner");
    using Route=int32_t(*)(void*,void*,bool,const void*);using Process=void(*)(void*,const void*);
    Route volatile route=&route_original;Process volatile process=&process_original;
    Instance first,second;
    auto finger1=reinterpret_cast<void*>(11),finger2=reinterpret_cast<void*>(22),mouse=reinterpret_cast<void*>(33);
    const void* method=&f.route;
    check(route(&first,finger1,true,method)==-37&&first.mode==17,"independent policy applied before first route and return preserved");
    route(&first,finger2,true,method);route(&first,finger1,false,method);route(&first,mouse,false,method);
    check(first.pressed.size()==3&&!first.pressed.at(finger1)&&first.pressed.at(finger2),"mouse update preserves released finger and other held finger in model");
    check(f.last_control==mouse&&!f.last_create&&f.last_method==method,"control/create/MethodInfo forwarded exactly");
    process(&second,&f.process);
    check(second.mode==17&&f.processes==1&&f.last_method==&f.process,"Process applies policy independently to another instance");
    const auto writes=f.writes;process(&second,&f.process);
    check(f.writes==writes,"already independent setting not redundantly written");
    first.mode=0;route(&first,finger1,true,method);
    check(first.mode==17,"later game mode changes repaired at next route");
    f.attached=false;second.mode=1;const auto reads=f.reads;
    route(&second,mouse,true,method);
    check(second.mode==1&&f.reads==reads,"unattached callback forwards without field API calls");f.attached=true;
    f.reject_write=true;route(&second,finger1,false,method);
    check(second.mode==1&&f.last_control==finger1&&!f.last_create,"failed readback still forwards original route");
    f.reject_write=false;route(&second,finger2,true,method);
    check(second.mode==1,"field failure leaves hooks in pass-through state");
    hooks.remove_fixture();first.mode=1;route(&first,mouse,true,method);
    check(first.mode==1,"fixture removal restores original code");
    f.independent.literal=9;
    check(hooks.install(a,Fixture::log),"pointer hooks can restart in owned fixture");
    for(unsigned index=0;index<20;++index) {Instance object;process(&object,&f.process);check(object.mode==9,"all instances use discovered enum value");}
    check(f.reports<=14,"application diagnostics remain bounded across many instances");
    hooks.deactivate();second.mode=1;process(&second,&f.process);
    check(second.mode==1,"explicit inactive gate preserves game setting");hooks.remove_fixture();
}
int32_t conflicting_route(void*,void*,bool,const void*){return 42;}
void conflict_test() {
    Fixture f;auto a=f.api();
    check(MH_Initialize()==MH_OK,"conflicting owner fixture initializes hooks");void* prior{};
    check(MH_CreateHook(f.route.entry,reinterpret_cast<void*>(&conflicting_route),&prior)==MH_OK,"conflicting owner reserves route");
    PointerHooks hooks;rejects([&]{hooks.install(a,Fixture::log);},"existing owner rejected without replacing it");
    hooks.remove_fixture();
    check(MH_RemoveHook(f.route.entry)==MH_OK,"pointer cleanup preserves another owner's hook");
    check(MH_Uninitialize()==MH_OK,"conflict fixture releases hooks");
}
}
int main() {
    try {contract_tests();routing_tests();conflict_test();std::cout<<"PASS: "<<checks<<" Endfield pointer metadata, timing, forwarding and isolation checks\n";return 0;}
    catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
