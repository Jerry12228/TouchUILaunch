#include "touch_state.hpp"
#include <iostream>
#include <stdexcept>
void check(bool v,const char* why){if(!v)throw std::runtime_error(why);}
int main(){
    touch::State s;
    s.down(100,{.25f,.75f});s.down(200,{.8f,.2f});
    auto a=s.begin_frame(1,1000,800,.016f);
    check(a.count==2&&a.touches[0].phase==touch::Began,"two concurrent begins");
    check(a.touches[0].position.x==250&&a.touches[0].position.y==200,"coordinate scaling and Y flip");
    auto id=a.touches[0].finger_id;
    s.move(100,{.5f,.5f});
    check(s.begin_frame(1,1000,800,.016f).touches[0].position.x==250,"frame snapshot must remain stable");
    a=s.begin_frame(2,1000,800,.016f);
    check(a.touches[0].finger_id==id&&a.touches[0].phase==touch::Moved,"stable finger identity");
    check(a.touches[0].delta_position.x==250&&a.touches[0].delta_position.y==200,"movement delta");
    check(a.touches[1].phase==touch::Stationary,"second finger held");
    s.up(100,{.5f,.5f});a=s.begin_frame(3,1000,800,.016f);
    check(a.touches[0].phase==touch::Ended,"release is visible for a frame");
    check(s.begin_frame(4,1000,800,.016f).count==1,"released pointer retired");
    s.cancel_all();check(s.begin_frame(5,1000,800,.016f).touches[0].phase==touch::Canceled,"capture/focus cancellation");
    check(s.begin_frame(6,1000,800,.016f).count==0,"cancellation retired");
    s.down(9,{0,0});s.up(9,{0,0});s.down(9,{1,1});
    a=s.begin_frame(7,1000,800,.016f);
    check(a.count==2&&a.touches[0].phase==touch::Began&&a.touches[0].finger_id!=a.touches[1].finger_id,"quick tap and recycled Windows ID");
    check(s.begin_frame(8,1000,800,.016f).touches[0].phase==touch::Ended,"quick tap release not lost");
    s.clear();for(int i=0;i<20;++i)s.down(i,{.5f,.5f});
    check(s.begin_frame(9,1000,800,.016f).count==10&&s.ignored_downs==10,"ten-touch bound");
    std::cout<<"Touch lifecycle, multi-touch, frame stability, coordinate conversion, ID reuse and capacity: PASS\n";
}
