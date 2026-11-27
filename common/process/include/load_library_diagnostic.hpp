#pragma once
#include "win_util.hpp"
#include <tlhelp32.h>
#include <optional>
#include <sstream>
#include <cstddef>
#include <psapi.h>

extern "C" const unsigned char UiDiagnosticLoadBegin[],UiDiagnosticLoadEnd[];
namespace touchui::loader {
inline void require(bool condition,const std::string& message) {
    if(!condition)throw std::runtime_error(message);
}
inline std::string hex(uintptr_t value) {std::ostringstream out;out<<"0x"<<std::hex<<value;return out.str();}
struct Module {uintptr_t base{};std::filesystem::path path;};
inline std::vector<Module> modules(DWORD pid) {
    HANDLE raw=INVALID_HANDLE_VALUE;
    for(unsigned attempt=0;attempt<8;++attempt) {
        raw=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
        if(raw!=INVALID_HANDLE_VALUE)break;
        const auto error=GetLastError();
        require(error==ERROR_BAD_LENGTH,"Cannot enumerate target modules, Windows error "+std::to_string(error));
        Sleep(10);
    }
    require(raw!=INVALID_HANDLE_VALUE,"Target module snapshot stayed unstable (ERROR_BAD_LENGTH)");
    Handle snapshot(raw);MODULEENTRY32W entry{};entry.dwSize=sizeof(entry);
    if(!Module32FirstW(snapshot,&entry)) {
        const auto error=GetLastError();
        require(error==ERROR_NO_MORE_FILES,"Module32FirstW failed, Windows error "+std::to_string(error));return {};
    }
    std::vector<Module> result;
    do {result.push_back({reinterpret_cast<uintptr_t>(entry.modBaseAddr),entry.szExePath});}
    while(Module32NextW(snapshot,&entry));
    const auto error=GetLastError();
    require(error==ERROR_NO_MORE_FILES,"Module32NextW failed, Windows error "+std::to_string(error));return result;
}
inline std::optional<Module> find_module(DWORD pid,const std::filesystem::path& name) {
    for(const auto& module:modules(pid))if(_wcsicmp(module.path.filename().c_str(),name.c_str())==0)return module;
    return {};
}
inline uintptr_t remote_api(DWORD pid,const char* name) {
    const auto local=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),name);HMODULE host{};
    require(local&&GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(local),&host),"Cannot locate system API host: "+std::string(name));
    const auto remote=find_module(pid,std::filesystem::path(module_path(host)).filename());
    require(remote.has_value(),"System API host is absent in target: "+std::string(name));
    return remote->base+reinterpret_cast<uintptr_t>(local)-reinterpret_cast<uintptr_t>(host);
}
// The loader list need not exist before the initial thread has run. Resolve
// the owning system image from its mapped pages in our newly-created child.
inline uintptr_t mapped_image(HANDLE process,const std::filesystem::path& name) {
    uintptr_t at{},last{},found{};MEMORY_BASIC_INFORMATION region{};
    while(VirtualQueryEx(process,reinterpret_cast<void*>(at),&region,sizeof(region))==sizeof(region)) {
        const auto base=reinterpret_cast<uintptr_t>(region.AllocationBase);
        if(region.Type==MEM_IMAGE&&base&&base!=last) {
            last=base;wchar_t path[32768]{};
            const auto count=GetMappedFileNameW(process,reinterpret_cast<void*>(base),path,32768);
            if(count&&count<32768&&_wcsicmp(std::filesystem::path(path).filename().c_str(),name.c_str())==0) {
                require(!found,"Ambiguous mapped system image");found=base;
            }
        }
        const auto next=reinterpret_cast<uintptr_t>(region.BaseAddress)+region.RegionSize;
        if(next<=at)break;at=next;
    }
    return found;
}
inline uintptr_t mapped_api(HANDLE process,const wchar_t* library,const char* name) {
    const auto local=GetProcAddress(GetModuleHandleW(library),name);HMODULE host{};
    require(local&&GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(local),&host),"Cannot locate mapped API host");
    const auto base=mapped_image(process,std::filesystem::path(module_path(host)).filename());
    require(base!=0,"System API image is not mapped in newly created child");
    const auto address=base+reinterpret_cast<uintptr_t>(local)-reinterpret_cast<uintptr_t>(host);
    MEMORY_BASIC_INFORMATION region{};
    require(VirtualQueryEx(process,reinterpret_cast<void*>(address),&region,sizeof(region))==sizeof(region)&&
        region.State==MEM_COMMIT&&reinterpret_cast<uintptr_t>(region.AllocationBase)==base&&
        !(region.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&
        (region.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)),
        "Mapped system API is not executable");
    return address;
}
struct DirectResult {
    Module module;DWORD thread_exit{};
};
inline DirectResult load_direct(HANDLE process,const std::filesystem::path& path,DWORD timeout=30000) {
    USHORT machine{},native{};
    require(IsWow64Process2(process,&machine,&native)&&machine==IMAGE_FILE_MACHINE_UNKNOWN&&native==IMAGE_FILE_MACHINE_AMD64,
        "Direct loader requires a Windows x64 child");
    const auto text=path.wstring();require(!text.empty()&&text.size()<32768,"Invalid direct loader path");
    // Normal LoadLibraryW thread entry: no executable remote stub. Some
    // suspended images have only ntdll mapped; initialize their normal loader.
    if(!mapped_image(process,L"kernel32.dll")) {
        const auto exit=mapped_api(process,L"ntdll.dll","RtlExitUserThread");
        Handle bootstrap(CreateRemoteThread(process,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(exit),nullptr,0,nullptr));
        require(bootstrap.value&&WaitForSingleObject(bootstrap,timeout)==WAIT_OBJECT_0,"Initial child loader did not complete");
        DWORD code{};require(GetExitCodeThread(bootstrap,&code)&&code==0,"Initial child loader failed");
    }
    const auto entry=mapped_api(process,L"kernel32.dll","LoadLibraryW");
    struct PathBlock {
        HANDLE process;void* value{};bool retained{};
        ~PathBlock(){if(value&&!retained)VirtualFreeEx(process,value,0,MEM_RELEASE);}
    } block{process};
    const auto size=(text.size()+1)*sizeof(wchar_t);
    block.value=VirtualAllocEx(process,nullptr,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    require(block.value!=nullptr,"Cannot allocate direct loader path, Windows error "+std::to_string(GetLastError()));
    SIZE_T wrote{};
    require(WriteProcessMemory(process,block.value,text.c_str(),size,&wrote)&&wrote==size,"Cannot write direct loader path");
    Handle thread(CreateRemoteThread(process,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(entry),block.value,0,nullptr));
    require(thread.value!=nullptr,"Cannot start LoadLibraryW thread, Windows error "+std::to_string(GetLastError()));
    block.retained=true;
    require(WaitForSingleObject(thread,timeout)==WAIT_OBJECT_0,"Direct LoadLibraryW timeout; path retained until owned child exits");
    block.retained=false;DWORD exit{};require(GetExitCodeThread(thread,&exit)!=FALSE,"Cannot read LoadLibraryW thread status");
    // The exit code truncates HMODULE and can also contain a termination code.
    // A nonzero exit is never used as success evidence.
    const auto loaded=find_module(GetProcessId(process),path.filename());
    require(loaded&&_wcsicmp(loaded->path.c_str(),path.c_str())==0,
        "Direct LoadLibraryW did not map expected DLL; thread exit="+hex(exit));
    return {*loaded,exit};
}
struct Context {
    uintptr_t path{},load{},get_error{},module{};
    DWORD error{},phase{};
};
static_assert(offsetof(Context,path)==0&&offsetof(Context,load)==8&&offsetof(Context,get_error)==16&&
    offsetof(Context,module)==24&&offsetof(Context,error)==32&&offsetof(Context,phase)==36&&sizeof(Context)==40);
struct Result {
    uintptr_t module{};DWORD error{},phase{},thread_exit{};
    bool success()const{return phase==2&&thread_exit==0&&module!=0;}
    std::string describe()const {
        return "LoadLibraryW: phase="+std::to_string(phase)+", HMODULE="+hex(module)+", Windows error="+
            std::to_string(error)+" ("+hex(error)+"), loader thread exit="+hex(thread_exit);
    }
    std::string failure()const {
        if(phase==0)return "Remote thread ended before the loader entry marker; LoadLibraryW was not called. "+describe();
        if(phase==1)return "Remote thread ended before LoadLibraryW returned. "+describe();
        if(phase==2&&!module)return "LoadLibraryW returned NULL. "+describe();
        return "Remote loader did not complete successfully. "+describe();
    }
};
inline std::string mitigations(HANDLE process) {
    std::ostringstream out;out<<"Process mitigations:";
    PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynamic{};
    if(GetProcessMitigationPolicy(process,ProcessDynamicCodePolicy,&dynamic,sizeof(dynamic)))
        out<<" prohibit_dynamic_code="<<dynamic.ProhibitDynamicCode;
    else out<<" dynamic_code_query_error="<<GetLastError();
    PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY signature{};
    if(GetProcessMitigationPolicy(process,ProcessSignaturePolicy,&signature,sizeof(signature)))
        out<<" microsoft_signed_only="<<signature.MicrosoftSignedOnly<<" store_signed_only="<<signature.StoreSignedOnly;
    else out<<" signature_query_error="<<GetLastError();
    PROCESS_MITIGATION_IMAGE_LOAD_POLICY image{};
    if(GetProcessMitigationPolicy(process,ProcessImageLoadPolicy,&image,sizeof(image)))
        out<<" no_remote_images="<<image.NoRemoteImages<<" no_low_label_images="<<image.NoLowMandatoryLabelImages;
    else out<<" image_load_query_error="<<GetLastError();
    return out.str();
}
inline Result load(HANDLE process,const std::filesystem::path& path,DWORD timeout=30000) {
    const auto pid=GetProcessId(process);require(pid!=0,"Invalid owned process handle for module loading");
    const auto text=path.wstring();require(!text.empty()&&text.size()<32768,"Invalid payload path length");
    struct Block {
        HANDLE process;void* base{};bool retained{};
        ~Block(){if(base&&!retained)VirtualFreeEx(process,base,0,MEM_RELEASE);}
    } block{process};
    const auto context_offset=uintptr_t{0x1000},path_offset=context_offset+sizeof(Context);
    const auto size=path_offset+(text.size()+1)*sizeof(wchar_t);
    // Both APIs must resolve before allocating/mutating the child.
    const auto load_api=remote_api(pid,"LoadLibraryW"),error_api=remote_api(pid,"GetLastError");
    block.base=VirtualAllocEx(process,nullptr,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    require(block.base!=nullptr,"Cannot allocate module loader, Windows error "+std::to_string(GetLastError()));
    const auto base=reinterpret_cast<uintptr_t>(block.base);
    const auto write=[&](uintptr_t at,const void* data,size_t length) {
        SIZE_T written{};
        const auto ok=WriteProcessMemory(process,reinterpret_cast<void*>(at),data,length,&written);
        const auto error=ok?ERROR_PARTIAL_COPY:GetLastError();
        require(ok&&written==length,"Cannot write module loader, Windows error "+std::to_string(error));
    };
    const auto code_size=reinterpret_cast<uintptr_t>(UiDiagnosticLoadEnd)-reinterpret_cast<uintptr_t>(UiDiagnosticLoadBegin);
    require(code_size>0&&code_size<0x1000,"Invalid diagnostic loader block size");
    Context context{base+path_offset,load_api,error_api};
    write(base,UiDiagnosticLoadBegin,code_size);write(base+context_offset,&context,sizeof(context));
    write(context.path,text.c_str(),(text.size()+1)*sizeof(wchar_t));
    DWORD previous{};
    const auto protected_code=VirtualProtectEx(process,block.base,0x1000,PAGE_EXECUTE_READ,&previous);
    const auto protection_error=GetLastError();
    require(protected_code!=FALSE,"Cannot protect module loader, Windows error "+std::to_string(protection_error));
    require(FlushInstructionCache(process,block.base,code_size)!=FALSE,"Cannot flush module loader instruction cache");
    Handle thread(CreateRemoteThread(process,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(base),
        reinterpret_cast<void*>(base+context_offset),0,nullptr));
    require(thread.value!=nullptr,"Cannot start module loader, Windows error "+std::to_string(GetLastError()));
    block.retained=true; // A running/unknown thread may still reference code, context and path.
    const auto wait=WaitForSingleObject(thread,timeout);
    require(wait==WAIT_OBJECT_0,"Module loader wait did not complete (wait="+hex(wait)+
        "); code/context/path retained until owned process exit; do not reinject");
    block.retained=false;
    DWORD exit{};require(GetExitCodeThread(thread,&exit)!=FALSE,"Cannot read module loader thread status");
    SIZE_T got{};const auto ok=ReadProcessMemory(process,reinterpret_cast<void*>(base+context_offset),&context,sizeof(context),&got);
    const auto error=ok?ERROR_PARTIAL_COPY:GetLastError();
    require(ok&&got==sizeof(context),"Cannot read module loader result, Windows error "+std::to_string(error)+"; thread exit="+hex(exit));
    return {context.module,context.error,context.phase,exit};
}
}
