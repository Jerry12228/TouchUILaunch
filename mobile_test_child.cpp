#include "win_util.hpp"
#include <fstream>
int wmain(int argc,wchar_t** argv) {
    if(argc>1) {
        // WW CLI integration uses this fixture under the game's executable name.
        // Report only after a delay, so premature launcher cleanup is detected.
        if(argc!=3||std::wstring_view(argv[1])!=L"-CloudGame"||
           std::wstring_view(argv[2])!=L"-CloudGamePlatform=Android")return 20;
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
