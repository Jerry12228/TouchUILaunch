#include "nte_relay.hpp"
#include "nte_launcher.hpp"
#include "game.hpp"
#include <fstream>
#include <iostream>

using namespace touchui::nte;
namespace fs=std::filesystem;
namespace {
unsigned checks{};
void check(bool condition,const char* message){++checks;if(!condition)throw std::runtime_error(message);}
std::wstring quote(const fs::path& path){return L"\""+path.wstring()+L"\"";}
int fixture(int argc,wchar_t** argv) {
    check(argc==5,"fixture arguments");
    const std::wstring mode=argv[2];const fs::path folder=argv[3],report=argv[4];
    const auto name=fs::path(module_path()).filename();
    if(name==L"HTGame.exe"||name==L"Other.exe") {
        const bool loaded=GetModuleHandleW(payload_filename)!=nullptr;
        check(loaded==(name==L"HTGame.exe"&&mode!=L"inactive"),"leaf payload identity");
        wchar_t value[64]{};
        if(GetEnvironmentVariableW(L"TouchUILaunch.ManagedFixture",value,64))
            check(std::wstring_view(value)==L"Unicode 中文","managed Process.Start preserves child Unicode environment");
        if(mode==L"direct-failure") {
            std::ofstream(report)<<GetCurrentProcessId()<<"\n";
            WNDCLASSW klass{};klass.hInstance=GetModuleHandleW(nullptr);klass.lpfnWndProc=DefWindowProcW;klass.lpszClassName=L"UnrealWindow";
            check(RegisterClassW(&klass)!=0,"register owned synthetic Unreal window");
            check(CreateWindowExW(0,klass.lpszClassName,L"NTE owned failure fixture",WS_POPUP|WS_VISIBLE,
                -32000,-32000,32,32,nullptr,nullptr,klass.hInstance,nullptr)!=nullptr,"create owned synthetic Unreal window");
            Sleep(10000);return 1; // The real worker must reject this non-UE host and the controller must clean it.
        }
        std::ofstream(report)<<"PASS: leaf mapping verified\n";return 0;
    }
    if(name==L"NTEGlobalLauncher.exe"&&mode==L"nested") {
        game::Child child;child.start(folder/L"NTEGlobalGame.exe",false,L"--fixture nested "+quote(folder)+L" "+quote(report));
        check(WaitForSingleObject(child.info.hProcess,20000)==WAIT_OBJECT_0,"middle launcher completes");
        DWORD exit{};check(GetExitCodeProcess(child.info.hProcess,&exit)&&exit==0,"middle launcher result");return 0;
    }
    const bool unrelated=mode==L"unrelated";
    const bool missing=mode==L"missing";
    const auto exe=folder/(unrelated?L"Other.exe":missing?L"Missing.exe":L"HTGame.exe");
    std::wstring command=quote(exe)+L" --fixture "+mode+L" "+quote(folder)+L" "+quote(report);
    PROCESS_INFORMATION info{};STARTUPINFOW startup{};startup.cb=sizeof(startup);
    const auto success=CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,mode==L"normal"?0:CREATE_SUSPENDED,nullptr,folder.c_str(),&startup,&info);
    const auto error=GetLastError();
    if(mode==L"reject"||missing) {
        check(!success&&!info.hProcess&&!info.hThread&&!info.dwProcessId,"failed child creation clears returned handles");
        check(error==static_cast<DWORD>(missing?ERROR_FILE_NOT_FOUND:ERROR_DLL_INIT_FAILED),"CreateProcess error preserved or reported");
        std::ofstream(report)<<"PASS: expected failure, Windows error="<<error<<"\n";return 0;
    }
    check(success!=FALSE,"intercepted child creation");
    Handle process(info.hProcess),thread(info.hThread);
    struct Cleanup {HANDLE process;~Cleanup(){TerminateProcess(process,2);WaitForSingleObject(process,5000);}} cleanup{process};
    if(mode!=L"normal") {
        const auto previous=SuspendThread(thread);
        check(previous==1,"caller-requested CREATE_SUSPENDED preserved");
        check(ResumeThread(thread)==2,"restore observer suspension only");
        check(!fs::exists(report),"leaf main has not run at hook return");
    }
    // Unrelated/inactive children have not initialized their loader yet.
    if(!unrelated&&mode!=L"inactive") {
        const auto mapped=touchui::loader::find_module(info.dwProcessId,payload_filename);
        check(mapped.has_value(),"real project DLL mapped before main thread resume");
    }
    if(mode!=L"normal")check(ResumeThread(thread)==1,"caller resumes its child exactly once");
    check(WaitForSingleObject(process,10000)==WAIT_OBJECT_0,"leaf exits");
    DWORD exit{};check(GetExitCodeProcess(process,&exit)&&exit==0,"leaf mapping assertions pass");return 0;
}
void run(const fs::path& managed_host={}) {
    const auto own=fs::path(module_path()),directory=own.parent_path();
    const auto folder=directory/(L"nte relay fixture 中文 "+std::to_wstring(GetCurrentProcessId()));
    check(fs::create_directory(folder),"exclusive fixture directory");
    struct Cleanup {fs::path path;~Cleanup(){std::error_code error;fs::remove_all(path,error);}} cleanup{folder};
    for(const auto name:{L"NTEGlobalLauncher.exe",L"NTEGlobalGame.exe",L"HTGame.exe",L"Other.exe"})fs::copy_file(own,folder/name);
    fs::create_directory(folder/L"NTEGlobal");
    fs::copy_file(directory/L"LoaderSuccessFixture.dll",folder/L"NTEGlobal"/L"WPGameClientSDK_64.dll");
    check(sdk_for(folder/L"HTGame.exe")==folder/L"NTEGlobal"/L"WPGameClientSDK_64.dll","discover installed SDK without launching official software");
    for(const auto mode:{L"nested",L"normal",L"unrelated",L"reject",L"missing",L"inactive"}) {
        const auto report=folder/(std::wstring(mode)+L".txt");
        const auto key=L"Local\\NteRelayTests."+std::to_wstring(GetCurrentProcessId())+L"."+mode;
        relay::Session session(key,true);
        relay::copy(session.mapping->game,(folder/L"HTGame.exe").wstring());
        relay::copy(session.mapping->root,folder.wstring());
        relay::copy(session.mapping->payload,(directory/payload_filename).wstring());
        InterlockedExchange(&session.mapping->active,1);
        Handle job(CreateJobObjectW(nullptr,nullptr));check(job.value!=nullptr,"owned fixture job");
        struct StopJob {HANDLE job;~StopJob(){TerminateJobObject(job,2);}} stop{job};
        game::Child child;child.start(folder/L"NTEGlobalLauncher.exe",true,
            L"--fixture "+std::wstring(mode)+L" "+quote(folder)+L" "+quote(report));
        check(AssignProcessToJobObject(job,child.info.hProcess)!=FALSE,"fixture descendants tracked");
        relay::Startup startup(child.info.dwProcessId,session,relay::Role::Bridge);
        startup.load(child.info.hProcess,directory/payload_filename);
        check(touchui::loader::find_module(child.info.dwProcessId,payload_filename).has_value(),"initial bridge DLL mapped in owned fixture");
        if(std::wstring_view(mode)==L"reject")relay::copy(session.mapping->payload,(directory/L"LoaderRejectFixture.dll").wstring());
        if(std::wstring_view(mode)==L"inactive")InterlockedExchange(&session.mapping->active,0);
        child.resume();
        check(WaitForSingleObject(child.info.hProcess,25000)==WAIT_OBJECT_0,"owned bridge fixture completes");
        DWORD exit{};check(GetExitCodeProcess(child.info.hProcess,&exit)&&exit==0,"owned bridge fixture passes child assertions");
        check(fs::exists(report),"fixture outcome report");
        if(std::wstring_view(mode)==L"reject")check(relay::read(session.mapping->state)==relay::Failed,"load rejection reaches controller");
        else if(std::wstring_view(mode)==L"nested"||std::wstring_view(mode)==L"normal")check(relay::read(session.mapping->game_pid)!=0,"launcher propagated game identity");
        else check(relay::read(session.mapping->state)==relay::Waiting,"unrelated, missing and inactive children do not activate UI");
        JOBOBJECT_BASIC_ACCOUNTING_INFORMATION accounting{};
        check(QueryInformationJobObject(job,JobObjectBasicAccountingInformation,&accounting,sizeof(accounting),nullptr)&&
            accounting.ActiveProcesses==0,"no owned fixture descendants leaked");
        Handle game_handle(reinterpret_cast<HANDLE>(InterlockedExchange64(&session.mapping->game_handle,0)));
        if(std::wstring_view(mode)==L"nested"||std::wstring_view(mode)==L"normal")
            check(game_handle.value&&WaitForSingleObject(game_handle,0)==WAIT_OBJECT_0,"controller receives observation handle from owned game creation");
        std::wcout<<L"PASS: relay scenario "<<mode<<L"\n";
    }
    auto config=relay::Shared{};relay::copy(config.root,folder.wstring());relay::copy(config.game,(folder/L"HTGame.exe").wstring());
    check(!relay::classify(config,folder.parent_path()/L"NTEGlobalGame.exe"),"launcher outside installation rejected");
    check(!relay::classify(config,folder/L"other"/L"HTGame.exe"),"different game path rejected");
    check(!relay::classify(config,folder/L"Browser.exe"),"unrelated official helper excluded");
    relay::copy(config.login_host,(folder.parent_path()/L"Nte.LoginHost.exe").wstring());
    check(relay::classify(config,config.login_host)==relay::Role::Bridge,"explicit SDK host outside installation accepted");
    check(!relay::classify(config,folder/L"Nte.LoginHost.exe"),"same host filename at different path rejected");
    {
        const auto client=folder/L"Client/WindowsNoEditor/HT/Binaries/Win64/HTGame.exe";
        fs::create_directories(client.parent_path());fs::copy_file(own,client);
        fs::copy_file(own,folder/L"NTEGlobal/NTEGlobalGame.exe");
        const auto package=folder/L"package";fs::create_directories(package/L"nte-login-host");
        fs::copy_file(own,package/L"nte-login-host/Nte.LoginHost.exe");
        const auto plan=login_host_for(client,package);
        check(plan.region==L"Global"&&relay::same_path(plan.root,folder),"global installed layout auto-detected");
        check(plan.arguments(L"-example=\"a b\"").find(L"--game-arguments")!=std::wstring::npos,"SDK host receives safely quoted game extras");
        fs::create_directory(folder/L"NTELauncher");fs::copy_file(own,folder/L"NTELauncher/NTEGame.exe");
        fs::copy_file(directory/L"LoaderSuccessFixture.dll",folder/L"NTELauncher/WPGameClientSDK_64.dll");
        bool rejected{};try{login_host_for(client,package);}catch(const std::exception&){rejected=true;}
        check(rejected,"ambiguous CN/global installation needs explicit region");
        check(login_host_for(client,package,L"CN").region==L"CN","CN explicitly selected without starting vendor software");
        rejected=false;try{login_host_for(client,package,L"invalid");}catch(const std::exception&){rejected=true;}
        check(rejected,"unknown direct login region rejected");
    }
    if(!managed_host.empty()) {
        relay::Session session(L"Local\\NteRelayTests.Managed."+std::to_wstring(GetCurrentProcessId()),true);
        relay::copy(session.mapping->game,(folder/L"HTGame.exe").wstring());
        relay::copy(session.mapping->root,folder.wstring());
        relay::copy(session.mapping->login_host,managed_host.wstring());
        relay::copy(session.mapping->payload,(directory/payload_filename).wstring());
        InterlockedExchange(&session.mapping->active,1);
        const auto report=folder/L"managed.txt";
        Handle job(CreateJobObjectW(nullptr,nullptr));check(job.value!=nullptr,"managed fixture job");
        struct StopJob {HANDLE job;~StopJob(){TerminateJobObject(job,2);}} stop{job};
        game::Child host;host.start(managed_host,true,L"--verify-owned-child "+quote(folder/L"HTGame.exe")+L" "+quote(folder)+L" "+quote(report));
        check(AssignProcessToJobObject(job,host.info.hProcess)!=FALSE,"managed fixture descendants tracked");
        relay::Startup startup(host.info.dwProcessId,session,relay::Role::Bridge);
        startup.load(host.info.hProcess,directory/payload_filename);host.resume();
        check(WaitForSingleObject(host.info.hProcess,45000)==WAIT_OBJECT_0,"managed host completes");
        DWORD exit{};check(GetExitCodeProcess(host.info.hProcess,&exit)&&exit==0,".NET Process.Start receives preloaded DLL");
        check(fs::exists(report)&&relay::read(session.mapping->game_pid)!=0,"managed game mapping propagated to controller");
        Handle observed(reinterpret_cast<HANDLE>(InterlockedExchange64(&session.mapping->game_handle,0)));
        check(observed.value&&WaitForSingleObject(observed,0)==WAIT_OBJECT_0,"managed child observation handle retained");
    }
    {
        relay::Session session(L"Local\\NteRelayTests.Direct."+std::to_wstring(GetCurrentProcessId()),true);
        relay::copy(session.mapping->game,(folder/L"HTGame.exe").wstring());
        relay::copy(session.mapping->root,folder.wstring());
        relay::copy(session.mapping->payload,(directory/payload_filename).wstring());
        InterlockedExchange(&session.mapping->active,1);
        const auto report=folder/L"direct.txt";
        game::Child child;child.start(folder/L"HTGame.exe",true,L"--fixture normal "+quote(folder)+L" "+quote(report));
        relay::Startup startup(child.info.dwProcessId,session,relay::Role::Game);
        startup.load(child.info.hProcess,directory/payload_filename);
        check(!fs::exists(report),"direct startup handshake precedes game main");
        check(SuspendThread(child.info.hThread)==1,"direct game remains suspended after DLL mapping");
        check(ResumeThread(child.info.hThread)==2,"restore direct fixture observation count");
        child.resume();
        check(WaitForSingleObject(child.info.hProcess,10000)==WAIT_OBJECT_0,"direct fixture main completes");
        DWORD exit{};check(GetExitCodeProcess(child.info.hProcess,&exit)&&exit==0&&fs::exists(report),"direct leaf verifies DLL without official launcher");
    }
    {
        std::vector<std::string> stages;bool rejected{};const auto report=folder/L"direct-failure.txt";
        try {
            launch_mobile(folder/L"HTGame.exe",{},directory/payload_filename,false,
                L"--fixture direct-failure "+quote(folder)+L" "+quote(report),
                [&](std::string_view message){stages.emplace_back(message);},true);
        }catch(const std::exception& error){rejected=std::string(error.what()).starts_with("NTE relay:");}
        check(rejected,"production direct coordinator receives unsupported UI-host failure");
        check(stages.size()>=4&&stages[0].find("directly")!=std::string::npos&&
            std::any_of(stages.begin(),stages.end(),[](const auto& stage){return stage.find("resume game main")!=std::string::npos;}),
            "production direct coordinator maps DLL before main-thread resume");
        DWORD pid{};{std::ifstream file(report);file>>pid;}
        check(pid!=0,"direct fixture recorded owned process identity");
        Handle process(OpenProcess(SYNCHRONIZE,FALSE,pid));
        check(!process.value||WaitForSingleObject(process,2000)==WAIT_OBJECT_0,"production direct failure cleans owned fixture game");
    }
}
}
int wmain(int argc,wchar_t** argv) {
    try {
        for(int i=1;i<argc;++i)if(std::wstring_view(argv[i])==L"--fixture") {
            if(i!=1) {
                check(i==4&&std::wstring_view(argv[1])==L"/Game/LoginAndCreate/Map/Updater/Updater_P"&&
                    std::wstring_view(argv[2])==L"--saveddirsuffix=Global"&&std::wstring_view(argv[3])==L"-SAVEWINPOS=1",
                    "production direct coordinator forwards international startup arguments before extras");
                std::vector<wchar_t> value(32768);
                const auto length=GetEnvironmentVariableW(sdk_environment_name.data(),value.data(),static_cast<DWORD>(value.size()));
                check(length>0&&length<value.size()&&std::wstring(value.data(),length)==sdk_base(fs::path(argv[6])/L"NTEGlobal"/L"WPGameClientSDK_64.dll"),
                    "production direct game receives Unicode SDK path in its own environment");
            }
            return fixture(argc-i+1,argv+i-1);
        }
        check(direct_arguments({})==direct_defaults,"direct defaults preserve official international entry map and Global profile");
        check(direct_arguments(L"--example \"two words\"")==std::wstring(direct_defaults)+L" --example \"two words\"",
            "direct extra arguments append without changing original quoting");
        {
            const auto block=sdk_environment(L"D:\\安装 目录\\WPGameClientSDK",{L"Path=C:\\Windows",L"arcgamesdksharepath=obsolete",L"=D:=D:\\WorkSpace",L"OTHER=中文"});
            std::vector<std::wstring> entries;for(auto* entry=block.data();*entry;entry+=wcslen(entry)+1)entries.emplace_back(entry);
            check(block.size()>2&&block[block.size()-1]==0&&block[block.size()-2]==0,"explicit environment is double-NUL terminated");
            check(entries.size()==4&&entries[0]==L"=D:=D:\\WorkSpace","SDK environment preserves drive-current-directory entry");
            check(std::find(entries.begin(),entries.end(),L"ArcGameSDKSharePath=D:\\安装 目录\\WPGameClientSDK")!=entries.end(),
                "SDK environment replaces mixed-case inherited key with Unicode path");
            check(std::find(entries.begin(),entries.end(),L"OTHER=中文")!=entries.end()&&std::find(entries.begin(),entries.end(),L"Path=C:\\Windows")!=entries.end(),
                "SDK environment retains other inherited entries");
        }
        run(argc==3&&std::wstring_view(argv[1])==L"--managed"?fs::absolute(argv[2]):fs::path{});
        std::cout<<"PASS: "<<checks<<" NTE relay ownership and propagation checks\n";return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
