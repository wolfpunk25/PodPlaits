#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>
#include "Moods.h"
// Dedicated 3.3V link: GPIO35 RX, GPIO27 TX. USB remains for updates/diagnostics.
HardwareSerial podLink(2);
TFT_eSPI tft;SPIClass touchSpi(VSPI);XPT2046_Touchscreen touch(33,36);Preferences preferences;
float ax=0,bx=0,cx=0,ay=0,by=0,cy=0;
const uint16_t bg=0x0000,panel=0x18E3,mint=0x7DB7,ink=0xBDF7,dim=0x7BEF;
const uint16_t accents[5]={0x4DF6,0xFCAA,0xB37F,0x8E51,0xFC8D};
const uint16_t sliderColors[9]={0x4DF6,0xFC8D,0xB37F,0xFCAA,0xFBAE,0x8E51,0xB37F,0x4DF6,0x7DB7};
int values[9]={500,500,500,333,650,260,250,150,400};int model=14,page=0;
bool controlOverlay=false;unsigned controlRevision=0xffffffff;uint32_t controlLastReport=0;
void drawControl();void hideControl();
int controlKind=0,controlValue=0,controlPhysical=0,controlPicked=1;
bool playing=false,drone=false,linked=false;uint32_t lastState=0,lastTouch=0;
int sceneSlot=0,sceneStatus=0;bool sceneExists=false,sceneDirty=true,sceneLocalName=false,sceneAwaitSave=false;
uint32_t sceneRevision=0xffffffff;
char sceneName[21]="Scene 01",editingName[21]="";
bool sceneEditing=false,sceneConfirm=false;
void draw();void drawScenes();void drawKeyboard();
const char* names[]={"Analog + filter","Phase distortion","FM bank 1","FM bank 2","FM bank 3","Wave terrain","String machine","Chiptune","Virtual analog","Waveshaper","FM","Granular formants","Harmonic oscillator","Wavetable","Chords","Speech","Saw swarm","Filtered noise","Particle noise","String resonator","Modal resonator","Bass drum","Snare drum","Hi-hat"};
// CYD common-anode RGB LED. Full-intensity, saturated colours for illumination through a case.
const uint8_t rearPins[3]={22,16,17};
bool rearLedReady=false;
void initRearLed(){
 bool ok=true;
 for(uint8_t pin:rearPins){pinMode(pin,OUTPUT);digitalWrite(pin,HIGH);if(!ledcAttach(pin,5000,8))ok=false;else ledcWrite(pin,255);}
 rearLedReady=ok;
 if(!ok)for(uint8_t pin:rearPins){ledcDetach(pin);pinMode(pin,OUTPUT);digitalWrite(pin,HIGH);}
}
void setRearLed(uint16_t color){
 if(!rearLedReady)return;
 const unsigned brightness=255;
 ledcWrite(rearPins[0],255-((color>>11)&31)*brightness/31);
 ledcWrite(rearPins[1],255-((color>>5)&63)*brightness/63);
 ledcWrite(rearPins[2],255-(color&31)*brightness/31);
}
void label(const char*s,int x,int y,int font=2,uint16_t color=ink){tft.setTextColor(color,bg);tft.drawString(s,x,y,font);}
void drawHeader(){if(controlOverlay)return;if(sceneEditing)return;tft.fillRect(0,0,320,23,bg);label("PLAITS / POD",14,5,1,accents[page]);label(linked?"POD CONNECTED":"WAITING FOR POD",200,5,1,linked?mint:dim);}
int rowY(int row){return page==0?110+row*32:page==1?86+row*40:62+row*46;}
void drawRow(int index,int y){if(controlOverlay)return;
 tft.fillRect(10,y,300,page==0?31:40,bg);
 const char* labels[]={"HARMONICS","TIMBRE","MORPH","ROOT NOTE","DECAY","NOTE RATE","REVERB","DRIFT","VOLUME"};
 label(labels[index],14,y,2,sliderColors[index]);char text[32];
 if(index==3){const char* notes[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};int n=36+(values[index]*36+500)/1000;snprintf(text,sizeof(text),"%s%d",notes[n%12],n/12-1);}
 else if(index==5)snprintf(text,sizeof(text),"%.2f / sec",.1f+values[5]*1.9f/1000);
 else snprintf(text,sizeof(text),"%d%%",values[index]/10);
 tft.setTextColor(sliderColors[index],bg);tft.drawRightString(text,304,y,2);
 int dy=page==0?22:23;tft.fillRoundRect(16,y+dy,288,8,4,panel);tft.fillRoundRect(16,y+dy,max(8,values[index]*288/1000),8,4,sliderColors[index]);
}
void drawModel(){if(controlOverlay)return;uint16_t engineColor=accents[model<8?2:model<16?0:1];tft.fillRect(8,60,304,44,bg);for(int x=10;x<=270;x+=260){tft.fillRoundRect(x,60,40,43,6,panel);tft.setTextColor(engineColor,panel);tft.drawCentreString(x==10?"<":">",x+20,68,4);}char text[32];snprintf(text,sizeof(text),"ENGINE %d / 24",model+1);label(text,58,61,1,engineColor);label(names[model],58,79,2,engineColor);}
void drawMode(){if(controlOverlay)return;tft.fillRoundRect(14,60,292,23,5,panel);tft.setTextColor(accents[1],panel);tft.drawCentreString(drone?"MODE: DRONE (tap to change)":"MODE: AUTO NOTES (tap to change)",160,66,1);}
void drawPlay(){if(controlOverlay)return;if(sceneEditing)return;tft.fillRoundRect(14,208,292,30,6,playing?panel:mint);tft.setTextColor(playing?ink:bg,playing?panel:mint);tft.drawCentreString(playing?"PAUSE SOUND":"START SOUND",160,215,2);}
void sceneSendSave(){
 char hex[41];const char* digits="0123456789ABCDEF";unsigned n=strlen(sceneName);
 for(unsigned i=0;i<n;i++){unsigned c=static_cast<unsigned char>(sceneName[i]);hex[2*i]=digits[c>>4];hex[2*i+1]=digits[c&15];}hex[2*n]=0;
 sceneAwaitSave=true;podLink.printf("SCENE SAVE %d %s\n",sceneSlot,hex);sceneConfirm=false;drawScenes();
}
void sceneButton(int x,int y,int width,const char* text,uint16_t color){tft.fillRoundRect(x,y,width,36,5,panel);tft.drawRoundRect(x,y,width,36,5,color);tft.setTextColor(color,panel);tft.drawCentreString(text,x+width/2,y+10,2);}
void drawScenes(){if(controlOverlay)return;
 if(sceneEditing)return;tft.fillRect(0,56,320,149,bg);
 if(sceneConfirm){label("REPLACE SAVED SCENE?",18,68,2,accents[1]);label(sceneName,18,96,2);sceneButton(14,145,140,"CANCEL",dim);sceneButton(166,145,140,"REPLACE",accents[1]);return;}
 for(int x=10;x<=270;x+=260){tft.fillRoundRect(x,60,40,43,6,panel);tft.setTextColor(accents[3],panel);tft.drawCentreString(x==10?"<":">",x+20,68,4);}
 char title[32];snprintf(title,sizeof(title),"SCENE %02d / 32",sceneSlot+1);label(title,76,61,1,accents[3]);tft.setTextColor(ink,bg);tft.drawCentreString(sceneName,160,81,2);
 const char* status=!linked?"Waiting for Pod":sceneStatus==3?"SD card unavailable":sceneStatus==4?"Scene file is invalid":sceneStatus==5?"SD read/write failed":sceneLocalName?"Name edited - tap SAVE":sceneStatus==2&&!sceneDirty?"Loaded - tap START SOUND":!sceneExists?"Empty slot":sceneDirty?"Current settings differ":sceneStatus==1?"Saved to SD card":sceneStatus==7?"Recovered backup":"Matches current settings";
 label(status,16,106,1,sceneStatus>=3&&sceneStatus<=5?accents[1]:dim);
 sceneButton(14,118,140,"LOAD",sceneExists?accents[3]:dim);sceneButton(166,118,140,"SAVE",accents[3]);sceneButton(14,164,292,"EDIT NAME",accents[3]);
}
int matchingMood(){
 for(unsigned m=0;m<moodCount;m++){bool match=model==moods[m].model&&drone==moods[m].drone;for(unsigned i=0;i<9;i++)if(values[i]!=moods[m].values[i])match=false;if(match)return m;}return -1;
}
void drawMoods(){if(controlOverlay)return;
 if(sceneEditing)return;tft.fillRect(0,56,320,149,bg);int active=matchingMood();
 for(unsigned i=0;i<moodCount;i++){int x=10+(i%2)*155,y=60+(i/2)*48;uint16_t color=accents[4];tft.fillRoundRect(x,y,145,43,5,panel);if(int(i)==active)tft.drawRoundRect(x,y,145,43,5,color);tft.setTextColor(color,panel);tft.drawString(moods[i].name,x+7,y+5,2);tft.setTextColor(ink,panel);tft.drawString(moods[i].description,x+7,y+28,1);}
}
void drawKeyboard(){if(controlOverlay)return;
 tft.fillScreen(bg);label("SCENE NAME (20 CHARACTERS)",14,3,1,dim);label(editingName,14,18,2,accents[3]);tft.fillRoundRect(250,8,56,28,4,panel);tft.setTextColor(accents[3],panel);tft.drawCentreString("CLEAR",278,18,1);
 const char* rows[]={"1234567890","QWERTYUIOP","ASDFGHJKL","ZXCVBNM-_"};
 for(int row=0;row<4;row++)for(unsigned k=0;rows[row][k];k++){int x=5+k*31,y=42+row*34;tft.fillRoundRect(x,y,28,30,3,panel);char c[2]={rows[row][k],0};tft.setTextColor(ink,panel);tft.drawCentreString(c,x+14,y+6,2);}
 tft.fillRoundRect(14,179,140,24,4,panel);tft.fillRoundRect(166,179,140,24,4,panel);tft.setTextColor(accents[3],panel);tft.drawCentreString("SPACE",84,183,2);tft.drawCentreString("DELETE",236,183,2);
 tft.fillRoundRect(14,208,140,30,5,panel);tft.fillRoundRect(166,208,140,30,5,panel);tft.setTextColor(dim,panel);tft.drawCentreString("CANCEL",84,215,2);tft.setTextColor(accents[3],panel);tft.drawCentreString("DONE",236,215,2);
}
void keyboardTouch(int x,int y){
 if(x>=250&&x<=306&&y>=8&&y<36){editingName[0]=0;drawKeyboard();return;}
 size_t n=strlen(editingName);
 if(y>=208){if(x>=166){bool visible=false;for(unsigned i=0;i<n;i++)if(editingName[i]!=' ')visible=true;if(!visible)return;snprintf(sceneName,sizeof(sceneName),"%s",editingName);sceneLocalName=true;}sceneEditing=false;draw();return;}
 if(y>=179&&y<203){if(x>=166){if(n)editingName[n-1]=0;}else if(x>=14&&x<154&&n<20){editingName[n]=' ';editingName[n+1]=0;}drawKeyboard();return;}
 if(y>=42&&y<178&&x>=5){int row=(y-42)/34,k=(x-5)/31;const char* rows[]={"1234567890","QWERTYUIOP","ASDFGHJKL","ZXCVBNM-_"};if(row<4&&k>=0&&k<int(strlen(rows[row]))&&n<20){editingName[n]=rows[row][k];editingName[n+1]=0;drawKeyboard();}}
}
void draw(){if(controlOverlay)return;
 if(sceneEditing){drawKeyboard();return;}
 tft.fillScreen(bg);drawHeader();const char* tabs[]={"SOUND","PLAY","SPACE","SCENES","MOODS"};
 for(int i=0;i<5;i++){tft.fillRoundRect(3+i*63,25,60,28,5,i==page?panel:bg);if(i==page)tft.drawRoundRect(3+i*63,25,60,28,5,accents[i]);else tft.drawFastHLine(8+i*63,51,50,accents[i]);tft.setTextColor(accents[i],i==page?panel:bg);tft.drawCentreString(tabs[i],33+i*63,31,2);}
 if(page==4)drawMoods();else if(page==3)drawScenes();else{if(page==0)drawModel();if(page==1)drawMode();for(int row=0;row<3;row++)drawRow(page*3+row,rowY(row));}drawPlay();
}
void hideControl(){if(!controlOverlay)return;controlOverlay=false;draw();}
void drawControl(){
 tft.fillRoundRect(10,69,300,121,9,panel);tft.drawRoundRect(10,69,300,121,9,mint);
 const char* title=controlKind==1?"KNOB 1 / TIMBRE":controlKind==2?"KNOB 2 / MORPH":controlKind==3?"ENCODER / ENGINE":controlKind==4?"BUTTON 1 / PLAYBACK":controlKind==5?"BUTTON 2 / MODE":"ENCODER PRESS";
 tft.setTextColor(mint,panel);tft.drawCentreString(title,160,81,2);
 char text[64],detail[64];detail[0]=0;
 if(controlKind<=2){snprintf(text,sizeof(text),"%d%%",(controlValue+5)/10);if(!controlPicked)snprintf(detail,sizeof(detail),"Knob %d%% - turn to %d%% to pick up",(controlPhysical+5)/10,(controlValue+5)/10);else snprintf(detail,sizeof(detail),"Release after adjusting");}
 else if(controlKind==3||controlKind==6){snprintf(text,sizeof(text),"%s",names[controlValue]);snprintf(detail,sizeof(detail),controlKind==6?"Engine %d / 24 - turn to change":"Engine %d / 24",controlValue+1);}
 else if(controlKind==4)snprintf(text,sizeof(text),"%s",controlValue?"PLAYING":"PAUSED");
 else snprintf(text,sizeof(text),"%s",controlValue?"DRONE":"AUTO NOTES");
 tft.setTextColor(ink,panel);tft.drawCentreString(text,160,116,2);tft.setTextColor(mint,panel);tft.drawCentreString(detail,160,157,1);
}
void calibrate() {
 float rx[3],ry[3];const float sx[3]={28,292,160},sy[3]={28,28,212};
 for(int i=0;i<3;i++) {
  tft.fillScreen(bg);label("TOUCH AND HOLD EACH CROSS",40,100,2);
  tft.drawLine(sx[i]-10,sy[i],sx[i]+10,sy[i],mint);tft.drawLine(sx[i],sy[i]-10,sx[i],sy[i]+10,mint);
  while(touch.touched())delay(10);
  unsigned n=0;float x=0,y=0;
  while(n<24) {if(touch.touched()) {auto p=touch.getPoint();if(p.z>200){x+=p.x;y+=p.y;n++;}}delay(12);}
  rx[i]=x/n;ry[i]=y/n;while(touch.touched())delay(10);
 }
 float det=rx[0]*(ry[1]-ry[2])+rx[1]*(ry[2]-ry[0])+rx[2]*(ry[0]-ry[1]);
 if(fabsf(det)<1000) {calibrate();return;}
 auto solve=[&](const float *v,float &a,float &b,float &c) {
  a=(v[0]*(ry[1]-ry[2])+v[1]*(ry[2]-ry[0])+v[2]*(ry[0]-ry[1]))/det;
  b=(v[0]*(rx[2]-rx[1])+v[1]*(rx[0]-rx[2])+v[2]*(rx[1]-rx[0]))/det;
  c=v[0]-a*rx[0]-b*ry[0];
 };
 solve(sx,ax,bx,cx);solve(sy,ay,by,cy);
 float calibration[]={ax,bx,cx,ay,by,cy};preferences.putBytes("cal",calibration,sizeof(calibration));
}
void setup() {
 Serial.begin(115200);podLink.setRxBufferSize(1024);podLink.begin(115200,SERIAL_8N1,35,27);initRearLed();pinMode(21,OUTPUT);digitalWrite(21,HIGH);
 tft.init();tft.setRotation(1);tft.invertDisplay(true);touchSpi.begin(25,39,32,33);touch.begin(touchSpi);touch.setRotation(1);
 preferences.begin("ambient",false);
 float c[6];bool valid=preferences.getBytesLength("cal")==sizeof(c);
 if(valid) {preferences.getBytes("cal",c,sizeof(c));ax=c[0];bx=c[1];cx=c[2];ay=c[3];by=c[4];cy=c[5];}
 delay(500);if(!valid || touch.touched())calibrate();draw();
}
void loop(){
 static char line[256];static unsigned used=0;static bool overflow=false,wasDown=false;
 while(podLink.available()){
  char c=podLink.read();if(c=='\n'){
   line[used]=0;int m,v[9],p,d,n;unsigned peak,missed;
   if(!overflow&&sscanf(line,"PSTATE %d %d %d %d %d %d %d %d %d %d %d %d %d %u %u",&m,&v[0],&v[1],&v[2],&v[3],&v[4],&v[5],&v[6],&v[7],&v[8],&p,&d,&n,&peak,&missed)==15){
    bool valid=m>=0&&m<24&&(p==0||p==1)&&(d==0||d==1);for(int i=0;i<9;i++)if(v[i]<0||v[i]>1000)valid=false;
    if(valid){if(model!=m){model=m;if(page==0)drawModel();}for(int i=0;i<9;i++)if(values[i]!=v[i]){values[i]=v[i];if(i/3==page)drawRow(i,rowY(i%3));}
     if(drone!=bool(d)){drone=d;if(page==1)drawMode();}if(playing!=bool(p)){playing=p;drawPlay();}if(!linked){linked=true;drawHeader();}lastState=millis();Serial.printf("DIRECT %s\n",line);if(page==4&&!sceneEditing){static int lastMood=-2;int active=matchingMood();if(lastMood!=active){lastMood=active;drawMoods();}}}
   }
   if(!overflow&&!strncmp(line,"CONTROL ",8)){
    unsigned revision;int kind,value,physical,picked;
    if(sscanf(line,"CONTROL %u %d %d %d %d",&revision,&kind,&value,&physical,&picked)==5&&kind>=0&&kind<=6&&physical>=0&&physical<=1000&&(picked==0||picked==1)&&value>=0&&value<=((kind==3||kind==6)?23:(kind==4||kind==5)?1:1000)){
     controlLastReport=millis();if(revision!=controlRevision){Serial.printf("FEEDBACK %u %d %d %d %d\n",revision,kind,value,physical,picked);controlRevision=revision;controlKind=kind;controlValue=value;controlPhysical=physical;controlPicked=picked;if(kind){controlOverlay=true;drawControl();}else hideControl();}
    }
   }
   if(!overflow&&!strncmp(line,"SCENE ",6)){
    int slot,status,exists,dirty;unsigned long revision;char hex[41];
    if(sscanf(line,"SCENE %d %lu %d %d %d %40s",&slot,&revision,&status,&exists,&dirty,hex)==6&&slot>=0&&slot<32&&status>=0&&status<=7&&(exists==0||exists==1)&&(dirty==0||dirty==1)){
     unsigned length=strlen(hex);char decoded[21];bool valid=length>0&&length<=40&&length%2==0;
     auto digit=[](char c){return c>='0'&&c<='9'?c-'0':c>='A'&&c<='F'?c-'A'+10:c>='a'&&c<='f'?c-'a'+10:-1;};
     for(unsigned i=0;valid&&i<length/2;i++){int a=digit(hex[2*i]),b=digit(hex[2*i+1]);if(a<0||b<0||a*16+b<32||a*16+b>126)valid=false;else decoded[i]=a*16+b;}
     if(valid&&(sceneRevision!=revision||sceneSlot!=slot)){
      decoded[length/2]=0;bool changedSlot=sceneSlot!=slot;
      if(changedSlot){sceneLocalName=false;sceneConfirm=false;sceneAwaitSave=false;}
      if(sceneAwaitSave&&status==1){sceneLocalName=false;sceneAwaitSave=false;}
      if(sceneAwaitSave&&status>=3&&status<=5)sceneAwaitSave=false;
      if(!sceneLocalName)snprintf(sceneName,sizeof(sceneName),"%s",decoded);
      sceneSlot=slot;sceneStatus=status;sceneExists=exists;sceneDirty=dirty;sceneRevision=revision;
      if(page==3&&!sceneEditing)drawScenes();
     }
    }
   }
   used=0;overflow=false;
  }else if(c!='\r'){if(used<sizeof(line)-1)line[used++]=c;else overflow=true;}
 }
 if(controlOverlay&&millis()-controlLastReport>1000)hideControl();
 if(linked&&millis()-lastState>2000){linked=false;drawHeader();}
 bool down=touch.touched();if(down&&millis()-lastTouch>45){auto p=touch.getPoint();if(p.z>200){int x=lroundf(ax*p.x+bx*p.y+cx),y=lroundf(ay*p.x+by*p.y+cy);
  if(controlOverlay&&!wasDown)hideControl();
  if(sceneEditing){if(!wasDown)keyboardTouch(x,y);}
  else if(!wasDown&&y>=25&&y<54&&x>=3&&x<318){int next=constrain((x-3)/63,0,4);if(next!=page){page=next;sceneConfirm=false;draw();}}
  else if(linked){
   if(y>=208&&y<240&&!wasDown)podLink.printf("PLAY %d\n",playing?0:1);
   else if(page==4&&!wasDown&&y>=60&&y<199){
    int column=x>=165?1:0,row=(y-60)/48,index=row*2+column;
    if(x>=10&&x<310&&(x-10)%155<145&&(y-60)%48<43&&index<int(moodCount)){
     snprintf(sceneName,sizeof(sceneName),"%s",moods[index].name);sceneLocalName=true;sceneConfirm=false;
     podLink.printf("MOOD %d\n",index);
    }
   }
   else if(page==3&&!wasDown){
    if(sceneConfirm){if(y>=145&&y<181){if(x>=166)sceneSendSave();else if(x>=14&&x<154){sceneConfirm=false;drawScenes();}}}
    else if(y>=60&&y<104){if(x>=8&&x<53)podLink.printf("SCENE SELECT %d\n",(sceneSlot+31)%32);else if(x>=268&&x<=312)podLink.printf("SCENE SELECT %d\n",(sceneSlot+1)%32);}
    else if(y>=118&&y<154){if(x>=14&&x<154&&sceneExists){sceneLocalName=false;podLink.printf("SCENE LOAD %d\n",sceneSlot);}else if(x>=166&&x<=306){if(sceneExists){sceneConfirm=true;drawScenes();}else sceneSendSave();}}
    else if(y>=164&&y<200&&x>=14&&x<=306){snprintf(editingName,sizeof(editingName),"%s",sceneName);sceneEditing=true;drawKeyboard();}
   }
   else if(page==0&&y>=60&&y<104&&!wasDown){if(x>=8&&x<53)podLink.printf("MODEL %d\n",(model+23)%24);else if(x>=268&&x<=312)podLink.printf("MODEL %d\n",(model+1)%24);}
   else if(page==1&&y>=60&&y<84&&!wasDown)podLink.printf("MODE %d\n",drone?0:1);
   else if(page<3){int top=page==0?110:page==1?86:62,step=page==0?32:page==1?40:46;if(y>=top&&y<top+step*3&&y<205&&x>=8&&x<=312){int row=(y-top)/step;podLink.printf("SET %d %d\n",page*3+row,constrain((x-16)*1000/288,0,1000));}}
  }wasDown=true;lastTouch=millis();}}
 if(!down)wasDown=false;
 static uint32_t lastPing=0;if(millis()-lastPing>=1000){lastPing=millis();podLink.print("PING\n");}
 static int rearColor=-1;const uint16_t rearColors[3]={0x07FF,0xFC00,0xF81F};uint16_t color=linked?rearColors[model<8?2:model<16?0:1]:0;
 if(rearColor!=color){setRearLed(color);rearColor=color;}
 delay(3);
}
