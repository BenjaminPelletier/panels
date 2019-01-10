/*
  Library for managing a set of switches or buttons.
	In addition to a simple pass-through digitalRead method (SwitchState),
	this library also monitors changes in switch states (during UpdateState)
	so that it's easy to take action only when a switch state changes
	(via GetSwitchUpdate).
*/

#ifndef SwitchArray_h
#define SwitchArray_h

#include "Arduino.h"

class SwitchArray {
  public:
  static const char BUTTON_NO_CHANGE = 'N';
  static const char BUTTON_DOWN = 'D';
  static const char BUTTON_UP = 'U';
  
  static const long DEBOUNCE = 10;
	
	byte nSwitches;
  
  void Init();
  SwitchArray(byte nSwitches, const int *switchPins, const int *switchDown);
  void UpdateState();
	char GetSwitchUpdate(byte index);
	bool GetSwitchState(byte index);
	void ForceUpdate(byte index);

  private:
  const int *switchPins;
	const int *switchDown;
  
  char *switchUpdate;
  bool *lastSwitchState;
  long *lastSwitchTransition;
};

#endif