#pragma once
#include "fatfs.h"
#include "scene_codec.h"
// Runs in the foreground. FIL contains a DMA sector buffer and must not live on the DTCM stack.
SdmmcHandler sceneSd;
FatFSInterface sceneFs;
alignas(32) FIL sceneFile;
alignas(32) uint8_t sceneBytes[64];
SceneData sceneCached;
int sceneSlot=0,sceneStatus=0;
bool sceneMounted=false,sceneExists=false,sceneDirty=true;
uint32_t sceneRevision=1;
int sceneReadPath(const char* path,SceneData& data){
 FRESULT open=f_open(&sceneFile,path,FA_READ);if(open==FR_NO_FILE||open==FR_NO_PATH)return 6;if(open!=FR_OK)return 5;
 UINT count=0;bool size=f_size(&sceneFile)==64;FRESULT read=size?f_read(&sceneFile,sceneBytes,64,&count):FR_OK;FRESULT close=f_close(&sceneFile);
 if(read!=FR_OK||close!=FR_OK)return 5;
 return size&&count==64&&DecodeScene(sceneBytes,count,data)?0:4;
}
int sceneRead(int slot,SceneData& data){
 char path[64];snprintf(path,sizeof(path),"/plaits/scenes/scene%02d.scn",slot+1);int code=sceneReadPath(path,data);
 if(code==0)return 0;
 snprintf(path,sizeof(path),"/plaits/scenes/scene%02d.bak",slot+1);int backup=sceneReadPath(path,data);
 if(backup==0)return 7;
 snprintf(path,sizeof(path),"/plaits/scenes/scene%02d.tmp",slot+1);if(sceneReadPath(path,data)==0)return 7;return code;
}
void sceneSelect(int slot){
 if(slot<0||slot>=32)return;sceneSlot=slot;sceneExists=false;
 if(sceneMounted){sceneStatus=sceneRead(slot,sceneCached);sceneExists=sceneStatus==0||sceneStatus==7;}
 else sceneStatus=3;
 if(!sceneExists){sceneCached=SceneData{};snprintf(sceneCached.name,sizeof(sceneCached.name),"Scene %02d",slot+1);}
 sceneDirty=true;sceneRevision++;
}
void sceneInit(){
 SdmmcHandler::Config config;config.Defaults();sceneSd.Init(config);sceneFs.Init(FatFSInterface::Config::MEDIA_SD);
 sceneMounted=f_mount(&sceneFs.GetSDFileSystem(),"/",1)==FR_OK;sceneSelect(0);
}
bool sceneMatches(){if(!sceneExists||sceneCached.model!=model||sceneCached.drone!=bool(drone))return false;for(unsigned i=0;i<9;i++)if(sceneCached.values[i]!=values[i])return false;return true;}
void sceneUpdateDirty(){bool dirty=!sceneMatches();if(dirty!=sceneDirty){sceneDirty=dirty;sceneRevision++;}}
void sceneSave(int slot,const char* hex){
 if(slot<0||slot>=32)return;
 SceneData data;data.model=model;data.drone=drone;for(unsigned i=0;i<9;i++)data.values[i]=values[i];
 if(!SceneNameFromHex(hex,data.name)){sceneStatus=4;sceneRevision++;return;}
 if(slot!=sceneSlot)sceneSelect(slot);
 if(!sceneMounted){sceneStatus=3;sceneRevision++;return;}
 FRESULT folder=f_mkdir("/plaits");if(folder!=FR_OK&&folder!=FR_EXIST){sceneStatus=5;sceneRevision++;return;}
 folder=f_mkdir("/plaits/scenes");if(folder!=FR_OK&&folder!=FR_EXIST){sceneStatus=5;sceneRevision++;return;}
 char final[64],temp[64],backup[64];snprintf(final,sizeof(final),"/plaits/scenes/scene%02d.scn",slot+1);snprintf(temp,sizeof(temp),"/plaits/scenes/scene%02d.tmp",slot+1);snprintf(backup,sizeof(backup),"/plaits/scenes/scene%02d.bak",slot+1);
 EncodeScene(data,sceneBytes);UINT written=0;
 bool ok=f_open(&sceneFile,temp,FA_WRITE|FA_CREATE_ALWAYS)==FR_OK;
 if(ok){ok=f_write(&sceneFile,sceneBytes,64,&written)==FR_OK&&written==64;if(ok)ok=f_sync(&sceneFile)==FR_OK;FRESULT close=f_close(&sceneFile);ok=ok&&close==FR_OK;}
 if(ok){
  FILINFO info;bool old=f_stat(final,&info)==FR_OK;
  if(old){FRESULT erase=f_unlink(backup);if(erase!=FR_OK&&erase!=FR_NO_FILE)ok=false;if(ok)ok=f_rename(final,backup)==FR_OK;}
  if(ok){ok=f_rename(temp,final)==FR_OK;if(!ok&&old)f_rename(backup,final);}
  // Keep the previous successful scene as a recovery copy until the next save.
 }
 if(ok){sceneCached=data;sceneExists=true;sceneDirty=false;sceneStatus=1;}else sceneStatus=5;
 sceneRevision++;
}
void sceneLoad(int slot){
 if(slot<0||slot>=32)return;sceneSelect(slot);if(!sceneExists)return;
 playing=false;benchmark=false;System::Delay(350);sceneLoading=true;System::Delay(3);
 model=sceneCached.model;drone=sceneCached.drone;for(unsigned i=0;i<9;i++)values[i]=sceneCached.values[i];
 reverb.Init(48000);reverb.SetFeedback(.85f);reverb.SetLpFreq(9000);
 resetScene=true;__DMB();sceneLoading=false;sceneDirty=false;sceneStatus=2;sceneRevision++;
}
