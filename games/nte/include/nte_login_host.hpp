#pragma once
#include "nte_relay.hpp"
#include "elevation.hpp"
#include <fstream>

namespace touchui::nte {
struct LoginHostPlan {
    std::filesystem::path executable,root;
    std::wstring region;
    std::wstring arguments(std::wstring_view extra) const {
        return elevation::parameters({L"--game-root",root.wstring(),L"--region",region,
            L"--game-arguments",std::wstring(extra),L"--language",L"zh-CN"});
    }
};
inline LoginHostPlan login_host_for(const std::filesystem::path& game,const std::filesystem::path& tool_directory,
    std::wstring_view requested={}) {
    namespace fs=std::filesystem;
    const auto relative=fs::path(L"Client/WindowsNoEditor/HT/Binaries/Win64/HTGame.exe");
    auto root=game;
    for(const auto& component:relative){(void)component;root=root.parent_path();}
    loader::require(relay::same_path(root/relative,game),"NTE direct login requires the installed Client directory layout");
    const auto has_cn=fs::is_regular_file(root/L"NTELauncher/NTEGame.exe");
    const auto has_global=fs::is_regular_file(root/L"NTEGlobal/NTEGlobalGame.exe");
    loader::require(requested.empty()||requested==L"CN"||requested==L"Global","--nte-region must be CN or Global");
    loader::require(!requested.empty()||has_cn!=has_global,
        "Cannot select a unique NTE SDK region; specify --nte-region CN or Global");
    std::wstring region=requested.empty()?(has_global?L"Global":L"CN"):std::wstring(requested);
    loader::require(region==L"CN"?has_cn:has_global,"Selected NTE region's installed SDK files are missing");
    const auto platform=root/(region==L"CN"?L"NTELauncher":L"NTEGlobal");
    loader::require(fs::is_regular_file(platform/L"WPGameClientSDK_64.dll"),"Installed NTE login SDK is missing");
    const auto host=tool_directory/L"nte-login-host/Nte.LoginHost.exe";
    loader::require(fs::is_regular_file(host),"NTE independent login host missing; build/package with -NteLoginHost");
    std::ifstream file(host,std::ios::binary);IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
    file.read(reinterpret_cast<char*>(&dos),sizeof(dos));
    loader::require(file.good()&&dos.e_magic==IMAGE_DOS_SIGNATURE&&dos.e_lfanew>=sizeof(dos)&&dos.e_lfanew<0x100000,
        "Invalid NTE login host PE header");
    file.seekg(dos.e_lfanew);file.read(reinterpret_cast<char*>(&nt),sizeof(nt));
    loader::require(file.good()&&nt.Signature==IMAGE_NT_SIGNATURE&&nt.FileHeader.Machine==IMAGE_FILE_MACHINE_AMD64&&
        nt.OptionalHeader.Magic==IMAGE_NT_OPTIONAL_HDR64_MAGIC,"NTE login host must be Windows x64");
    return {host,root,region};
}
}
