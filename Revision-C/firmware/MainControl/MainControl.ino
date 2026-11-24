#include <Arduino.h>
#include <Encoder.h>
#include <EEPROM.h>
#include <Wire.h>
#include "DriveSupervisor.h"
#include "LineProtocol.h"
#include "Motion.h"
#include "Heading.h"

Encoder e1(22,23), e2(24,25), e3(26,27), e4(14,37), roller(30,31);
Encoder *const encoders[]={&e1,&e2,&e3,&e4,&roller};
const int currentPins[]={15,16,17,20,21};
linebus::Receiver ring;
DriveSupervisor gate;
Boundary boundary;
Heading heading;
uint8_t frame[9],used=0,lastSequence=0;
uint16_t cameraX,cameraY,orangePixels;
uint32_t lastFrame;
bool haveFrame=false;
uint8_t frameCrc(const uint8_t *data, size_t count) {
  uint8_t crc = 0;
  for (size_t i = 0; i < count; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit)
      crc = crc & 0x80 ? (crc << 1) ^ 0x07 : crc << 1;
  }
  return crc;
}

void readCamera() {
  // Bound the work per loop even if the link is noisy.
  for (int i = 0; i < 48 && Serial1.available(); ++i) {
    frame[used++] = Serial1.read();
    if (used < sizeof(frame)) continue;
    if (frame[0] == 0xB6 && frameCrc(frame, 8) == frame[8]) {
      cameraX = uint16_t(frame[2]) << 8 | frame[3];
      cameraY = uint16_t(frame[4]) << 8 | frame[5];
      orangePixels = uint16_t(frame[6]) << 8 | frame[7];
      if (!haveFrame || frame[1] != lastSequence) lastFrame = millis();
      haveFrame = true;
      lastSequence = frame[1];
      used = 0;
    } else {
      memmove(frame, frame + 1, 8);
      used = 8;
    }
  }
}

struct Settings {
  uint32_t id; float centerX,centerY,front,hand;
  int8_t driveSign[4],encoderSign[4];uint8_t optics; uint16_t checksum;
};
Settings cfg{};
uint8_t commandId=0;
bool commissioned=false, trip=false, enabled=false;
int jogMotor=-1;
uint32_t jogRequest=0,jogStart=0;
bool jogRunning=false,jogFinished=false;
uint32_t lastWheel, stallSince[4]={};
float batteryV=0; bool batteryOK=false;
uint32_t lastPower=0; uint16_t excess[5]={}; bool rollerOn=false;
void powerCheck(uint32_t now){
  uint32_t elapsed=now-lastPower;
  if(elapsed<1)return;
  lastPower=now;
  float reading=analogRead(38)*(3.3f/4095)*(127.f/27);
  if(!batteryV)batteryV=reading;
  batteryV+=(reading-batteryV)*min(1.f,elapsed/100.f);
  if(batteryV<10.2f)batteryOK=false;
  if(!digitalRead(32)&&batteryV>10.65f)batteryOK=true;
  for(int i=0;i<5;++i){
    // G2 feedback is valid during PWM drive. Accumulate elevated samples; decay during brake.
    float amps=(analogRead(currentPins[i])*(3.3f/4095)-.050f)/.020f;
    if(enabled && amps>(i==4?3.f:1.25f))excess[i]=min(1000,int(excess[i])+3);
    else if(excess[i])--excess[i];
    if(excess[i]>48)trip=true;
  }
}
int duty[4]={}; long countOld[4]={}; float filtered[4]={},integral[4]={};
char command[90];size_t usedCommand=0;
const float tangentX[]={.7071068f,.7071068f,-.7071068f,-.7071068f};
const float tangentY[]={-.7071068f,.7071068f,.7071068f,-.7071068f};

bool cameraLive(uint32_t now){return haveFrame && now-lastFrame<200;}
void zeroWheels(){
  for(int i=0;i<4;++i){analogWrite(2+i,0);countOld[i]=encoders[i]->read();duty[i]=0;integral[i]=filtered[i]=0;stallSince[i]=0;}
  analogWrite(6,0);rollerOn=false;lastWheel=micros();
}
void handleCommand(char *cmd){
  if(!strcmp(cmd,"status")){
    Serial.printf("permit=%d role=%d ball=%d ring_age_ms=%lu cal=%u weak=%08lx%08lx adc_fault=%u camera=%d candidate=%d trip=%d saved=%d\n",
      digitalRead(32),!digitalRead(39),!digitalRead(40),millis()-ring.received,ring.frame.status,
      (unsigned long)(ring.frame.weak>>32),(unsigned long)ring.frame.weak,ring.frame.badBanks,cameraLive(millis()),orangePixels>0,trip,commissioned);
    return;
  }
  if(!strcmp(cmd,"line status")){
    Serial.printf("green=%d white=%d saved=%d bad_adc=%02x weak=%08lx%08lx command_ack=%u\n",
      !!(ring.frame.status&1),!!(ring.frame.status&2),!!(ring.frame.status&4),ring.frame.badBanks,
      (unsigned long)(ring.frame.weak>>32),(unsigned long)ring.frame.weak,ring.frame.lastCommand);return;
  }
  if(!strcmp(cmd,"sensors")){
    Serial.printf("battery=%.2fV counts",analogRead(38)*(3.3f/4095)*(127.f/27));
    for(auto e:encoders)Serial.printf(" %ld",(long)e->read());
    Serial.print(" current_adc");for(int pin:currentPins)Serial.printf(" %d",analogRead(pin));
    Serial.printf(" camera_xy=%u,%u aux_state=%d\n",cameraX,cameraY,digitalRead(36));return;
  }
  if(digitalRead(32)){Serial.println("Open local RUN for setup");return;}
  if(!strncmp(cmd,"line ",5)){
    int action=!strcmp(cmd+5,"green")?'g':!strcmp(cmd+5,"white")?'w':!strcmp(cmd+5,"save")?'s':0;
    if(!action)return;
    if(ring.frame.lastCommand==commandId || !commandId)commandId=ring.frame.lastCommand;
    if(++commandId==0)++commandId;
    uint8_t wire[6];linebus::command(commandId,action,wire);Serial8.write(wire,6);
    Serial.printf("line command %u sent\n",commandId);return;
  }
  if(!strcmp(cmd,"aux")){
    Serial.printf("aux7_state=%d queued=%d\n",digitalRead(36),Serial7.available());
    while(Serial7.available())Serial.printf("%02x ",Serial7.read());
    Serial7.write(0x55);Serial.println();return;
  }
  if(!strcmp(cmd,"imu")){
    Serial.printf("heading_ready=%d\n",heading.healthy(millis()));
    for(int addr:{0x1c,0x1e,0x6a,0x6b}){Wire.beginTransmission(addr);Serial.printf("%02x %s\n",addr,Wire.endTransmission()?"absent":"present");}return;
  }
  int x,y,ds,es;
  if(sscanf(cmd,"jog %d",&x)==1 && x>=1 && x<=5){
    jogMotor=x-1;jogRequest=millis();jogRunning=false;jogFinished=false;return;
  }
  if(sscanf(cmd,"center %d %d",&x,&y)==2 && x>=0 && x<320 && y>=0 && y<240){
    cfg.centerX=x;cfg.centerY=y;cfg.optics=0;commissioned=false;
  } else if(!strcmp(cmd,"front")||!strcmp(cmd,"right")){
    if(!cameraLive(millis())||!orangePixels||hypotf(cameraX-cfg.centerX,cameraY-cfg.centerY)<8)return;
    float a=atan2f(cameraY-cfg.centerY,cameraX-cfg.centerX);
    if(!strcmp(cmd,"front")){cfg.front=a;cfg.optics=1;}
    else if(cfg.optics&1){
      float s=sinf(a-cfg.front);if(fabsf(s)<.7f)return;
      cfg.hand=s>0?-1:1;cfg.optics|=2;
    }commissioned=false;
  } else if(sscanf(cmd,"sign %d %d %d",&x,&ds,&es)==3 && x>=1 && x<=4 && abs(ds)==1 && abs(es)==1){
    cfg.driveSign[x-1]=ds;cfg.encoderSign[x-1]=es;commissioned=false;
  } else if(!strcmp(cmd,"save") && cfg.optics==3){
    for(int i=0;i<4;++i)if(abs(cfg.encoderSign[i])!=1||abs(cfg.driveSign[i])!=1){Serial.println("Set four sign pairs first");return;}
    cfg.id=0x52434a43;cfg.checksum=linebus::crc((uint8_t*)&cfg,offsetof(Settings,checksum));EEPROM.put(0,cfg);commissioned=true;
  } else return;
  Serial.println("ok");
}
void terminal(){
  for(int j=0;j<24 && Serial.available();++j){
    char c=Serial.read();
    if(c=='\n'||c=='\r'){if(usedCommand){command[usedCommand]=0;handleCommand(command);usedCommand=0;}}
    else if(usedCommand<sizeof(command)-1)command[usedCommand++]=c;
  }
}
void setup(){
  gate.begin();Serial.begin(115200);Serial1.begin(115200);Serial8.begin(500000);Serial7.begin(115200);Wire.begin();Wire.setClock(400000);
  for(int p:{39,40,36})pinMode(p,INPUT);
  for(int i=0;i<5;++i){digitalWrite(7+i,LOW);pinMode(7+i,OUTPUT);}
  analogWriteResolution(8);analogReadResolution(12);EEPROM.get(0,cfg);
  commissioned=cfg.id==0x52434a43 && cfg.optics==3 && cfg.checksum==linebus::crc((uint8_t*)&cfg,offsetof(Settings,checksum));
  if(!commissioned){cfg={};cfg.centerX=160;cfg.centerY=120;}
  zeroWheels();
}
void wheels(Motion target){
  uint32_t t=micros(),elapsed=t-lastWheel;if(elapsed<10000)return;
  lastWheel=t;float dt=elapsed*1e-6f;
  for(int i=0;i<4;++i){
    long count=encoders[i]->read();int32_t dc=int32_t(uint32_t(count)-uint32_t(countOld[i]));countOld[i]=count;
    float speed=dc*cfg.encoderSign[i]*(0.00030159f/dt);
    filtered[i]+=(dt/(.035f+dt))*(speed-filtered[i]);
    float yaw=(fabsf(target.x)+fabsf(target.y)>.02f)?heading.correction():0;
    float goal=tangentX[i]*target.x+tangentY[i]*target.y+.0735f*yaw;float err=goal-filtered[i];
    if(fabsf(goal)<.008f){integral[i]=0;duty[i]=0;}
    else{
      float next=constrain(integral[i]+err*dt*100,-18.f,18.f);
      float pwm=185*goal+60*err+next;
      if(fabsf(pwm)<82||pwm*err<0)integral[i]=next;
      duty[i]=int(constrain(185*goal+60*err+integral[i],-82.f,82.f));
    }
    uint32_t now=millis();
    if(abs(duty[i])>26 && fabsf(goal)>.07f && filtered[i]*goal<.015f*fabsf(goal)){
      if(!stallSince[i])stallSince[i]=now;
      if(now-stallSince[i]>300){trip=true;zeroWheels();return;}
    }else stallSince[i]=0;
    int signedDuty=duty[i]*cfg.driveSign[i];
    digitalWrite(7+i,signedDuty<0);analogWrite(2+i,abs(signedDuty));
  }
  // Positive dribbler polarity must draw the ball in. Test wiring with the roller lifted.
  static long lastRoller=0;
  static uint32_t lastRollerMotion=0;
  long rc=roller.read();uint32_t now=millis();
  if(!rollerOn || rc!=lastRoller)lastRollerMotion=now;
  lastRoller=rc;
  rollerOn=target.capture;
  if(rollerOn && now-lastRollerMotion>300){trip=true;zeroWheels();return;}
  analogWrite(6,rollerOn?36:0);
}
void loop(){
  uint32_t now=millis();readCamera();
  for(int i=0;i<96 && Serial8.available();++i)ring.accept(Serial8.read(),now);
  if(!digitalRead(32))trip=false;
  heading.update(now,!digitalRead(32));
  powerCheck(now);
  // Raised-wheel test. A new STOP is required after each jog before autonomous motion.
  if(jogMotor>=0){
    bool play=digitalRead(32);
    if(!play && (jogRunning || jogFinished || now-jogRequest>5000)){
      jogMotor=-1;jogRunning=jogFinished=false;
    }else{
      if(!jogRunning && now-jogRequest>5000)jogFinished=true;
      if(jogRunning && now-jogStart>=180)jogFinished=true;
      enabled=gate.service(now,batteryOK && !trip && !jogFinished);
      if(enabled && play){
        if(!jogRunning){zeroWheels();jogRunning=true;jogStart=now;}
        digitalWrite(7+jogMotor,LOW);analogWrite(2+jogMotor,38);
      }else{
        zeroWheels();
        if(play && jogRunning)jogFinished=true;
      }
      boundary.reset();terminal();return;
    }
  }
  bool healthy=heading.healthy(now) && batteryOK && commissioned && ring.healthy(now) && cameraLive(now) && !trip;
  enabled=gate.service(now,healthy);
  if(!enabled){zeroWheels();boundary.reset();}
  else{
    Motion want{};
    if(orangePixels && hypotf(cameraX-cfg.centerX,cameraY-cfg.centerY)>=8){
      float a=cfg.hand*(atan2f(cameraY-cfg.centerY,cameraX-cfg.centerX)-cfg.front);
      // Body +X is front and +Y is left. A fitted role strap selects the slower robot.
      float speed=digitalRead(39)?.24f:.14f;
      want.x=speed*cosf(a);want.y=speed*sinf(a);want.capture=!digitalRead(40);
    }
    Motion target=boundary.update(want,ring.frame.hits,now);
    wheels(target);
    if(trip)gate.service(now,false);
  }
  terminal();
}
