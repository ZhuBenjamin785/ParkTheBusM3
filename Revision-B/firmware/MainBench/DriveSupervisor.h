// Motion-loop interlock for the Revision B main PCB. Call from the foreground loop.
// Publish fresh motion targets only after checking encoders, currents and line data.
// service() uses the millisecond timestamp of the newest validated command.
#pragma once
#include <Arduino.h>
#include <initializer_list>
class DriveSupervisor {
 public:
  void begin() {
    sawStop = false;
    running = false;
    for (int pin : {arm, heartbeat}) {
      digitalWrite(pin, LOW);
      pinMode(pin, OUTPUT);
    }
    pinMode(play, INPUT);
    pinMode(fault, INPUT);
    for (int pin = 2; pin <= 6; ++pin) {
      analogWrite(pin, 0);
      pinMode(pin, OUTPUT);
    }
  }
  bool service(uint32_t commandTime, bool sensorsHealthy) {
    uint32_t now = millis();
    if (!digitalRead(play)) {
      sawStop = true;
      stop();
      return false;
    }
    bool missedDeadline = running && now - lastService > 5;
    if (!digitalRead(fault) || !sensorsHealthy || now - commandTime > 5 || missedDeadline) {
      sawStop = false; // Require an observed STOP and fresh PLAY following a fault.
      stop();
      return false;
    }
    if (!sawStop) { stop(); return false; }
    digitalWrite(arm, HIGH);
    if (!running || now-lastBeat >= 2) {
      digitalWrite(heartbeat, HIGH);
      delayMicroseconds(2);
      digitalWrite(heartbeat, LOW);
      lastBeat = now;
    }
    lastService = now;
    running = true;
    return true;
  }
 private:
  static constexpr int play=32, fault=41, arm=33, heartbeat=12;
  bool sawStop=false, running=false;
  uint32_t lastBeat=0, lastService=0;
  void stop() {
    digitalWrite(arm, LOW);
    digitalWrite(heartbeat, LOW);
    for (int pin = 2; pin <= 6; ++pin) analogWrite(pin, 0);
    running=false;
  }
};
