#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <deque>
#include <vector>
#include <cstdio>
#include <initializer_list>
using std::min;
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
extern uint32_t us;extern int pins[64],pwms[64],adc[64];
inline uint32_t millis(){return us/1000;}inline uint32_t micros(){return us;}
inline void delayMicroseconds(uint32_t t){us+=t;}
inline void pinMode(int,int){}inline void digitalWrite(int p,int v){pins[p]=v;}
inline int digitalRead(int p){return pins[p];}inline int analogRead(int p){return adc[p];}
inline void analogReadResolution(int){}inline void analogWriteResolution(int){}
inline void analogWrite(int p,int v){pwms[p]=v;}
template<class T> T constrain(T a,T b,T c){return std::clamp(a,b,c);}
struct Port{std::deque<unsigned char> in;std::vector<unsigned char> out;
 void begin(int){}int available(){return in.size();}int read(){if(in.empty())return -1;int c=in.front();in.pop_front();return c;}
 void write(unsigned char c){out.push_back(c);}void write(const unsigned char *p,size_t n){out.insert(out.end(),p,p+n);}
 void print(const char*){}void println(const char* = nullptr){}template<class...T>void printf(const char*,T...){}};
extern Port Serial,Serial1,Serial4,Serial5,Serial7,Serial8;
