#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdint>
using UINT=unsigned;
enum FRESULT{FR_OK,FR_NO_FILE,FR_NO_PATH,FR_EXIST,FR_DISK_ERR};
const int FA_READ=1,FA_WRITE=2,FA_CREATE_ALWAYS=4;
struct FIL{std::string path;};struct FILINFO{};
std::map<std::string,std::vector<uint8_t>> disk;
bool failWrite=false,failRename=false;
FRESULT f_open(FIL*f,const char*p,int mode){if(mode&FA_CREATE_ALWAYS)disk[p]={};else if(!disk.count(p))return FR_NO_FILE;f->path=p;return FR_OK;}
size_t f_size(FIL*f){return disk[f->path].size();}
FRESULT f_read(FIL*f,void*b,UINT n,UINT*c){*c=std::min<size_t>(n,disk[f->path].size());memcpy(b,disk[f->path].data(),*c);return FR_OK;}
FRESULT f_write(FIL*f,const void*b,UINT n,UINT*c){if(failWrite){*c=0;return FR_DISK_ERR;}disk[f->path]=std::vector<uint8_t>((const uint8_t*)b,(const uint8_t*)b+n);*c=n;return FR_OK;}
FRESULT f_close(FIL*){return FR_OK;}FRESULT f_sync(FIL*){return FR_OK;}
FRESULT f_mkdir(const char*){return FR_OK;}
FRESULT f_stat(const char*p,FILINFO*){return disk.count(p)?FR_OK:FR_NO_FILE;}
FRESULT f_unlink(const char*p){return disk.erase(p)?FR_OK:FR_NO_FILE;}
FRESULT f_rename(const char*a,const char*b){if(failRename&&std::string(a).find(".tmp")!=std::string::npos)return FR_DISK_ERR;if(!disk.count(a))return FR_NO_FILE;if(disk.count(b))return FR_EXIST;disk[b]=disk[a];disk.erase(a);return FR_OK;}
struct SdmmcHandler{struct Config{void Defaults(){}};void Init(Config){}};
struct FatFSInterface{struct Config{enum {MEDIA_SD};};void Init(int){} int fs;int& GetSDFileSystem(){return fs;}};
FRESULT f_mount(int*,const char*,int){return FR_OK;}
