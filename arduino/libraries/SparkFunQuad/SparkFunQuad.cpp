#include "SparkFunQuad.h"

SparkFunQuad::SparkFunQuad(const int *colorPins, const int *ledPins, const int *switchPins) {
  this->colorPins = colorPins;
  this->ledPins = ledPins;
  this->switchPins = switchPins;
}

inline bool SparkFunQuad::GetSwitchState(byte buttonIndex) {
	return digitalRead(switchPins[buttonIndex]) == LOW;
}

void SparkFunQuad::Init() {
	long t = millis();
	
  for (byte i=0; i<SparkFunQuad::NBUTTONS; i++) {
		pinMode(switchPins[i], INPUT);
		digitalWrite(switchPins[i], HIGH);
		pinMode(ledPins[i], OUTPUT);
		lastSwitchState[i] = GetSwitchState(i);
		lastSwitchTransition[i] = t;
		switchUpdate[i] = BUTTON_NO_CHANGE;
  }
  
  for (byte i=0; i<SparkFunQuad::NCOLORS; i++)
    pinMode(colorPins[i], OUTPUT);
}

void SparkFunQuad::SetLedColor(byte button, byte color) {
  ledColors[button] = color;
}

void SparkFunQuad::UpdateState() {
  // Cycle to next color and enable appropriate common LED lines for that color
  for (byte i=0; i<SparkFunQuad::NBUTTONS; i++)
    digitalWrite(ledPins[i], LOW);
    
  for (byte i=0; i<SparkFunQuad::NCOLORS; i++)
    digitalWrite(colorPins[i], (currentColor == i) ? LOW : HIGH);

  for (byte i=0; i<SparkFunQuad::NBUTTONS; i++)
    digitalWrite(ledPins[i], ((ledColors[i] >> currentColor) & 1) ? HIGH : LOW);

  currentColor = (currentColor + 1) % SparkFunQuad::NCOLORS;

  // Look for switch state changes
  long t = millis();
  for (byte i=0; i<SparkFunQuad::NBUTTONS; i++) {
		bool s = GetSwitchState(i);
		if (s != lastSwitchState[i] && t - lastSwitchTransition[i] > DEBOUNCE) {
			switchUpdate[i] = s ? BUTTON_DOWN : BUTTON_UP;
			lastSwitchState[i] = s;
			lastSwitchTransition[i] = t;
		}
  }
}

char SparkFunQuad::GetSwitchUpdate(byte button) {
	char result = switchUpdate[button];
	switchUpdate[button] = BUTTON_NO_CHANGE;
	return result;
}

void SparkFunQuad::ForceUpdate(byte index) {
  switchUpdate[index] = GetSwitchState(index) ? BUTTON_DOWN : BUTTON_UP;
}