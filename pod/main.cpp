#include "daisy_pod.h"
#include "plaits/dsp/voice.h"
#include "plaits/user_data.h"
#include "stmlib/utils/buffer_allocator.h"
#include "Effects/reverbsc.h"
#include "fatfs.h"
#include <cmath>
#include <cstdio>
#include <cstring>
using namespace daisy;
DaisyPod hw;
plaits::Voice voice;
alignas(16) uint8_t voiceMemory[32768];
daisysp::ReverbSc DSY_SDRAM_BSS reverb;
volatile int values[9]={500,500,500,333,650,260,250,150,400};
volatile int model=14;
volatile bool playing=false,drone=false,benchmark=false;
volatile int currentNote=48;
volatile unsigned cpuPeak=0,overruns=0;
volatile bool sceneLoading=false,resetScene=false;
volatile unsigned rxwrite=0,rxread=0;
char ring[512];
uint32_t randomState=0x7487ac12;
uint32_t RandomWord(){randomState^=randomState<<13;randomState^=randomState>>17;randomState^=randomState<<5;return randomState;}
#include "scenes.h"
void Receive(uint8_t* data,uint32_t* count){for(unsigned i=0;i<*count;i++){unsigned next=(rxwrite+1)%sizeof(ring);if(next==rxread)break;ring[rxwrite]=data[i];__DMB();rxwrite=next;}}
void Audio(AudioHandle::InputBuffer,AudioHandle::OutputBuffer out,size_t count){
 if(sceneLoading){for(unsigned i=0;i<count;i++)out[0][i]=out[1][i]=0;return;}
 uint32_t begin=System::GetTick();
 static float level=0,wet=.175f,phase=0,until=0;
 static int degree=0,previousModel=-1;
 static bool wasPlaying=false;
 static plaits::Voice::Frame frames[96];
 if(resetScene){level=0;wet=values[6]/1000.f*.7f;phase=0;until=0;degree=0;wasPlaying=false;randomState=0x7487ac12;resetScene=false;}
 if(model!=previousModel){previousModel=model;until=0;}
 if(playing&&!wasPlaying)until=0;wasPlaying=playing;
 bool active=playing||benchmark,trigger=false;
 const float rate=.1f+1.9f*values[5]/1000.f;
 if(active){until-=count;if(until<=0){until=48000.f/rate;trigger=true;int step=int(RandomWord()%3)-1;degree+=step;if(degree<0)degree=0;if(degree>9)degree=9;}}
 static const int scale[5]={0,3,5,7,10};
 int root=36+(values[3]*36+500)/1000;
 currentNote=drone?root:root+scale[degree%5]+12*(degree/5);
 phase+=count/(48000.f*37.f);if(phase>1)phase-=1;
 float drift=values[7]/1000.f;
 auto clamp=[](float x){return fmaxf(0,fminf(1,x));};
 plaits::Patch patch{};
 patch.engine=model;patch.note=currentNote;
 patch.harmonics=clamp(values[0]/1000.f+.13f*drift*sinf(6.2831853f*phase));
 patch.timbre=clamp(values[1]/1000.f+.25f*drift*sinf(6.2831853f*phase+2.1f/6.2831853f));
 patch.morph=clamp(values[2]/1000.f+.25f*drift*cosf(6.2831853f*phase+.7f/6.2831853f));
 patch.decay=values[4]/1000.f;patch.lpg_colour=.5f;
 plaits::Modulations mod{};
 mod.trigger_patched=!drone;mod.trigger=trigger?1:0;mod.level=1;
 voice.Render(patch,mod,frames,count);
 float gain=values[8]/1000.f*.65f;
 for(unsigned i=0;i<count;i++){
  level+=.0005f*((playing?1.f:0.f)-level);wet+=.001f*(values[6]/1000.f*.7f-wet);
  float left=frames[i].out/32768.f*gain*level,right=frames[i].aux/32768.f*gain*level,l,r;
  reverb.Process(left,right,&l,&r);
  out[0][i]=.8f*tanhf(((1-wet)*left+wet*l)/.8f)*level;
  out[1][i]=.8f*tanhf(((1-wet)*right+wet*r)/.8f)*level;
 }
 unsigned usec=static_cast<unsigned>((System::GetTick()-begin)*(1000000.f/System::GetTickFreq()));
 if(usec/10>cpuPeak)cpuPeak=usec/10;if(usec>=1000)overruns++;
}
void Command(char* line){
 if(!strcmp(line,"RESTART")){playing=false;benchmark=false;System::Delay(350);System::ResetToBootloader(System::DAISY_SKIP_TIMEOUT);return;}
 if(!strcmp(line,"BOOT")){playing=false;System::Delay(350);System::ResetToBootloader();return;}
 if(!strcmp(line,"UPDATE")){playing=false;System::Delay(350);System::ResetToBootloader(System::DAISY_INFINITE_TIMEOUT);return;}
 if(!strcmp(line,"BENCH 1")){benchmark=true;cpuPeak=overruns=0;return;}
 if(!strcmp(line,"BENCH 0")){benchmark=false;return;}
 int index,value;char hex[41];
 if(sscanf(line,"SCENE SELECT %d",&index)==1){sceneSelect(index);return;}
 if(sscanf(line,"SCENE SAVE %d %40s",&index,hex)==2){sceneSave(index,hex);return;}
 if(sscanf(line,"SCENE LOAD %d",&index)==1){sceneLoad(index);return;}
 if(sscanf(line,"SET %d %d",&index,&value)==2&&index>=0&&index<9&&value>=0&&value<=1000)values[index]=value;
 else if(sscanf(line,"MODEL %d",&value)==1&&value>=0&&value<24){model=value;cpuPeak=overruns=0;}
 else if(!strcmp(line,"PLAY 1"))playing=true;
 else if(!strcmp(line,"PLAY 0"))playing=false;
 else if(!strcmp(line,"MODE 1"))drone=true;
 else if(!strcmp(line,"MODE 0"))drone=false;
}
int main(){
 hw.Init();
 hw.led1.Set(0,0,0);hw.led2.Set(0,0,0);hw.UpdateLeds();hw.seed.SetLed(false);
 hw.SetAudioBlockSize(48);hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
 stmlib::BufferAllocator allocator(voiceMemory,sizeof(voiceMemory));voice.Init(&allocator);
 reverb.Init(48000);reverb.SetFeedback(.85f);reverb.SetLpFreq(9000);
 hw.seed.usb_handle.Init(UsbHandle::FS_INTERNAL);hw.seed.usb_handle.SetReceiveCallback(Receive,UsbHandle::FS_INTERNAL);
 hw.StartAdc();hw.StartAudio(Audio);
 sceneInit();
 char line[96];unsigned used=0;bool overflow=false;uint32_t lastReport=0,lastControls=0;
 float previous[2]={0,0};bool caught[2]={false,false};int targetSeen[2]={values[1],values[2]};
 static char message[448];
 for(;;){
  while(rxread!=rxwrite){__DMB();char c=ring[rxread];rxread=(rxread+1)%sizeof(ring);if(c=='\n'){if(!overflow){line[used]=0;Command(line);}used=0;overflow=false;}else if(c!='\r'){if(used<sizeof(line)-1)line[used++]=c;else overflow=true;}}
  uint32_t now=System::GetNow();
  if(now-lastControls>=5){lastControls=now;hw.ProcessAllControls();
   for(int i=0;i<2;i++){int p=i+1;float v=i?hw.knob2.Process():hw.knob1.Process(),target=values[p]/1000.f;
    if(values[p]!=targetSeen[i])caught[i]=false;
    if(fabsf(v-target)<.025f||(v-target)*(previous[i]-target)<0)caught[i]=true;
    if(caught[i]&&fabsf(v-target)>.004f)values[p]=int(v*1000);
    targetSeen[i]=values[p];previous[i]=v;}
   int increment=hw.encoder.Increment();if(increment){model=(model+increment+24)%24;cpuPeak=overruns=0;}
   if(hw.button1.RisingEdge())playing=!playing;if(hw.button2.RisingEdge())drone=!drone;
   // LEDs stay off; software PWM at the control-loop rate causes visible flicker.
  }
  sceneUpdateDirty();
  if(now-lastReport>=100){lastReport=now;
   int n=snprintf(message,sizeof(message),"PSTATE %d %d %d %d %d %d %d %d %d %d %d %d %d %u %u\n",model,values[0],values[1],values[2],values[3],values[4],values[5],values[6],values[7],values[8],playing?1:0,drone?1:0,currentNote,cpuPeak,overruns);
   char sceneHex[41];SceneNameToHex(sceneCached.name,sceneHex);
   n+=snprintf(message+n,sizeof(message)-n,"SCENE %d %lu %d %d %d %s\n",sceneSlot,static_cast<unsigned long>(sceneRevision),sceneStatus,sceneExists?1:0,sceneDirty?1:0,sceneHex);
   hw.seed.usb_handle.TransmitInternal(reinterpret_cast<uint8_t*>(message),n);
  }System::Delay(1);
 }
}
