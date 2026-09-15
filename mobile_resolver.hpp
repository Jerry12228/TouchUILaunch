#pragma once
// UI signatures and call sequence adapted from Genshin_StarRail_fps_unlocker
// (c) 2024 NullName, MIT; see third_party/gi_sr/LICENSE.txt.
#include "x64_reader.hpp"
#include <string_view>

namespace mobile {
inline void require(bool ok,const std::string& message) {
    if(!ok)throw std::runtime_error("Mobile UI: "+message);
}
struct Pattern {
    const char* name;
    std::vector<uint8_t> bytes,mask;
    Pattern(const char* label,std::string_view text):name(label) {
        const auto nibble=[](char c)->int {if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;return -1;};
        while(!text.empty()) {
            if(text.front()==' '){text.remove_prefix(1);continue;}
            require(text.size()>=2,"invalid signature");
            if(text.substr(0,2)=="??"){bytes.push_back(0);mask.push_back(0);}
            else {const auto a=nibble(text[0]),b=nibble(text[1]);require(a>=0&&b>=0,"invalid signature hex");bytes.push_back(static_cast<uint8_t>(a*16+b));mask.push_back(255);}
            text.remove_prefix(2);
        }
    }
    scan_rules::Pattern rule()const{return {name,bytes,mask};}
};
inline const Pattern sr_patterns[]={
    {"SR long branch","80 B9 ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ?? C7 05 ?? ?? ?? ?? 03 00 00 00 48 83 C4 20 5E C3"},
    {"SR short branch","80 B9 ?? ?? ?? ?? 00 74 ?? C7 05 ?? ?? ?? ?? 03 00 00 00 48 83 C4 20 5E C3"},
    {"SR call branch","75 05 E8 ?? ?? ?? ?? C7 05 ?? ?? ?? ?? 03 00 00 00 48 83 C4 28 C3"}
};
inline const Pattern gi_patterns[]={
    {"GI UI setters (notify)","48 8B 05 ?? ?? ?? ?? 48 8B 88 ?? ?? ?? ?? 48 85 C9 0F ?? ?? ?? ?? ?? BA 02 00 00 00 41 B0 01 E8 ?? ?? ?? ?? 48 89 F9 BA 03 00 00 00 45 31 C0 E8 ?? ?? ?? ??"},
    {"GI UI setters (legacy)","48 8B 05 ?? ?? ?? ?? 48 8B 88 ?? ?? ?? ?? 48 85 C9 0F ?? ?? ?? ?? ?? BA 02 00 00 00 E8 ?? ?? ?? ?? 48 89 F9 BA 03 00 00 00 E8 ?? ?? ?? ??"}
};
inline const Pattern gi_input{"GI input object","48 8B 05 ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? 48 8B B8 ?? ?? ?? ?? 48 85 FF 0F 84 ?? ?? ?? ?? 83 BF ?? ?? ?? ?? 03"};
inline const Pattern gi_init{"GI initialization call","E8 ?? ?? ?? ?? 48 89 D9 E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? 48 8B 0D"};
inline std::vector<uintptr_t> matches(const discovery::Image& image,const Pattern& pattern) {
    auto candidates=image.find(pattern.rule(),64,8);
    std::erase_if(candidates,[&](uintptr_t at){const auto* section=image.section(at,1);return std::string_view(section->name,6)!="il2cpp";});
    return candidates;
}
struct Match {uintptr_t at{};size_t variant{};};
inline std::vector<Match> collect(const discovery::Image& image,std::span<const Pattern> patterns) {
    std::vector<Match> found;
    for(size_t i=0;i<patterns.size();++i)for(const auto at:matches(image,patterns[i]))found.push_back({at,i});
    return found;
}
inline Match unique(const discovery::Image& image,std::span<const Pattern> patterns) {
    const auto found=collect(image,patterns);
    require(found.size()==1,std::string(patterns.front().name)+": expected one match across variants, got "+std::to_string(found.size()));
    return found.front();
}
inline void data(const discovery::Image& image,uintptr_t at,size_t size) {
    const auto* section=image.section(at,size);
    require(section&&section->data()&&at%size==0,"target is not aligned writable image data");
}
inline uintptr_t resolve_sr(const discovery::Image& image) {
    const auto match=unique(image,sr_patterns);
    constexpr uintptr_t offsets[]={13,9,7};
    const auto ins=discovery::x64::decode(image,match.at+offsets[match.variant]);
    require(ins.h.opcode==0xc7&&ins.h.modrm_reg==0&&ins.imm()==3&&!ins.h.rex_w,"invalid SR UI write");
    const auto target=ins.storage(image);data(image,target,4);return target;
}
struct GiResolution {
    uintptr_t init{},ui{},input{},klass{};uint32_t ui_offset{},input_offset{};
    bool operator==(const GiResolution&)const=default;
};
inline GiResolution gi_setters(const discovery::Image& image,Match match) {
    GiResolution result;
    result.klass=discovery::x64::decode(image,match.at).storage(image);data(image,result.klass,8);
    result.ui_offset=image.read<uint32_t>(match.at+10);
    const auto call_ui=discovery::x64::decode(image,match.at+(match.variant?28:31));
    const auto call_input=discovery::x64::decode(image,match.at+(match.variant?41:47));
    require(call_ui.call()&&call_input.call(),"invalid GI setter calls");
    result.ui=call_ui.relative(image);result.input=call_input.relative(image);
    image.code(result.ui);image.code(result.input);
    return result;
}
inline GiResolution resolve_gi(const discovery::Image& image) {
    // GI 7.0 repeats the same UI setter pair at two call sites and the same
    // input object access at four sites. Require unique decoded targets, not
    // a unique occurrence of an instruction sequence. Never take the first
    // match when another site describes different functions/objects.
    const auto setters=collect(image,gi_patterns);
    require(!setters.empty(),"GI UI setters: no matching call sites");
    auto result=gi_setters(image,setters.front());
    for(const auto& match:setters)require(gi_setters(image,match)==result,"GI UI setters: conflicting decoded targets");
    const auto objects=matches(image,gi_input);
    require(!objects.empty(),"GI input object: no matching access sites");
    result.input_offset=image.read<uint32_t>(objects.front()+16);
    for(const auto at:objects) {
        require(discovery::x64::decode(image,at).storage(image)==result.klass,"GI input object uses a different class slot");
        require(image.read<uint32_t>(at+16)==result.input_offset,"GI input object: conflicting decoded offsets");
    }
    for(const auto offset:{result.ui_offset,result.input_offset})require(offset>0&&offset<0x100000&&offset%8==0,"invalid GI object offset");
    require(result.ui_offset!=result.input_offset,"GI UI/input objects overlap");
    const auto initializers=matches(image,gi_init);
    require(!initializers.empty(),"GI initialization: no matching call sites");
    for(const auto at:initializers) {
        const auto call=discovery::x64::decode(image,at+8);require(call.call(),"invalid GI initialization call");
        const auto target=call.relative(image);image.code(target,16);
        require(!result.init||result.init==target,"GI initialization: conflicting decoded targets");result.init=target;
    }
    require(result.init!=result.ui&&result.init!=result.input&&result.ui!=result.input,"GI functions overlap");
    // No trampoline: original bytes are restored before invoking the function.
    // Still reject an existing branch hook rather than overwrite another writer.
    const auto first=discovery::x64::decode(image,result.init);
    require(!first.jump()&&!(first.h.opcode==0xff&&first.h.modrm_reg==4),"GI initialization function already hooked");
    return result;
}
}
