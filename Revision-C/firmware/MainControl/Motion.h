#pragma once
#include <math.h>
#include <stdint.h>
struct Motion { float x=0, y=0; bool capture=false; };
class Boundary {
  float previousX=0, previousY=0, retreatX=0, retreatY=0;
  uint32_t touched=0, clearAt=0;
  bool active=false, locked=false, clearing=false;
 public:
  void reset(){ *this=Boundary(); }
  Motion update(Motion wanted,uint64_t hits,uint32_t now){
    if(hits){
      clearing=false;
      if(!active){
        active=true;touched=now;
        // Use travel history to select the side we entered from, even for two opposed arcs.
        float length=hypotf(previousX,previousY);
        locked=length<0.04f;
        if(!locked){
          float ux=previousX/length,uy=previousY/length,nx=0,ny=0;
          // Channels are ordered counterclockwise from body +X. The sign of the
          // dot product chooses the entering side of a stripe with opposite contacts.
          for(int i=0;i<64;++i){
            if(!(hits & (uint64_t(1)<<i)))continue;
            float angle=(i+.5f)*.0981747704f;
            float x=cosf(angle),y=sinf(angle),weight=x*ux+y*uy;
            if(weight>.1f){nx+=weight*x;ny+=weight*y;}
          }
          float norm=hypotf(nx,ny);
          if(norm>.1f){ux=nx/norm;uy=ny/norm;}
          retreatX=-.13f*ux;retreatY=-.13f*uy;
        }
      }
      if(hits==~uint64_t(0)||now-touched>400)locked=true;
      return locked?Motion{}:Motion{retreatX,retreatY,false};
    }
    if(active){
      if(!clearing){clearing=true;clearAt=now;}
      if(now-clearAt<100)return locked?Motion{}:Motion{retreatX,retreatY,false};
      active=false;locked=false;clearing=false;
    }
    previousX=wanted.x;previousY=wanted.y;return wanted;
  }
};
