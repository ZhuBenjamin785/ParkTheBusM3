#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace linebus {
constexpr uint8_t sync = 0xD3, version = 3, size = 24;
inline uint16_t crc(const uint8_t *p, size_t n) {
  uint16_t r = 0xffff;
  while (n--) {
    r ^= uint16_t(*p++) << 8;
    for (int i=0; i<8; ++i) r = (r & 0x8000) ? (r << 1) ^ 0x1021 : r << 1;
  }
  return r;
}
inline void put64(uint8_t *p, uint64_t v) {
  for (int i=0; i<8; ++i) p[i] = uint8_t(v >> (8*i));
}
inline uint64_t get64(const uint8_t *p) {
  uint64_t v=0;
  for (int i=0; i<8; ++i) v |= uint64_t(p[i]) << (8*i);
  return v;
}
// Commands travel main -> ring while local RUN is open. Repeated identifiers are idempotent.
inline void command(uint8_t id,uint8_t action,uint8_t *p) {
  p[0]=0xC7;p[1]=version;p[2]=id;p[3]=action;
  uint16_t c=crc(p,4);p[4]=c>>8;p[5]=c;
}
class Commands {
  uint8_t wire[6]{}, used=0;
public:
  uint8_t acknowledged=0;
  int accept(uint8_t byte) {
    if(!used && byte!=0xc7)return -1;
    wire[used++]=byte;
    if(used<6)return -1;
    if(wire[0]!=0xc7 || wire[1]!=version || crc(wire,4)!=(uint16_t(wire[4])<<8|wire[5])){
      memmove(wire,wire+1,5);used=5;return -1;
    }
    used=0;
    if(wire[2]==acknowledged || (wire[3]!='g' && wire[3]!='w' && wire[3]!='s'))return -1;
    acknowledged=wire[2];return wire[3];
  }
};
struct Frame {
  uint64_t hits=0, weak=~uint64_t(0);
  uint8_t sequence=0, status=0, badBanks=0, lastCommand=0;
};
inline void encode(const Frame& f, uint8_t *p) {
  p[0]=sync; p[1]=version; p[2]=f.sequence; p[3]=f.status;
  put64(p+4,f.hits); put64(p+12,f.weak); p[20]=f.badBanks; p[21]=f.lastCommand;
  uint16_t c=crc(p,22); p[22]=c>>8; p[23]=c;
}
class Receiver {
  uint8_t p[size], used=0;
public:
  Frame frame;
  uint32_t received=0;
  bool seen=false;
  bool accept(uint8_t byte, uint32_t now) {
    if (!used && byte!=sync) return false;
    p[used++]=byte;
    if (used<size) return false;
    if (p[0]!=sync || p[1]!=version || crc(p,22)!=(uint16_t(p[22])<<8 | p[23])) {
      memmove(p,p+1,size-1); --used; return false;
    }
    used=0;
    if (seen && p[2]==frame.sequence) return false;
    frame.sequence=p[2]; frame.status=p[3]; frame.hits=get64(p+4);
    frame.weak=get64(p+12); frame.badBanks=p[20];frame.lastCommand=p[21]; received=now;seen=true;return true;
  }
  bool healthy(uint32_t now) const {
    return seen && now-received<100 && (frame.status&3)==3 && !frame.weak && !frame.badBanks;
  }
};
}
