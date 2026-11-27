#include "nte_relay.hpp"
#include "MinHook.h"
#include <fstream>
#include <mutex>

namespace touchui::nte::relay {
namespace {
using CreateProcess=decltype(&CreateProcessW);
CreateProcess original{};
std::unique_ptr<Session> session;
std::ofstream log;
std::mutex log_mutex;
void record(const std::string& message) {
    std::lock_guard lock(log_mutex);
    if(log.is_open())log<<message<<"\n"<<std::flush;
}
BOOL WINAPI create_process(LPCWSTR application,LPWSTR command,LPSECURITY_ATTRIBUTES process_attributes,
    LPSECURITY_ATTRIBUTES thread_attributes,BOOL inherit,DWORD flags,LPVOID environment,LPCWSTR directory,
    LPSTARTUPINFOW startup,LPPROCESS_INFORMATION information) {
    // Our original trampoline is published before the hook becomes callable.
    if(!session->alive()||!information)return original(application,command,process_attributes,thread_attributes,
        inherit,flags,environment,directory,startup,information);
    const auto result=original(application,command,process_attributes,thread_attributes,inherit,
        flags|CREATE_SUSPENDED,environment,directory,startup,information);
    const auto saved_error=GetLastError();
    if(!result){SetLastError(saved_error);return FALSE;}
    try {
        const auto path=image_path(information->hProcess);
        const auto role=classify(session->mapping.data(),path);
        if(role) {
            loader::require(session->alive(),"NTE relay session ended during child creation");
            if(*role==Role::Game)loader::require(InterlockedCompareExchange(&session->mapping->game_pid,
                static_cast<LONG>(information->dwProcessId),0)==0,"NTE relay already owns a game launch");
            record("New owned child PID="+std::to_string(information->dwProcessId)+
                (*role==Role::Game?" role=game":" role=launcher"));
            Startup context(information->dwProcessId,*session,*role);
            context.load(information->hProcess,session->mapping->payload);
            record("Verified DLL mapping and startup handshake for PID="+std::to_string(information->dwProcessId));
            if(*role==Role::Game) {
                Handle controller(OpenProcess(PROCESS_DUP_HANDLE,FALSE,session->mapping->controller_pid));HANDLE duplicate{};
                loader::require(controller.value&&DuplicateHandle(GetCurrentProcess(),information->hProcess,controller,&duplicate,
                    SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,0),"Cannot transfer owned game observation handle");
                InterlockedExchange64(&session->mapping->game_handle,reinterpret_cast<LONG64>(duplicate));
                InterlockedCompareExchange(&session->mapping->state,Mapped,Waiting);session->notify();
            }
        }
        // Respect a suspension explicitly requested by the official launcher.
        // Remove exactly the one count added here for a normal child.
        if(!(flags&CREATE_SUSPENDED))loader::require(ResumeThread(information->hThread)==1,"NTE relay could not resume owned child");
        SetLastError(saved_error);return TRUE;
    } catch(const std::exception& error) {
        record(std::string("ERROR: ")+error.what());session->fail(error.what());
    } catch(...) {session->fail("Unexpected NTE relay child initialization failure");}
    // No handles escape on a failed CreateProcess call. Only this callback's
    // newly-created child is terminated; never reopen a discovered game PID.
    TerminateProcess(information->hProcess,2);WaitForSingleObject(information->hProcess,5000);
    CloseHandle(information->hThread);CloseHandle(information->hProcess);*information={};
    SetLastError(ERROR_DLL_INIT_FAILED);return FALSE;
}
void install(HMODULE self,std::unique_ptr<Session> context) {
    session=std::move(context);
    if(session->mapping->logging) {
        const auto folder=std::filesystem::path(module_path(self)).parent_path()/L"logs";
        std::filesystem::create_directories(folder);
        log.open(folder/(L"nte-relay-"+std::to_wstring(GetCurrentProcessId())+L".log"),std::ios::app);
        loader::require(log.good(),"Cannot create NTE relay log");
    }
    HMODULE pinned{};
    loader::require(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&create_process),&pinned)!=FALSE,"Cannot pin NTE relay callbacks");
    loader::require(MH_Initialize()==MH_OK,"Cannot initialize NTE process hook");
    const auto target=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"CreateProcessW");
    loader::require(target&&MH_CreateHook(reinterpret_cast<void*>(target),reinterpret_cast<void*>(&create_process),
        reinterpret_cast<void**>(&original))==MH_OK,"Cannot prepare CreateProcessW trampoline");
    loader::require(MH_EnableHook(reinterpret_cast<void*>(target))==MH_OK,"Cannot enable NTE process hook");
    record("CreateProcessW relay ready; only new children with configured paths qualify");
}
}
Activation activate(HMODULE self) {
    const auto pid=GetCurrentProcessId();
    Handle probe(OpenFileMappingW(FILE_MAP_READ,FALSE,event_name(pid,L"Config").c_str()));
    if(!probe.value)return {}; // Legacy owned fixture startup events remain supported.
    Handle ready(OpenEventW(EVENT_MODIFY_STATE,FALSE,event_name(pid,L"BootReady").c_str()));
    Handle failed(OpenEventW(EVENT_MODIFY_STATE,FALSE,event_name(pid,L"BootFailed").c_str()));
    std::unique_ptr<Session> context;
    try {
        loader::require(ready.value&&failed.value,"Missing NTE relay startup events");
        Mapping<Config> config(event_name(pid,L"Config"),false);
        loader::require(config->magic==protocol&&config->session[191]==0,"Invalid NTE relay activation");
        context=std::make_unique<Session>(config->session,false);
        loader::require(context->alive(),"Inactive NTE relay controller");
        const auto role=classify(context->mapping.data(),image_path(GetCurrentProcess()));
        loader::require(role&&*role==config->role,"NTE relay process path/role mismatch");
        const auto bridge=*role==Role::Bridge;
        if(bridge)install(self,std::move(context));
        loader::require(SetEvent(ready)!=FALSE,"Cannot signal NTE relay startup");
        return {true,bridge,std::move(context)};
    } catch(const std::exception& error) {
        if(context)context->fail(error.what());
        else if(session)session->fail(error.what());
        if(failed.value)SetEvent(failed);throw;
    }
}
}
