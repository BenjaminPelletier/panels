/*
  Library for managing a set of lights that may be either active-low or active-high
*/

#ifndef LightArray_h
#define LightArray_h

#include "Arduino.h"

class LightArray {
  public:
	byte nLights;
  
  void Init();
  LightArray(byte nLights, const int *lightPins, const int *lightLit);
	void SetLight(int index, bool lit);

  private:
  const int *lightPins;
	const int *lightLit;
};

#endif