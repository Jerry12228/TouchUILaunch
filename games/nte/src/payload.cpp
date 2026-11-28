#include "nte_callbacks.hpp"
#include "nte_launch.hpp"
#include "nte_relay.hpp"
#include "nte_input.hpp"
#include "nte_input_trace.hpp"
#include "nte_pager.hpp"
#include "payload_entry.hpp"
#include <fstream>

namespace {
HMODULE self{};
DWORD WINAPI initialize(void*) {
    using namespace touchui::nte;
    const auto pid=GetCurrentProcessId();
    Handle ready(OpenEventW(EVENT_MODIFY_STATE,FALSE,event_name(pid,L"Ready").c_str()));
    Handle failed(OpenEventW(EVENT_MODIFY_STATE,FALSE,event_name(pid,L"Failed").c_str()));
    // No independent activation: startup is coordinated by our owned launcher.
    if(!ready.value||!failed.value)return 1;
    Handle logging(OpenEventW(SYNCHRONIZE,FALSE,event_name(pid,L"Log").c_str()));
    std::ofstream log;
    touchui::nte::relay::Activation activation;
    EarlyHookBuffer early_buffer;
    try {
        activation=relay::activate(self,[](void* context) {
            static_cast<EarlyHookBuffer*>(context)->reserve(GetModuleHandleW(nullptr));
        },&early_buffer);
        if(activation.bridge)return 0;
        if((activation.session&&activation.session->mapping->logging)||
            (logging.value&&WaitForSingleObject(logging,0)==WAIT_OBJECT_0)) {
            const auto folder=std::filesystem::path(module_path(self)).parent_path()/L"logs";
            std::filesystem::create_directories(folder);
            log.open(folder/(L"nte-"+std::to_wstring(pid)+L".log"),std::ios::app);
            require(log.good(),"cannot create diagnostic log");
            log<<"Early nearby hook buffer="<<(early_buffer.slot()?"reserved":"unavailable")<<"\n"<<std::flush;
        }
        if(activation.configured) {
            if(log.is_open())log<<"Early coordinated load confirmed; waiting for Unreal window before module capture\n"<<std::flush;
            const auto deadline=GetTickCount64()+120000;
            while(!window_ready(pid)) {
                require(activation.session->alive(),"NTE launch coordinator exited or canceled");
                require(GetTickCount64()<deadline,"Unreal window unavailable within 120 s");Sleep(100);
            }
        }
        if(log.is_open())log<<"Experimental NTE mobile UI initialization; capturing loaded module once\n"<<std::flush;
        const auto module=capture_main_module();
        auto plan=discover_static(*module);
        if(log.is_open())log<<"Static discovery passed; waiting for reflection caches\n"<<std::flush;
        bool bound{};
        for(unsigned attempt=0;attempt<300;++attempt) {
            try {bind_metadata(plan,module->base);bound=true;break;}
            catch(const MetadataPending& error) {
                if(attempt==0&&log.is_open())log<<error.what()<<"\n"<<std::flush;
                Sleep(100);
            }
        }
        require(bound,"reflection initialization did not complete within 30 s");
        if(log.is_open())log<<"Resolved native predicate RVA=0x"<<std::hex<<plan.mobile_predicate
            <<"; navigation slot RVA=0x"<<plan.navigation_slot
            <<"; native installation="<<(plan.native_patch==NativePatch::SavedRegisterDetour?"saved-register detour":"atomic stack entry")<<"\n"<<std::flush;
        // A callback can run as soon as its slot changes. Publish originals and
        // pin this DLL before any pointer/code becomes visible to the game.
        const auto callbacks=prepare_callbacks(module->base,plan);
        HMODULE pinned{};
        require(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&initialize),&pinned)!=FALSE,"cannot pin callback DLL");
        Hooks hooks;
        hooks.install(*module,plan,callbacks);
        // Input fixes are part of ordinary NTE startup. Logging only controls
        // observers; it must not select a different touch implementation.
        const auto input_plan=input::discover(*module);
        const auto result=input::install_on_window_thread(module->base,input_plan,game_window(pid),input::Mode::FilterPromotedMouse);
        if(log.is_open())log<<"Window-thread native input: fallback preserved="<<unsigned(result.fallback)
            <<"; duplicate mouse filter "<<result.duplicate_before<<" -> "<<result.duplicate_after
            <<"; thread="<<std::dec<<result.thread<<"\n"<<std::flush;
        if(log.is_open()) {
            try {
                const auto trace_path=std::filesystem::path(module_path(self)).parent_path()/L"logs"/(L"nte-input-"+std::to_wstring(pid)+L".csv");
                input_trace::start(game_window(pid),trace_path);
                log<<"Passive own-window input trace enabled (bounded; no input replacement).\n"<<std::flush;
            }catch(const std::exception& error){log<<"Input trace unavailable: "<<error.what()<<"\n"<<std::flush;}
        }
        const auto pager_plan=pager::discover(*module);
        auto* pager_state=pager::install(module->base,pager_plan);
        pager::enable_capture_guard(*pager_state);
        pager_state->initialize_start.store(true,std::memory_order_release);
        if(log.is_open()) {
            log<<"Phone paging input installed; table RVA=0x"<<std::hex<<pager_plan.vtable
                <<"; native capture/release retained. First swipe after opening remains a known limitation.\n"<<std::flush;
            try {
                const auto list=pager::discover_list(*module,pager_plan);
                pager::install_list(module->base,list);
                log<<"Passive list movement metrics installed; table RVA=0x"<<std::hex<<list.input.vtable
                    <<"; scroll offset=0x"<<list.scroll_offset<<"; geometry scale=0x"<<list.geometry_scale<<".\n"<<std::flush;
            }catch(const std::exception& error){log<<"List metrics unavailable: "<<error.what()<<"\n"<<std::flush;}
        }
        if(log.is_open())log<<"Installed mobile predicates and Touch navigation request. Actual UI/input behavior requires in-game verification.\n"<<std::flush;
        require(SetEvent(ready)!=FALSE,"cannot signal successful installation");
        if(activation.session) {
            InterlockedExchange(&activation.session->mapping->state,relay::Ready);
            activation.session->notify();
        }
        return 0;
    } catch(const std::exception& error) {
        if(log.is_open())log<<"ERROR: "<<error.what()<<"\n"<<std::flush;
        if(activation.session)activation.session->fail(error.what());
        SetEvent(failed);return 1;
    }
}
}
DWORD touchui::nte::run_payload(HMODULE module) {
    self=module;
    return initialize(nullptr);
}
