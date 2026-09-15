#include "elevation.hpp"
#include <iostream>

namespace {
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
const std::vector<std::wstring> tricky_args={L"",L"simple",L"a b",L"a\tb",L"D:\\游戏目录\\ZenlessZoneZero.exe",L"C:\\ends in slash\\",L"quoted\"value",L"slashes\\\\\"quote",L"\\\\",L"& %PATH% $(literal)"};
std::wstring expected_exe,expected_directory;
std::vector<std::wstring> expected_args;
int shell_calls{};
DWORD shell_error{};
BOOL WINAPI fake_shell(SHELLEXECUTEINFOW* info) {
    ++shell_calls;
    check(std::wstring(info->lpVerb)==L"runas","must use Windows runas verb");
    check(info->lpFile==expected_exe&&info->lpDirectory==expected_directory,"preserve executable and working directory");
    check((info->fMask&SEE_MASK_NOCLOSEPROCESS)!=0,"request child handle for exit status");
    int count{};auto command=L"test.exe "+std::wstring(info->lpParameters);
    auto argv=CommandLineToArgvW(command.c_str(),&count);
    check(argv!=nullptr,"parse forwarded parameters");
    bool match=count==static_cast<int>(expected_args.size()+1);
    for(int i=1;match&&i<count;++i)match=expected_args[i-1]==argv[i];
    LocalFree(argv);check(match,"all forwarded parameters must survive quoting");
    if(shell_error){SetLastError(shell_error);return FALSE;}
    // Use only this test binary, without elevation, to exercise waiting and exit propagation.
    auto child_command=elevation::quote_argument(expected_exe)+L" --test-child";
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION child{};
    if(!CreateProcessW(expected_exe.c_str(),child_command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,expected_directory.c_str(),&startup,&child))return FALSE;
    CloseHandle(child.hThread);info->hProcess=child.hProcess;return TRUE;
}
void test() {
    check(elevation::needs_relaunch(false,false),"standard token requires UAC");
    check(!elevation::needs_relaunch(true,false)&&!elevation::needs_relaunch(true,true),"elevated token never prompts again");
    bool stopped{};
    try {elevation::needs_relaunch(false,true);}catch(const std::runtime_error&){stopped=true;}
    check(stopped,"failed elevation must stop instead of looping");
    expected_exe=module_path();expected_directory=std::filesystem::current_path().wstring();
    // Also check against the real MSVC argv parser in a child process, including empty args.
    auto args=tricky_args;args.insert(args.begin(),L"--check-argv");
    auto command=elevation::quote_argument(expected_exe)+L" "+elevation::parameters(args);
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION child{};
    check(CreateProcessW(expected_exe.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child)!=FALSE,"start owned argument round-trip child");
    Handle process(child.hProcess),thread(child.hThread);
    check(WaitForSingleObject(process,10000)==WAIT_OBJECT_0,"argument child completes");DWORD code{};
    check(GetExitCodeProcess(process,&code)&&code==0,"real CRT argv round-trip");
    expected_args={L"--ZZZ",L"--game",L"D:\\游戏目录\\ZenlessZoneZero.exe",L"--log",elevation::relaunch_flag};
    shell_error=ERROR_CANCELLED;shell_calls=0;
    check(elevation::relaunch(expected_exe,expected_args,expected_directory,fake_shell)==ERROR_CANCELLED&&shell_calls==1,"UAC cancellation returns once without retry");
    shell_error=ERROR_ACCESS_DENIED;stopped=false;
    try {elevation::relaunch(expected_exe,expected_args,expected_directory,fake_shell);}catch(const std::runtime_error& e){stopped=std::string(e.what()).find("error 5")!=std::string::npos;}
    check(stopped,"non-cancellation shell error is reported");
    shell_error=0;
    for(const auto flag:{L"--GI",L"--SR",L"--ZZZ"}) {
        expected_args={L"--log",L"--game",L"D:\\游戏 目录\\example.exe",flag,elevation::relaunch_flag};
        check(elevation::relaunch(expected_exe,expected_args,expected_directory,fake_shell)==37,"preserve game selection and propagate elevated child exit code");
    }
    std::cout<<"Token elevated="<<elevation::is_elevated()<<"; quoting, relaunch guard, cancellation, errors and child exit propagation: PASS\n";
}
}
int wmain(int argc,wchar_t** argv) {
    if(argc==2&&std::wstring(argv[1])==L"--test-child")return 37;
    if(argc>=2&&std::wstring(argv[1])==L"--check-argv") {
        if(argc!=static_cast<int>(tricky_args.size()+2))return 10;
        for(int i=2;i<argc;++i)if(tricky_args[i-2]!=argv[i])return 11;
        return 0;
    }
    try {test();return 0;}catch(const std::exception& ex){std::cerr<<"FAIL: "<<ex.what()<<"\n";return 1;}
}
