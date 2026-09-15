#include "win_util.hpp"
int main() {
    // Only a fixture, never a game. Parent tests verify main remains suspended
    // while the loader and UI preparation run in separately created threads.
    Handle started(CreateEventW(nullptr,TRUE,TRUE,(L"Local\\MobileUITests.Main."+std::to_wstring(GetCurrentProcessId())).c_str()));
    Sleep(200);
    return 19;
}
