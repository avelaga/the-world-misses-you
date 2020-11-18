
#ifndef Raindrop_h
#define Raindrop_h

#include "Arduino.h"

class Raindrop
{
  public:
    Raindrop();
    int updatePos();
  private:
    int pos;
};

#endif
