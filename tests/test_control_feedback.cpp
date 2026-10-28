#include "../pod/control_feedback.h"
#include <cassert>
int main(){
 ControlFeedback f;
 // A held button persists beyond the movement timeout, then clears on release.
 f.Set(4,1,0,1,100);f.Tick(100,false,false,false);assert(f.kind==4);
 f.Tick(5000,false,false,false);assert(f.kind==4);
 f.Tick(5001,true,false,false);assert(f.kind==0);
 f.Set(5,1,0,1,6000);f.Tick(6001,true,false,false);assert(f.kind==5);
 f.Tick(6002,false,true,false);assert(f.kind==0);
 f.Set(6,14,0,1,7000);f.Tick(7001,false,false,true);assert(f.kind==0);
 // Movement refreshes expiry and preserves pickup/target values.
 f.Set(1,600,300,0,8000);f.Tick(8500,false,false,false);assert(f.kind==1&&f.value==600&&!f.picked);
 f.Set(1,600,400,0,8500);f.Tick(9000,false,false,false);assert(f.kind==1);
 f.Tick(9151,false,false,false);assert(f.kind==0);
 // Timer wrap must not leave a movement overlay stuck on screen.
 f.Set(3,23,0,1,UINT32_MAX-100);f.Tick(600,false,false,false);assert(f.kind==0);
}
