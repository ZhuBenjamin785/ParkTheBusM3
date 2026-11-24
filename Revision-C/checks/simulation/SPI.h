#pragma once
#include "Arduino.h"
#define MSBFIRST 0
#define SPI_MODE0 0
#define SPI_MODE3 3
struct SPISettings{SPISettings(int,int,int){}};
extern int contrast[64],failedBank;
struct Bus{int phase=0,result=0;void begin(){}void beginTransaction(SPISettings){}void endTransaction(){}
 int transfer(int v){
 int bank=0;while(bank<8&&pins[2+bank])++bank;
 if(bank==8)return 0;
 if(phase++==0){return 0;}
 if(phase==2){int ch=(v>>4)&7;result=bank==failedBank?1023:900-(pins[14+bank/4]?contrast[8*bank+ch]:0);return result>>8;}
 phase=0;return result&255;
 }};
extern Bus SPI;
