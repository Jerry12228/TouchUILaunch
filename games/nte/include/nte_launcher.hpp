#pragma once
#include "nte_relay.hpp"
#include "game.hpp"
#include "nte_startup.hpp"
#include "nte_login_host.hpp"
#include <functional>
#include <fstream>

namespace touchui::nte {
inline std::filesystem::path launcher_for(const std::filesystem::path& game,const std::filesystem::path& requested={}) {
    auto launcher=requested;
    if(launcher.empty()) {
        for(auto folder=game.parent_path();folder!=folder.root_path();folder=folder.parent_path()) {
            for(const auto name:{L"NTEGlobalLauncher.exe",L"NTEGlobalGame.exe",L"NTEGame.exe",
                L"NTEGlobal/NTEGlobalGame.exe",L"NTELauncher/NTEGame.exe"}) {
                if(std::filesystem::is_regular_file(folder/name)){launcher=folder/name;break;}
            }
            if(!launcher.empty())break;
        }
    }
    loader::require(!launcher.empty()&&std::filesystem::is_regular_file(launcher)&&relay::launcher_name(launcher),
        "Official NTE launcher not found; specify --nte-launcher <NTEGlobalLauncher.exe or NTEGlobalGame.exe>");
    // Validate before UAC. No executing the file to discover its architecture.
    std::ifstream file(launcher,std::ios::binary);IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
    file.read(reinterpret_cast<char*>(&dos),sizeof(dos));
    loader::require(file.good()&&dos.e_magic==IMAGE_DOS_SIGNATURE&&dos.e_lfanew>=sizeof(dos)&&dos.e_lfanew<0x100000,
        "Invalid NTE launcher PE header");
    file.seekg(dos.e_lfanew);file.read(reinterpret_cast<char*>(&nt),sizeof(nt));
    loader::require(file.good()&&nt.Signature==IMAGE_NT_SIGNATURE&&nt.FileHeader.Machine==IMAGE_FILE_MACHINE_AMD64&&
        nt.OptionalHeader.Magic==IMAGE_NT_OPTIONAL_HDR64_MAGIC,"Official NTE launcher must be Windows x64");
    return launcher;
}
inline std::filesystem::path installation_root(const std::filesystem::path& game,const std::filesystem::path& launcher) {
    auto root=launcher.parent_path();
    while(!relay::within(game,root)&&root!=root.root_path())root=root.parent_path();
    loader::require(root!=root.root_path(),"NTE launcher and client must share an installation directory");
    return root;
}
inline void require_launcher_stopped() {
    Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));
    loader::require(snapshot.value!=INVALID_HANDLE_VALUE,"Cannot enumerate NTE launcher processes");
    PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);
    loader::require(Process32FirstW(snapshot,&entry)!=FALSE,"Cannot inspect NTE launcher processes");
    do {
        loader::require(!relay::launcher_name(entry.szExeFile),
            "NTE official launcher is already running. Exit it first; existing launchers are not injected.");
    }while(Process32NextW(snapshot,&entry));
    loader::require(GetLastError()==ERROR_NO_MORE_FILES,"NTE launcher process enumeration failed");
}
inline void launch_mobile(const std::filesystem::path& game_path,const std::filesystem::path& launcher,
    const std::filesystem::path& payload,bool logging,std::wstring_view arguments,const std::function<void(std::string_view)>& stage,
    bool direct=false,const LoginHostPlan* host=nullptr) {
    if(!direct||host)require_launcher_stopped();
    const auto key=L"Local\\TouchUILaunch.NTE.Session."+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(GetTickCount64());
    relay::Session session(key,true);
    relay::copy(session.mapping->game,game_path.wstring());
    relay::copy(session.mapping->root,(host?host->root:direct?game_path.parent_path():installation_root(game_path,launcher)).wstring());
    if(host)relay::copy(session.mapping->login_host,host->executable.wstring());
    relay::copy(session.mapping->payload,payload.wstring());session.mapping->logging=logging;
    InterlockedExchange(&session.mapping->active,1);
    struct Stop {relay::Session& session;~Stop(){InterlockedExchange(&session.mapping->active,0);}} stop{session};
    struct OwnedJob {
        Handle handle{CreateJobObjectW(nullptr,nullptr)};bool released{};
        ~OwnedJob(){if(handle.value&&!released)TerminateJobObject(handle,2);}
    } job;
    loader::require(job.handle.value!=nullptr,"Cannot create owned NTE launch job");
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    loader::require(SetInformationJobObject(job.handle,JobObjectExtendedLimitInformation,&limits,sizeof(limits))!=FALSE,
        "Cannot configure owned NTE launch cleanup");
    stage(host?"NTE: create independent SDK login host suspended":direct?"NTE: create game directly with main thread suspended":"NTE: create official launcher suspended");
    if(direct&&!host)stage("NTE: international startup map Updater_P; saved directory suffix Global");
    std::vector<wchar_t> environment;
    if(direct&&!host) {
        environment=sdk_environment(sdk_for(game_path));
        stage("NTE: configured child-only ArcGameSDKSharePath for the installed client SDK");
    }
    game::Child child;child.start(host?host->executable:direct?game_path:launcher,true,
        host?host->arguments(arguments):direct?direct_arguments(arguments):std::wstring(arguments),
        direct&&!host?environment.data():nullptr);
    loader::require(AssignProcessToJobObject(job.handle,child.info.hProcess)!=FALSE,"Cannot track owned NTE launcher descendants");
    stage(host?"NTE: load CreateProcessW relay in owned SDK host":direct?"NTE: early-load mobile UI DLL in owned game":"NTE: load CreateProcessW relay in owned official launcher");
    relay::Startup startup(child.info.dwProcessId,session,direct&&!host?relay::Role::Game:relay::Role::Bridge);
    startup.load(child.info.hProcess,payload);
    if(direct&&!host) {
        HANDLE duplicate{};
        loader::require(DuplicateHandle(GetCurrentProcess(),child.info.hProcess,GetCurrentProcess(),&duplicate,
            SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,0),"Cannot retain owned NTE game observation handle");
        InterlockedExchange64(&session.mapping->game_handle,reinterpret_cast<LONG64>(duplicate));
        InterlockedExchange(&session.mapping->game_pid,static_cast<LONG>(child.info.dwProcessId));
        InterlockedExchange(&session.mapping->state,relay::Mapped);
        stage("NTE: direct DLL mapping and startup verified; resume game main thread");
    } else stage(host?"NTE: independent login host owns SDK session and game creation":"NTE: relay installed; start the game from the official launcher (up to 5 min)");
    child.resume();
    auto deadline=GetTickCount64()+300000;bool mapped{};Handle game_process;
    for(;;) {
        ResetEvent(session.changed);
        if(!game_process.value)game_process.value=reinterpret_cast<HANDLE>(InterlockedExchange64(&session.mapping->game_handle,0));
        const auto state=relay::read(session.mapping->state);
        if(state==relay::Failed)throw std::runtime_error(std::string("NTE relay: ")+session.mapping->error);
        if(game_process.value&&WaitForSingleObject(game_process,0)==WAIT_OBJECT_0) {
            DWORD code{};GetExitCodeProcess(game_process,&code);
            throw std::runtime_error("Owned NTE game exited before UI acceptance, status="+loader::hex(code));
        }
        if(state==relay::Ready) {
            limits.BasicLimitInformation.LimitFlags=0;
            loader::require(SetInformationJobObject(job.handle,JobObjectExtendedLimitInformation,&limits,sizeof(limits))!=FALSE,
                "Cannot release owned NTE launch cleanup after successful initialization");
            stage("NTE: mobile predicates installed; navigation requests Touch (experimental)");
            if(host)stage("NTE: SDK login host remains active; UI Ready does not confirm login or world entry");
            job.released=true;child.release();return;
        }
        if(state==relay::Mapped&&!mapped) {
            mapped=true;deadline=GetTickCount64()+150000;
            stage("NTE: verified DLL in owned game; waiting for UI initialization");
        }
        JOBOBJECT_BASIC_ACCOUNTING_INFORMATION accounting{};
        loader::require(QueryInformationJobObject(job.handle,JobObjectBasicAccountingInformation,&accounting,sizeof(accounting),nullptr)!=FALSE,
            "Cannot query owned NTE launcher descendants");
        loader::require(accounting.ActiveProcesses!=0,"Owned NTE launcher/game tree exited before UI initialization");
        loader::require(GetTickCount64()<deadline,mapped?"NTE UI initialization timed out":"No game launch observed from owned NTE launcher within 5 min");
        WaitForSingleObject(session.changed,200);
    }
}
}
