// Teensy 4.1 ring commissioning. All five motor drivers remain disabled.
// Serial commands: g = record green, w = record white, r = print one raw frame.
#include <Arduino.h>
#include <initializer_list>
#include <math.h>
constexpr int count = 32;
constexpr int addressPins[] = {34, 35, 36, 37};
constexpr int adcPins[] = {14, 40};
constexpr int led = 39;
int green[count], white[count], frame[count];
bool haveGreen = false, haveWhite = false;

void phase(bool on, int values[]) {
  digitalWrite(led, on);
  delayMicroseconds(100);  // Start conservatively; measure response on carpet.
  for (int ch = 0; ch < 16; ++ch) {
    for (int bit = 0; bit < 4; ++bit) digitalWrite(addressPins[bit], (ch >> bit) & 1);
    delayMicroseconds(12);
    for (int bank = 0; bank < 2; ++bank) {
      analogRead(adcPins[bank]); // Discard first sample after ADC input change.
      values[2*ch + bank] = analogRead(adcPins[bank]);
    }
  }
}

void scan(int values[]) {
  int ambient[count];
  phase(false, ambient);
  phase(true, values);
  digitalWrite(led, LOW);
  for (int i = 0; i < count; ++i) values[i] = ambient[i] - values[i];
}

void calibrate(int destination[]) {
  long total[count] = {};
  for (int pass = 0; pass < 32; ++pass) {
    scan(frame);
    for (int i = 0; i < count; ++i) total[i] += frame[i];
  }
  for (int i = 0; i < count; ++i) destination[i] = total[i] / 32;
}

void setup() {
  for (int pin : {12, 33, led}) { digitalWrite(pin, LOW); pinMode(pin, OUTPUT); }
  for (int pin : addressPins) pinMode(pin, OUTPUT);
  analogReadResolution(12);
  Serial.begin(115200);
}

void loop() {
  int command = Serial.read();
  if (command == 'g') { calibrate(green); haveGreen = true; }
  if (command == 'w') { calibrate(white); haveWhite = true; }
  unsigned long start = micros();
  scan(frame);
  unsigned long duration = micros() - start;
  if (command == 'r') {
    Serial.print("off_minus_on");
    for (int value : frame) { Serial.print(','); Serial.print(value); }
    Serial.println();
  }
  if (!haveGreen || !haveWhite) return;
  uint32_t hits=0, invalid=0;
  int strength[count] = {};
  for (int i=0; i<count; ++i) {
    int span=white[i]-green[i];
    if (span<64) invalid |= uint32_t(1)<<i; // Flag weak or inverted calibration.
    else {
      int scaled = (frame[i] - green[i]) * 1024 / span;
      strength[i] = scaled < 0 ? 0 : (scaled > 1024 ? 1024 : scaled);
      if (strength[i] > 512) hits |= uint32_t(1)<<i;
    }
  }
  static unsigned long last=0;
  if (millis()-last < 100) return;
  last=millis();
  Serial.print("us=");Serial.print(duration);
  Serial.print(" white_mask=");Serial.print(hits, HEX);
  Serial.print(" weak_mask=");Serial.print(invalid, HEX);
  // Keep distinct contact arcs when the ring straddles a stripe.
  if (hits==0xffffffffu) Serial.print(" all_white");
  for (int i=0; i<count; ++i) {
    auto active=[hits](int ch){return (hits>>(ch%32))&1u;};
    if (!active(i) || active(i+31)) continue;
    int length=1;
    while(length<count && active(i+length)) ++length;
    // Interpolate both half-contrast crossings to estimate the middle of this arc.
    int previous = (i + count - 1) % count;
    int lastHit = (i + length - 1) % count;
    int next = (lastHit + 1) % count;
    float left = i - 0.5f, right = i + length - 0.5f;
    if (!(invalid & (uint32_t(1) << previous))) {
      left = i - 1 + float(512 - strength[previous]) / (strength[i] - strength[previous]);
    }
    if (!(invalid & (uint32_t(1) << next))) {
      right = i + length - 1 + float(strength[lastHit] - 512) / (strength[lastHit] - strength[next]);
    }
    float angle = fmodf(365.625f + 11.25f * (left + right) * 0.5f, 360.f);
    Serial.print(" arc_deg=");Serial.print(angle,1);
  }
  Serial.println();
}
