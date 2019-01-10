#include "LightArray.h"

LightArray::LightArray(byte nLights, const int *lightPins, const int *lightLit) {
	this->nLights = nLights;
  this->lightPins = lightPins;
	this->lightLit = lightLit;
}

void LightArray::Init() {
  for (byte i=0; i<nLights; i++) {
		pinMode(lightPins[i], OUTPUT);
		SetLight(i, false);
  }
}

void LightArray::SetLight(int index, bool lit) {
	digitalWrite(lightPins[index], (lit == (lightLit[index] == HIGH)) ? HIGH : LOW);
}