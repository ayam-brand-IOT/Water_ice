#include "Controller.h"

Controller controller;

Button start_btn(PRESENCE_SENSOR);

void handleIDLE();
void handleIceFilling();
void handleWaterFilling();

void setup() {
  controller.init();
  start_btn.begin();
}

void loop() {
  delay(100);
  const ControllerState current_state = controller.getState();

  switch (current_state) {
    case IDLE:
      handleIDLE();
      break;
    case ICE_FILLING:
      handleIceFilling();
      break;
    case WATER_FILLING:
      handleWaterFilling();
      break;
    default:
      break;
  }
  
}

void handleIDLE() {
  if(!start_btn.released()) return;

  Serial.println("IDLE");

  controller.setState(ICE_FILLING);
  controller.writeDigitalOutput(ICE_PUMP, HIGH);
}

void handleIceFilling() {
  if (!controller.readDigitalInput(ICE_READY)) return;

  Serial.println("ICE_READY");

  controller.writeDigitalOutput(ICE_PUMP, LOW);
  controller.setState(WATER_FILLING);
  controller.writeDigitalOutput(WATER_PUMP, HIGH);
    // SAVE ICE WEIGHT
  
}

void handleWaterFilling() {
  if (!controller.readDigitalInput(WATER_READY)) return;

  Serial.println("WATER_READY");

  controller.writeDigitalOutput(WATER_PUMP, LOW);
  controller.setState(IDLE);
  // SAVE ICE AND WATER WEIGHTS
  
}