// Rev B integration bench. The firmware never applies motor PWM.
#include <Arduino.h>
#include <Encoder.h>
#include <Wire.h>
#include "DriveSupervisor.h"

Encoder wheel1(22, 23), wheel2(24, 25), wheel3(26, 27), wheel4(28, 29), roller(30, 31);
Encoder *const encoders[] = {&wheel1, &wheel2, &wheel3, &wheel4, &roller};
const int currentPins[] = {15, 16, 17, 20, 21};
DriveSupervisor gate;
uint8_t frame[9], used = 0, lastSequence = 0;
uint16_t cameraX, cameraY, orangePixels;
uint32_t lastFrame, lastReport, testStart;
bool haveFrame = false, testing = false, gateActive = false;

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

void probeImu(uint8_t address) {
  Wire.beginTransmission(address);
  Serial.printf(" %02x=%s", address, Wire.endTransmission() == 0 ? "present" : "absent");
}

void setup() {
  gate.begin();
  analogReadResolution(12);
  Serial.begin(115200);
  Serial1.begin(115200);
  Wire.begin();
}

void loop() {
  uint32_t now = millis();
  int command = Serial.read();
  if (command == 't' && digitalRead(32) == LOW) { testing = true; testStart = now; }
  if (command == 's' || now - testStart >= 3000) testing = false;
  gateActive = gate.service(now, testing);
  readCamera();

  if (command == 'i' && !testing) {
    Serial.print("IMU I2C");
    for (uint8_t address : {0x1c, 0x1e, 0x6a, 0x6b}) probeImu(address);
    Serial.println();
  }
  if (now - lastReport < 100 || Serial.availableForWrite() < 180) return;
  lastReport = now;
  Serial.printf("play=%d fault=%d gate_test=%d battery=%.2fV enc=",
    digitalRead(32), !digitalRead(41), gateActive,
    analogRead(38) * (3.3f / 4095) * (127.0f / 27.0f));
  for (auto encoder : encoders) Serial.printf(" %ld", (long)encoder->read());
  Serial.print(" current_adc=");
  for (int pin : currentPins) Serial.printf(" %d", analogRead(pin));
  if (!haveFrame || now - lastFrame > 250) Serial.println(" camera=stale");
  else if (orangePixels == 0) Serial.println(" camera=no_candidate");
  else Serial.printf(" orange_xy=%u,%u pixels=%u\n", cameraX, cameraY, orangePixels);
}
