
#ifndef Raindrop_h
#define Raindrop_h

#include "Arduino.h"

class Raindrop
{
  public:
    Raindrop();
    int updatePos();
    int getHue();
  private:
    int pos;
    int hue;
};

#endif
