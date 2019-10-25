#include <Arduino.h>
#include <SimpleLEDStripControl.h>


SimpleLEDStripControl *control;

void setup() {
  Serial.begin(115200);
  control = new SimpleLEDStripControl();
  control->setup();
  
}

void loop() {
  control->loop();
}