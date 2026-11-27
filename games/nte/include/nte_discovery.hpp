#pragma once
#include "nte_image.hpp"
#include "x64_reader.hpp"
#include <map>
#include <set>
#include <sstream>
#include <iomanip>

namespace touchui::nte {
namespace dx=discovery::x64;
struct Function {
    uintptr_t exec{},cache{},class_cache{},object{},owner{};
    std::vector<uintptr_t> candidates;
};
enum class NativePatch {AtomicStack,SavedRegisterDetour};
struct Plan {
    uintptr_t mobile_predicate{},navigation_slot{},navigation_original{};
    uint32_t predicate_original{};
    NativePatch native_patch{NativePatch::AtomicStack};
    Function cloud,util_mobile,navigation_mobile;
};
inline std::vector<dx::Ins> instructions(const ModuleImage& module,uintptr_t at,size_t limit=256) {
    const auto& image=module.image();std::vector<dx::Ins> result;
    auto* section=image.section(at,1);require(section&&section->code(),"expected native code");
    const auto end=std::min<uintptr_t>(at+limit,section->rva+section->size);
    while(at<end) {
        auto ins=dx::decode(image,at);if(ins.next()>end)break;
        result.push_back(ins);at=ins.next();if(ins.ret()||ins.jump())break;
    }
    return result;
}
inline std::vector<dx::Ins> reachable_instructions(const ModuleImage& module,uintptr_t start,size_t limit) {
    const auto& image=module.image();std::map<uintptr_t,dx::Ins> decoded;
    std::vector<uintptr_t> pending={start};
    while(!pending.empty()) {
        const auto at=pending.back();pending.pop_back();
        if(at<start||at-start>=limit||decoded.contains(at))continue;
        const auto ins=dx::decode(image,at);if(ins.next()-start>limit)continue;
        for(const auto& [other,previous]:decoded)
            require(ins.next()<=other||at>=previous.next(),"overlapping control flow in setter evidence");
        decoded.emplace(at,ins);
        if(ins.conditional()||ins.jump())pending.push_back(ins.relative(image));
        if(!ins.ret()&&!ins.jump())pending.push_back(ins.next());
    }
    std::vector<dx::Ins> result;
    for(const auto& [at,ins]:decoded){(void)at;result.push_back(ins);}return result;
}
inline std::optional<uintptr_t> lazy_cache(const ModuleImage& module,uintptr_t fn) {
    if(!module.code(fn))return {};
    try {
        auto ins=instructions(module,fn,128);
        for(size_t i=0;i<std::min<size_t>(8,ins.size());++i)if(ins[i].mov_load()&&ins[i].reg()==0&&ins[i].base()==dx::rip) {
            for(size_t j=i+1;j<std::min(ins.size(),i+4);++j)if(ins[j].test(0)) {
                const auto slot=ins[i].storage(module.image());
                if(module.data(slot,8,true)&&slot%8==0)return slot;
            }
        }
    } catch(const std::exception&) {}
    return {};
}
struct Registration {std::string name;uintptr_t at{},exec{},array{};};
struct Registrations {
    std::vector<Registration> native;
    std::map<std::string,std::vector<uintptr_t>> constructors;
};
inline Registrations registrations(const ModuleImage& module) {
    const auto& image=module.image();
    const std::set<std::string> wanted={"IsMobileCloudGameOperation","IsMobilePlatform","VehicleIsMobilePlatform","InputTypePin"};
    std::map<uint64_t,std::string> strings;
    for(const auto& s:image.sections)if(!s.code()&&(s.flags&IMAGE_SCN_MEM_READ)) {
        auto bytes=image.view(s.rva,s.size);
        for(const auto& name:wanted) {
            auto first=bytes.begin();
            while(first!=bytes.end()) {
                auto it=std::search(first,bytes.end(),name.begin(),name.end());
                if(it==bytes.end())break;
                const auto pos=static_cast<size_t>(it-bytes.begin());
                if(pos+name.size()<bytes.size()&&bytes[pos+name.size()]==0)strings[module.base+s.rva+pos]=name;
                first=it+1;
            }
        }
    }
    Registrations out;
    const auto pair=[&](uintptr_t at) {
        if(!module.data(at,16))return false;
        const auto name=image.read<uint64_t>(at),pointer=image.read<uint64_t>(at+8);
        if(name<module.base||pointer<module.base||name-module.base>=image.image_size||pointer-module.base>=image.image_size)return false;
        auto text=module.ascii(static_cast<uintptr_t>(name-module.base));
        return !text.empty()&&module.code(static_cast<uintptr_t>(pointer-module.base));
    };
    for(const auto& s:image.sections)if(!s.code()&&(s.flags&IMAGE_SCN_MEM_READ)) {
        for(uintptr_t at=(s.rva+7)&~uintptr_t{7};at+16<=s.rva+s.size;at+=8) {
            const auto found=strings.find(image.read<uint64_t>(at));if(found==strings.end())continue;
            const auto after=image.read<uint64_t>(at+8);
            const auto before=at>=s.rva+8?image.read<uint64_t>(at-8):0;
            if(before>=module.base&&before-module.base<image.image_size&&lazy_cache(module,static_cast<uintptr_t>(before-module.base))) {
                // FClassFunctionLinkInfo is {constructor, name}.
                out.constructors[found->second].push_back(static_cast<uintptr_t>(before-module.base));
            } else if(after>=module.base&&after-module.base<image.image_size&&module.code(static_cast<uintptr_t>(after-module.base))&&!lazy_cache(module,static_cast<uintptr_t>(after-module.base))) {
                // FNameNativePtrPair has the opposite ordering: {name, exec}.
                auto start=at;
                for(unsigned n=0;n<4096&&start>=16&&pair(start-16);++n)start-=16;
                out.native.push_back({found->second,at,static_cast<uintptr_t>(after-module.base),start});
            }
        }
    }
    require(!out.native.empty(),"UE native registration tables are not ready");return out;
}
inline std::map<uintptr_t,std::vector<uintptr_t>> lea_references(const ModuleImage& module,const std::set<uintptr_t>& targets) {
    const auto& image=module.image();std::map<uintptr_t,std::vector<uintptr_t>> result;
    for(const auto& s:image.sections)if(s.code()) {
        auto bytes=image.view(s.rva,s.size);
        for(size_t p=0;p+7<=bytes.size();++p) {
            if((bytes[p]&0xf8)!=0x48||bytes[p+1]!=0x8d||(bytes[p+2]&0xc7)!=5)continue;
            int32_t disp{};std::memcpy(&disp,bytes.data()+p+3,4);
            const auto target=int64_t(s.rva+p+7)+disp;
            if(target<0||!targets.contains(static_cast<uintptr_t>(target)))continue;
            try {
                const auto fn=module.function_start(s.rva+p);
                auto ins=instructions(module,fn,256);
                if(std::any_of(ins.begin(),ins.end(),[&](const auto& i){return i.at==s.rva+p&&i.h.opcode==0x8d&&i.base()==dx::rip;}))
                    result[static_cast<uintptr_t>(target)].push_back(fn);
            } catch(const std::exception&) {}
        }
    }
    for(auto& [key,values]:result){(void)key;std::sort(values.begin(),values.end());values.erase(std::unique(values.begin(),values.end()),values.end());}
    return result;
}
inline uintptr_t class_getter(const ModuleImage& module,uintptr_t registrar,std::wstring_view expected) {
    auto list=instructions(module,registrar,160);
    for(const auto& i:list)if(i.call()) {
        auto target=i.relative(module.image());
        for(const auto& part:instructions(module,target,192))if(part.h.opcode==0x8d&&part.reg()==2&&part.base()==dx::rip&&module.wide_equals(part.storage(module.image()),expected))return target;
        break;
    }
    throw std::runtime_error("NTE: registration owner class mismatch");
}
inline uintptr_t primary_vtable(const ModuleImage& module,uintptr_t getter) {
    auto list=instructions(module,getter,192);uintptr_t ctor{};
    for(size_t i=0;i+1<list.size();++i)if(list[i].h.opcode==0x8d&&list[i].reg()==0&&list[i].base()==dx::rip&&
        list[i+1].mov_store()&&list[i+1].reg()==0&&list[i+1].base()==11&&list[i+1].disp()==-0x30)ctor=list[i].storage(module.image());
    require(ctor,"class constructor relationship is missing");
    auto thunk=instructions(module,ctor,48);uintptr_t native{};
    for(const auto& i:thunk)if(i.condition(5)||i.jump()) {native=i.relative(module.image());break;}
    require(native,"class constructor thunk is unsupported");
    auto body=instructions(module,native,192);std::set<uintptr_t> tables;
    for(size_t i=0;i<body.size();++i)if(body[i].h.opcode==0x8d&&body[i].base()==dx::rip) {
        const auto table=body[i].storage(module.image());if(table%8||!module.data(table,0x3a8))continue;
        for(size_t j=i+1;j<std::min(body.size(),i+6);++j)if(body[j].mov_store()&&body[j].reg()==body[i].reg()&&body[j].disp()==0&&body[j].base()>=0)tables.insert(table);
    }
    require(tables.size()==1,"expected one primary navigation vtable");return *tables.begin();
}
inline Plan discover_static(const ModuleImage& module) {
    const auto& image=module.image();auto regs=registrations(module);
    std::set<uintptr_t> arrays;for(const auto& r:regs.native)arrays.insert(r.array);
    auto refs=lea_references(module,arrays);
    const auto select=[&](std::string_view name,std::wstring_view owner) {
        std::vector<std::pair<Registration,uintptr_t>> matches;
        for(const auto& r:regs.native)if(r.name==name)for(const auto fn:refs[r.array]) {
            try {matches.push_back({r,class_getter(module,fn,owner)});}catch(const std::exception&) {}
        }
        std::string owner_text;for(const auto c:owner)owner_text.push_back(static_cast<char>(c));
        require(matches.size()==1,"missing or ambiguous "+owner_text+"."+std::string(name)+
            " owner registration (matches="+std::to_string(matches.size())+")");return matches.front();
    };
    const auto cloud=select("IsMobileCloudGameOperation",L"HTOneSDKUtil");
    const auto util=select("IsMobilePlatform",L"HTUtil");
    const auto nav=select("IsMobilePlatform",L"HTUINavigationSubsystem");
    const auto pin=select("InputTypePin",L"HTUINavigationSubsystem");
    require(pin.first.array==nav.first.array,"navigation functions belong to different tables");
    Plan plan;
    const auto function=[&](const auto& entry) {
        Function f;f.exec=entry.first.exec;f.class_cache=lazy_cache(module,entry.second).value_or(0);
        require(f.class_cache,"owner class cache missing");return f;
    };
    plan.cloud=function(cloud);plan.util_mobile=function(util);plan.navigation_mobile=function(nav);
    const auto candidates=[&](Function& fn,const std::string& name) {
        for(const auto ctor:regs.constructors[name])if(auto cache=lazy_cache(module,ctor))fn.candidates.push_back(*cache);
        std::sort(fn.candidates.begin(),fn.candidates.end());
        fn.candidates.erase(std::unique(fn.candidates.begin(),fn.candidates.end()),fn.candidates.end());
        require(!fn.candidates.empty(),"generated function cache missing: "+name);
    };
    candidates(plan.cloud,"IsMobileCloudGameOperation");candidates(plan.util_mobile,"IsMobilePlatform");candidates(plan.navigation_mobile,"IsMobilePlatform");
    std::vector<uintptr_t> calls;
    for(const auto& i:instructions(module,plan.cloud.exec,128))if(i.call())calls.push_back(i.relative(image));
    require(calls.size()==1,"cloud exec must call exactly one native predicate");
    plan.mobile_predicate=calls.front();
    auto entry=dx::decode(image,plan.mobile_predicate);
    const bool atomic_stack=plan.mobile_predicate%4==0&&entry.h.len==4&&entry.stack_adjust(true)&&entry.imm()==0x28;
    bool saved_register{};
    if(entry.h.len==2&&image.read<uint16_t>(plan.mobile_predicate)==0x5340) {
        const auto stack=dx::decode(image,entry.next());
        saved_register=stack.h.len==4&&stack.stack_adjust(true)&&stack.imm()==0x20&&
            module.function_start(plan.mobile_predicate)==plan.mobile_predicate;
    }
    if(!atomic_stack&&!saved_register) {
        std::ostringstream detail;detail<<"native bool prologue is unsupported; RVA=0x"<<std::hex<<plan.mobile_predicate<<"; entry=";
        for(const auto byte:image.view(plan.mobile_predicate,32))detail<<std::setfill('0')<<std::setw(2)<<unsigned(byte);
        throw std::runtime_error("NTE: "+detail.str());
    }
    plan.native_patch=saved_register?NativePatch::SavedRegisterDetour:NativePatch::AtomicStack;
    plan.predicate_original=image.read<uint32_t>(plan.mobile_predicate);
    const auto table=primary_vtable(module,nav.second);
    plan.navigation_slot=table+0x3a0;
    plan.navigation_original=module.rva(image.read<uint64_t>(plan.navigation_slot));
    auto setter=instructions(module,plan.navigation_original,32);
    require(setter.size()==3&&setter[0].cmp_zero_byte(1)&&setter[0].disp()==0x148&&setter[1].condition(4)&&setter[2].ret(),"navigation setter override is unsupported");
    const auto base_setter=setter[1].relative(image);
    bool reads_type{},writes_type{};
    for(const auto& i:reachable_instructions(module,base_setter,240)) {
        reads_type|=i.h.opcode==0x0f&&i.h.opcode2==0xb6&&i.memory(1,0x14b);
        writes_type|=i.h.opcode==0x88&&i.memory(1,0x14b);
    }
    require(reads_type&&writes_type,"navigation type storage relationship is unsupported");
    return plan;
}
// Retry only small live metadata reads while UE finishes initialization. The
// immutable module snapshot and expensive registration/code scan are reused.
inline void bind_metadata(Plan& plan,uintptr_t base,const MemoryReader& reader=read_memory) {
    const auto bind=[&](Function& fn) {
        std::vector<std::pair<uintptr_t,uintptr_t>> matches;
        uintptr_t owner_class{};
        if(!read_value(reader,base+fn.class_cache,owner_class)||!owner_class)throw MetadataPending("owner class is not ready");
        for(const auto cache:fn.candidates) {
            uintptr_t object{},callback{},owner{};
            if(!read_value(reader,base+cache,object))throw MetadataPending("function cache is unreadable");
            if(object&&read_value(reader,object+0xd8,callback)&&callback==base+fn.exec&&
               read_value(reader,object+0x20,owner)&&owner==owner_class)matches.emplace_back(cache,object);
        }
        if(matches.empty())throw MetadataPending("function metadata is not ready or unsupported");
        require(matches.size()==1,"ambiguous live function metadata");
        fn.cache=matches.front().first;fn.object=matches.front().second;fn.owner=owner_class;
        uint32_t flags{};uint8_t parameters{};uint16_t size{},result{};
        require(fn.object%8==0&&read_value(reader,fn.object+0xb0,flags)&&read_value(reader,fn.object+0xb4,parameters)&&
            read_value(reader,fn.object+0xb6,size)&&read_value(reader,fn.object+0xb8,result)&&
            (flags&0x400)&&parameters==1&&size==1&&result==0,"unsupported no-argument bool reflection ABI");
    };
    bind(plan.cloud);bind(plan.util_mobile);bind(plan.navigation_mobile);
}
inline Plan discover(const ModuleImage& module,const MemoryReader& reader=read_memory) {
    auto plan=discover_static(module);bind_metadata(plan,module.base,reader);return plan;
}
}
