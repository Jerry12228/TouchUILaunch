#include "profile.hpp"
#include "file_probe.hpp"
#include "win_util.hpp"
#include "elevation.hpp"
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
std::vector<DWORD> find_games() {
    Handle snap(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));
    if(snap.value==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot enumerate processes");
    PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);std::vector<DWORD> result;
    if(Process32FirstW(snap,&entry))do {if(_wcsicmp(entry.szExeFile,L"ZenlessZoneZero.exe")==0)result.push_back(entry.th32ProcessID);}while(Process32NextW(snap,&entry));
    return result;
}
void probe_assembly(const fs::path& path) {
    if(log_enabled)std::wcout<<L"Checking GameAssembly: "<<path<<L"\n";
    const auto result=discovery::probe_file(path);
    if(log_enabled)discovery::print_json(std::cout,result);
}
DWORD start_game(const fs::path& path) {
    if(_wcsicmp(path.filename().c_str(),L"ZenlessZoneZero.exe")!=0||!fs::is_regular_file(path))throw std::runtime_error("--game must name an existing ZenlessZoneZero.exe");
    std::wstring command=L"\""+path.wstring()+L"\"";
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION info{};
    if(!CreateProcessW(path.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,path.parent_path().c_str(),&startup,&info))throw std::runtime_error("CreateProcess failed, Windows error "+std::to_string(GetLastError()));
    Handle thread(info.hThread);Handle process(info.hProcess);
    if(log_enabled)std::cout<<"Started game, PID "<<info.dwProcessId<<"\n";return info.dwProcessId;
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
    if(!fs::is_regular_file(payload))throw std::runtime_error("ZZZTouchUI.dll must be beside the launcher");
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
    std::cout<<"ZZZTouchLauncher (Windows x64, experimental)\n"
        "  --probe [--game <ZenlessZoneZero.exe>]   Verify files only; never launch/inject\n"
        "  --probe-auto [--game <path>]            Compatibility alias for --probe\n"
        "  [--game <path>]                       Attach, or launch if no game is running\n"
        "  --log                                 Enable console and DLL file logs\n"
        "  --help                                Show this help\n"
        "Game commands request Windows administrator approval automatically when needed.\n"
        "--help, --probe and --probe-auto run without requesting elevation.\n"
        "No anti-cheat bypass, driver installation or game-file modification.\n";
}
int wmain(int argc,wchar_t** argv) {
    // Read the logging option before validation, including errors preceding --log.
    for(int i=1;i<argc;++i) {
        if(std::wstring_view(argv[i])==L"--game"&&i+1<argc){++i;continue;}
        if(std::wstring_view(argv[i])==L"--log")log_enabled=true;
    }
    try {
        fs::path own_dir=fs::path(module_path()).parent_path();
        const auto root=fs::weakly_canonical(own_dir/L".."/L"..");
        fs::path game=root/L"Client"/L"3.2"/L"ZenlessZoneZero.exe";
        if(!fs::is_regular_file(game)) {
            for(const auto& relative:{fs::path(L"Client/ZenlessZoneZero Game/ZenlessZoneZero.exe"),fs::path(L"ZenlessZoneZero Game/ZenlessZoneZero.exe")}) {
                if(fs::is_regular_file(root/relative)){game=root/relative;break;}
            }
        }
        fs::path payload=own_dir/L"ZZZTouchUI.dll";
        std::wstring action;bool explicit_game{},explicit_action{},elevation_relaunch{};
        std::vector<std::wstring> forwarded_args;
        for(int i=1;i<argc;++i) {
            std::wstring arg=argv[i];
            if(arg==L"--help"||arg==L"-h"){usage();return 0;}
            if(arg==elevation::relaunch_flag){if(elevation_relaunch)throw std::runtime_error("Duplicate elevation marker");elevation_relaunch=true;continue;}
            forwarded_args.push_back(arg);
            if(arg==L"--game"&&i+1<argc){game=fs::weakly_canonical(fs::absolute(argv[++i]));explicit_game=true;forwarded_args.push_back(game.wstring());}
            else if(arg==L"--probe"||arg==L"--probe-auto") {
                if(explicit_action)throw std::runtime_error("Choose only one action");action=arg.substr(2);explicit_action=true;
            } else if(arg==L"--log") {
                // Already applied above; forwarded unchanged for UAC relaunch.
            } else throw std::runtime_error("Unknown or incomplete argument (use --help)");
        }
        if(action==L"probe"||action==L"probe-auto") {
            if(!fs::is_regular_file(game))throw std::runtime_error("Game executable not found; specify --game");
            probe_assembly(game.parent_path()/L"GameAssembly.dll");
            if(!fs::is_regular_file(payload))throw std::runtime_error("Payload DLL missing");
            if(log_enabled)std::wcout<<L"Payload: "<<payload<<L"\nProbe passed. No game process was launched or modified.\n";return 0;
        }
        // Elevate the launcher before any game lookup, launch or injection, so the
        // parent cannot perform the action twice. The child verifies its token.
        if(elevation::needs_relaunch(elevation::is_elevated(),elevation_relaunch)) {
            forwarded_args.emplace_back(elevation::relaunch_flag);
            if(log_enabled)std::cout<<"Requesting Windows administrator approval...\n"<<std::flush;
            const auto result=elevation::relaunch(module_path(),forwarded_args,fs::current_path().wstring());
            if(log_enabled&&result==ERROR_CANCELLED)std::cerr<<"Administrator approval canceled. No game action was performed.\n";
            return static_cast<int>(result);
        }
        auto games=find_games();
        if(explicit_game)std::erase_if(games,[&](DWORD candidate){Handle p(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,candidate));return !p.value||_wcsicmp(process_path(p).c_str(),game.c_str())!=0;});
        if(games.size()>1)throw std::runtime_error("Multiple game processes found; specify --game or close extra instances");
        const DWORD pid=games.empty()?start_game(game):games.front();
        Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,pid));
        if(!process.value)throw std::runtime_error("Cannot query game process, Windows error "+std::to_string(GetLastError()));
        if(_wcsicmp(process_path(process).filename().c_str(),L"ZenlessZoneZero.exe")!=0)throw std::runtime_error("PID does not belong to ZenlessZoneZero.exe");
        auto assembly=find_module(pid,L"GameAssembly.dll");
        if(!assembly&&log_enabled)std::cout<<"Waiting for GameAssembly.dll (up to 120 s)...\n";
        for(int i=0;!assembly&&i<600;++i) {
            if(WaitForSingleObject(process,200)==WAIT_OBJECT_0)throw std::runtime_error("Game exited before GameAssembly loaded");
            assembly=find_module(pid,L"GameAssembly.dll");
        }
        if(!assembly)throw std::runtime_error("GameAssembly module unavailable");
        auto loaded=find_module(pid,payload.filename());
        if(loaded&&_wcsicmp(loaded->path.c_str(),payload.c_str())!=0)throw std::runtime_error("Another ZZZTouchUI.dll is already loaded; restart game before using this build");
        // The DLL retains this event after startup, so the latest launcher invocation
        // controls logging even after this process exits (including an existing DLL).
        Handle logging(CreateEventW(nullptr,TRUE,log_enabled,log_event_name(pid).c_str()));
        if(!logging.value)throw std::runtime_error("Cannot create payload logging event");
        if(!(log_enabled?SetEvent(logging):ResetEvent(logging)))throw std::runtime_error("Cannot configure payload logging");
        if(!loaded)inject(pid,payload);
        HANDLE started{};
        for(int i=0;i<200&&!started;++i) {
            started=OpenEventW(SYNCHRONIZE,FALSE,started_event_name(pid).c_str());
            if(!started)Sleep(100);
        }
        Handle started_handle(started);
        if(!started)throw std::runtime_error("Payload startup unavailable; rerun with --log for diagnostics");
        if(log_enabled)std::cout<<"Payload started. Input hooks and UI initialize asynchronously.\n";
        if(log_enabled)std::wcout<<L"Log: "<<own_dir/L"logs"/(L"touch-"+std::to_wstring(pid)+L".log")<<L"\n";
        return 0;
    } catch(const std::exception& ex){if(log_enabled)std::cerr<<"ERROR: "<<ex.what()<<"\n";return 2;}
}
