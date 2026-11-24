#include <Arduino.h>
#include <SPI.h>
#include <EEPROM.h>
#include "LineProtocol.h"

struct Calibration {
  uint32_t magic;
  int16_t green[64], white[64];
  uint16_t checksum;
};
Calibration cal{};
constexpr uint32_t signature=0x4c433634;
bool saved=false;
uint8_t state=0, sequence=0, badCount[8]{};
int16_t sample[64];
linebus::Frame report;
linebus::Commands commands;
constexpr int strobe[2]={14,15};

int readAdc(int bank,int channel) {
  digitalWrite(2+bank,LOW);
  SPI.transfer(1);
  int high=SPI.transfer((8+channel)<<4) & 3;
  int low=SPI.transfer(0);
  digitalWrite(2+bank,HIGH);
  return (high<<8)|low;
}
void readHalf(int half,int *values) {
  for (int i=0;i<32;++i) {
    int bank=4*half+i/8;
    readAdc(bank,i%8); // Allow the sample capacitor to settle after channel switching.
    values[i]=readAdc(bank,i%8);
  }
}
void scan() {
  int dark[32], lit[32];report.badBanks=0;
  SPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE0));
  for (int half=0;half<2;++half) {
    delayMicroseconds(100);readHalf(half,dark);
    digitalWrite(strobe[half],HIGH);delayMicroseconds(100);readHalf(half,lit);
    digitalWrite(strobe[half],LOW);
    for (int b=0;b<4;++b) {
      bool allLow=true,allHigh=true;
      for(int j=0;j<8;++j){
        int i=8*b+j;
        sample[half*32+i]=dark[i]-lit[i];
        allLow &= dark[i]<=1 && lit[i]<=1;
        allHigh &= dark[i]>=1022 && lit[i]>=1022;
      }
      int bank=4*half+b;
      badCount[bank]=allLow||allHigh ? min(4,int(badCount[bank])+1) : 0;
      if (badCount[bank]>=4) report.badBanks |= 1<<bank;
    }
  }
  SPI.endTransaction();
}
void sendFrame() {
  report.sequence=sequence++;report.status=state | (saved?4:0);report.hits=0;report.weak=0;
  for(int i=0;i<64;++i){
    int span=cal.white[i]-cal.green[i];
    if(state!=3 || span<16) report.weak |= uint64_t(1)<<i;
    else if(2*(sample[i]-cal.green[i])>span) report.hits |= uint64_t(1)<<i;
  }
  uint8_t packet[linebus::size];linebus::encode(report,packet);Serial1.write(packet,sizeof(packet));
}
void calibrate(bool white) {
  int32_t total[64]={};saved=false;state &= white?1:2;
  for(int n=0;n<32;++n){ scan();sendFrame();for(int i=0;i<64;++i)total[i]+=sample[i]; }
  for(int i=0;i<64;++i)(white?cal.white:cal.green)[i]=total[i]/32;
  state |= white?2:1;
}
void setup(){
  Serial.begin(115200);Serial1.begin(500000);
  for(int i=2;i<=10;++i){digitalWrite(i,HIGH);pinMode(i,OUTPUT);}
  for(int pin:strobe){digitalWrite(pin,LOW);pinMode(pin,OUTPUT);}
  pinMode(18,INPUT_PULLUP);pinMode(22,OUTPUT);digitalWrite(22,LOW);
  Serial4.begin(115200);Serial5.begin(115200);SPI.begin();EEPROM.get(0,cal);
  if(cal.magic==signature && cal.checksum==linebus::crc((uint8_t*)&cal,offsetof(Calibration,checksum))){state=3;saved=true;}
}
void loop(){
  static uint32_t lidarBytes=0, auxBytes=0;
  for(int i=0;i<128 && Serial5.available();++i){Serial5.read();++lidarBytes;}
  for(int i=0;i<128 && Serial4.available();++i){Serial4.read();++auxBytes;}
  int cmd=Serial.read();
  for(int i=0;i<24 && Serial1.available();++i){
    int action=commands.accept(Serial1.read());
    if(action>=0){cmd=action;break;}
  }
  if(cmd=='p'){
    SPI.beginTransaction(SPISettings(500000,MSBFIRST,SPI_MODE3));
    digitalWrite(10,LOW);delayMicroseconds(2);SPI.transfer(0);delayMicroseconds(180);
    int product=SPI.transfer(0);digitalWrite(10,HIGH);SPI.endTransaction();
    Serial.printf("flow_register_00=%02x motion_pin=%d lidar_bytes=%lu aux4_bytes=%lu\n",product,digitalRead(18),lidarBytes,auxBytes);
  }
  if(cmd=='g'||cmd=='w')calibrate(cmd=='w');
  scan();sendFrame();
  if(cmd=='r') {for(int i=0;i<64;++i)Serial.printf("%d%c",sample[i],i==63?'\n':',');}
  if(cmd=='s' && state==3 && !report.weak && !report.badBanks){
    cal.magic=signature;cal.checksum=linebus::crc((uint8_t*)&cal,offsetof(Calibration,checksum));EEPROM.put(0,cal);saved=true;Serial.println("calibration saved");
  }
  report.lastCommand=commands.acknowledged;
  if(cmd=='?')Serial.printf("cal=%u weak=%08lx%08lx bad_banks=%02x\n",state,(unsigned long)(report.weak>>32),(unsigned long)report.weak,report.badBanks);
}
