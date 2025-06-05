#include "Controller.h"
#include "Stage.h"

Controller controller;

#ifndef false
Button start_btn(START_BTN);
Button stop_btn(STOP_BTN);
#endif

uint32_t waterTimer = 0UL;
uint32_t iceTimer = 0UL;

typedef struct {
  uint16_t id;
  uint32_t water_weight;
  uint32_t ice_weight;
  uint32_t tote_weight;
  uint32_t raw_weight;
} tote_data;

tote_data tote = {0, 0, 0, 0, 0};

void handleIDLE();
void initStage1();
void initStage2();
void initStage3();
void destroyStage1();
void destroyStage2();
void destroyStage3();
void handleIceFilling();
void handleWaterFilling();
void handleToteReady();

// Stages
Stage stage_1(2, initStage1, destroyStage1);
Stage stage_2(2, initStage2, destroyStage2);
Stage stage_3(2, initStage3, destroyStage3);

void setup() {

controller.init();

#ifndef false
  start_btn.begin();
  stop_btn.begin();
#else
  pinMode(START_BTN, INPUT_PULLUP);
  pinMode(STOP_BTN, INPUT_PULLUP);
#endif

  delay(1000);
  Serial.println("Starting...");
}

void loop() {
  delay(100);
  const ControllerState current_state = controller.getState();

  switch (current_state) {
    case IDLE:
      handleIDLE();
      break;  
    case WATER_FILLING:
      handleWaterFilling();
      break;
    case ICE_FILLING:
      handleIceFilling();
      break;
    case TOTE_READY:
      handleToteReady();
      break;
    default:
      break;
  }
  
}


void handleIDLE() {
#ifndef false
  if(!start_btn.released()) return;
#else
  if(controller.readDigitalInput(START_BTN)) return;
#endif


  const uint16_t current_weight = GRATER_THAN_MIN ? controller.getWeight() : MIN_WEIGHT - 1;

  if(current_weight < MIN_WEIGHT) {
    Serial.println("Weight is negative");
    return;
  }

  Serial.println("IDLE");
  controller.setTare();

  controller.setState(WATER_FILLING);
}

void handleWaterFilling() {
  // Init stage 1
  if (stage_1.getCurrentStep() == 0) {
    stage_1.init();
    stage_1.nextStep();
    waterTimer = millis();
  }

  else if (stage_1.getCurrentStep() == 1){
    if (!controller.hasIntervalPassed(waterTimer, 4000, false)) {
      Serial.print(".");
      return;
    }
    Serial.println();

    stage_1.nextStep();
  }


  if (stage_1.getCurrentStep() == 2) {
    stage_1.destroy();
    controller.setState(ICE_FILLING);
  }
}

void handleIceFilling() {
  // Init stage 2
  if (stage_2.getCurrentStep() == 0) {
    stage_2.init();
    stage_2.nextStep();
    iceTimer = millis();
  }

  else if (stage_2.getCurrentStep() == 1){
    if (!controller.hasIntervalPassed(iceTimer, 4000, false)) {
      Serial.print(".");
      return;
    }
    Serial.println();
    stage_2.nextStep();
  }

  if (stage_2.getCurrentStep() == 2) {
    stage_2.destroy();
    controller.setState(TOTE_READY);
  }
}

void handleToteReady() {
  // Init stage 3
  if (stage_3.getCurrentStep() == 0) {
    stage_3.init();

    tote.id = HAS_ID ? 5 : 0;
    if (tote.id == 0) {
      Serial.println("Tote ID not set");
      delay(1000);
    }
    else {
      Serial.println("Waiting for confirmation..."); 
      delay(1000);
      stage_3.nextStep();
    }
  }

  if (stage_3.getCurrentStep() == 1) {
    delay(1000);
    Serial.println("Confirmed!");
    stage_3.nextStep();
  }


  if (stage_3.getCurrentStep() == 2) {
    stage_3.destroy();
    controller.setState(IDLE);
  }
}

void initStage1() {
  Serial.println("Stage 1 Filling Water");
  tote.tote_weight = controller.getWeight();
  controller.setTare();
  controller.writeDigitalOutput(WATER_PUMP, HIGH);
}

void initStage2() {
  Serial.println("Stage 2 Filling Ice");
  controller.setTare();
  controller.writeDigitalOutput(ICE_PUMP, HIGH);
}

void initStage3() {
  Serial.println("Stage 3 Tote Ready");
 
}

void destroyStage1() {

  const uint32_t water_weight = controller.getWeight();
  Serial.print("Water: ");
  Serial.print(water_weight);
  Serial.println(" kg");

  tote.water_weight = water_weight;

  controller.writeDigitalOutput(WATER_PUMP, LOW);

  Serial.println("Water filling completed");
  Serial.println("Stage 1 destroyed");
}

void destroyStage2() {
  const uint32_t ice_weight = controller.getWeight();

  Serial.print("Ice: ");
  Serial.print(ice_weight);
  Serial.println(" kg");

  tote.ice_weight = ice_weight;

  controller.writeDigitalOutput(ICE_PUMP, LOW);
  Serial.println("Ice filling completed");
  Serial.println("Stage 2 destroyed");
} 

void destroyStage3() {
  // show all the data
  Serial.print("Water: ");
  Serial.print(tote.water_weight);
  Serial.println(" kg");
  Serial.print("Ice: ");
  Serial.print(tote.ice_weight);
  Serial.println(" kg");
  Serial.print("Tote: ");
  Serial.print(tote.tote_weight);
  Serial.println(" kg");
  Serial.print("Raw: ");
  Serial.print(controller.getWeight());
  Serial.println(" kg");
  Serial.print("ID: ");
  Serial.print(tote.id);
  Serial.println("");
  //reset the tare
  controller.setTare();
  tote = {0, 0, 0, 0, 0};
  Serial.println("Stage 3 destroyed");

}