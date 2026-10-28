#pragma once
#include <stdint.h>
struct ControlFeedback {
 unsigned revision=0;int kind=0,value=0,physical=0,picked=1;uint32_t changed=0;
 void Set(int k,int v,int position,int pickup,uint32_t now){revision++;kind=k;value=v;physical=position;picked=pickup;changed=now;}
 void Tick(uint32_t now,bool release1,bool release2,bool releaseEncoder){
  if((kind==4&&release1)||(kind==5&&release2)||(kind==6&&releaseEncoder)||(kind>0&&kind<4&&uint32_t(now-changed)>650))Set(0,0,0,1,now);
 }
};
