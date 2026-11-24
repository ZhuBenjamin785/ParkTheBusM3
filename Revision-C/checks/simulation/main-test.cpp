#include <cassert>
#include "MainControl.ino"
uint32_t us=0;int pins[64],pwms[64],adc[64];
Port Serial,Serial1,Serial4,Serial5,Serial7,Serial8;I2C Wire;Memory EEPROM;
uint8_t camCounter=0,ringCounter=0;
void ticks(int ms,bool link=true,bool cam=true,uint64_t hits=0){
 for(int i=0;i<ms;i++){
  us+=1000;
  if(cam&&i%12==0){uint8_t d[]={0xb6,camCounter++,0,240,0,120,0,92,0};d[8]=frameCrc(d,8);Serial1.in.insert(Serial1.in.end(),d,d+9);}
  if(link&&i%8==0){linebus::Frame f;f.weak=0;f.status=3;f.hits=hits;f.sequence=ringCounter++;uint8_t p[24];linebus::encode(f,p);Serial8.in.insert(Serial8.in.end(),p,p+24);}
  loop();
 }
}
int main(){
 pins[39]=pins[40]=pins[41]=1; adc[38]=3150;
 setup();ticks(1800);assert(heading.healthy(millis()));assert(!enabled);
 handleCommand((char*)"center 160 120");handleCommand((char*)"front");cameraX=160;cameraY=200;handleCommand((char*)"right");
 for(int i=1;i<5;++i){char cmd[20];snprintf(cmd,sizeof(cmd),"sign %d 1 1",i);handleCommand(cmd);}
 handleCommand((char*)"save");assert(commissioned);
 pins[32]=1;ticks(33);assert(enabled&&pwms[2]>0&&!pins[7]);assert(pwms[6]==0);
 pins[40]=0;ticks(12);assert(pwms[6]>0);
 ticks(24,true,true,0x8000000000000001ULL);assert(enabled&&pwms[2]>0&&pins[7]&&pwms[6]==0);
 ticks(110,false,true);assert(!enabled&&!pins[33]&&!pwms[2]);
 ticks(12);assert(!enabled);
 pins[32]=0;ticks(3);pins[32]=1;ticks(22);assert(enabled);
 ticks(210,true,false);assert(!enabled&&!pwms[2]);
 pins[32]=0;ticks(10);pins[32]=1;ticks(22);assert(enabled);
 pins[41]=0;ticks(2);assert(!enabled&&!pwms[2]);pins[41]=1;ticks(15);assert(!enabled);
 pins[32]=0;ticks(5);pins[32]=1;ticks(22);assert(enabled);
 Wire.live=false;ticks(22);assert(!enabled);

 pins[32]=0;pins[41]=1;ticks(5);char jog[]="jog 2";handleCommand(jog);ticks(3);
 pins[32]=1;ticks(25);assert(pwms[3]==38 && !pwms[2] && !pwms[6]);ticks(185);
 assert(!pins[33] && !pwms[3]);ticks(30);assert(!pins[33]);pins[32]=0;ticks(3);
 char calibrate[]="line green";handleCommand(calibrate); assert(Serial8.out.size()>=6);
 linebus::Commands parser;int action=-1;
 for(size_t i=Serial8.out.size()-6;i<Serial8.out.size();++i)action=parser.accept(Serial8.out[i]);
 assert(action=='g');
}
