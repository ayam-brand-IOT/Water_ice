#include "main.h"

Scheduler runner;
Controller controller;
TaskHandle_t detached_task;

// Stages
Stage stage_1(2, initStage1, destroyStage1);
Stage stage_2(2, initStage2, destroyStage2);
Stage stage_3(2, initStage3, destroyStage3);

Task buttons_routine(20, TASK_FOREVER, &onButtonPressed);
Task stop_ice_routine(100, TASK_ONCE,[]() {
  controller.writeDigitalOutput(ICE_PUMP, LOW);
  Serial.println("Ice pump turned off");
});
Task stop_water_routine(100, TASK_ONCE, []() {
  controller.writeDigitalOutput(WATER_PUMP, LOW);
  Serial.println("Water pump turned off");
});

Task broadcast_weight_routine(200, TASK_FOREVER, []() {
  static float last_weight = 0;
  static uint32_t last_broadcast = 0;
  const float current_weight = controller.getWeight();
  const uint32_t now = millis();

  if (current_weight != last_weight || (now - last_broadcast) >= 1000) {
    last_weight = current_weight;
    last_broadcast = now;
    controller.broadcastWeight(current_weight);
  }
});

button_action stop_btn          = {STOP, STOP_IO, onStop};
button_action start_btn         = {START, START_IO, onStart};
button_action manual_ice_btn    = {MANUAL_ICE, MANUAL_ICE_IO, onManualIce};
button_action manual_water_btn  = {MANUAL_WATER, MANUAL_WATER_IO, onManualWater};

Stage stages[] = {stage_1, stage_2, stage_3};
button_action buttons[] = { stop_btn, start_btn, manual_ice_btn, manual_water_btn };

uint32_t iceTimer = 0UL;
uint32_t waterTimer = 0UL;
tote_data tote = {0, 0, 0, 0, 0};

void setup() {
  for (auto &b : buttons) b.button.begin(); 

  controller.init();
  controller.setUpWiFi(U_SSID, U_PASS, "tote-inbound");
  controller.connectToWiFi(/* web_server */ true, /* web_serial */ true, /* OTA */ true);
  controller.wifi.addToteIDcallback(&setToteID);

  xTaskCreatePinnedToCore(communicationTask, "communicationTask", 12000, NULL, 1, &detached_task, 0);

  runner.init();
  runner.addTask(buttons_routine);
  runner.addTask(stop_ice_routine);
  runner.addTask(stop_water_routine);
  runner.addTask(broadcast_weight_routine);
  buttons_routine.enable();
  broadcast_weight_routine.enable();

  delay(1000);
  Serial.println("Starting...");
}

void loop() {
  delay(20);
  const ControllerState current_state = controller.getState();

  runner.execute();

  switch (current_state) {
    case IDLE:
      // FUCK off

      break;  
    case WATER_FILLING:
      onWaterFilling();
      break;
    case ICE_FILLING:
      onIceFilling();
      break;
    case TOTE_READY:
      onToteReady();
      break;
    default:
      break;
  }
}

void communicationTask(void* pvParameters) {
  for (;;) {
    controller.WiFiLoop();
    
    if(controller.isWiFiConnected()) {
      controller.loopOTA();
    }
    // printStackUsage(); // Monitorea el uso de la pila
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void onWaterFilling() {
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

void onIceFilling() {
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

void onToteReady() {
  // Init stage 3
  if (stage_3.getCurrentStep() == 0) {
    stage_3.init();


    if (tote.id[0] == '\0') {
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

void onButtonPressed() {
  handleInputs();
  readButtonTypeFromSerial(); // Read button type from serial input
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
  delay(200);
  controller.writeDigitalOutput(ICE_PUMP, LOW);
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

  controller.writeDigitalOutput(ICE_STOP, HIGH);
  delay(200);
  controller.writeDigitalOutput(ICE_STOP, LOW);

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

button_type handleInputs(button_type override){
  // iterate buttons
  for (auto &btn : buttons) {
    if (btn.button.released() || override == btn.type) {
      btn.handler();
      return btn.type;
    }
  }
  return NONE;
}

void onStart() {
  if (controller.getState() != IDLE) {
    Serial.println("Already started");
    return;
  }

  const uint16_t current_weight = true ? controller.getWeight() : MIN_WEIGHT - 1;

  if(current_weight < MIN_WEIGHT) {
    Serial.println("Weight is negative");
    return;
  }

  Serial.println("START");
  controller.setTare();

  controller.setState(WATER_FILLING);
}

void onStop() {
  Serial.println("STOP");
  controller.setState(IDLE);

  controller.writeDigitalOutput(ICE_PUMP, LOW);
  controller.writeDigitalOutput(WATER_PUMP, LOW);

  stop_ice_routine.cancel();
  stop_water_routine.cancel();
}

void onManualIce() {
  Serial.println("Manual Ice");
  controller.writeDigitalOutput(ICE_PUMP, HIGH);
  stop_ice_routine.restartDelayed(5000);
}

void onManualWater() {
  Serial.println("Manual Water");
  controller.writeDigitalOutput(WATER_PUMP, HIGH);
  stop_water_routine.restartDelayed(5000);
  
}

void readButtonTypeFromSerial() {
  if (Serial.available()) {
    const uint8_t buttonType = Serial.parseInt();

    if (buttonType >= 0 && buttonType < BTN_COUNT) { // Valid button types are 0 to 5
      Serial.print("Button type received: ");
      Serial.println(buttonType);
      handleInputs(static_cast<button_type>(buttonType));
    }
    else {
      Serial.println("Invalid button type. Please enter a number between 0 and 5.");
    }
  }
}


void setToteID(const String& id) {
  if (id.length() >= ID_SIZE) {
    Serial.println("Tote ID is too long");
    return;
  }

  strncpy(tote.id, id.c_str(), ID_SIZE);
  tote.id[ID_SIZE - 1] = '\0'; // Ensure null termination
  Serial.print("Tote ID set to: ");
  Serial.println(tote.id);
}
