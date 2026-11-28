#pragma once
#include "game.hpp"
#include <wintrust.h>
#include <softpub.h>
#include <algorithm>
#include <cwchar>

namespace touchui::ef {
// The installed client requires an official launcher parent. Resolve and
// verify an installed GRYPHLINK image; never attach to an existing launcher.
inline HANDLE verify_parent(const std::filesystem::path& path) {
    if(_wcsicmp(path.filename().c_str(),L"Games.exe")!=0)
        throw std::runtime_error("EF: specify the installed GRYPHLINK Games.exe with --ef-launcher");
    Handle file(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));
    if(file.value==INVALID_HANDLE_VALUE)throw std::runtime_error("EF: cannot read official GRYPHLINK launcher");
    IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};DWORD got{};
    if(!ReadFile(file,&dos,sizeof(dos),&got,nullptr)||got!=sizeof(dos)||dos.e_magic!=IMAGE_DOS_SIGNATURE||
        dos.e_lfanew<sizeof(dos)||dos.e_lfanew>0x100000)throw std::runtime_error("EF: invalid official launcher PE image");
    LARGE_INTEGER offset{};offset.QuadPart=dos.e_lfanew;
    if(!SetFilePointerEx(file,offset,nullptr,FILE_BEGIN)||!ReadFile(file,&nt,sizeof(nt),&got,nullptr)||got!=sizeof(nt)||
        nt.Signature!=IMAGE_NT_SIGNATURE||nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||
        nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC)throw std::runtime_error("EF: official launcher must be Windows x64");
    WINTRUST_FILE_INFO info{};info.cbStruct=sizeof(info);info.pcwszFilePath=path.c_str();info.hFile=file;
    WINTRUST_DATA trust{};trust.cbStruct=sizeof(trust);trust.dwUIChoice=WTD_UI_NONE;
    trust.fdwRevocationChecks=WTD_REVOKE_NONE;trust.dwUnionChoice=WTD_CHOICE_FILE;trust.pFile=&info;
    trust.dwStateAction=WTD_STATEACTION_VERIFY;trust.dwProvFlags=WTD_CACHE_ONLY_URL_RETRIEVAL;
    GUID policy=WINTRUST_ACTION_GENERIC_VERIFY_V2;
    const auto status=WinVerifyTrust(nullptr,&policy,&trust);
    trust.dwStateAction=WTD_STATEACTION_CLOSE;WinVerifyTrust(nullptr,&policy,&trust);
    if(status!=ERROR_SUCCESS)throw std::runtime_error("EF: official launcher signature is not trusted");
    DWORD ignored{};const auto size=GetFileVersionInfoSizeW(path.c_str(),&ignored);
    std::vector<uint8_t> version(size);
    if(!size||!GetFileVersionInfoW(path.c_str(),0,size,version.data()))throw std::runtime_error("EF: launcher product information unavailable");
    struct Translation {WORD language,codepage;};Translation* languages{};UINT bytes{};
    if(!VerQueryValueW(version.data(),L"\\VarFileInfo\\Translation",reinterpret_cast<void**>(&languages),&bytes))
        throw std::runtime_error("EF: launcher product translation unavailable");
    bool gryphlink{};
    for(UINT index=0;index<bytes/sizeof(Translation);++index) {
        wchar_t key[96]{};swprintf_s(key,L"\\StringFileInfo\\%04x%04x\\ProductName",languages[index].language,languages[index].codepage);
        wchar_t* name{};UINT length{};
        if(VerQueryValueW(version.data(),key,reinterpret_cast<void**>(&name),&length)&&length&&_wcsicmp(name,L"GRYPHLINK")==0)gryphlink=true;
    }
    if(!gryphlink)throw std::runtime_error("EF: signed launcher product is not GRYPHLINK");
    HANDLE result=file.value;file.value=nullptr;return result;
}
inline std::vector<unsigned long> version_parts(const std::filesystem::path& folder) {
    std::vector<unsigned long> parts;const auto text=folder.filename().wstring();const wchar_t* at=text.c_str();
    while(*at) {
        wchar_t* end{};const auto part=std::wcstoul(at,&end,10);
        if(end==at||(*end&&*end!=L'.'))return {};
        parts.push_back(part);if(!*end)break;at=end+1;
    }
    return parts;
}
inline std::filesystem::path parent_for(const std::filesystem::path& requested={}) {
    if(!requested.empty()){Handle checked(verify_parent(requested));return requested;}
    wchar_t program_files[32768]{};const auto count=GetEnvironmentVariableW(L"ProgramFiles",program_files,32768);
    if(!count||count>=32768)throw std::runtime_error("EF: ProgramFiles is unavailable; specify --ef-launcher <Games.exe>");
    const auto root=std::filesystem::path(program_files)/L"GRYPHLINK";
    std::vector<std::filesystem::path> candidates;std::error_code error;
    if(std::filesystem::is_regular_file(root/L"Games.exe",error))candidates.push_back(root/L"Games.exe");
    for(std::filesystem::directory_iterator at(root,error),end;!error&&at!=end;at.increment(error)) {
        if(at->is_directory(error)&&!version_parts(at->path()).empty()&&
            std::filesystem::is_regular_file(at->path()/L"Games.exe",error))candidates.push_back(at->path()/L"Games.exe");
    }
    std::sort(candidates.begin(),candidates.end(),[](const auto& left,const auto& right){
        return version_parts(left.parent_path())>version_parts(right.parent_path());
    });
    for(const auto& candidate:candidates) {
        try {Handle checked(verify_parent(candidate));return candidate;}catch(const std::runtime_error&){}
    }
    throw std::runtime_error("EF: trusted GRYPHLINK Games.exe not found; specify --ef-launcher <Games.exe>");
}
class OfficialParent {
    Handle lifetime_{CreateJobObjectW(nullptr,nullptr)};
    game::Child child_;
public:
    explicit OfficialParent(const std::filesystem::path& path) {
        Handle verified(verify_parent(path));
        if(!lifetime_.value)throw std::runtime_error("EF: cannot track owned official parent lifetime");
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE|JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
        if(!SetInformationJobObject(lifetime_,JobObjectExtendedLimitInformation,&limits,sizeof(limits)))
            throw std::runtime_error("EF: cannot configure owned official parent cleanup");
        child_.start(path,true);
        if(!AssignProcessToJobObject(lifetime_,child_.info.hProcess))
            throw std::runtime_error("EF: cannot track owned official parent process");
        // The owned image is now mapped. Release the verification read lock.
    }
    HANDLE process()const{return child_.info.hProcess;}
};
}
