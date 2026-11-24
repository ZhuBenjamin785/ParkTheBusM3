#pragma once
#include "Arduino.h"
struct Memory {uint8_t store[2048]={};template<class T>void get(int a,T& v){memcpy(&v,store+a,sizeof v);}template<class T>void put(int a,const T& v){memcpy(store+a,&v,sizeof v);}};
extern Memory EEPROM;
