#pragma once
#include "gi_joystick_code.hpp"
#include "game.hpp"
#include <winternl.h>
#include <memory>
#include <iostream>

namespace touchui::gi {
// A file lock protects preflight through child creation; no hash/version gate.
class PreparedImage {
    Handle file_, mapping_;
    struct View { const uint8_t* bytes{}; ~View(){ if(bytes) UnmapViewOfFile(bytes); } } view_;
    std::unique_ptr<discovery::Image> image_;
public:
    Plan plan;
    uint32_t header_size{};
    explicit PreparedImage(const std::filesystem::path& path)
        : file_(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr)) {
        check(file_.value!=INVALID_HANDLE_VALUE,"cannot open executable for discovery");
        LARGE_INTEGER size{};
        check(GetFileSizeEx(file_,&size)&&size.QuadPart>=256&&size.QuadPart<=0x80000000LL,"invalid executable size");
        mapping_.value=CreateFileMappingW(file_,nullptr,PAGE_READONLY,0,0,nullptr);
        check(mapping_.value!=nullptr,"cannot map executable");
        view_.bytes=static_cast<const uint8_t*>(MapViewOfFile(mapping_,FILE_MAP_READ,0,0,0));
        check(view_.bytes!=nullptr,"cannot read executable mapping");
        image_=std::make_unique<discovery::Image>(std::span(view_.bytes,static_cast<size_t>(size.QuadPart)));
        uint32_t pe{}; std::memcpy(&pe,view_.bytes+0x3c,4);
        std::memcpy(&header_size,view_.bytes+pe+24+60,4);
        check(header_size<=0x100000,"excessive image headers");
        plan=resolve(*image_);
    }
    uint32_t image_size()const{return image_->image_size;}
};

inline void read_process(HANDLE process,uintptr_t address,void* data,size_t size) {
    SIZE_T done{};
    check(ReadProcessMemory(process,reinterpret_cast<const void*>(address),data,size,&done)&&done==size,"GI: remote read failed");
}
inline uintptr_t child_image_base(const game::Child& child) {
    using Query=NTSTATUS(NTAPI*)(HANDLE,PROCESSINFOCLASS,PVOID,ULONG,PULONG);
    const auto query=reinterpret_cast<Query>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"NtQueryInformationProcess"));
    check(query!=nullptr,"GI: process information API unavailable");
    PROCESS_BASIC_INFORMATION basic{};
    check(query(child.info.hProcess,ProcessBasicInformation,&basic,sizeof(basic),nullptr)>=0&&basic.PebBaseAddress,
          "GI: cannot locate owned child PEB");
    // Windows x64 PEB.ImageBaseAddress. Toolhelp module enumeration is not ready
    // before the suspended primary thread has run the loader.
    uintptr_t base{};
    read_process(child.info.hProcess,reinterpret_cast<uintptr_t>(basic.PebBaseAddress)+0x10,&base,sizeof(base));
    check(base!=0,"GI: owned child image base unavailable");
    return base;
}
class RemoteCode {
    HANDLE process_;
    uintptr_t base_;
    uint32_t size_;
    uintptr_t address(uint32_t rva,size_t size)const {
        check(rva<=size_&&size<=size_-rva&&base_<=UINTPTR_MAX-size_,"GI: remote range outside image");
        return base_+rva;
    }
public:
    RemoteCode(HANDLE process,uintptr_t base,uint32_t size):process_(process),base_(base),size_(size){}
    std::vector<uint8_t> read(uint32_t rva,size_t size)const {
        std::vector<uint8_t> result(size);read_process(process_,address(rva,size),result.data(),size);return result;
    }
    void write_code(uint32_t rva,std::span<const uint8_t> bytes) {
        auto* target=reinterpret_cast<void*>(address(rva,bytes.size()));
        MEMORY_BASIC_INFORMATION info{};
        check(VirtualQueryEx(process_,target,&info,sizeof(info))==sizeof(info)&&info.State==MEM_COMMIT&&
              !(info.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&
              (info.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_WRITECOPY))&&
              reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(info.BaseAddress)+bytes.size()<=info.RegionSize,
              "GI: patch target is not a single executable region");
        DWORD previous{};
        check(VirtualProtectEx(process_,target,bytes.size(),PAGE_EXECUTE_READWRITE,&previous)!=0,"GI: cannot make UI code writable");
        SIZE_T done{};
        const bool written=WriteProcessMemory(process_,target,bytes.data(),bytes.size(),&done)&&done==bytes.size();
        const bool flushed=written&&FlushInstructionCache(process_,target,bytes.size());
        DWORD ignored{};
        const bool restored=VirtualProtectEx(process_,target,bytes.size(),previous,&ignored)!=0;
        check(written&&flushed&&restored,"GI: UI write/cache/protection failure; owned child will be terminated");
    }
};

class JoystickCode {
    HANDLE process_;
    uint8_t* allocation_{};
public:
    explicit JoystickCode(HANDLE process, uintptr_t image_base) : process_(process) {
        // All supported patch sites are in one <=2GB image. Allocate close to
        // its base, then range-check every actual CALL before publishing it.
        constexpr uintptr_t step = 0x10000;
        for (uintptr_t distance = step; distance < 0x70000000 && !allocation_; distance += step) {
            for (auto address : {image_base + distance, image_base > distance ? image_base - distance : uintptr_t{0}}) {
                if (!address) continue;
                MEMORY_BASIC_INFORMATION info{};
                if (VirtualQueryEx(process_, reinterpret_cast<void*>(address), &info, sizeof(info)) != sizeof(info) ||
                    info.State != MEM_FREE) continue;
                allocation_ = static_cast<uint8_t*>(VirtualAllocEx(process_, reinterpret_cast<void*>(address),
                    4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
                if (allocation_) break;
            }
        }
        check(allocation_ != nullptr, "cannot allocate nearby joystick helper");
    }
    ~JoystickCode() { if (allocation_) VirtualFreeEx(process_, allocation_, 0, MEM_RELEASE); }
    JoystickCode(const JoystickCode&) = delete;
    JoystickCode& operator=(const JoystickCode&) = delete;
    void prepare(Plan& plan, uintptr_t image_base) {
        std::vector<uint8_t> code;
        for (auto& site : plan) {
            if (site.hook == Hook::none) continue;
            while (code.size() % 16) code.push_back(0xcc);
            const auto target = reinterpret_cast<uintptr_t>(allocation_) + code.size();
            auto helper = joystick_code(site);
            site.replacement = call_patch(image_base + site.rva + site.offset, target, site.replacement.size());
            code.insert(code.end(), helper.begin(), helper.end());
        }
        check(!code.empty() && code.size() <= 4096, "joystick helper exceed allocation");
        SIZE_T done{};
        check(WriteProcessMemory(process_, allocation_, code.data(), code.size(), &done) && done == code.size(),
              "cannot write joystick helper");
        std::vector<uint8_t> readback(code.size());
        read_process(process_, reinterpret_cast<uintptr_t>(allocation_), readback.data(), readback.size());
        check(readback == code, "joystick helper readback mismatch");
        DWORD previous{};
        check(VirtualProtectEx(process_, allocation_, 4096, PAGE_EXECUTE_READ, &previous) != 0 &&
              FlushInstructionCache(process_, allocation_, code.size()), "cannot finalize executable joystick helper");
    }
    void release() { allocation_ = nullptr; } // Kept until the successfully started child exits.
};

template<class Progress>
void initialize(game::Child& child, const PreparedImage& file, bool logging, Progress&& progress) {
    check(child.info.hProcess && child.info.hThread, "owned suspended child required");
    progress("GI: search loaded UI and joystick instructions");
    const auto base = child_image_base(child);
    std::vector<uint8_t> headers(file.header_size);
    read_process(child.info.hProcess, base, headers.data(), headers.size());
    discovery::Image image(headers, [&](uintptr_t rva, std::span<uint8_t> out) {
        check(rva <= file.image_size() && out.size() <= file.image_size() - rva &&
              base <= UINTPTR_MAX - file.image_size(), "snapshot range outside image");
        read_process(child.info.hProcess, base + rva, out.data(), out.size());
    });
    check(image.image_size == file.image_size(), "file and loaded image sizes disagree");
    auto plan = resolve(image);
    check(plan.size() == file.plan.size(), "file and loaded plan sizes disagree");
    for (size_t i = 0; i < plan.size(); ++i)
        check(plan[i].rva == file.plan[i].rva && plan[i].expected == file.plan[i].expected,
              "file and loaded instruction evidence disagree");
    RemoteCode memory(child.info.hProcess, base, image.image_size);
    validate_plan(memory, plan);
    JoystickCode joystick(child.info.hProcess, base);
    joystick.prepare(plan, base);
    apply_plan(memory, plan);
    joystick.release();
    progress("GI: Mobile layout, TouchScreen input and joystick setup installed");
    if (logging) {
        std::cout << "GI: " << plan.size() << " instruction windows located by signatures and validated. "
            "Joystick diameter/travel: 1x (0.2.0 baseline); gesture delta/pinch: unmodified; touch DPI baseline: 360.\n";
        for (const auto& site : plan)
            std::cout << "GI: " << site.name << " at RVA 0x" << std::hex << site.rva << std::dec << '\n';
    }
}
}
