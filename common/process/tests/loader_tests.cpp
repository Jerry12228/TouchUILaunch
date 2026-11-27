#include "load_library_diagnostic.hpp"
#include "game.hpp"
#include <fstream>
#include <iostream>

namespace {
unsigned checks{};
void check(bool condition,const char* message){++checks;if(!condition)throw std::runtime_error(message);}
std::wstring event_name(DWORD pid,const wchar_t* state){return L"Local\\LoaderTests."+std::wstring(state)+L"."+std::to_wstring(pid);}
void run() {
    using namespace touchui::loader;
    check(Result{0,0,0,0xdeadc0de}.failure().find("LoadLibraryW was not called")!=std::string::npos,
        "pre-entry thread termination is distinguished from DLL initialization failure");
    check(Result{0,0,1,0xc0000005}.failure().find("before LoadLibraryW returned")!=std::string::npos,
        "incomplete loader call is distinguished from a normal NULL return");
    const auto own=std::filesystem::path(module_path()),directory=own.parent_path();
    {
        std::cout<<"Checking direct loading in suspended child\n"<<std::flush;
        game::Child early;early.start(own,true,L"--child");
        const auto direct=load_direct(early.info.hProcess,directory/L"LoaderSuccessFixture.dll");
        check(direct.module.base!=0,"direct LoadLibraryW maps DLL before main thread resumes");
        check(SuspendThread(early.info.hThread)==1,"direct loader preserves initial main-thread suspension");
        check(ResumeThread(early.info.hThread)==2,"direct loader observer restores only added suspension");
    }
    std::cout<<"Checking diagnostic loader in running child\n"<<std::flush;
    game::Child child;child.start(own,false,L"--child");
    Handle ready;
    for(unsigned attempt=0;attempt<100&&!ready.value;++attempt) {
        ready.value=OpenEventW(SYNCHRONIZE,FALSE,event_name(child.info.dwProcessId,L"Ready").c_str());
        if(!ready.value)Sleep(10);
    }
    check(ready.value&&WaitForSingleObject(ready,2000)==WAIT_OBJECT_0,"owned loader child initialized");
    const auto normal=load(child.info.hProcess,directory/L"LoaderSuccessFixture.dll");
    check(normal.success()&&normal.phase==2&&normal.error==0&&normal.thread_exit==0,"normal loader completes with full result");
    const auto mapped=find_module(child.info.dwProcessId,L"LoaderSuccessFixture.dll");
    check(mapped&&mapped->base==normal.module,"full 64-bit HMODULE agrees with module list");
    const auto fixture=directory/(L"loader fixture 中文 "+std::to_wstring(GetCurrentProcessId()));
    check(std::filesystem::create_directory(fixture),"new owned loader fixture directory");
    struct Cleanup {std::filesystem::path directory;~Cleanup(){std::error_code ignored;std::filesystem::remove_all(directory,ignored);}} cleanup{fixture};
    const auto invalid=fixture/L"invalid image.dll";
    {std::ofstream file(invalid,std::ios::binary);file<<"This is a test, not a PE image.";}
    const auto bad_image=load(child.info.hProcess,invalid);
    check(!bad_image.success()&&bad_image.phase==2&&bad_image.error==ERROR_BAD_EXE_FORMAT,"invalid image captures remote error 193");
    const auto missing=load(child.info.hProcess,fixture/L"missing library.dll");
    check(!missing.success()&&missing.phase==2&&missing.error==ERROR_MOD_NOT_FOUND,"missing image captures remote error 126");
    const auto unicode=fixture/L"拒绝初始化 with spaces.dll";
    std::filesystem::copy_file(directory/L"LoaderRejectFixture.dll",unicode);
    const auto rejected=load(child.info.hProcess,unicode);
    check(!rejected.success()&&rejected.phase==2&&rejected.error==ERROR_DLL_INIT_FAILED,"Unicode path and rejected DllMain capture remote error 1114");
    check(rejected.failure().find("LoadLibraryW returned NULL")!=std::string::npos,"completed failed load has an explicit NULL-return diagnosis");
    check(!find_module(child.info.dwProcessId,unicode.filename()),"rejected DLL absent from module list");
    check(rejected.describe().find("Windows error=1114")!=std::string::npos&&rejected.describe().find("loader thread exit=0x0")!=std::string::npos,
        "diagnostic distinguishes loader failure from completed remote thread");
    Handle done(CreateEventW(nullptr,TRUE,FALSE,event_name(child.info.dwProcessId,L"SlowDone").c_str()));
    check(done.value!=nullptr,"owned slow-loader event");
    bool timed_out{};
    try {load(child.info.hProcess,directory/L"LoaderSlowFixture.dll",1);}
    catch(const std::runtime_error& error){timed_out=std::string(error.what()).find("retained until owned process exit")!=std::string::npos;}
    check(timed_out,"timeout retains remote code, context and path");
    check(WaitForSingleObject(done,5000)==WAIT_OBJECT_0&&WaitForSingleObject(child.info.hProcess,0)==WAIT_TIMEOUT,
        "slow DllMain finishes safely after caller's timeout");
    // Only the child created here is terminated by Child's failure-ownership
    // destructor. No discovered process or game is opened by these tests.
}
}
int wmain(int argc,wchar_t** argv) {
    if(argc==2&&std::wstring_view(argv[1])==L"--child") {
        Handle ready(CreateEventW(nullptr,TRUE,TRUE,event_name(GetCurrentProcessId(),L"Ready").c_str()));
        Sleep(30000);return 0;
    }
    try {run();std::cout<<"PASS: "<<checks<<" owned-child loader diagnostic checks\n";return 0;}
    catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
