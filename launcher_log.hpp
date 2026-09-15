#pragma once
#include "win_util.hpp"
#include <fstream>
#include <iostream>
#include <streambuf>
#include <algorithm>
#include <string_view>

namespace launcher_log {
inline std::string utf8(std::wstring_view value) {
    const auto size=WideCharToMultiByte(CP_UTF8,0,value.data(),static_cast<int>(value.size()),nullptr,0,nullptr,nullptr);
    std::string result(size,0);
    if(size)WideCharToMultiByte(CP_UTF8,0,value.data(),static_cast<int>(value.size()),result.data(),size,nullptr,nullptr);
    return result;
}
inline std::wstring wide(std::string_view value) {
    const auto size=MultiByteToWideChar(CP_UTF8,0,value.data(),static_cast<int>(value.size()),nullptr,0);
    std::wstring result(size,0);
    if(size)MultiByteToWideChar(CP_UTF8,0,value.data(),static_cast<int>(value.size()),result.data(),size);
    return result;
}
class Tee final:public std::streambuf {
    std::streambuf* console_{};
    std::streambuf* file_{};
protected:
    std::streamsize xsputn(const char* text,std::streamsize count)override {
        const auto console_count=console_->sputn(text,count);
        const auto file_count=file_->sputn(text,count);file_->pubsync();
        return std::max(console_count,file_count);
    }
    int_type overflow(int_type value)override {
        if(traits_type::eq_int_type(value,traits_type::eof()))return traits_type::not_eof(value);
        const char c=traits_type::to_char_type(value);return xsputn(&c,1)==1?value:traits_type::eof();
    }
    int sync()override {console_->pubsync();return file_->pubsync();}
public:
    void bind(std::streambuf* console,std::streambuf* file){console_=console;file_=file;}
};
inline bool should_show_error(bool logging,bool relaunched,bool interactive,DWORD console_processes) {
    return logging&&relaunched&&interactive&&console_processes==1;
}
class Session {
    bool enabled_{};
    std::ofstream file_;
    std::filesystem::path path_;
    Tee out_,err_;
    std::streambuf* previous_out_{};
    std::streambuf* previous_err_{};
    std::string stage_="argument validation";
public:
    explicit Session(bool enabled):enabled_(enabled){}
    Session(const Session&)=delete;Session& operator=(const Session&)=delete;
    ~Session(){if(previous_out_)std::cout.rdbuf(previous_out_);if(previous_err_)std::cerr.rdbuf(previous_err_);}
    void open(const std::filesystem::path& own_directory) {
        if(!enabled_)return;
        std::error_code error;
        std::filesystem::create_directories(own_directory/L"logs",error);
        if(!error) {
            path_=own_directory/L"logs"/(L"launcher-"+std::to_wstring(GetCurrentProcessId())+L".log");
            file_.open(path_,std::ios::binary|std::ios::app);
        }
        if(!file_.is_open()) {path_.clear();std::cerr<<"WARNING: cannot create launcher log beside the executable.\n";return;}
        previous_out_=std::cout.rdbuf();previous_err_=std::cerr.rdbuf();
        out_.bind(previous_out_,file_.rdbuf());err_.bind(previous_err_,file_.rdbuf());
        std::cout.rdbuf(&out_);std::cerr.rdbuf(&err_);
        SYSTEMTIME time{};GetLocalTime(&time);
        std::cout<<"\nLauncher session "<<time.wYear<<"-"<<time.wMonth<<"-"<<time.wDay<<" "<<time.wHour<<":"<<time.wMinute<<":"<<time.wSecond
            <<" PID="<<GetCurrentProcessId()<<"\nLauncher log: "<<utf8(path_.wstring())<<std::endl;
    }
    void stage(std::string_view text) {stage_=text;if(enabled_)std::cout<<"Stage: "<<stage_<<std::endl;}
    const std::filesystem::path& path()const{return path_;}
    void failure(std::string_view reason,bool relaunched=false) {
        if(!enabled_)return;
        const auto message="ERROR at "+stage_+": "+std::string(reason);
        // Keep the ERROR: prefix for scripts and existing diagnostic consumers.
        std::cerr<<"ERROR: stage="<<stage_<<"; "<<reason<<std::endl;
        DWORD processes[2]{},mode{};
        const bool interactive=GetConsoleWindow()!=nullptr&&GetConsoleMode(GetStdHandle(STD_ERROR_HANDLE),&mode)!=FALSE;
        const auto count=interactive?GetConsoleProcessList(processes,2):0;
        if(should_show_error(enabled_,relaunched,interactive,count)) {
            auto text=wide(message);
            if(!path_.empty())text+=L"\n\nLauncher log:\n"+path_.wstring();
            MessageBoxW(nullptr,text.c_str(),L"ZZZTouchLauncher - launch failed",MB_OK|MB_ICONERROR);
        }
    }
};
}
