#pragma once
#include <windows.h>
#include <bcrypt.h>
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
inline std::wstring log_event_name(DWORD pid) {return L"Local\\ZZZTouchUI.Log."+std::to_wstring(pid);}
inline std::wstring started_event_name(DWORD pid) {return L"Local\\ZZZTouchUI.Started."+std::to_wstring(pid);}
inline std::string file_sha256(const std::filesystem::path& path) {
    Handle file(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr));
    if(file.value==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot open file for SHA-256");
    BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE hash{};
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("BCrypt algorithm error");
    if(BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)<0){BCryptCloseAlgorithmProvider(alg,0);throw std::runtime_error("BCrypt hash error");}
    std::array<unsigned char,65536> data{};std::array<unsigned char,32> digest{};DWORD n{};bool ok=true;
    for(;;){if(!ReadFile(file,data.data(),static_cast<DWORD>(data.size()),&n,nullptr)){ok=false;break;}if(!n)break;if(BCryptHashData(hash,data.data(),n,0)<0){ok=false;break;}}
    if(BCryptFinishHash(hash,digest.data(),static_cast<ULONG>(digest.size()),0)<0)ok=false;
    BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(alg,0);
    if(!ok)throw std::runtime_error("File hash read failed");
    const char* hex="0123456789abcdef";std::string result;
    for(auto b:digest){result+=hex[b>>4];result+=hex[b&15];}return result;
}
inline bool executable_pointer(const void* pointer) {
    MEMORY_BASIC_INFORMATION mbi{};
    if(!pointer||pointer==reinterpret_cast<const void*>(~uintptr_t{})||!VirtualQuery(pointer,&mbi,sizeof(mbi)))return false;
    if(mbi.State!=MEM_COMMIT||(mbi.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    return (mbi.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))!=0;
}
