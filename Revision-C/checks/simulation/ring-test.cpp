#include <cassert>
#include "LineNode.ino"
uint32_t us=0;int pins[64],pwms[64],adc[64],contrast[64],failedBank=-1;
Port Serial,Serial1,Serial4,Serial5,Serial7,Serial8;Bus SPI;Memory EEPROM;
int main(){
 setup();assert(!state);
 for(int i=0;i<64;++i)contrast[i]=80+i;
 scan();for(int i=0;i<64;++i)assert(sample[i]==80+i);
 calibrate(false);assert(state==1);
 for(int i=0;i<64;++i)contrast[i]=320+i;
 calibrate(true);
 for(int i=0;i<64;++i)contrast[i]=80+i;
 contrast[0]=320;contrast[63]=383;
 scan();sendFrame();assert(state==3&&!report.weak&&report.hits==0x8000000000000001ULL);
 Serial.in.push_back('s');loop();state=0;setup();assert(state==3);
 failedBank=4;for(int i=0;i<5;++i)scan();assert(report.badBanks==16);
 failedBank=-1;scan();assert(!report.badBanks);
 cal.white[8]=cal.green[8]+1;sendFrame();assert(report.weak==(1ULL<<8));
 assert(!pins[14]&&!pins[15]);

 uint8_t cmd[6];linebus::command(93,'w',cmd);
 Serial1.in.insert(Serial1.in.end(),cmd,cmd+6);loop();
 assert(report.lastCommand==93 || commands.acknowledged==93);assert(cal.white[63]==383);
 linebus::Receiver rx;linebus::Frame f;f.sequence=255;f.status=7;f.weak=0;f.hits=1ULL<<63;
 uint8_t bytes[24];linebus::encode(f,bytes);rx.accept(84,0);
 for(auto c:bytes)rx.accept(c,4);assert(rx.healthy(8)&&rx.frame.hits==f.hits);
 for(auto c:bytes)rx.accept(c,54);assert(!rx.healthy(105));
 f.sequence=0;linebus::encode(f,bytes);bytes[11]^=32;for(auto c:bytes)rx.accept(c,110);
 assert(!rx.healthy(110));linebus::encode(f,bytes);for(auto c:bytes)rx.accept(c,120);assert(rx.healthy(124));
}
