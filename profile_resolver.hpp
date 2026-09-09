#pragma once
#include "discover_profile.hpp"
#include "win_util.hpp"
#include <map>
#include <mutex>
#include <ostream>

namespace discovery {
class MappedFile {
    Handle file_,mapping_;
    const uint8_t* view_{};
    size_t size_{};
public:
    explicit MappedFile(const std::filesystem::path& path) {
        // Disallow writes/replacement for the duration of hashing and scanning.
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
inline Result resolve(const std::filesystem::path& path,bool force_automatic=false) {
    const auto hash=file_sha256(path);
    if(const auto* known=profile::find(hash);known && !force_automatic) {
        Result result;result.build=*known;return result;
    }
    MappedFile file(path);
    require(file_sha256(path)==hash,"DLL changed while preparing discovery");
    // Only process-local results are cached; the file is rehashed on every use.
    static std::mutex mutex;
    static std::map<std::string,Result> cache;
    std::lock_guard lock(mutex);
    if(auto it=cache.find(hash);it!=cache.end())return it->second;
    Result result=discover(Image(file.bytes()));
    require(hash.size()==64,"invalid content hash");
    std::copy(hash.begin(),hash.end(),result.build.sha256);
    result.build.sha256[64]=0;
    cache.emplace(hash,result);
    return result;
}
inline void print_json(std::ostream& out,const Result& result) {
    const auto& p=result.build;
    out<<"{\n  \"mode\": \""<<(result.automatic?"automatic":"known")<<"\",\n  \"sha256\": \""<<p.sha256<<"\",\n  \"version\": \""<<p.version<<"\",\n  \"values\": {\n";
    bool first=true;
    for(const auto& field:profile::fields) {
        if(!first)out<<",\n";first=false;
        out<<"    \""<<field.name<<"\": \"0x"<<std::hex<<p.*(field.value)<<std::dec<<"\"";
    }
    out<<"\n  },\n  \"setter_candidates\": "<<result.setter_candidates<<"\n}\n";
}
inline void validate_loaded_code(uintptr_t base,const Result& result) {
    require(result.automatic && result.code_checks.size()==9,"missing live-code evidence");
    for(const auto& check:result.code_checks) {
        require(base<=UINTPTR_MAX-check.rva,"module address overflow");
        std::vector<uint8_t> actual(check.bytes.size());SIZE_T read{};
        require(ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(base+check.rva),actual.data(),actual.size(),&read) && read==actual.size(),"cannot read loaded code before hook installation");
        require(actual==check.bytes,"loaded code differs from scanned file; no hooks installed");
    }
}
}
