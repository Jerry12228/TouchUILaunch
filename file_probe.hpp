#pragma once
#include "discover_profile.hpp"
#include "win_util.hpp"
#include <bcrypt.h>
#include <array>
#include <ostream>

namespace discovery {
// Read-only diagnostics only. Never included by the injected DLL.
class MappedFile {
    Handle file_,mapping_;
    const uint8_t* view_{};
    size_t size_{};
public:
    explicit MappedFile(const std::filesystem::path& path) {
        file_.value=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        require(file_.value!=INVALID_HANDLE_VALUE,"cannot open a stable read-only DLL file (Windows error "+std::to_string(GetLastError())+")");
        LARGE_INTEGER length{};
        require(GetFileSizeEx(file_,&length) && length.QuadPart>0 && length.QuadPart<=0xffffffff,"invalid DLL file size");
        size_=static_cast<size_t>(length.QuadPart);
        mapping_.value=CreateFileMappingW(file_,nullptr,PAGE_READONLY,0,0,nullptr);
        require(mapping_.value!=nullptr,"cannot create read-only file mapping");
        view_=static_cast<const uint8_t*>(MapViewOfFile(mapping_,FILE_MAP_READ,0,0,0));
        require(view_!=nullptr,"cannot map DLL data");
    }
    ~MappedFile(){if(view_)UnmapViewOfFile(view_);}
    MappedFile(const MappedFile&)=delete;
    MappedFile& operator=(const MappedFile&)=delete;
    std::span<const uint8_t> bytes()const{return {view_,size_};}
};
inline std::string sha256(std::span<const uint8_t> bytes) {
    BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE hash{};
    require(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0,"BCrypt algorithm error");
    if(BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)<0){BCryptCloseAlgorithmProvider(alg,0);throw std::runtime_error("BCrypt hash error");}
    std::array<uint8_t,32> digest{};bool ok=true;
    while(!bytes.empty()) {
        const auto size=static_cast<ULONG>(std::min<size_t>(bytes.size(),0x100000));
        if(BCryptHashData(hash,const_cast<PUCHAR>(bytes.data()),size,0)<0){ok=false;break;}
        bytes=bytes.subspan(size);
    }
    if(BCryptFinishHash(hash,digest.data(),static_cast<ULONG>(digest.size()),0)<0)ok=false;
    BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(alg,0);
    require(ok,"diagnostic hash failed");
    const char* hex="0123456789abcdef";std::string result;
    for(auto b:digest){result+=hex[b>>4];result+=hex[b&15];}return result;
}
struct Probe {Result resolution;std::string sha256;};
inline Probe probe_file(const std::filesystem::path& path) {
    const MappedFile file(path);
    // Hash and discovery use the same read-only mapping; no hash lookup/cache.
    const auto hash=sha256(file.bytes());
    return {discover(Image(file.bytes())),hash};
}
inline void print_json(std::ostream& out,const Probe& probe) {
    const auto& p=probe.resolution.build;
    out<<"{\n  \"mode\": \"automatic\",\n  \"source\": \"file\",\n  \"sha256\": \""<<probe.sha256<<"\",\n  \"version\": \"auto\",\n  \"values\": {\n";
    bool first=true;
    for(const auto& field:profile::fields) {
        if(!first)out<<",\n";first=false;
        out<<"    \""<<field.name<<"\": \"0x"<<std::hex<<p.*(field.value)<<std::dec<<"\"";
    }
    out<<"\n  },\n  \"setter_candidates\": "<<probe.resolution.setter_candidates<<"\n}\n";
}
}
