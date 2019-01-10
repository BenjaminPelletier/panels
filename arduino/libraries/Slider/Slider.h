/*
  Library for managing a sliding potentiometer
*/

#ifndef Slider_h
#define Slider_h

#include "Arduino.h"

class Slider {
  public:
	byte nLights;
  
	Slider(int sliderPin, int minChangeForUpdate);
  void Init();
	void UpdateState();
	bool GetSliderUpdate(int *newValue);
	int GetValue();
	void ForceUpdate();

  private:
	int sliderPin;
	int currentValue;
	int lastUpdateValue;
	int minChangeForUpdate;
	bool updateAvailable;
};

#endif