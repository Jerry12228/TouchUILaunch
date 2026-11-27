#pragma once
#include "win_util.hpp"
#include <algorithm>
#include <fstream>
#include <string_view>

namespace touchui::nte {
inline constexpr std::wstring_view sdk_environment_name=L"ArcGameSDKSharePath";
inline std::filesystem::path sdk_for(const std::filesystem::path& game) {
    for(auto folder=game.parent_path();folder!=folder.root_path();folder=folder.parent_path()) {
        for(const auto& candidate:{folder/L"NTEGlobal"/L"WPGameClientSDK_64.dll",folder/L"WPGameClientSDK_64.dll"}) {
            if(!std::filesystem::is_regular_file(candidate))continue;
            std::ifstream file(candidate,std::ios::binary);IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
            file.read(reinterpret_cast<char*>(&dos),sizeof(dos));
            if(!file.good()||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<sizeof(dos)||dos.e_lfanew>=0x100000)
                throw std::runtime_error("Invalid NTE client SDK PE header");
            file.seekg(dos.e_lfanew);file.read(reinterpret_cast<char*>(&nt),sizeof(nt));
            if(!file.good()||nt.Signature!=IMAGE_NT_SIGNATURE||nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||
                nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC)throw std::runtime_error("NTE client SDK must be Windows x64");
            return candidate;
        }
    }
    throw std::runtime_error("NTE client SDK not found: keep NTEGlobal/WPGameClientSDK_64.dll in the installation; direct mode needs SDK files, not a running launcher");
}
inline std::wstring sdk_base(const std::filesystem::path& sdk) {
    // Both shipped bridge DLLs append _64.dll to ArcGameSDKSharePath.
    auto base=sdk;base.replace_filename(L"WPGameClientSDK");return base.wstring();
}
inline std::vector<wchar_t> sdk_environment(std::wstring_view base,std::vector<std::wstring> inherited) {
    if(base.empty()||base.find(L'\0')!=std::wstring_view::npos)throw std::runtime_error("Invalid NTE SDK environment path");
    std::erase_if(inherited,[](const auto& entry) {
        const auto separator=entry.find(L'=',entry.starts_with(L"=")?1:0);
        const auto name=entry.substr(0,separator);
        return _wcsicmp(name.c_str(),sdk_environment_name.data())==0;
    });
    inherited.push_back(std::wstring(sdk_environment_name)+L"="+std::wstring(base));
    std::sort(inherited.begin(),inherited.end(),[](const auto& a,const auto& b) {
        return CompareStringOrdinal(a.c_str(),-1,b.c_str(),-1,TRUE)==CSTR_LESS_THAN;
    });
    std::vector<wchar_t> block;
    for(const auto& entry:inherited) {
        if(entry.empty()||entry.find(L'\0')!=std::wstring::npos)throw std::runtime_error("Invalid inherited NTE environment entry");
        block.insert(block.end(),entry.begin(),entry.end());block.push_back(0);
    }
    block.push_back(0);return block;
}
inline std::vector<wchar_t> sdk_environment(const std::filesystem::path& sdk) {
    auto* raw=GetEnvironmentStringsW();if(!raw)throw std::runtime_error("Cannot read inherited process environment");
    struct Free {wchar_t* value;~Free(){FreeEnvironmentStringsW(value);}} free{raw};
    std::vector<std::wstring> inherited;
    for(auto* entry=raw;*entry;entry+=wcslen(entry)+1)inherited.emplace_back(entry);
    return sdk_environment(sdk_base(sdk),std::move(inherited));
}
}
