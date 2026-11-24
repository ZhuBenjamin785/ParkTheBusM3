#pragma once
#include <Arduino.h>
#include <Wire.h>

// MinIMU-9 v6 LSM6DSO. Install its +Z arrow toward the top of the robot.
class Heading {
  uint8_t address=0;
  uint16_t samples=0;
  int32_t total=0;
  float bias=0, angle=0, speed=0;
  uint32_t probeTime=0, lastRead=0, validAt=0;

  bool readRegister(uint8_t reg,uint8_t *out,uint8_t n){
    Wire.beginTransmission(address);Wire.write(reg);
    if(Wire.endTransmission(false) || Wire.requestFrom(address,n)!=n)return false;
    for(int i=0;i<n;++i)out[i]=Wire.read();
    return true;
  }
  bool writeRegister(uint8_t reg,uint8_t value){
    Wire.beginTransmission(address);Wire.write(reg);Wire.write(value);
    return !Wire.endTransmission();
  }
 public:
  void update(uint32_t now,bool stopped){
    if(!address){
      if(!stopped || now-probeTime<250)return;
      probeTime=now;
      for(uint8_t candidate:{0x6a,0x6b}){
        uint8_t id=0; address=candidate;
        if(readRegister(0x0f,&id,1) && id==0x6c && writeRegister(0x12,0x44) && writeRegister(0x11,0x40))break;
        address=0;
      }
      samples=0;total=0;lastRead=now;return;
    }
    uint32_t elapsed=now-lastRead;
    if(elapsed<10)return;
    lastRead=now;uint8_t data[2];
    if(!readRegister(0x26,data,2)){address=0;return;}
    int16_t raw=int16_t(uint16_t(data[0]) | (uint16_t(data[1])<<8));
    if(samples<128){
      if(!stopped || abs(raw)>570){samples=0;total=0;return;}
      total+=raw;
      if(++samples==128)bias=total/128.f;
      return;
    }
    validAt=now;
    speed=(raw-bias)*.00875f*.017453293f;
    if(stopped)angle=0;
    else if(elapsed<50)angle=remainderf(angle+speed*elapsed*.001f,6.2831853f);
  }
  bool healthy(uint32_t now) const{return address && samples==128 && now-validAt<50;}
  float correction() const{return constrain(-2.f*angle-.25f*speed,-1.f,1.f);}
};
