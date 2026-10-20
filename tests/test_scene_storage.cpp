#include <cassert>
#include <cstdio>
#include <cstdint>
int model=14,values[9]={500,500,500,333,650,260,250,150,400};
bool resetPickup=false,drone=false,playing=true,benchmark=false,sceneLoading=false,resetScene=false;
struct System{static void Delay(int){}};
struct Reverb{void Init(int){}void SetFeedback(float){}void SetLpFreq(int){}}reverb;
void __DMB(){}
#include "../pod/scenes.h"
int main(){
 sceneInit();assert(sceneMounted&&!sceneExists&&sceneStatus==6);
 sceneSave(0,"464952535420534F554E44");assert(sceneStatus==1&&sceneExists&&!sceneDirty&&disk.size()==1);
 auto first=disk["/plaits/scenes/scene01.scn"];
 model=3;values[0]=777;drone=true;sceneUpdateDirty();assert(sceneDirty);
 sceneLoad(0);assert(sceneStatus==2&&!playing&&model==14&&values[0]==500&&!drone&&resetScene);
 model=7;sceneSave(0,"5345434F4E44");assert(sceneStatus==1&&disk["/plaits/scenes/scene01.bak"]==first);
 auto second=disk["/plaits/scenes/scene01.scn"];
 failRename=true;model=9;sceneSave(0,"5448495244");assert(sceneStatus==5&&disk["/plaits/scenes/scene01.scn"]==second);failRename=false;
 failWrite=true;sceneSave(0,"5448495244");assert(sceneStatus==5&&disk["/plaits/scenes/scene01.scn"]==second);failWrite=false;
 sceneSelect(0);assert(sceneExists&&sceneCached.model==7);
 disk["/plaits/scenes/scene01.bak"]=first;disk["/plaits/scenes/scene01.scn"][5]^=1;sceneSelect(0);assert(sceneStatus==7&&sceneCached.model==14);
 disk.clear();disk["/plaits/scenes/scene01.tmp"]=second;sceneSelect(0);assert(sceneStatus==7&&sceneCached.model==7);
 sceneLoad(0);assert(!playing&&model==7);sceneUpdateDirty();assert(!sceneDirty);
 sceneSelect(31);assert(!sceneExists&&sceneStatus==6);model=23;sceneLoad(31);assert(model==23);
 sceneMounted=false;sceneSave(1,"4E4F205344");assert(sceneStatus==3);
 auto before=disk;
 for(unsigned n=0;n<moodCount;n++){
  playing=true;benchmark=true;moodLoad(n);assert(!playing&&!benchmark&&model==moods[n].model&&drone==moods[n].drone);
  for(unsigned i=0;i<9;i++)assert(values[i]==moods[n].values[i]&&values[i]<=1000);
  assert(SceneNameValid(moods[n].name));assert(disk==before);
 }
 int last=model;moodLoad(-1);moodLoad(moodCount);assert(model==last);
 puts("PASS: all moods apply valid settings paused, without SD writes; invalid mood rejected");
 puts("PASS: actual storage save/load, overwrite backup, failed write, failed rename rollback, corrupt-file and temp recovery, empty slot and no-card handling");
}
