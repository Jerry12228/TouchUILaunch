#include "launcher_log.hpp"
#include <sstream>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct ConsoleCapture {
    std::ostringstream output,error;
    std::streambuf* previous_out=std::cout.rdbuf(output.rdbuf());
    std::streambuf* previous_err=std::cerr.rdbuf(error.rdbuf());
    ~ConsoleCapture(){std::cout.rdbuf(previous_out);std::cerr.rdbuf(previous_err);}
};
void test() {
    namespace fs=std::filesystem;
    const auto root=fs::path(module_path()).parent_path()/(L"launcher log 中文 "+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
    fs::path log;
    {
        ConsoleCapture console;
        {
            launcher_log::Session disabled(false);disabled.open(root);disabled.stage("silent phase");disabled.failure("silent failure");
            check(!fs::exists(root)&&disabled.path().empty(),"no directory or file without --log");
        }
        check(console.output.str().empty()&&console.error.str().empty(),"no output without --log");
        {
            launcher_log::Session enabled(true);enabled.open(root);log=enabled.path();
            check(!log.empty(),"launcher log created under Unicode directory");
            enabled.stage("GI: resolve UI functions and input object");
            std::cout<<"resolver stdout marker\n";std::cerr<<"resolver stderr marker\n";
            enabled.failure("conflicting target fixture");
            // Read before destruction: output must survive abrupt console closure.
            std::ifstream file(log,std::ios::binary);
            const std::string text((std::istreambuf_iterator<char>(file)),{});
            check(text.find("GI: resolve UI functions")!=std::string::npos&&text.find("conflicting target fixture")!=std::string::npos,
                  "phase and error flushed before shutdown");
            check(text.find("resolver stdout marker")!=std::string::npos&&text.find("resolver stderr marker")!=std::string::npos,"stdout and stderr both persisted");
            check(text.find(launcher_log::utf8(log.wstring()))!=std::string::npos,"log path encoded as UTF-8");
        }
        check(std::cout.rdbuf()==console.output.rdbuf()&&std::cerr.rdbuf()==console.error.rdbuf(),"stream buffers restored on exit");
        check(console.error.str().find("ERROR:")!=std::string::npos,"error remains visible to CLI consumers");
    }
    // All paths were constructed beneath this test executable; remove exact
    // files/directories individually, never recursively remove a computed path.
    fs::remove(log);fs::remove(root/L"logs");fs::remove(root);
    check(launcher_log::should_show_error(true,true,true,1),"isolated elevated console reports a durable dialog");
    check(!launcher_log::should_show_error(false,true,true,1)&&!launcher_log::should_show_error(true,false,true,1)&&
          !launcher_log::should_show_error(true,true,false,1)&&!launcher_log::should_show_error(true,true,true,2),
          "silent, redirected and shared consoles never show dialogs");
}
}
int main() {
    try {test();std::cout<<"PASS: launcher diagnostics persistence, default silence, stream restoration and dialog policy\n";return 0;}
    catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;}
}
