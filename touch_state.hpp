#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace touch {
struct Vec2 { float x{},y{}; };
enum Phase : int { Began=0, Moved=1, Stationary=2, Ended=3, Canceled=4 };
// UnityEngine.Touch in the supplied Windows build. Offsets exclude a boxed header.
struct UnityTouch {
    int finger_id{};
    Vec2 position{},raw_position{},delta_position{};
    float delta_time{};
    int tap_count{1};
    Phase phase{Began};
    int type{};
    float pressure{1},maximum_pressure{1},radius{1},radius_variance{},altitude{},azimuth{};
};
static_assert(sizeof(UnityTouch)==68);
static_assert(offsetof(UnityTouch,phase)==36);
struct Snapshot { std::array<UnityTouch,10> touches{}; int count{}; };
class State {
    struct Contact {
        uint64_t key;
        int finger;
        Vec2 position,last;
        bool began{},terminal{},terminal_emitted{};
        Phase finish{Ended};
    };
    std::vector<Contact> contacts;
    Snapshot snapshot;
    int next_id=1;
    int64_t frame=-1;
public:
    uint64_t accepted_downs{},ignored_downs{};
    void down(uint64_t key,Vec2 p) {
        for(auto& c:contacts)if(c.key==key&&!c.terminal)return;
        if(contacts.size()>=10){++ignored_downs;return;}
        contacts.push_back({key,next_id++,p,p});++accepted_downs;
    }
    void move(uint64_t key,Vec2 p) {
        for(auto& c:contacts)if(c.key==key&&!c.terminal)c.position=p;
    }
    void up(uint64_t key,Vec2 p,bool cancel=false) {
        for(auto& c:contacts)if(c.key==key&&!c.terminal){c.position=p;c.terminal=true;c.finish=cancel?Canceled:Ended;}
    }
    void cancel_all() {for(auto& c:contacts)if(!c.terminal){c.terminal=true;c.finish=Canceled;}}
    bool has_contacts() const {return !contacts.empty();}
    void clear() {contacts.clear();snapshot={};frame=-1;}
    const Snapshot& begin_frame(int64_t id,int width,int height,float dt) {
        if(frame==id)return snapshot;
        frame=id;snapshot={};
        std::erase_if(contacts,[](const Contact& c){return c.terminal_emitted;});
        for(auto& c:contacts){
            auto& t=snapshot.touches[snapshot.count++];
            t.finger_id=c.finger;t.position={c.position.x*width,(1-c.position.y)*height};
            t.raw_position=t.position;t.delta_time=std::max(dt,0.000001f);
            if(!c.began){t.phase=Began;c.began=true;}
            else {
                t.delta_position={(c.position.x-c.last.x)*width,(c.last.y-c.position.y)*height};
                t.phase=c.terminal?c.finish:((t.delta_position.x!=0||t.delta_position.y!=0)?Moved:Stationary);
                if(c.terminal)c.terminal_emitted=true;
            }
            c.last=c.position;
        }
        return snapshot;
    }
};
}
