#include "payload_entry.hpp"
#include "nte_launch.hpp"
#include "game.hpp"
#include <iostream>

namespace {
unsigned checks{};
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
void child(std::wstring_view mode) {
    const auto pid=GetCurrentProcessId();
    const bool zzz=mode==L"zzz"||mode==L"mixed"||mode==L"partial";
    const bool nte=mode==L"nte"||mode==L"mixed";
    Handle logging(zzz?CreateEventW(nullptr,TRUE,FALSE,log_event_name(pid).c_str()):nullptr);
    Handle ready(nte?CreateEventW(nullptr,TRUE,FALSE,touchui::nte::event_name(pid,L"Ready").c_str()):nullptr);
    Handle failed(nte||mode==L"partial"?CreateEventW(nullptr,TRUE,FALSE,touchui::nte::event_name(pid,L"Failed").c_str()):nullptr);
    Handle boot_failed(CreateEventW(nullptr,TRUE,FALSE,touchui::nte::event_name(pid,L"BootFailed").c_str()));
    check((!zzz||logging.value)&&(!nte||ready.value)&&boot_failed.value,"create owned activation events");
    const auto dll=std::filesystem::path(module_path()).parent_path()/touchui::payload::filename;
    check(LoadLibraryW(dll.c_str())!=nullptr,"load unified production DLL in owned fixture");
    // The production DLL and its globals remain loaded until this child exits.
    if(mode==L"zzz") {
        Handle started;
        const auto deadline=GetTickCount64()+5000;
        while(!started.value&&GetTickCount64()<deadline) {
            started.value=OpenEventW(SYNCHRONIZE,FALSE,started_event_name(pid).c_str());
            if(!started.value)Sleep(10);
        }
        check(started.value&&WaitForSingleObject(started,0)==WAIT_OBJECT_0,"ZZZ worker publishes its own startup event");
        check(WaitForSingleObject(boot_failed,0)==WAIT_TIMEOUT,"ZZZ activation never starts or rejects NTE");
    } else {
        check(failed.value&&WaitForSingleObject(failed,5000)==WAIT_OBJECT_0,"NTE or rejected activation signals failure");
        Handle started(OpenEventW(SYNCHRONIZE,FALSE,started_event_name(pid).c_str()));
        check(!started.value,"NTE and rejected activation never start the ZZZ worker");
        check(WaitForSingleObject(boot_failed,0)==(mode==L"nte"?WAIT_TIMEOUT:WAIT_OBJECT_0),
            "conflicting/incomplete activation rejected before entering a game worker");
        if(ready.value)check(WaitForSingleObject(ready,0)==WAIT_TIMEOUT,"unsupported fixture never claims UI Ready");
    }
}
void run() {
    using touchui::payload::Kind;
    using touchui::payload::select;
    check(select(false,false,false,false)==Kind::Inactive,"unconfigured DLL stays inactive");
    check(select(true,false,false,false)==Kind::ZZZ,"existing quiet ZZZ logging object selects ZZZ");
    check(select(false,true,true,false)==Kind::NTE,"owned legacy NTE fixture events supported");
    check(select(false,true,true,true)==Kind::NTE,"coordinated NTE session supported");
    check(select(true,true,true,true)==Kind::Inactive,"mixed activation rejected");
    check(select(false,false,false,true)==Kind::Inactive,"config without lifecycle events rejected");
    check(select(false,true,false,false)==Kind::Inactive,"partial NTE activation rejected");
    for(const auto mode:{L"zzz",L"nte",L"mixed",L"partial"}) {
        game::Child process;process.start(module_path(),false,L"--child "+std::wstring(mode));
        check(WaitForSingleObject(process.info.hProcess,15000)==WAIT_OBJECT_0,"owned dispatch fixture finishes");
        DWORD code{};check(GetExitCodeProcess(process.info.hProcess,&code)&&code==0,"unified DLL dispatch fixture passes");
    }
}
}
int wmain(int argc,wchar_t** argv) {
    try {
        if(argc==3&&std::wstring_view(argv[1])==L"--child")child(argv[2]);
        else {check(argc==1,"fixture command arguments");run();}
        std::cout<<"PASS: "<<checks<<" unified payload dispatch checks\n";return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
