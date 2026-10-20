#include "../pod/scene_codec.h"
#include <cassert>
#include <cstdio>
int main(){
 SceneData a; a.model=23;a.drone=true;strcpy(a.name,"DREAM SCENE 12345678");for(int i=0;i<9;i++)a.values[i]=i%2?1000:0;
 uint8_t bytes[64];assert(EncodeScene(a,bytes));SceneData b;assert(DecodeScene(bytes,64,b));assert(b.model==23&&b.drone&&strcmp(a.name,b.name)==0);for(int i=0;i<9;i++)assert(b.values[i]==a.values[i]);
 for(unsigned i=0;i<64;i++){bytes[i]^=1;assert(!DecodeScene(bytes,64,b));bytes[i]^=1;}
 for(unsigned n=0;n<64;n++)assert(!DecodeScene(bytes,n,b));assert(!DecodeScene(bytes,65,b));
 a.values[0]=1001;assert(!EncodeScene(a,bytes));a.values[0]=0;a.model=24;assert(!EncodeScene(a,bytes));a.model=0;strcpy(a.name,"  ");assert(!EncodeScene(a,bytes));
 char name[21],hex[41];assert(SceneNameFromHex("464952535420534F554E44",name));assert(!strcmp(name,"FIRST SOUND"));SceneNameToHex(name,hex);assert(!strcmp(hex,"464952535420534F554E44"));assert(!SceneNameFromHex("00",name));assert(!SceneNameFromHex("2020",name));assert(!SceneNameFromHex("GG",name));assert(!SceneNameFromHex("0",name));
 puts("PASS: scene round-trip, bounds, names, checksum corruption and truncated-file rejection");
}
