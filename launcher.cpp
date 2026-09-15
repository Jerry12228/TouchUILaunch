#include "profile.hpp"
#include "file_probe.hpp"
#include "win_util.hpp"
#include "elevation.hpp"
#include "game.hpp"
#include "mobile_runtime.hpp"
#include "launcher_log.hpp"
#include <tlhelp32.h>
#include <iostream>
#include <optional>
#include <algorithm>

namespace fs=std::filesystem;
bool log_enabled{};
struct Module {uintptr_t base{};fs::path path;};
std::vector<Module> modules(DWORD pid) {
    Handle snap(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid));
    if(snap.value==INVALID_HANDLE_VALUE)return {};
    MODULEENTRY32W entry{};entry.dwSize=sizeof(entry);std::vector<Module> result;
    if(Module32FirstW(snap,&entry))do {result.push_back({reinterpret_cast<uintptr_t>(entry.modBaseAddr),entry.szExePath});}while(Module32NextW(snap,&entry));
    return result;
}
std::optional<Module> find_module(DWORD pid,const fs::path& name) {
    for(auto& m:modules(pid))if(_wcsicmp(m.path.filename().c_str(),name.c_str())==0)return m;
    return {};
}
fs::path process_path(HANDLE process) {
    std::vector<wchar_t> text(32768);DWORD length=static_cast<DWORD>(text.size());
    if(!QueryFullProcessImageNameW(process,0,text.data(),&length))throw std::runtime_error("Cannot read process path");
    return std::wstring(text.data(),length);
}
std::vector<DWORD> find_games(game::Kind kind) {
    Handle snap(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));
    if(snap.value==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot enumerate processes");
    PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);std::vector<DWORD> result;
    if(Process32FirstW(snap,&entry))do {if(game::accepts(kind,entry.szExeFile))result.push_back(entry.th32ProcessID);}while(Process32NextW(snap,&entry));
    return result;
}
void probe_assembly(const fs::path& path) {
    if(log_enabled)std::wcout<<L"Checking GameAssembly: "<<path<<L"\n";
    const auto result=discovery::probe_file(path);
    if(log_enabled)discovery::print_json(std::cout,result);
}
void* remote_load_library(DWORD pid) {
    auto local=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
    HMODULE host{};
    if(!local||!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(local),&host))throw std::runtime_error("Cannot locate LoadLibraryW host module");
    auto remote=find_module(pid,fs::path(module_path(host)).filename());
    if(!remote)throw std::runtime_error("LoadLibraryW host module not present in target");
    const uintptr_t offset=reinterpret_cast<uintptr_t>(local)-reinterpret_cast<uintptr_t>(host);
    return reinterpret_cast<void*>(remote->base+offset);
}
void inject(DWORD pid,const fs::path& payload) {
    if(!fs::is_regular_file(payload))throw std::runtime_error("TouchUILaunch.dll must be beside the launcher");
    Handle process(OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ|SYNCHRONIZE,FALSE,pid));
    if(!process.value)throw std::runtime_error("OpenProcess for injection failed, Windows error "+std::to_string(GetLastError()));
    // Recheck the handle's identity immediately before the mutation (PID reuse).
    if(_wcsicmp(process_path(process).filename().c_str(),L"ZenlessZoneZero.exe")!=0)throw std::runtime_error("Target process identity changed");
    auto assembly=find_module(pid,L"GameAssembly.dll");
    if(!assembly)throw std::runtime_error("GameAssembly module disappeared");
    const auto text=payload.wstring();SIZE_T bytes=(text.size()+1)*sizeof(wchar_t);
    auto loader=remote_load_library(pid);
    void* remote=VirtualAllocEx(process,nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!remote)throw std::runtime_error("VirtualAllocEx failed, Windows error "+std::to_string(GetLastError()));
    SIZE_T written{};
    if(!WriteProcessMemory(process,remote,text.c_str(),bytes,&written)||written!=bytes){VirtualFreeEx(process,remote,0,MEM_RELEASE);throw std::runtime_error("WriteProcessMemory failed");}
    Handle thread(CreateRemoteThread(process,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(loader),remote,0,nullptr));
    if(!thread.value){auto error=GetLastError();VirtualFreeEx(process,remote,0,MEM_RELEASE);throw std::runtime_error("CreateRemoteThread failed, Windows error "+std::to_string(error));}
    DWORD wait=WaitForSingleObject(thread,30000);
    if(wait!=WAIT_OBJECT_0)throw std::runtime_error("LoadLibrary wait did not complete within 30 s. Remote path allocation retained until process exit; do not reinject blindly.");
    VirtualFreeEx(process,remote,0,MEM_RELEASE);
    // GetExitCodeThread truncates a 64-bit HMODULE. Verify the actual module instead.
    auto loaded=find_module(pid,payload.filename());
    if(!loaded||_wcsicmp(loaded->path.c_str(),payload.c_str())!=0)throw std::runtime_error("Payload module was not loaded at the expected path");
    if(log_enabled)std::cout<<"DLL loaded. Initializing input hooks...\n";
}
void usage() {
    std::cout<<"TouchUILaunch (Windows x64, experimental)\n"
        "  --GI | --SR | --ZZZ | --WW             Required: choose exactly one game\n"
        "  --game <exe>                          Required for GI/SR/WW; optional for ZZZ\n"
        "  --WW                                  Launch with -CloudGame -CloudGamePlatform=Android\n"
        "  --ZZZ --probe [--game <exe>]           Verify files only; never launch/inject\n"
        "  --probe-auto                          Compatibility alias for --probe (ZZZ)\n"
        "  --log                                 Console + launcher file logs; ZZZ DLL log\n"
        "  --help                                Show this help\n"
        "Game commands request Windows administrator approval automatically when needed.\n"
        "WW starts with cloud UI arguments after elevation, without injection.\n"
        "--help, --probe and --probe-auto run without requesting elevation.\n"
        "Launch only: exit the selected game first. No attaching to running games.\n"
        "No anti-cheat bypass, driver installation or game-file modification.\n";
}
int wmain(int argc,wchar_t** argv) {
    // Read the logging option before validation, including errors preceding --log.
    for(int i=1;i<argc;++i) {
        if(std::wstring_view(argv[i])==L"--game"&&i+1<argc){++i;continue;}
        if(std::wstring_view(argv[i])==L"--log")log_enabled=true;
    }
    launcher_log::Session diagnostics(log_enabled);
    bool elevation_relaunch{};
    try {
        fs::path own_dir=fs::path(module_path()).parent_path();
        diagnostics.open(own_dir);
        const auto root=fs::weakly_canonical(own_dir/L".."/L"..");
        fs::path game=root/L"Client"/L"3.2"/L"ZenlessZoneZero.exe";
        if(!fs::is_regular_file(game)) {
            for(const auto& relative:{fs::path(L"Client/ZenlessZoneZero Game/ZenlessZoneZero.exe"),fs::path(L"ZenlessZoneZero Game/ZenlessZoneZero.exe")}) {
                if(fs::is_regular_file(root/relative)){game=root/relative;break;}
            }
        }
        fs::path payload=own_dir/L"TouchUILaunch.dll";
        std::wstring action;bool explicit_game{},explicit_action{};
        std::optional<game::Kind> selected;
        std::vector<std::wstring> forwarded_args;
        for(int i=1;i<argc;++i) {
            std::wstring arg=argv[i];
            if(arg==L"--help"||arg==L"-h"){usage();return 0;}
            if(arg==elevation::relaunch_flag){if(elevation_relaunch)throw std::runtime_error("Duplicate elevation marker");elevation_relaunch=true;continue;}
            forwarded_args.push_back(arg);
            if(const auto kind=game::parse(arg)){game::select(selected,*kind);}
            else if(arg==L"--game"&&i+1<argc){if(explicit_game)throw std::runtime_error("Duplicate --game");game=fs::weakly_canonical(fs::absolute(argv[++i]));explicit_game=true;forwarded_args.push_back(game.wstring());}
            else if(arg==L"--probe"||arg==L"--probe-auto") {
                if(explicit_action)throw std::runtime_error("Choose only one action");action=arg.substr(2);explicit_action=true;
            } else if(arg==L"--log") {
                // Already applied above; forwarded unchanged for UAC relaunch.
            } else throw std::runtime_error("Unknown or incomplete argument (use --help)");
        }
        game::validate(selected,game,explicit_game,!action.empty());
        if(log_enabled)std::cout<<"Selected game: "<<launcher_log::utf8(game::flag(*selected))<<"; executable: "<<launcher_log::utf8(game.wstring())<<std::endl;
        if(action==L"probe"||action==L"probe-auto") {
            diagnostics.stage("ZZZ: read-only file probe");
            if(!fs::is_regular_file(game))throw std::runtime_error("Game executable not found; specify --game");
            probe_assembly(game.parent_path()/L"GameAssembly.dll");
            if(!fs::is_regular_file(payload))throw std::runtime_error("Payload DLL missing");
            if(log_enabled)std::wcout<<L"Payload: "<<payload<<L"\nProbe passed. No game process was launched or modified.\n";return 0;
        }
        game::require_stopped(!find_games(*selected).empty());
        if(*selected==game::Kind::ZZZ&&!fs::is_regular_file(payload))throw std::runtime_error("Payload DLL missing");
        // Elevate the launcher before any launch or injection, so the
        // parent cannot perform the action twice. The child verifies its token.
        diagnostics.stage("check administrator privileges");
        if(elevation::needs_relaunch(elevation::is_elevated(),elevation_relaunch)) {
            diagnostics.stage("request administrator privileges");
            forwarded_args.emplace_back(elevation::relaunch_flag);
            if(log_enabled)std::cout<<"Requesting Windows administrator approval...\n"<<std::flush;
            const auto result=elevation::relaunch(module_path(),forwarded_args,fs::current_path().wstring());
            if(log_enabled&&result==ERROR_CANCELLED)std::cerr<<"Administrator approval canceled. No game action was performed.\n";
            if(log_enabled&&result&&result!=ERROR_CANCELLED)std::cerr<<"ERROR: elevated launcher exited with code "<<result<<". See logs/launcher-*.log beside this executable.\n";
            return static_cast<int>(result);
        }
        // Serialize launches of this game type, including launches from other tool copies.
        const auto lock_name=L"Local\\TouchUILaunch.Launch."+std::wstring(game::flag(*selected));
        Handle launch_lock(CreateMutexW(nullptr,FALSE,lock_name.c_str()));
        if(!launch_lock.value)throw std::runtime_error("Cannot create game launch mutex");
        const auto lock_result=WaitForSingleObject(launch_lock,0);
        if(lock_result!=WAIT_OBJECT_0&&lock_result!=WAIT_ABANDONED)throw std::runtime_error("Another launcher is starting this game");
        struct Unlock {HANDLE handle;~Unlock(){ReleaseMutex(handle);}} unlock{launch_lock};
        game::require_stopped(!find_games(*selected).empty());
        diagnostics.stage("create new game process");
        const bool mobile_patch=*selected==game::Kind::GI||*selected==game::Kind::SR;
        const auto arguments=*selected==game::Kind::WW?L"-CloudGame -CloudGamePlatform=Android":L"";
        if(log_enabled&&*selected==game::Kind::WW)std::cout<<"WW launch arguments: "<<launcher_log::utf8(arguments)<<std::endl;
        game::Child child;child.start(game,mobile_patch,arguments);
        const DWORD pid=child.info.dwProcessId;
        if(log_enabled)std::cout<<"Started game, PID "<<pid<<"\n";
        if(*selected==game::Kind::WW) {
            child.release();
            diagnostics.stage("WW: game launched with Android cloud UI arguments");
            return 0;
        }
        if(mobile_patch) {
            mobile::initialize(child,*selected,game,log_enabled,[&](std::string_view stage){diagnostics.stage(stage);});
            diagnostics.stage("resume game main thread");
            child.resume();child.release();
            if(log_enabled)std::cout<<"Game main thread resumed.\n";
            return 0;
        }
        Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,pid));
        if(!process.value)throw std::runtime_error("Cannot query game process, Windows error "+std::to_string(GetLastError()));
        if(_wcsicmp(process_path(process).filename().c_str(),L"ZenlessZoneZero.exe")!=0)throw std::runtime_error("PID does not belong to ZenlessZoneZero.exe");
        diagnostics.stage("ZZZ: wait for GameAssembly.dll");
        auto assembly=find_module(pid,L"GameAssembly.dll");
        if(!assembly&&log_enabled)std::cout<<"Waiting for GameAssembly.dll (up to 120 s)...\n";
        for(int i=0;!assembly&&i<600;++i) {
            if(WaitForSingleObject(process,200)==WAIT_OBJECT_0)throw std::runtime_error("Game exited before GameAssembly loaded");
            assembly=find_module(pid,L"GameAssembly.dll");
        }
        if(!assembly)throw std::runtime_error("GameAssembly module unavailable");
        if(find_module(pid,payload.filename()))throw std::runtime_error("Payload unexpectedly loaded in newly created game");
        // The DLL retains the logging event after this launcher exits.
        Handle logging(CreateEventW(nullptr,TRUE,log_enabled,log_event_name(pid).c_str()));
        if(!logging.value)throw std::runtime_error("Cannot create payload logging event");
        if(!(log_enabled?SetEvent(logging):ResetEvent(logging)))throw std::runtime_error("Cannot configure payload logging");
        diagnostics.stage("ZZZ: load touch payload");inject(pid,payload);
        HANDLE started{};
        for(int i=0;i<200&&!started;++i) {
            started=OpenEventW(SYNCHRONIZE,FALSE,started_event_name(pid).c_str());
            if(!started)Sleep(100);
        }
        Handle started_handle(started);
        if(!started)throw std::runtime_error("Payload startup unavailable; rerun with --log for diagnostics");
        if(log_enabled)std::cout<<"Payload started. Input hooks and UI initialize asynchronously.\n";
        if(log_enabled)std::wcout<<L"Log: "<<own_dir/L"logs"/(L"touch-"+std::to_wstring(pid)+L".log")<<L"\n";
        child.release();
        return 0;
    } catch(const std::exception& ex){diagnostics.failure(ex.what(),elevation_relaunch);return 2;}
}
