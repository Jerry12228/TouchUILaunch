#pragma once
#include <windows.h>
#include <cstdint>
#include <filesystem>
#include <string>
#include <array>
#include <vector>
#include <stdexcept>
struct Handle {
    HANDLE value{};
    explicit Handle(HANDLE h=nullptr):value(h){}
    ~Handle(){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
    Handle(const Handle&)=delete;Handle& operator=(const Handle&)=delete;
    operator HANDLE()const{return value;}
};
inline std::wstring module_path(HMODULE module=nullptr) {
    std::vector<wchar_t> b(32768);
    DWORD n=GetModuleFileNameW(module,b.data(),static_cast<DWORD>(b.size()));
    if(!n||n>=b.size())throw std::runtime_error("GetModuleFileName failed");
    return {b.data(),n};
}
inline std::wstring log_event_name(DWORD pid) {return L"Local\\TouchUILaunch.Log."+std::to_wstring(pid);}
inline std::wstring started_event_name(DWORD pid) {return L"Local\\TouchUILaunch.Started."+std::to_wstring(pid);}
inline bool executable_pointer(const void* pointer) {
    MEMORY_BASIC_INFORMATION mbi{};
    if(!pointer||pointer==reinterpret_cast<const void*>(~uintptr_t{})||!VirtualQuery(pointer,&mbi,sizeof(mbi)))return false;
    if(mbi.State!=MEM_COMMIT||(mbi.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    return (mbi.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))!=0;
}
