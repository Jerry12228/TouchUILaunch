#include "win_util.hpp"
#include <fstream>

namespace {
bool matches(int argc,wchar_t** argv,std::initializer_list<std::wstring_view> expected) {
    if(argc!=static_cast<int>(expected.size()+1))return false;
    int index=1;
    for(const auto value:expected)if(std::wstring_view(argv[index++])!=value)return false;
    return true;
}
}

int wmain(int argc,wchar_t** argv) {
    if(argc>1) {
        const std::initializer_list<std::wstring_view> extra={L"--fixture-extra",L"two words",L"中文 路径",L"embedded \"quote\"",L"trailing\\"};
        if(matches(argc,argv,extra))return 23;
        // WW CLI integration uses this fixture under the game's executable name.
        // Report only after a delay, so premature launcher cleanup is detected.
        if(!matches(argc,argv,{L"-CloudGame",L"-CloudGamePlatform=Android",L"--fixture-extra",L"two words",L"中文 路径",L"embedded \"quote\"",L"trailing\\"}) &&
           !matches(argc,argv,{L"-CloudGame",L"-CloudGamePlatform=Android"}))return 20;
        const auto directory=std::filesystem::path(module_path()).parent_path();
        if(std::filesystem::current_path()!=directory)return 21;
        Sleep(200);
        std::ofstream report(directory/L"ww-launch-passed.txt");
        report<<"PASS: exact WW arguments, working directory, and child lifetime\n";
        return report.good()?0:22;
    }
    // Only a fixture, never a game. Parent tests verify main remains suspended
    // while the loader and UI preparation run in separately created threads.
    Handle started(CreateEventW(nullptr,TRUE,TRUE,(L"Local\\MobileUITests.Main."+std::to_wstring(GetCurrentProcessId())).c_str()));
    Sleep(200);
    return 19;
}
