#pragma once
#include "game.hpp"
#include "mobile_resolver.hpp"
#include <psapi.h>
#include <iostream>

extern "C" const unsigned char GiUiBegin[],GiUiContext[],GiUiEnd[],SrUiBegin[],SrUiEnd[],UiLoadBegin[],UiLoadEnd[];
namespace mobile {
using Progress=std::function<void(std::string_view)>;
inline void read(HANDLE process,uintptr_t at,std::span<uint8_t> bytes) {
    SIZE_T got{};
    const auto success=ReadProcessMemory(process,reinterpret_cast<const void*>(at),bytes.data(),bytes.size(),&got);
    const auto error=success?ERROR_PARTIAL_COPY:GetLastError();
    require(success&&got==bytes.size(),"ReadProcessMemory failed ("+std::to_string(error)+")");
}
template<class T> T read(HANDLE process,uintptr_t at) {
    T value{};read(process,at,{reinterpret_cast<uint8_t*>(&value),sizeof(value)});return value;
}
inline void write(HANDLE process,uintptr_t at,const void* bytes,size_t size) {
    SIZE_T wrote{};
    const auto success=WriteProcessMemory(process,reinterpret_cast<void*>(at),bytes,size,&wrote);
    const auto error=success?ERROR_PARTIAL_COPY:GetLastError();
    require(success&&wrote==size,"WriteProcessMemory failed ("+std::to_string(error)+")");
}
template<class T> void write(HANDLE process,uintptr_t at,const T& value){write(process,at,&value,sizeof(value));}
inline void range(HANDLE process,uintptr_t at,size_t size,uintptr_t allocation,DWORD allowed) {
    require(at&&size&&size<=UINTPTR_MAX-at,"invalid memory range");
    const auto end=at+size;
    while(at<end) {
        MEMORY_BASIC_INFORMATION region{};
        require(VirtualQueryEx(process,reinterpret_cast<void*>(at),&region,sizeof(region))==sizeof(region),"cannot query target memory");
        require(region.State==MEM_COMMIT&&!(region.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&(region.Protect&allowed)&&
                reinterpret_cast<uintptr_t>(region.AllocationBase)==allocation,"invalid target memory ownership/protection");
        const auto next=reinterpret_cast<uintptr_t>(region.BaseAddress)+region.RegionSize;
        require(next>at,"invalid memory region size");at=std::min(next,end);
    }
}
constexpr DWORD readable=PAGE_READONLY|PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY;
constexpr DWORD executable=PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY;
// Toolhelp's module list may not exist yet for a CREATE_SUSPENDED process.
// Enumerate mapped images instead; system API addresses are rebased to the
// actual owning image in the child, never assumed equal across processes.
inline uintptr_t mapped_module(HANDLE process,const std::filesystem::path& name) {
    uintptr_t at{},last_allocation{},found{};
    MEMORY_BASIC_INFORMATION region{};
    while(VirtualQueryEx(process,reinterpret_cast<void*>(at),&region,sizeof(region))==sizeof(region)) {
        const auto allocation=reinterpret_cast<uintptr_t>(region.AllocationBase);
        if(region.Type==MEM_IMAGE&&allocation&&allocation!=last_allocation) {
            last_allocation=allocation;
            wchar_t path[32768]{};
            const auto length=GetMappedFileNameW(process,reinterpret_cast<void*>(allocation),path,32768);
            if(length&&length<32768&&_wcsicmp(std::filesystem::path(path).filename().c_str(),name.c_str())==0) {
                require(!found,"ambiguous mapped module name");found=allocation;
            }
        }
        const auto next=reinterpret_cast<uintptr_t>(region.BaseAddress)+region.RegionSize;
        if(next<=at)break;at=next;
    }
    return found;
}
inline uintptr_t remote_api(HANDLE process,const wchar_t* module,const char* name) {
    const auto local=GetProcAddress(GetModuleHandleW(module),name);HMODULE host{};
    require(local&&GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                    reinterpret_cast<LPCWSTR>(local),&host),"cannot resolve system API");
    const auto base=mapped_module(process,std::filesystem::path(module_path(host)).filename());
    require(base!=0,"system API host image is missing in child");
    const auto result=base+reinterpret_cast<uintptr_t>(local)-reinterpret_cast<uintptr_t>(host);
    range(process,result,1,base,executable);return result;
}
inline void bootstrap(HANDLE process) {
    USHORT machine{},native{};
    require(IsWow64Process2(process,&machine,&native)&&
        (machine==IMAGE_FILE_MACHINE_AMD64||(machine==IMAGE_FILE_MACHINE_UNKNOWN&&native==IMAGE_FILE_MACHINE_AMD64)),
        "target must be a Windows x64 process");
    const auto exit_thread=remote_api(process,L"ntdll.dll","RtlExitUserThread");
    Handle thread(CreateRemoteThread(process,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(exit_thread),nullptr,0,nullptr));
    require(thread.value!=nullptr,"cannot initialize child loader");
    require(WaitForSingleObject(thread,30000)==WAIT_OBJECT_0,"child loader initialization timed out");
    DWORD code{};require(GetExitCodeThread(thread,&code)&&code==0,"child loader initialization failed");
}
struct RemoteBlock {
    HANDLE process;uintptr_t base{};bool retained{};
    explicit RemoteBlock(HANDLE target):process(target) {
        base=reinterpret_cast<uintptr_t>(VirtualAllocEx(process,nullptr,0x2000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
        require(base!=0,"cannot allocate UI block");
    }
    RemoteBlock(const RemoteBlock&)=delete;RemoteBlock& operator=(const RemoteBlock&)=delete;
    ~RemoteBlock(){if(base&&!retained)VirtualFreeEx(process,reinterpret_cast<void*>(base),0,MEM_RELEASE);}
    uintptr_t context()const{return base+0x1000;}
    void code(const unsigned char* begin,const unsigned char* end) {
        const auto size=reinterpret_cast<uintptr_t>(end)-reinterpret_cast<uintptr_t>(begin);
        require(size>0&&size<0x1000,"invalid UI code block size");write(process,base,begin,size);
    }
    void executable_code() {
        DWORD old{};require(VirtualProtectEx(process,reinterpret_cast<void*>(base),0x1000,PAGE_EXECUTE_READ,&old),"cannot protect UI code");
        require(FlushInstructionCache(process,reinterpret_cast<void*>(base),0x1000),"cannot flush UI code");
    }
};
inline uintptr_t load_library(HANDLE process,const std::filesystem::path& path) {
    require(std::filesystem::is_regular_file(path),"required game module not found: "+path.filename().string());
    RemoteBlock block(process);block.code(UiLoadBegin,UiLoadEnd);
    const auto text=path.wstring();require((text.size()+1)*sizeof(wchar_t)<=0x1000-32,"module path is too long");
    struct Context {uintptr_t path,load,module;} context{block.context()+32,remote_api(process,L"kernel32.dll","LoadLibraryW"),0};
    write(process,block.context(),context);write(process,context.path,text.c_str(),(text.size()+1)*sizeof(wchar_t));block.executable_code();
    Handle thread(CreateRemoteThread(process,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(block.base),reinterpret_cast<void*>(block.context()),0,nullptr));
    require(thread.value!=nullptr,"cannot start module loader");
    block.retained=true; // A timeout must never free code/path under a live thread.
    require(WaitForSingleObject(thread,30000)==WAIT_OBJECT_0,"game module loading timed out");
    block.retained=false;
    DWORD code{};require(GetExitCodeThread(thread,&code)&&code==0,"game module loader failed");
    const auto result=read<Context>(process,block.context()).module;
    require(result!=0&&mapped_module(process,path.filename())==result,"game module was not loaded at the expected mapping");return result;
}
inline discovery::Image capture(HANDLE process,uintptr_t base) {
    const auto reader=[=](uintptr_t rva,std::span<uint8_t> target) {
        require(rva<=UINTPTR_MAX-base,"module address overflow");
        range(process,base+rva,target.size(),base,readable);read(process,base+rva,target);
    };
    std::array<uint8_t,64> dos{};reader(0,dos);
    uint32_t pe{};std::memcpy(&pe,dos.data()+0x3c,4);require(pe>=64&&pe<0x100000-256,"invalid loaded PE offset");
    std::array<uint8_t,88> nt{};reader(pe,nt);
    uint32_t headers{};std::memcpy(&headers,nt.data()+24+60,4);require(headers>=pe+88&&headers<=0x100000,"invalid loaded headers");
    std::vector<uint8_t> bytes(headers);reader(0,bytes);return discovery::Image(bytes,reader);
}
struct GiContext {
    uintptr_t target{},ui{},input{},klass{};
    uint32_t ui_offset{},input_offset{};
    std::array<uint8_t,16> original{};
    uintptr_t protect{},flush{};
    DWORD original_protection{},unused_protection{};
    LONG state{},outcome{};
};
struct SrContext {uintptr_t target{},sleep{};LONG stop{},started{};};
static_assert(offsetof(GiContext,original)==40&&offsetof(GiContext,protect)==56&&offsetof(GiContext,state)==80&&sizeof(GiContext)==88);
static_assert(offsetof(SrContext,stop)==16&&offsetof(SrContext,started)==20&&sizeof(SrContext)==24);
inline uintptr_t install_gi(HANDLE process,uintptr_t base,const GiResolution& resolved) {
    GiContext context{base+resolved.init,base+resolved.ui,base+resolved.input,base+resolved.klass,resolved.ui_offset,resolved.input_offset};
    range(process,context.target,16,base,executable);
    range(process,context.ui,1,base,executable);range(process,context.input,1,base,executable);
    range(process,context.klass,8,base,readable);
    require((context.target&0xfff)<=0xff0,"GI hook crosses a protection page");
    read(process,context.target,context.original);
    context.protect=remote_api(process,L"kernel32.dll","VirtualProtect");
    context.flush=remote_api(process,L"kernel32.dll","FlushInstructionCache");
    RemoteBlock block(process);block.code(GiUiBegin,GiUiEnd);
    const auto context_offset=reinterpret_cast<uintptr_t>(GiUiContext)-reinterpret_cast<uintptr_t>(GiUiBegin);
    write(process,block.base+context_offset,block.context());block.executable_code();
    require(VirtualProtectEx(process,reinterpret_cast<void*>(context.target),16,PAGE_EXECUTE_READWRITE,&context.original_protection),"cannot prepare GI hook page");
    try {
        write(process,block.context(),context);
        std::array<uint8_t,16> jump{0xff,0x25,0x02,0,0,0,0x90,0x90};
        std::memcpy(jump.data()+8,&block.base,8);
        block.retained=true; // From first target write onward, child owns callback lifetime.
        write(process,context.target,jump.data(),jump.size());
        require(FlushInstructionCache(process,reinterpret_cast<void*>(context.target),16),"cannot flush GI hook");
    } catch(...) {
        SIZE_T ignored{};WriteProcessMemory(process,reinterpret_cast<void*>(context.target),context.original.data(),16,&ignored);
        DWORD old{};VirtualProtectEx(process,reinterpret_cast<void*>(context.target),16,context.original_protection,&old);
        FlushInstructionCache(process,reinterpret_cast<void*>(context.target),16);throw;
    }
    return block.context();
}
struct InstalledTask {uintptr_t context;DWORD thread_id;};
inline InstalledTask install_sr(HANDLE process,uintptr_t base,uintptr_t rva) {
    const auto target=base+rva;range(process,target,4,base,PAGE_READWRITE|PAGE_WRITECOPY);
    SrContext context{target,remote_api(process,L"kernel32.dll","Sleep")};
    RemoteBlock block(process);block.code(SrUiBegin,SrUiEnd);write(process,block.context(),context);block.executable_code();
    DWORD thread_id{};
    Handle thread(CreateRemoteThread(process,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(block.base),reinterpret_cast<void*>(block.context()),0,&thread_id));
    require(thread.value!=nullptr,"cannot start SR UI task");block.retained=true;
    for(int i=0;i<100;++i) {
        if(read<SrContext>(process,block.context()).started)return {block.context(),thread_id};
        require(WaitForSingleObject(thread,10)==WAIT_TIMEOUT,"SR UI task exited before initialization");
    }
    throw std::runtime_error("Mobile UI: SR UI task startup timed out");
}
inline void initialize(game::Child& child,game::Kind kind,const std::filesystem::path& path,bool logging,const Progress& progress={}) {
    const auto stage=[&](std::string_view message){if(progress)progress(message);else if(logging)std::cout<<"Stage: "<<message<<std::endl;};
    stage("initialize suspended process loader");
    const auto process=child.info.hProcess;bootstrap(process);
    if(kind==game::Kind::SR) {
        stage("SR: load GameAssembly.dll");
        const auto base=load_library(process,path.parent_path()/L"GameAssembly.dll");
        stage("SR: capture loaded module");
        const auto image=capture(process,base);
        stage("SR: resolve UI state");
        const auto resolved=resolve_sr(image);
        if(logging)std::cout<<"SR GameAssembly base=0x"<<std::hex<<base<<" UI state RVA=0x"<<resolved<<std::dec<<"\n";
        stage("SR: install UI task");install_sr(process,base,resolved);
        if(logging)std::cout<<"SR mobile UI task installed (type 2, 500 ms). In-game UI is not yet verified.\n";
    } else {
        stage("GI: locate main image");
        auto base=mapped_module(process,path.filename());require(base!=0,"GI main image missing");
        stage("GI: capture main image");
        auto image=capture(process,base);
        const auto has_il2cpp=std::any_of(image.sections.begin(),image.sections.end(),[](const auto& section){return std::string_view(section.name,6)=="il2cpp";});
        if(!has_il2cpp) {
            stage("GI: load legacy UserAssembly.dll");
            base=load_library(process,path.parent_path()/(path.stem().wstring()+L"_Data")/L"Native"/L"UserAssembly.dll");
            stage("GI: capture UserAssembly.dll");image=capture(process,base);
        }
        stage("GI: resolve UI functions and input object");
        const auto resolved=resolve_gi(image);
        if(logging)std::cout<<"GI "<<(has_il2cpp?"main image":"UserAssembly")<<" base=0x"<<std::hex<<base
            <<" init RVA=0x"<<resolved.init<<" UI RVA=0x"<<resolved.ui<<" input RVA=0x"<<resolved.input
            <<" class RVA=0x"<<resolved.klass<<" UI offset=0x"<<resolved.ui_offset<<" input offset=0x"<<resolved.input_offset<<std::dec<<"\n";
        stage("GI: install initialization hook");install_gi(process,base,resolved);
        if(logging)std::cout<<"GI mobile UI initialization hook installed. In-game UI is not yet verified.\n";
    }
}
}
