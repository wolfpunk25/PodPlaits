#pragma once
#include <cstdint>
#include <cstring>
struct SceneData { uint8_t model=14; bool drone=false; uint16_t values[9]={500,500,500,333,650,260,250,150,400}; char name[21]="Scene 01"; };
inline uint32_t SceneCrc(const uint8_t* bytes,unsigned n){uint32_t c=0xffffffff;for(unsigned i=0;i<n;i++){c^=bytes[i];for(unsigned j=0;j<8;j++)c=(c>>1)^(0xedb88320u&uint32_t(-int32_t(c&1)));}return ~c;}
inline bool SceneNameValid(const char* s){bool visible=false;for(unsigned i=0;i<=20;i++){unsigned char c=s[i];if(!c)return visible;if(i==20||c<32||c>126)return false;if(c!=' ')visible=true;}return false;}
inline bool SceneValid(const SceneData& s){if(s.model>=24||!SceneNameValid(s.name))return false;for(auto v:s.values)if(v>1000)return false;return true;}
inline bool EncodeScene(const SceneData& s,uint8_t* b){
 if(!SceneValid(s))return false;
 memset(b,0,64);memcpy(b,"PLSCN01",7);b[8]=1;b[9]=s.model;b[10]=s.drone?1:0;
 for(unsigned i=0;i<9;i++){b[12+i*2]=s.values[i]&255;b[13+i*2]=s.values[i]>>8;}
 memcpy(b+30,s.name,strlen(s.name)+1);uint32_t crc=SceneCrc(b,60);for(unsigned i=0;i<4;i++)b[60+i]=crc>>(8*i);return true;
}
inline bool DecodeScene(const uint8_t* b,unsigned n,SceneData& s){
 if(n!=64||memcmp(b,"PLSCN01",8)||b[8]!=1||b[10]>1)return false;
 uint32_t crc=0;for(unsigned i=0;i<4;i++)crc|=uint32_t(b[60+i])<<(8*i);if(crc!=SceneCrc(b,60))return false;
 SceneData out;out.model=b[9];out.drone=b[10];for(unsigned i=0;i<9;i++)out.values[i]=uint16_t(b[12+i*2])|(uint16_t(b[13+i*2])<<8);
 memcpy(out.name,b+30,21);if(!SceneValid(out))return false;s=out;return true;
}
inline int SceneHexDigit(char c){return c>='0'&&c<='9'?c-'0':c>='A'&&c<='F'?c-'A'+10:c>='a'&&c<='f'?c-'a'+10:-1;}
inline bool SceneNameFromHex(const char* hex,char* name){unsigned n=strlen(hex);if(!n||n>40||n%2)return false;for(unsigned i=0;i<n/2;i++){int a=SceneHexDigit(hex[i*2]),b=SceneHexDigit(hex[i*2+1]);if(a<0||b<0)return false;unsigned c=a*16+b;if(c<32||c>126)return false;name[i]=c;}name[n/2]=0;return SceneNameValid(name);}
inline void SceneNameToHex(const char* name,char* hex){const char* digits="0123456789ABCDEF";unsigned i=0;for(;name[i]&&i<20;i++){unsigned char c=name[i];hex[2*i]=digits[c>>4];hex[2*i+1]=digits[c&15];}hex[2*i]=0;}
