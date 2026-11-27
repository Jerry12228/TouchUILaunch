#pragma once
#include "nte_callbacks.hpp"

namespace nte_fixture {
// Entirely synthetic UE-like registrations and executable leaf functions.
// No preserved game instructions or game binaries are needed by default tests.
struct Fixture {
    std::vector<uint8_t> bytes=std::vector<uint8_t>(0xe000);
    uintptr_t base{};
    std::vector<std::pair<uint32_t,uint32_t>> functions;
    static constexpr uintptr_t predicate=0x2000,vtable=0x7000,nav_original=0x2100;
    explicit Fixture(bool saved_register=false) {
        base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,bytes.size(),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
        touchui::nte::require(base!=0,"fixture allocation failed");
        put<uint16_t>(0,0x5a4d);put<uint32_t>(0x3c,0x80);put<uint32_t>(0x80,0x4550);
        put<uint16_t>(0x84,0x8664);put<uint16_t>(0x86,4);put<uint16_t>(0x94,0xf0);
        put<uint16_t>(0x98,0x20b);put<uint32_t>(0x98+56,static_cast<uint32_t>(bytes.size()));put<uint32_t>(0x98+60,0x400);
        const auto section=[&](size_t index,const char* name,uint32_t at,uint32_t size,uint32_t flags) {
            const auto p=0x188+index*40;std::memcpy(bytes.data()+p,name,std::strlen(name));
            put<uint32_t>(p+8,size);put<uint32_t>(p+12,at);put<uint32_t>(p+16,size);put<uint32_t>(p+20,at);put<uint32_t>(p+36,flags);
        };
        section(0,".text",0x1000,0x4000,0x60000020);section(1,".rdata",0x5000,0x4000,0x40000040);
        section(2,".data",0x9000,0x3000,0xc0000040);section(3,".pdata",0xc000,0x2000,0x40000040);
        std::fill(bytes.begin()+0x1000,bytes.begin()+0x5000,uint8_t{0xcc});
        text(0x5000,"IsMobileCloudGameOperation");text(0x5040,"IsMobilePlatform");
        text(0x5080,"VehicleIsMobilePlatform");text(0x50c0,"InputTypePin");
        wide(0x5200,L"HTOneSDKUtil");wide(0x5280,L"HTUtil");wide(0x5300,L"HTUINavigationSubsystem");
        // Native tables have {name,exec}; generated tables have {ctor,name}.
        pair(0x6000,0x5000,0x1800);pair(0x6040,0x5040,0x1900);pair(0x6050,0x5080,0x1900);
        pair(0x6090,0x50c0,0x1b00);pair(0x60a0,0x5040,0x1a00);
        pair(0x6200,0x3000,0x5000);pair(0x6210,0x3100,0x5040);pair(0x6220,0x3200,0x5040);
        for(unsigned i=0;i<3;++i) {
            const uintptr_t getter=0x2400+i*0x100,registrar=0x2800+i*0x100,ctor=0x3000+i*0x100;
            lazy(getter,0x9000+i*8);lazy(ctor,0x9040+i*8);
            // Replace getter's return with class-name / constructor evidence.
            emit(getter+12,{0x48,0x8d,0x15,0,0,0,0});rel(getter+15,getter+19,0x5200+i*0x80);
            if(i==2) {
                emit(getter+19,{0x48,0x8d,0x05,0,0,0,0,0x49,0x89,0x43,0xd0,0xc3});rel(getter+22,getter+26,0x3400);
            } else emit(getter+19,{0xc3});
            emit(registrar,{0xe8,0,0,0,0,0x48,0x8d,0x15,0,0,0,0,0xc3});
            rel(registrar+1,registrar+5,getter);rel(registrar+8,registrar+12,i==0?0x6000:i==1?0x6040:0x6090);
            functions.emplace_back(static_cast<uint32_t>(registrar),static_cast<uint32_t>(registrar+13));
            const auto object=0xa000+i*0x100;const auto owner=0xa800+i*0x100;
            put<uintptr_t>(0x9000+i*8,base+owner);put<uintptr_t>(0x9040+i*8,base+object);
            put<uintptr_t>(object+0x20,base+owner);put<uintptr_t>(object+0xd8,base+0x1800+i*0x100);
            put<uint32_t>(object+0xb0,0x400);put<uint8_t>(object+0xb4,1);put<uint16_t>(object+0xb6,1);put<uint16_t>(object+0xb8,0);
        }
        emit(0x1800,{0x48,0x83,0xec,0x28,0xe8,0,0,0,0,0x48,0x83,0xc4,0x28,0xc3});rel(0x1805,0x1809,predicate);
        emit(0x1900,{0xc3});emit(0x1a00,{0xc3});emit(0x1b00,{0xc3});
        emit(predicate,{0x48,0x83,0xec,0x28,0x33,0xc0,0x48,0x83,0xc4,0x28,0xc3});
        if(saved_register) {
            emit(predicate,{0x40,0x53,0x48,0x83,0xec,0x20,0x33,0xc0,0x48,0x83,0xc4,0x20,0x5b,0xc3});
            functions.emplace_back(static_cast<uint32_t>(predicate),static_cast<uint32_t>(predicate+14));
        }
        emit(nav_original,{0x80,0xb9,0x48,0x01,0,0,0,0x0f,0x84,0,0,0,0,0xc3});rel(nav_original+9,nav_original+13,0x2200);
        emit(0x2200,{0x0f,0xb6,0x81,0x4b,0x01,0,0,0x88,0x91,0x4b,0x01,0,0,0xc3});
        emit(0x3400,{0x48,0x85,0xc9,0x0f,0x85,0,0,0,0,0xc3});rel(0x3405,0x3409,0x3500);
        emit(0x3500,{0x48,0x8d,0x05,0,0,0,0,0x48,0x89,0x03,0xc3});rel(0x3503,0x3507,vtable);
        put<uintptr_t>(vtable+0x3a0,base+nav_original);
        std::sort(functions.begin(),functions.end());
        put<uint32_t>(0x98+112+3*8,0xc000);put<uint32_t>(0x98+116+3*8,static_cast<uint32_t>(functions.size()*12));
        size_t entry=0xc000;
        for(const auto& [begin,end]:functions) {put<uint32_t>(entry,begin);put<uint32_t>(entry+4,end);put<uint32_t>(entry+8,0xd000);entry+=12;}
        put<uint8_t>(0xd000,1);sync();
    }
    Fixture(const Fixture&)=delete;Fixture& operator=(const Fixture&)=delete;
    ~Fixture(){if(base)VirtualFree(reinterpret_cast<void*>(base),0,MEM_RELEASE);}
    template<class T> void put(uintptr_t at,T value){std::memcpy(bytes.data()+at,&value,sizeof(value));}
    template<class T> T live(uintptr_t at)const {T result{};std::memcpy(&result,reinterpret_cast<void*>(base+at),sizeof(T));return result;}
    void emit(uintptr_t at,std::initializer_list<uint8_t> code){std::copy(code.begin(),code.end(),bytes.begin()+at);}
    void rel(uintptr_t at,uintptr_t next,uintptr_t target){put<int32_t>(at,static_cast<int32_t>(int64_t(target)-int64_t(next)));}
    void text(uintptr_t at,const char* value){std::memcpy(bytes.data()+at,value,std::strlen(value)+1);}
    void wide(uintptr_t at,const wchar_t* value){std::memcpy(bytes.data()+at,value,(std::wcslen(value)+1)*2);}
    void pair(uintptr_t at,uintptr_t first,uintptr_t second){put<uintptr_t>(at,base+first);put<uintptr_t>(at+8,base+second);}
    void lazy(uintptr_t fn,uintptr_t cache) {
        emit(fn,{0x48,0x8b,0x05,0,0,0,0,0x48,0x85,0xc0,0x75,0x00,0xc3});rel(fn+3,fn+7,cache);
    }
    void sync() {
        DWORD previous{};touchui::nte::require(VirtualProtect(reinterpret_cast<void*>(base),bytes.size(),PAGE_READWRITE,&previous)!=FALSE,"fixture writable");
        std::memcpy(reinterpret_cast<void*>(base),bytes.data(),bytes.size());
        touchui::nte::require(VirtualProtect(reinterpret_cast<void*>(base+0x1000),0x4000,PAGE_EXECUTE_READ,&previous)!=FALSE,"fixture executable");
        touchui::nte::require(VirtualProtect(reinterpret_cast<void*>(base+0x5000),0x4000,PAGE_READONLY,&previous)!=FALSE,"fixture read-only data");
        FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(base),bytes.size());
    }
    touchui::nte::ModuleImage snapshot()const{return {bytes,base};}
};
}
