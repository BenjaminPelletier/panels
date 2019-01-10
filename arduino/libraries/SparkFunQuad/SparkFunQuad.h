/*
  Library for the SparkFun quad-button, RGB LEDs part
*/

#ifndef SparkFunQuad_h
#define SparkFunQuad_h

#include "Arduino.h"

class SparkFunQuad {
  public:
  static const byte NCOLORS = 3;
  static const byte NBUTTONS = 4;
  
  static const char BUTTON_NO_CHANGE = 'N';
  static const char BUTTON_DOWN = 'D';
  static const char BUTTON_UP = 'U';
	
	static const byte BLACK = 0;
	static const byte RED = 1;
	static const byte GREEN = 2;
	static const byte BLUE = 4;
	static const byte WHITE = RED | GREEN | BLUE;
  
  static const long DEBOUNCE = 10;
  
  void Init();
	bool GetSwitchState(byte buttonIndex);
  SparkFunQuad(const int *colorPins, const int *ledPins, const int *switchPins);
  void SetLedColor(byte button, byte color);
  void UpdateState();
	char GetSwitchUpdate(byte button);
	void ForceUpdate(byte index);

  private:
  const int *colorPins;
  const int *ledPins;
  const int *switchPins;
  
  byte ledColors[SparkFunQuad::NBUTTONS];
  byte currentColor = 0;
  
  char switchUpdate[SparkFunQuad::NBUTTONS];
  bool lastSwitchState[SparkFunQuad::NBUTTONS];
  long lastSwitchTransition[SparkFunQuad::NBUTTONS];
};

#endif