#pragma once
#include "Arduino.h"
struct I2C{int reg=0,idx=0,count=0;bool live=true;void begin(){}void setClock(int){}void beginTransmission(int){count=0;}
 void write(int v){if(!count++)reg=v;}int endTransmission(bool=true){return live?0:1;}int requestFrom(int,int n){idx=0;return live?n:0;}
 int read(){++idx;return reg==0x0f?0x6c:0;}};
extern I2C Wire;
