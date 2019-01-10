#include "SwitchArray.h"

SwitchArray::SwitchArray(byte nSwitches, const int *switchPins, const int *switchDown) {
	this->nSwitches = nSwitches;
  this->switchPins = switchPins;
	this->switchDown = switchDown;
	switchUpdate = (char*)malloc(sizeof(char) * nSwitches);
  lastSwitchState = (bool*)malloc(sizeof(bool) * nSwitches);
  lastSwitchTransition = (long*)malloc(sizeof(long) * nSwitches);
}

inline bool SwitchArray::GetSwitchState(byte index) {
	return digitalRead(switchPins[index]) == switchDown[index];
}

void SwitchArray::Init() {
	long t = millis();
	
  for (byte i=0; i<nSwitches; i++) {
		pinMode(switchPins[i], INPUT);
		if (switchDown[i] == LOW)
			digitalWrite(switchPins[i], HIGH);
		lastSwitchState[i] = GetSwitchState(i);
		lastSwitchTransition[i] = t;
  }
}

void SwitchArray::UpdateState() {
  long t = millis();
  for (byte i=0; i<nSwitches; i++) {
		bool s = GetSwitchState(i);
		if (s != lastSwitchState[i] && t - lastSwitchTransition[i] > DEBOUNCE) {
			switchUpdate[i] = s ? BUTTON_DOWN : BUTTON_UP;
			lastSwitchState[i] = s;
			lastSwitchTransition[i] = t;
		}
  }
}

char SwitchArray::GetSwitchUpdate(byte index) {
	char result = switchUpdate[index];
	switchUpdate[index] = BUTTON_NO_CHANGE;
	return result;
}

void SwitchArray::ForceUpdate(byte index) {
	switchUpdate[index] = GetSwitchState(index) ? BUTTON_DOWN : BUTTON_UP;
}