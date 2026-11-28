#pragma once
#include "win_util.hpp"
#include <algorithm>
#include <atomic>
#include <fstream>
#include <mutex>

namespace touchui::nte::input_trace {
struct Record {
    ULONGLONG time{};UINT message{};DWORD id{},flags{};LONG x{},y{};uintptr_t extra{};HWND window{};char channel{},kind{};
};
struct State {
    std::atomic<HWND> window{};std::atomic<DWORD> thread{};HHOOK queued{},dispatch{};HANDLE writer{};HMODULE module{};
    std::atomic<bool> stop{},bound{};std::atomic<ULONGLONG> seen{},last_touch{};
    std::mutex mutex;std::array<Record,512> records{};size_t count{},dropped{};
    std::filesystem::path path;
};
inline std::atomic<State*> active{};
inline void record(State& state,const Record& event) {
    ++state.seen;std::lock_guard lock(state.mutex);
    if(state.count<state.records.size())state.records[state.count++]=event;else ++state.dropped;
}
// Read only. In particular, the game retains ownership of HTOUCHINPUT; this
// observer never closes it, modifies a message or generates replacement input.
inline void inspect(State& state,char channel,HWND window,UINT message,WPARAM wparam,LPARAM lparam) {
    if(state.stop.load())return;
    if(window!=state.window) {
        DWORD pid{};const auto thread=GetWindowThreadProcessId(window,&pid);
        if(pid!=GetCurrentProcessId()||thread!=state.thread)return;
    }
    const auto now=GetTickCount64();
    Record event{now,message,0,0,0,0,static_cast<uintptr_t>(GetMessageExtraInfo()),window,channel,0};
    if(message==WM_TOUCH) {
        state.last_touch=now;const UINT count=LOWORD(wparam);std::array<TOUCHINPUT,256> contacts{};
        if(count&&count<=contacts.size()&&GetTouchInputInfo(reinterpret_cast<HTOUCHINPUT>(lparam),count,contacts.data(),sizeof(TOUCHINPUT))) {
            for(UINT i=0;i<count;++i) {
                event.kind='T';event.id=contacts[i].dwID;event.flags=contacts[i].dwFlags;
                event.x=contacts[i].x;event.y=contacts[i].y;record(state,event);
            }
        } else {event.kind='E';event.flags=GetLastError();record(state,event);}
    } else if(message==WM_POINTERDOWN||message==WM_POINTERUPDATE||message==WM_POINTERUP) {
        state.last_touch=now;POINTER_INFO info{};
        event.id=GET_POINTERID_WPARAM(wparam);
        if(GetPointerInfo(event.id,&info)) {
            event.kind=info.pointerType==PT_TOUCH?'P':'O';event.flags=info.pointerFlags;
            event.x=info.ptPixelLocation.x;event.y=info.ptPixelLocation.y;
        } else {event.kind='E';event.flags=GetLastError();}
        record(state,event);
    } else if(message==WM_CAPTURECHANGED||message==WM_POINTERCAPTURECHANGED||message==WM_CANCELMODE||
        message==WM_KILLFOCUS||message==WM_ACTIVATE||message==WM_MOUSELEAVE) {
        event.kind='C';event.flags=static_cast<DWORD>(wparam);record(state,event);
    } else if(message==WM_LBUTTONDOWN||message==WM_LBUTTONUP||message==WM_LBUTTONDBLCLK||
        message==WM_MOUSEWHEEL||message==WM_MOUSEHWHEEL||
        (message==WM_MOUSEMOVE&&((wparam&(MK_LBUTTON|MK_RBUTTON))||now-state.last_touch.load()<1500))) {
        event.kind='M';event.flags=static_cast<DWORD>(wparam);
        event.x=static_cast<SHORT>(LOWORD(lparam));event.y=static_cast<SHORT>(HIWORD(lparam));record(state,event);
    }
}
inline LRESULT CALLBACK queued_hook(int code,WPARAM wparam,LPARAM lparam) {
    try {
        if(code>=0&&wparam==PM_REMOVE)if(auto* state=active.load(std::memory_order_acquire)) {
            const auto* message=reinterpret_cast<const MSG*>(lparam);
            inspect(*state,'Q',message->hwnd,message->message,message->wParam,message->lParam);
        }
    }catch(...){}
    return CallNextHookEx(nullptr,code,wparam,lparam);
}
inline LRESULT CALLBACK dispatch_hook(int code,WPARAM wparam,LPARAM lparam) {
    try {
        if(code>=0)if(auto* state=active.load(std::memory_order_acquire)) {
            const auto* message=reinterpret_cast<const CWPSTRUCT*>(lparam);
            inspect(*state,'D',message->hwnd,message->message,message->wParam,message->lParam);
        }
    }catch(...){}
    return CallNextHookEx(nullptr,code,wparam,lparam);
}
inline void remove_hooks(State& state) {
    state.bound=false;
    if(state.queued){UnhookWindowsHookEx(state.queued);state.queued=nullptr;}
    if(state.dispatch){UnhookWindowsHookEx(state.dispatch);state.dispatch=nullptr;}
}
inline bool bind(State& state,HWND window) {
    DWORD pid{};const auto thread=GetWindowThreadProcessId(window,&pid);
    if(!window||!thread||pid!=GetCurrentProcessId())return false;
    remove_hooks(state);state.window=window;state.thread=thread;
    state.queued=SetWindowsHookExW(WH_GETMESSAGE,queued_hook,state.module,thread);
    state.dispatch=SetWindowsHookExW(WH_CALLWNDPROC,dispatch_hook,state.module,thread);
    if(!state.queued||!state.dispatch){remove_hooks(state);return false;}
    state.bound=true;
    return true;
}
inline HWND replacement_window() {
    HWND found{};
    EnumWindows([](HWND window,LPARAM context)->BOOL {
        DWORD pid{};GetWindowThreadProcessId(window,&pid);wchar_t type[64]{};
        if(pid==GetCurrentProcessId()&&IsWindowVisible(window)&&GetClassNameW(window,type,64)&&
            std::wstring_view(type)==L"UnrealWindow") {
            *reinterpret_cast<HWND*>(context)=window;return FALSE;
        }
        return TRUE;
    },reinterpret_cast<LPARAM>(&found));return found;
}
inline DWORD WINAPI write_trace(void* parameter) {
    auto& state=*static_cast<State*>(parameter);
    try {
        std::ofstream file(state.path,std::ios::trunc);
        file<<"# Passive own-window input trace. Q=removed from queue; D=call-window hook. Channels can observe the same message.\n"
            <<"# T coordinates are Windows hundredths of screen pixels; P coordinates screen pixels; M coordinates message client pixels (wheel uses screen pixels).\n"
            <<"# S=Slate pager callback; B/A=before/after; message=vtable byte slot; id=pointer; flags bytes=enabled,active,state,isTouch (state bits=pressed,dragging,handled); x/y=float delta bits; extra=widget; window=thread ID.\n"
            <<"# L=list move with the S column contract; G=list metric with x=geometry scale float bits, y=scroll-offset float bits.\n"
            <<"# initial_window=0x"<<std::hex<<reinterpret_cast<uintptr_t>(state.window.load())
            <<"; thread="<<std::dec<<state.thread.load()<<"\n"
            <<"ms,channel,kind,message,id,flags,x,y,extra,window\n"<<std::flush;
        const auto deadline=GetTickCount64()+15*60*1000;size_t written{};
        for(;;) {
            // The initial Unreal window can be a temporary startup surface.
            // Keep the bounded writer alive and rebind only to our own game.
            if(!IsWindow(state.window.load())&&!state.stop.load()) {
                if(const auto next=replacement_window()) {
                    const bool bound=bind(state,next);
                    file<<"# rebind_window=0x"<<std::hex<<reinterpret_cast<uintptr_t>(next)
                        <<"; thread="<<std::dec<<state.thread.load()<<"; ok="<<bound<<'\n';
                    if(!bound)state.stop=true;
                }
            }
            const char* reason=!file.good()?"file_error":GetTickCount64()>=deadline?"time_limit":written>=4096?"record_limit":nullptr;
            if(reason)state.stop=true;
            const bool stopping=state.stop.load();
            std::array<Record,512> events{};size_t count{},dropped{};
            {
                std::lock_guard lock(state.mutex);count=state.count;dropped=state.dropped;
                std::copy_n(state.records.begin(),count,events.begin());state.count=0;state.dropped=0;
            }
            for(size_t i=0;i<count&&written<4096;++i,++written) {
                const auto& e=events[i];
                file<<std::dec<<e.time<<','<<e.channel<<','<<e.kind<<",0x"<<std::hex<<e.message<<std::dec<<','<<e.id
                    <<",0x"<<std::hex<<e.flags<<std::dec<<','<<e.x<<','<<e.y<<",0x"<<std::hex<<e.extra
                    <<",0x"<<reinterpret_cast<uintptr_t>(e.window)<<'\n';
            }
            if(dropped)file<<"# dropped="<<std::dec<<dropped<<'\n';
            if(stopping)file<<"# stopped="<<(reason?reason:"requested")<<"; records="<<std::dec<<written<<'\n';
            file.flush();if(stopping)break;Sleep(250);
        }
    }catch(...) {state.stop=true;}
    remove_hooks(state);return 0; // Context remains alive for any in-flight callback.
}
inline State* start(HWND window,const std::filesystem::path& path) {
    DWORD pid{};const auto thread=GetWindowThreadProcessId(window,&pid);
    if(!window||!thread||pid!=GetCurrentProcessId())throw std::runtime_error("NTE trace requires an owned process window");
    HMODULE module{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&queued_hook),&module))throw std::runtime_error("cannot pin NTE trace callback");
    auto* state=new State;state->window=window;state->thread=thread;state->path=path;state->module=module;State* empty{};
    if(!active.compare_exchange_strong(empty,state,std::memory_order_release))throw std::runtime_error("NTE trace already active");
    if(!bind(*state,window)) {
        state->stop=true;remove_hooks(*state);throw std::runtime_error("cannot install passive own-window input trace");
    }
    state->writer=CreateThread(nullptr,0,write_trace,state,0,nullptr);
    if(!state->writer) {state->stop=true;remove_hooks(*state);throw std::runtime_error("cannot start NTE trace writer");}
    return state;
}
inline bool finish(State& state) {
    state.stop=true;return state.writer&&WaitForSingleObject(state.writer,5000)==WAIT_OBJECT_0;
}
}
