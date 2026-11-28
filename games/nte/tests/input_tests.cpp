#include "nte_fixture.hpp"
#include "nte_input.hpp"
#include "nte_input_trace.hpp"
#include "nte_pager.hpp"
#include <fstream>
#include <future>
#include <iostream>
#include <thread>

namespace {
using namespace touchui::nte;
unsigned checks{};
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F action,const char* message) {
    bool rejected{};try{action();}catch(const std::exception&){rejected=true;}check(rejected,message);
}
struct InputFixture:nte_fixture::Fixture {
    static constexpr uintptr_t ctor=0x3800,factory=0x3a00,registration=0x3b00;
    static constexpr uintptr_t key=0x7600,section=0x7680,duplicate_key=0x7700;
    static constexpr uintptr_t slate_vtable=0x7900,singleton=0x9100,duplicate=0x9120,object=0xb000;
    static constexpr int32_t fallback=0x230; // Deliberately differs from the game sample.
    InputFixture() {
        wide(key,L"bTouchFallbackToMouse");wide(section,L"MobileSlateUI");
        wide(duplicate_key,L"Slate.PreventDuplicateMouseEventsForTouchForWindows7");
        wide(0x7780,L"Synthetic duplicate filter help");
        uintptr_t at=ctor;
        const auto op=[&](std::initializer_list<uint8_t> data){emit(at,data);at+=data.size();};
        op({0x48,0x8b,0xd9});
        op({0x48,0x8d,0x05,0,0,0,0});rel(at-4,at,slate_vtable);
        op({0x48,0x89,0x03});
        op({0x66,0xc7,0x83,0,0,0,0,0x01,0});put<int32_t>(at-6,fallback);
        op({0xf0,0xff,0x83,0x80,0x01,0,0}); // Valid interlocked reference count in the constructor prefix.
        op({0x4c,0x8d,0x8b,0,0,0,0});put<int32_t>(at-4,fallback);
        op({0x48,0x89,0x44,0x24,0x20}); // Fifth GetBool argument is passed on the stack.
        op({0x4c,0x8d,0x05,0,0,0,0});rel(at-4,at,key);
        op({0x48,0x8d,0x15,0,0,0,0});rel(at-4,at,section);
        op({0xe8,0,0,0,0});rel(at-4,at,0x3c00);op({0xc3});
        functions.emplace_back(static_cast<uint32_t>(ctor),static_cast<uint32_t>(at));
        emit(factory,{0xe8,0,0,0,0,0x48,0x8b,0xd8,0x48,0x89,0x1d,0,0,0,0,0xc3});
        rel(factory+1,factory+5,ctor);rel(factory+11,factory+15,singleton);
        functions.emplace_back(static_cast<uint32_t>(factory),static_cast<uint32_t>(factory+16));
        emit(registration,{0x4c,0x8d,0x05,0,0,0,0,0x48,0x8d,0x15,0,0,0,0,
            0x4c,0x8d,0x0d,0,0,0,0,0xff,0x50,0x40,0xc3});
        rel(registration+3,registration+7,duplicate);rel(registration+10,registration+14,duplicate_key);
        rel(registration+17,registration+21,0x7780);
        functions.emplace_back(static_cast<uint32_t>(registration),static_cast<uint32_t>(registration+25));
        emit(0x3c00,{0xc3});put<uintptr_t>(slate_vtable,base+0x3c00);
        put<uintptr_t>(singleton,base+object);put<uintptr_t>(object,base+slate_vtable);
        put<uint8_t>(object+fallback,1);put<LONG>(duplicate,0);finish();
    }
    void finish() {
        std::sort(functions.begin(),functions.end());size_t entry=0xc000;
        for(const auto& [lo,hi]:functions){put<uint32_t>(entry,lo);put<uint32_t>(entry+4,hi);put<uint32_t>(entry+8,0xd000);entry+=12;}
        put<uint32_t>(0x98+116+3*8,static_cast<uint32_t>(functions.size()*12));sync();
    }
};
struct PagerFixture:InputFixture {
    static constexpr uintptr_t table=0x7a00,type_name=0x7c00;
    static constexpr int32_t enabled=0x130,armed=0x100,pressed=0x140,touch=40,pointer=32;
    PagerFixture() {
        text(type_name,"SHTPageScrollBox");
        emit(0x4000,{0x48,0x8d,0x15,0,0,0,0,0xe8,0,0,0,0,0xc3});rel(0x4003,0x4007,type_name);rel(0x4008,0x400c,0x4100);
        emit(0x4100,{0xe8,0,0,0,0,0xc3});rel(0x4101,0x4105,0x4200);
        emit(0x4200,{0xb9,0x10,0x02,0,0,0xe8,0,0,0,0,0x48,0x8d,0x05,0,0,0,0,0x48,0x89,0x43,0x10,0xc3});
        rel(0x4206,0x420a,0x4e80);rel(0x420d,0x4211,table);emit(0x4e80,{0xc3});
        emit(0x4300,{0x80,0xb9,0x30,0x01,0,0,0,0x48,0x8b,0xf8,0x49,0x8b,0xf8,
            0x41,0x80,0x78,0x28,0,0x80,0xb9,0,0x01,0,0,0,0x8b,0x4f,0x20,0xc3});
        // Save the third argument (R8), not RAX. The extra MOV is irrelevant.
        emit(0x4400,{0x48,0x8b,0xf9,0xc6,0x81,0x40,0x01,0,0,1,0x48,0x81,0xc1,0x44,0x01,0,0,
            0xe8,0,0,0,0,0x48,0x8d,0x8f,0x44,0x01,0,0,0xe8,0,0,0,0,0x48,0x8d,0x48,0x30,
            0xe8,0,0,0,0,0xe8,0,0,0,0,0xc3});
        rel(0x4412,0x4416,0x4c00);rel(0x441e,0x4422,0x4c10);rel(0x4427,0x442b,0x4c20);rel(0x442c,0x4430,0x4600);
        emit(0x4c00,{0xc3});emit(0x4c10,{0xc3});emit(0x4c20,{0xc3});
        emit(0x4500,{0x49,0x8b,0x41,0x10,0x33,0xdb,0x41,0x38,0x59,0x28,0x38,0x99,0,0x01,0,0,0x80,0x89,0x32,0x01,0,0,1,
            0x48,0x8b,0xc9,0xe8,0,0,0,0,0x84,0xc0,0x75,0,0xf3,0x41,0x0f,0x5e,0x48,8,0xc3});rel(0x451b,0x451f,0x4a00);
        emit(0x4a00,{0x48,0x8b,0x05,0,0,0,0,0x48,0x8b,0x59,0x10,0x48,0x8b,0x90,0x80,0x01,0,0,0xc3});rel(0x4a03,0x4a07,0x9140);
        emit(0x4a80,{0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0xb0,1,0xc3});
        emit(0x4b00,{0x48,0x83,0xec,0x28,0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xd0,0x48,0x83,0xc4,0x28,0xc3});
        put<uintptr_t>(0x4b06,base+0x4a80);
        emit(0x4600,{0x48,0x8b,0xc2,0xc3});emit(0x4700,{0xc3});
        for(size_t i=0;i<pager::slots.size();++i) {
            const auto rva=i==2?0x4500:i==3?0x4300:i==5?0x4400:0x4600;
            put<uintptr_t>(table+pager::slots[i],base+rva);
        }
        functions.emplace_back(0x4000,0x400d);functions.emplace_back(0x4100,0x4106);
        functions.emplace_back(0x4200,0x4216);functions.emplace_back(0x4300,0x431d);
        functions.emplace_back(0x4400,0x4431);functions.emplace_back(0x4500,0x452a);
        functions.emplace_back(0x4600,0x4604);functions.emplace_back(0x4700,0x4701);
        functions.emplace_back(0x4a00,0x4a13);finish();
    }
};
struct ListFixture:PagerFixture {
    static constexpr uintptr_t list_table=0x7e00,list_name=0x7d00;
    ListFixture() {
        text(list_name,"SHTScrollBox");
        emit(0x4c40,{0x48,0x8d,0x15,0,0,0,0,0xe8,0,0,0,0,0xc3});rel(0x4c43,0x4c47,list_name);rel(0x4c48,0x4c4c,0x4d00);
        emit(0x4c80,{0xc3});
        emit(0x4d00,{0xb9,0x10,0x03,0,0,0xe8,0,0,0,0,0xe8,0,0,0,0,0xc3});rel(0x4d06,0x4d0a,0x4c80);rel(0x4d0b,0x4d0f,0x4e00);
        emit(0x4e00,{0xe8,0,0,0,0,0x48,0x8d,0x05,0,0,0,0,0x48,0x89,0x03,0xc3});rel(0x4e01,0x4e05,0x4e80);rel(0x4e08,0x4e0c,list_table);
        emit(0x4e40,{0xc6,0x81,0x60,0x01,0,0,1,0xc3});
        emit(0x4f00,{0xe8,0,0,0,0,0xe8,0,0,0,0,0xc3});rel(0x4f01,0x4f05,0x4700);rel(0x4f06,0x4f0a,0x4f80);
        emit(0x4f80,{0xf3,0x0f,0x10,0x81,0x80,0x01,0,0,0xc3});
        for(const auto slot:pager::slots)put<uintptr_t>(list_table+slot,live<uintptr_t>(PagerFixture::table+slot));
        put<uintptr_t>(list_table+0x58,base+0x4e40);put<uintptr_t>(list_table+0xe8,base+0x4600);put<uintptr_t>(list_table+0x10,base+0x4f00);
        functions.emplace_back(0x4c40,0x4c4d);functions.emplace_back(0x4d00,0x4d10);functions.emplace_back(0x4e00,0x4e10);
        functions.emplace_back(0x4f00,0x4f0b);finish();
    }
};
uintptr_t forwarded_widget{},forwarded_geometry{},forwarded_event{};
unsigned reply_calls{},enter_calls{},leave_calls{};
bool owner_valid=true;unsigned begin_calls{};uintptr_t weak_argument{},begin_argument{};
std::array<uint8_t,64> owner_object{};
bool fixture_weak_valid(uintptr_t member){weak_argument=member;return owner_valid;}
uintptr_t fixture_weak_resolve(uintptr_t member){weak_argument=member;return reinterpret_cast<uintptr_t>(owner_object.data());}
void fixture_begin(uintptr_t delegate){++begin_calls;begin_argument=delegate;}
uintptr_t fixture_reply(uintptr_t widget,uintptr_t result,uintptr_t geometry,uintptr_t event) {
    ++reply_calls;
    forwarded_widget=widget;forwarded_geometry=geometry;forwarded_event=event;
    *reinterpret_cast<uintptr_t*>(result)=0x12345678;return result;
}
void fixture_enter(uintptr_t widget,uintptr_t geometry,uintptr_t event) {
    ++enter_calls;
    forwarded_geometry=geometry;forwarded_event=event;*reinterpret_cast<uint8_t*>(widget+PagerFixture::armed)=1;
}
void fixture_leave(uintptr_t widget,uintptr_t event) {
    ++leave_calls;
    forwarded_event=event;*reinterpret_cast<uint8_t*>(widget+PagerFixture::armed)=0;
}
void pager_tests() {
    PagerFixture f;auto module=f.snapshot();auto plan=pager::discover(module);
    check(plan.vtable==PagerFixture::table&&plan.size==0x200&&plan.enabled==PagerFixture::enabled&&plan.active==PagerFixture::armed&&
        plan.pressed==PagerFixture::pressed&&plan.touch==PagerFixture::touch&&plan.pointer==PagerFixture::pointer&&
        plan.delta==16&&plan.drag_flags==0x132&&plan.capture_query==0x4a00&&plan.capture_return==0x451f&&
        plan.weak_offset==0x144&&plan.start_delegate==0x30&&plan.weak_valid==0x4c00&&plan.weak_resolve==0x4c10&&plan.broadcast_start==0x4c20,
        "pager fields and table are derived from construction and touch handlers");
    {
        PagerFixture bad;bad.text(PagerFixture::type_name,"OtherPagerType");bad.finish();auto image=bad.snapshot();
        rejects([&]{pager::discover(image);},"missing pager type rejected");
    }
    {
        PagerFixture bad;bad.put<uintptr_t>(PagerFixture::table+0xf0,bad.base+0x4700);bad.finish();auto image=bad.snapshot();
        rejects([&]{pager::discover(image);},"unrelated touch-move handler rejected");
    }
    {
        PagerFixture bad;bad.put<uint8_t>(0x4509,41);bad.finish();auto image=bad.snapshot();
        rejects([&]{pager::discover(image);},"inconsistent event layout rejected");
    }
    const auto functions=pager::callbacks();
    for(size_t i=0;i<functions.size();++i) {
        const auto original=i==3?reinterpret_cast<uintptr_t>(&fixture_enter):i==4?reinterpret_cast<uintptr_t>(&fixture_leave):
            reinterpret_cast<uintptr_t>(&fixture_reply);
        plan.originals[i]=original-f.base;f.put<uintptr_t>(PagerFixture::table+pager::slots[i],original);
    }
    f.sync();auto* state=pager::install(f.base,plan);
    struct Cleanup {pager::State* state;~Cleanup(){for(size_t i=pager::slots.size();i-->0;)pager::exchange(*state,i,true);}} cleanup{state};
    std::array<uint8_t,0x200> widget{};std::array<uint8_t,128> event{};uintptr_t result{},geometry{};
    const auto w=reinterpret_cast<uintptr_t>(widget.data()),e=reinterpret_cast<uintptr_t>(event.data()),g=reinterpret_cast<uintptr_t>(&geometry);
    input_trace::State output;input_trace::active.store(&output);
    struct OutputCleanup {~OutputCleanup(){input_trace::active=nullptr;}} output_cleanup;
    widget[PagerFixture::enabled]=1;event[PagerFixture::touch]=1;
    const DWORD pointer=7;std::memcpy(event.data()+PagerFixture::pointer,&pointer,sizeof(pointer));
    for(const size_t index:{0u,1u,2u,5u,6u,7u}) {
        const auto returned=reinterpret_cast<pager::Reply>(functions[index])(w,reinterpret_cast<uintptr_t>(&result),g,e);
        check(returned==reinterpret_cast<uintptr_t>(&result)&&result==0x12345678&&forwarded_widget==w&&forwarded_geometry==g&&forwarded_event==e,
            "passive pager observer preserves all arguments and the original reply buffer");
    }
    pager::enter(w,g,e);check(widget[PagerFixture::armed]==1&&forwarded_geometry==g&&forwarded_event==e,"enter observer preserves native state changes");
    pager::leave(w,e);check(widget[PagerFixture::armed]==0&&forwarded_event==e,"leave observer preserves native state changes");
    check(reply_calls==6&&enter_calls==1&&leave_calls==1,"each observer calls its original exactly once");
    check(output.count==16&&output.records[0].kind=='S'&&output.records[0].id==7&&output.records[0].flags==0x01000001&&
        output.records[13].flags==0x01000101,"pager trace captures before/after native state and the event pointer index");
    input_trace::active=nullptr;
    const auto count_before=reply_calls;
    check(reinterpret_cast<pager::Reply>(functions[2])(w,reinterpret_cast<uintptr_t>(&result),g,e)==reinterpret_cast<uintptr_t>(&result)&&
        reply_calls==count_before+1&&output.count==16,"ordinary paging forwards without an active diagnostic recorder");
    state->plan.capture_query=0x4a80;state->plan.capture_return=0x4b10;
    pager::enable_capture_guard(*state);
    struct QueryCleanup {void* target;~QueryCleanup(){MH_DisableHook(target);MH_RemoveHook(target);}} query_cleanup{reinterpret_cast<void*>(f.base+0x4a80)};
    const auto query=reinterpret_cast<bool(*)(uintptr_t)>(f.base+0x4a80);
    const auto native_move_query=reinterpret_cast<bool(*)(uintptr_t)>(f.base+0x4b00);
    check(query(w)&&native_move_query(w),"capture query is unchanged outside a touch-move scope");
    {
        pager::CaptureScope scope(*state,2,w,e);
        check(!native_move_query(w)&&query(w)&&native_move_query(w+8),"only the scoped pager and verified native call site bypass the pre-drag query");
    }
    check(native_move_query(w),"capture scope restores normal query behavior on exit");
    {
        event[PagerFixture::touch]=0;pager::CaptureScope scope(*state,2,w,e);
        check(native_move_query(w),"ordinary mouse movement retains capture query behavior");
    }
    {
        event[PagerFixture::touch]=1;widget[0x132]=1;pager::CaptureScope scope(*state,2,w,e);
        check(native_move_query(w),"already-started native drags retain capture query behavior");widget[0x132]=0;
    }
    {
        pager::CaptureScope scope(*state,5,w,e);
        check(native_move_query(w),"touch start and end retain capture query behavior");
    }
    state->plan.weak_valid=reinterpret_cast<uintptr_t>(&fixture_weak_valid)-f.base;
    state->plan.weak_resolve=reinterpret_cast<uintptr_t>(&fixture_weak_resolve)-f.base;
    state->plan.broadcast_start=reinterpret_cast<uintptr_t>(&fixture_begin)-f.base;
    state->initialize_start=true;widget[0x132]=0;widget[PagerFixture::pressed]=0;
    pager::complete_missing_start(*state,w);
    check(begin_calls==0&&widget[PagerFixture::pressed]==0,"a gesture below the native drag threshold does not synthesize a pager press");
    widget[0x132]=1;pager::complete_missing_start(*state,w);
    check(begin_calls==1&&widget[PagerFixture::pressed]==1&&weak_argument==w+0x144&&
        begin_argument==reinterpret_cast<uintptr_t>(owner_object.data())+0x30,"new native drag initializes the original pager owner/delegate once");
    pager::complete_missing_start(*state,w);
    check(begin_calls==1,"an existing original or completed pager press is not notified twice");
    widget[PagerFixture::pressed]=0;owner_valid=false;pager::complete_missing_start(*state,w);
    check(widget[PagerFixture::pressed]==1&&begin_calls==1,"expired owner preserves the native pressed-state transition without broadcasting");owner_valid=true;
    // Ownership restoration must not overwrite a later writer.
    auto* slot=reinterpret_cast<void**>(f.base+PagerFixture::table+pager::slots[0]);DWORD old{},ignored{};
    VirtualProtect(slot,8,PAGE_READWRITE,&old);*slot=reinterpret_cast<void*>(&fixture_leave);VirtualProtect(slot,8,old,&ignored);
    pager::exchange(*state,0,true);
    check(reinterpret_cast<uintptr_t>(*slot)==reinterpret_cast<uintptr_t>(&fixture_leave),"diagnostic restoration preserves a later slot owner");
}
uintptr_t fixture_list_move(uintptr_t widget,uintptr_t result,uintptr_t geometry,uintptr_t event) {
    const float offset=12.5f;std::memcpy(reinterpret_cast<void*>(widget+0x180),&offset,sizeof(offset));
    return fixture_reply(widget,result,geometry,event);
}
void list_tests() {
    ListFixture f;auto module=f.snapshot();const auto pager_plan=pager::discover(module);auto plan=pager::discover_list(module,pager_plan);
    check(plan.input.vtable==ListFixture::list_table&&plan.input.size==0x300&&plan.input.pressed==0x160&&
        plan.scroll_offset==0x180&&plan.geometry_scale==8,"list metrics derive shared base, allocation, pressed flag and scalar scroll getter");
    {
        ListFixture bad;bad.text(ListFixture::list_name,"OtherScrollType");bad.finish();auto image=bad.snapshot();
        rejects([&]{pager::discover_list(image,pager_plan);},"missing list type rejected");
    }
    {
        ListFixture bad;bad.rel(0x4e01,0x4e05,0x4c80);bad.finish();auto image=bad.snapshot();
        const auto bad_pager=pager::discover(image);
        rejects([&]{pager::discover_list(image,bad_pager);},"unrelated list base constructor rejected");
    }
    plan.input.originals[2]=reinterpret_cast<uintptr_t>(&fixture_list_move)-f.base;
    f.put<uintptr_t>(ListFixture::list_table+0x70,reinterpret_cast<uintptr_t>(&fixture_list_move));f.sync();
    auto* state=pager::install_list(f.base,plan);
    struct Cleanup {pager::ListState* state;~Cleanup(){pager::exchange(state->input,2,true,reinterpret_cast<uintptr_t>(&pager::list_move));}} cleanup{state};
    input_trace::State output;input_trace::active=&output;
    struct OutputCleanup {~OutputCleanup(){input_trace::active=nullptr;}} output_cleanup;
    std::array<uint8_t,0x300> widget{};std::array<uint8_t,128> event{};std::array<uint8_t,32> geometry{};uintptr_t result{};
    const float scale=0.5f;std::memcpy(geometry.data()+8,&scale,sizeof(scale));event[PagerFixture::touch]=1;
    const auto w=reinterpret_cast<uintptr_t>(widget.data()),e=reinterpret_cast<uintptr_t>(event.data()),g=reinterpret_cast<uintptr_t>(geometry.data());
    const auto returned=pager::list_move(w,reinterpret_cast<uintptr_t>(&result),g,e);
    check(returned==reinterpret_cast<uintptr_t>(&result)&&result==0x12345678&&forwarded_widget==w&&forwarded_geometry==g&&forwarded_event==e,
        "list observer preserves original arguments, native state updates and the reply buffer");
    check(output.count==4&&output.records[0].kind=='L'&&output.records[1].kind=='G'&&output.records[1].x==0x3f000000&&
        output.records[1].y==0&&output.records[3].y==0x41480000,"list metrics distinguish geometry scale from before/after native scroll distance");
}
void discovery_tests() {
    InputFixture f;auto module=f.snapshot();const auto plan=input::discover(module);
    check(plan.singleton==InputFixture::singleton&&plan.vtable==InputFixture::slate_vtable&&
        plan.fallback_offset==InputFixture::fallback&&plan.duplicate_cvar==InputFixture::duplicate,
        "input configuration is derived from names and code relationships");
    rejects([&]{dx::decode(module.image(),InputFixture::ctor+22);},"ordinary matching retains strict lock-prefix rejection");
    const auto configured=input::configure(f.base,plan);
    check(configured.fallback==1&&configured.duplicate_before==0&&configured.duplicate_after==0&&configured.thread==GetCurrentThreadId(),"observation records values and calling thread");
    check(f.live<uint8_t>(InputFixture::object+InputFixture::fallback)==1&&f.live<LONG>(InputFixture::duplicate)==0,
        "default observation changes no input setting");
    const auto filtered=input::configure(f.base,plan,input::Mode::FilterPromotedMouse);
    check(filtered.fallback==1&&filtered.duplicate_after==1&&f.live<uint8_t>(InputFixture::object+InputFixture::fallback)==1,
        "duplicate-mouse filter preserves touch fallback");
    const auto again=input::configure(f.base,plan,input::Mode::FilterPromotedMouse);
    check(again.fallback==1&&again.duplicate_before==1,"already configured filter remains valid");
    f.put<uintptr_t>(InputFixture::object,f.base+0x7800);f.sync();
    rejects([&]{input::configure(f.base,plan);},"changed Slate object identity rejected before writes");
    check(f.live<uint8_t>(InputFixture::object+InputFixture::fallback)==1&&f.live<LONG>(InputFixture::duplicate)==0,
        "identity rejection preserves original fields");
    f.put<uintptr_t>(InputFixture::object,f.base+InputFixture::slate_vtable);f.sync();DWORD previous{};
    check(VirtualProtect(reinterpret_cast<void*>(f.base+0x9000),0x1000,PAGE_READONLY,&previous)!=FALSE,"make only fixture CVar storage read-only");
    check(input::configure(f.base,plan).duplicate_after==0,"read-only CVar can be observed without protection changes");
    rejects([&]{input::configure(f.base,plan,input::Mode::FilterPromotedMouse);},"read-only filter rejected without changing protections");
    check(f.live<uint8_t>(InputFixture::object+InputFixture::fallback)==1,"writability rejection occurs before the fallback write");
    const auto malformed=[](auto change,const char* reason) {
        InputFixture bad;change(bad);bad.finish();auto image=bad.snapshot();rejects([&]{input::discover(image);},reason);
    };
    malformed([](InputFixture& bad){bad.wide(InputFixture::key,L"NotTheTouchFallbackName");},"missing fallback name rejected");
    malformed([](InputFixture& bad){bad.wide(0x7800,L"bTouchFallbackToMouse");},"duplicate fallback name rejected");
    malformed([](InputFixture& bad){bad.put<uint8_t>(InputFixture::ctor+20,0);},"unvalidated fallback default rejected");
    malformed([](InputFixture& bad){bad.rel(InputFixture::registration+3,InputFixture::registration+7,0x7900);},"read-only CVar storage rejected");
    malformed([](InputFixture& bad){
        bad.emit(0x3d00,{0xe8,0,0,0,0,0xc3});bad.rel(0x3d01,0x3d05,InputFixture::ctor);
        bad.functions.emplace_back(0x3d00,0x3d06);
    },"multiple Slate factories rejected");
}
std::atomic<unsigned> mouse_down{},mouse_up{},mouse_move{};
std::atomic<bool> replacing_window{};
LRESULT CALLBACK fixture_window(HWND window,UINT message,WPARAM wparam,LPARAM lparam) {
    if(message==WM_LBUTTONDOWN)++mouse_down;
    if(message==WM_LBUTTONUP)++mouse_up;
    if(message==WM_MOUSEMOVE)++mouse_move;
    if(message==WM_DESTROY){if(!replacing_window.load())PostQuitMessage(0);return 0;}
    return DefWindowProcW(window,message,wparam,lparam);
}
void window_thread_test() {
    InputFixture f;auto module=f.snapshot();const auto plan=input::discover(module);
    std::promise<std::pair<HWND,HWND>> ready;auto future=ready.get_future();
    std::thread thread([&]{
        WNDCLASSW type{};type.hInstance=GetModuleHandleW(nullptr);type.lpfnWndProc=fixture_window;type.lpszClassName=L"TouchUILaunchNteInputFixture";
        if(!RegisterClassW(&type)){ready.set_value({nullptr,nullptr});return;}
        auto game_type=type;game_type.lpszClassName=L"UnrealWindow";
        if(!RegisterClassW(&game_type)){ready.set_value({nullptr,nullptr});UnregisterClassW(type.lpszClassName,type.hInstance);return;}
        const auto window=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,type.lpszClassName,L"",WS_POPUP,0,0,16,16,nullptr,nullptr,type.hInstance,nullptr);
        const auto replacement=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,game_type.lpszClassName,L"",WS_POPUP|WS_VISIBLE,
            -32000,-32000,1,1,nullptr,nullptr,type.hInstance,nullptr);
        ready.set_value({window,replacement});
        if(window){MSG message{};while(GetMessageW(&message,nullptr,0,0)>0){TranslateMessage(&message);DispatchMessageW(&message);}}
        if(IsWindow(replacement))DestroyWindow(replacement);
        if(IsWindow(window))DestroyWindow(window);
        UnregisterClassW(game_type.lpszClassName,type.hInstance);
        UnregisterClassW(type.lpszClassName,type.hInstance);
    });
    const auto [window,replacement]=future.get();
    struct Cleanup {HWND first,last;std::thread& thread;~Cleanup(){if(IsWindow(last))PostMessageW(last,WM_CLOSE,0,0);else if(IsWindow(first))PostMessageW(first,WM_CLOSE,0,0);if(thread.joinable())thread.join();}} cleanup{window,replacement,thread};
    check(window!=nullptr&&replacement!=nullptr,"create owned startup and offscreen replacement windows on a separate thread");
    const auto configured=input::install_on_window_thread(f.base,plan,window,input::Mode::FilterPromotedMouse);
    check(configured.thread==GetWindowThreadProcessId(window,nullptr)&&configured.thread!=GetCurrentThreadId(),
        "configuration handshake runs on the real window thread");
    check(f.live<uint8_t>(InputFixture::object+InputFixture::fallback)==1&&f.live<LONG>(InputFixture::duplicate)==1,
        "window-thread filter preserves the fallback setting");
    const auto path=std::filesystem::temp_directory_path()/(L"TouchUILaunch-NTE-input-"+std::to_wstring(GetCurrentProcessId())+L".csv");
    auto* trace=input_trace::start(window,path);
    struct TraceCleanup {
        input_trace::State* state;std::filesystem::path path;
        ~TraceCleanup(){input_trace::finish(*state);CloseHandle(state->writer);std::error_code error;std::filesystem::remove(path,error);}
    } trace_cleanup{trace,path};
    check(PostMessageW(window,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(10,20))&&
        PostMessageW(window,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(30,40))&&
        PostMessageW(window,WM_LBUTTONUP,0,MAKELPARAM(30,40)),"post owned input fixture messages");
    const auto deadline=GetTickCount64()+2000;
    while(mouse_up.load()<1&&GetTickCount64()<deadline)Sleep(1);
    SendMessageW(window,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(20,30));
    SendMessageW(window,WM_LBUTTONUP,0,MAKELPARAM(20,30));
    replacing_window=true;SendMessageW(window,WM_CLOSE,0,0);replacing_window=false;
    const auto rebind_deadline=GetTickCount64()+2000;
    while((trace->window.load()!=replacement||!trace->bound.load())&&!trace->stop.load()&&GetTickCount64()<rebind_deadline)Sleep(1);
    check(trace->window.load()==replacement&&trace->bound.load()&&!trace->stop.load(),"observer survives startup window destruction and binds the owned Unreal replacement");
    SendMessageW(replacement,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(40,50));
    SendMessageW(replacement,WM_LBUTTONUP,0,MAKELPARAM(40,50));
    check(input_trace::finish(*trace),"passive trace flushes and removes its thread hooks");
    check(mouse_down==3&&mouse_up==3&&mouse_move==1,"trace forwards every original message without filtering or duplication");
    std::ifstream trace_file(path);std::string records((std::istreambuf_iterator<char>(trace_file)),{});
    check(records.find(",Q,M,0x201,")!=std::string::npos&&records.find(",D,M,0x202,")!=std::string::npos,
        "trace distinguishes queued input from sent window messages");
    check(records.find("# initial_window=")!=std::string::npos&&records.find("# rebind_window=")!=std::string::npos&&
        records.find("# stopped=requested")!=std::string::npos,"trace records window rebinding and an explicit termination reason");
}
void sample(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);require(file.good(),"cannot open captured module");
    const auto length=file.tellg();require(length>=4096&&length<=0x40000000,"invalid captured module size");
    std::vector<uint8_t> bytes(static_cast<size_t>(length));file.seekg(0);file.read(reinterpret_cast<char*>(bytes.data()),length);
    require(file.good(),"cannot read captured module");ModuleImage module(std::move(bytes),0x140000000);
    const auto plan=input::discover(module);
    std::cout<<"PASS: read-only input discovery; singleton=0x"<<std::hex<<plan.singleton<<"; vtable=0x"<<plan.vtable
        <<"; fallback_offset=0x"<<plan.fallback_offset<<"; duplicate_cvar=0x"<<plan.duplicate_cvar<<"\n";
    const auto pager_plan=pager::discover(module);
    std::cout<<"PASS: read-only pager discovery; vtable=0x"<<std::hex<<pager_plan.vtable<<"; enabled=0x"<<pager_plan.enabled
        <<"; active=0x"<<pager_plan.active<<"; pressed=0x"<<pager_plan.pressed<<"; touch=0x"<<pager_plan.touch<<"; pointer=0x"<<pager_plan.pointer<<"\n";
    const auto list=pager::discover_list(module,pager_plan);
    std::cout<<"PASS: read-only list discovery; vtable=0x"<<std::hex<<list.input.vtable<<"; offset=0x"<<list.scroll_offset
        <<"; scale=0x"<<list.geometry_scale<<"; pressed=0x"<<list.input.pressed<<"\n";
}
}
int wmain(int argc,wchar_t** argv) {
    try {
        if(argc==3&&std::wstring_view(argv[1])==L"--sample"){sample(argv[2]);return 0;}
        require(argc==1,"usage: NteInputTests [--sample <captured module>]");discovery_tests();pager_tests();list_tests();window_thread_test();
        std::cout<<"PASS: "<<checks<<" NTE input configuration checks\n";return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
