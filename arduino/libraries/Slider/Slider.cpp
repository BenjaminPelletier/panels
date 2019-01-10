#include "Slider.h"

Slider::Slider(int sliderPin, int minChangeForUpdate) {
	this->sliderPin = sliderPin;
  this->minChangeForUpdate = minChangeForUpdate;
}

void Slider::Init() {
  pinMode(sliderPin, INPUT);
	digitalWrite(sliderPin, LOW);
	currentValue = analogRead(sliderPin);
}

void Slider::UpdateState() {
	int newValue = analogRead(sliderPin);
	bool newUpdate = (newValue > lastUpdateValue && newValue - lastUpdateValue >= minChangeForUpdate) ||
									 (newValue < lastUpdateValue && lastUpdateValue - newValue >= minChangeForUpdate);
	currentValue = newValue;
	if (newUpdate) {
		lastUpdateValue = newValue;
		updateAvailable = true;
	}
}

bool Slider::GetSliderUpdate(int *newValue) {
	if (updateAvailable)
		*newValue = currentValue;
	bool result = updateAvailable;
	updateAvailable = false;
	return result;
}

int Slider::GetValue() {
	return currentValue;
}

void Slider::ForceUpdate() {
	updateAvailable = true;
}