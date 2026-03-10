#include "main.h"

Scheduler runner;
Controller controller;
TaskHandle_t detached_task;
ToteWebSocketClient wsClient;  // WebSocket client instance
BLEQRClient bleQRClient;       // BLE Central – connects to QR-Reader peripheral

// Function prototypes
void startICEPump();
void stopICEPump();

// Stages
Stage stage_1(2, initStage1, destroyStage1);
Stage stage_2(2, initStage2, destroyStage2);
Stage stage_3(2, initStage3, destroyStage3);

Task buttons_routine(20, TASK_FOREVER, &onButtonPressed);
Task ice_start_pulse(200, TASK_ONCE, []() {
  controller.writeDigitalOutput(ICE_PUMP, LOW);
  Serial.println("Ice pump start pulse ended");
});
Task ice_stop_pulse(200, TASK_ONCE, []() {
  controller.writeDigitalOutput(ICE_STOP, LOW);
  Serial.println("Ice pump stop pulse ended");
});
Task auto_stop_ice_routine(100, TASK_ONCE, []() {
  stopICEPump();
  Serial.println("Ice pump turned off");
});
Task stop_water_routine(100, TASK_ONCE, []() {
  controller.writeDigitalOutput(WATER_PUMP, LOW);
  Serial.println("Water pump turned off");
});

Task broadcast_weight_routine(200, TASK_FOREVER, []() {
  static float last_weight = 0;
  static uint32_t last_broadcast = 0;
  const uint32_t now = millis();

  const float current_weight = controller.getWeight();
  if (isnan(current_weight)) {
    Serial.println("Weight reading is NaN, skipping broadcast");
    return;
  }
    const float DELTA = 0.02f;  // 20 g minimum change
    bool weight_changed = isnan(last_weight) || fabs(current_weight - last_weight) >= DELTA;
    bool time_elapsed   = (now - last_broadcast) >= 1000;

  if (weight_changed || time_elapsed) {
    last_weight = current_weight;
    last_broadcast = now;
    // Send weight via WebSocket instead of HTTP broadcast
    wsClient.sendWeight(current_weight);
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

void startICEPump() {
  controller.writeDigitalOutput(ICE_PUMP, HIGH);
  ice_start_pulse.restartDelayed(200);
}

void stopICEPump() {
  controller.writeDigitalOutput(ICE_STOP, HIGH);
  ice_stop_pulse.restartDelayed(200);
}

void setup() {
  // Inicializar controller PRIMERO (configura outputs antes que Serial)
  controller.init();
  
  for (auto &b : buttons) b.button.begin();
  controller.setUpWiFi(U_SSID, U_PASS, "tote-inbound");
  controller.connectToWiFi(/* web_server */ false, /* web_serial */ true, /* OTA */ true);
  
  // Initialize WebSocket client
  wsClient.begin(BACKEND_HOST, BACKEND_WS_PORT, "/esp32");
  wsClient.setMessageCallback(onWebSocketMessage);

  // Initialize BLE QR client – scans for "QR-Reader" peripheral
  bleQRClient.begin([](const String& qr) {
    if (qr == "NO_QR") {
      Serial.println("[BLE-QR] No QR in reader buffer yet");
      return;
    }
    Serial.printf("[BLE-QR] QR received via BLE: %s\n", qr.c_str());
    setToteIdFromUI(qr);
  });

  xTaskCreatePinnedToCore(communicationTask, "communicationTask", 12000, NULL, 1, &detached_task, 0);

  runner.init();
  runner.addTask(buttons_routine);
  runner.addTask(ice_start_pulse);
  runner.addTask(ice_stop_pulse);
  runner.addTask(auto_stop_ice_routine);
  runner.addTask(stop_water_routine);
  runner.addTask(broadcast_weight_routine);
  buttons_routine.enable();
  broadcast_weight_routine.enable();

  delay(1000);
  Serial.println("Starting...");
}

void loop() {
  delay(20);
  controller.task();  // Process Modbus communication
  wsClient.loop();     // Process WebSocket communication
  bleQRClient.loop();  // Drive BLE scan / connect state machine
  runner.execute();
  handleToteState();
}

void handleToteState(){
    switch (toteState) {
    case ToteState::IDLE:
      // Wait for start command (button)
      onIDLE();
      break;
    
    case ToteState::DISPENSING_ICE:
      onIceFilling();
      break;

    case ToteState::DISPENSING_WATER:
      onWaterFilling();
      break;

    case ToteState::WAITING_TOTE_ID:
      onWaitingToteID();
      break;

    case ToteState::COMPLETED:
      onToteReady();
      break;

    case ToteState::CANCELED:
      onCanceled();
      break;

    case ToteState::ERROR:
      // TODO: show error, wait for intervention
      break;
  }
}


void communicationTask(void* pvParameters) {
  for (;;) {
    controller.WiFiLoop();
    
    if(controller.isWiFiConnected()) {
      controller.loopOTA();
    }
    // printStackUsage(); // Monitor stack usage
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void onWaterFilling() {
  // Init stage 2 (was stage 1 before)
  if (stage_2.getCurrentStep() == 0) {
    stage_2.init();
    stage_2.nextStep();
    wsClient.sendStateChange("DISPENSING_WATER");
  }

  else if (stage_2.getCurrentStep() == 1){
    const float current_weight = controller.getWeight();
    const float weight_delta = current_weight - tote.initial_weight;  // Calculate delta
    const float target_total = TARGET_ICE_KG + TARGET_WATER_KG;
    
    if (weight_delta < target_total) {
      // Target weight not reached yet
      static uint32_t lastPrint = 0;
      if (millis() - lastPrint > 500) {
        Serial.printf("Water: %.2f / %.2f kg (Total: %.2f)\r", weight_delta - TARGET_ICE_KG, TARGET_WATER_KG, weight_delta);
        lastPrint = millis();
      }
      return;
    }
    Serial.println();
    Serial.printf("✓ Target water weight reached: %.2f kg (Total: %.2f kg)\n", weight_delta - TARGET_ICE_KG, weight_delta);
    stage_2.nextStep();
  }

  if (stage_2.getCurrentStep() == 2) {
    stage_2.destroy();
    // After water, wait for tote ID
    toteState = ToteState::WAITING_TOTE_ID;
    wsClient.sendStateChange("WAITING_TOTE_ID");
    Serial.println("Transitioning to WAITING_TOTE_ID");
  }
}

void onIceFilling() {
  // Init stage 1 (ice is first now)
  if (stage_1.getCurrentStep() == 0) {
    stage_1.init();
    stage_1.nextStep();
    wsClient.sendStateChange("DISPENSING_ICE");
  }

  else if (stage_1.getCurrentStep() == 1){
    const float current_weight = controller.getWeight();
    const float weight_delta = current_weight - tote.initial_weight;  // Calculate delta
    
    if (weight_delta < TARGET_ICE_KG) {
      // Target weight not reached yet
      static uint32_t lastPrint = 0;
      if (millis() - lastPrint > 500) {
        Serial.printf("Ice: %.2f / %.2f kg\r", weight_delta, TARGET_ICE_KG);
        lastPrint = millis();
      }
      return;
    }
    Serial.println();
    Serial.printf("✓ Target ice weight reached: %.2f kg\n", weight_delta);
    stage_1.nextStep();
  }

  if (stage_1.getCurrentStep() == 2) {
    stage_1.destroy();
    // After ice, fill water
    toteState = ToteState::DISPENSING_WATER;
    Serial.println("Transitioning to DISPENSING_WATER");
  }
}

void onToteReady() {
  // Init stage 3
  if (stage_3.getCurrentStep() == 0) {
    stage_3.init();
    stage_3.nextStep();
    wsClient.sendToteCompleted(tote.id);
  }

  if (stage_3.getCurrentStep() == 1) {
    delay(1000);
    Serial.println("Tote completed and sent!");
    stage_3.nextStep();
  }

  if (stage_3.getCurrentStep() == 2) {
    stage_3.destroy();
    // Return to IDLE to wait for next tote
    toteState = ToteState::IDLE;
    wsClient.sendStateChange("IDLE");
    Serial.println("\n=== Ready for next tote ===");
  }
}

void onButtonPressed() {
  handleInputs();
  readButtonTypeFromSerial(); // Read button type from serial input
}

void initStage1() {
  Serial.println("\n=== Stage 1: Dispensing Ice ===");
  // Save initial weight to calculate delta (workaround if TARE doesn't work)
  tote.initial_weight = controller.getWeight();
  Serial.printf("Initial weight saved: %.2f kg\n", tote.initial_weight);
  controller.setTare();  // Try TARE anyway
  startICEPump();
}

void initStage2() {
  Serial.println("\n=== Stage 2: Filling Water ===");
  // Do NOT setTare here - we want to measure cumulative weight (ice + water)
  controller.writeDigitalOutput(WATER_PUMP, HIGH);
}

void initStage3() {
  Serial.println("Stage 3 Tote Ready");
 
}

void destroyStage1() {
  const uint32_t ice_kg = controller.getWeight();
  Serial.print("Ice dispensed: ");
  Serial.print(ice_kg);
  Serial.println(" kg");

  tote.ice_kg = ice_kg - tote.initial_weight;  // Solo el hielo (TARE ya eliminó el peso del tote)

  stopICEPump();

  // Send ice dispensed value to frontend
  wsClient.sendIceDispensed(tote.ice_kg);

  Serial.println("Ice dispensing completed");
  Serial.println("Stage 1 destroyed");
}

void destroyStage2() {
  const uint32_t water_out_kg = controller.getWeight();

  Serial.print("Water filled: ");
  Serial.print(water_out_kg);
  Serial.println(" kg");

  tote.water_kg = water_out_kg - tote.initial_weight - tote.ice_kg;  // Solo restar el hielo

  controller.writeDigitalOutput(WATER_PUMP, LOW);

  // Send water dispensed value to frontend
  wsClient.sendWaterDispensed(tote.water_kg);

  Serial.println("Water filling completed");
  Serial.println("Stage 2 destroyed");
} 

void destroyStage3() {
  // Show all completed tote data
  Serial.println("\n=== Tote Summary ===");
  Serial.print("ID: ");
  Serial.println(tote.id);
  Serial.print("Tote: ");
  Serial.print(tote.tote_kg);
  Serial.println(" kg");
  Serial.print("Ice: ");
  Serial.print(tote.ice_kg);
  Serial.println(" kg");
  Serial.print("Water: ");
  Serial.print(tote.water_kg);
  Serial.println(" kg");
  Serial.print("Raw: ");
  Serial.print(controller.getWeight());
  Serial.println(" kg");
  Serial.println("==================\n");
  
  // Calculate raw_kg and water_kg
  uint32_t raw_kg = controller.getWeight();
  
  // Send data to backend with POST
  bool success = createToteInBackend(
    tote.id,
    tote.tote_kg,
    tote.water_kg,
    tote.ice_kg,
    raw_kg
  );
  
  if (success) {
    Serial.println("✓ Tote created in backend successfully!");
  } else {
    Serial.println("✗ Failed to create tote in backend");
    Serial.println("  Please check backend connection or try again.");
  }
  
  // Reset the tare
  controller.setTare();
  
  // Clear data for next tote
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
  if (toteState != ToteState::IDLE) {
    Serial.println("Already in process");
    return;
  }

  const uint16_t current_weight = controller.getWeight();

  if(current_weight < MIN_WEIGHT) {
    Serial.println("Weight is too low to start");
    return;
  }

  Serial.println("\n=== System Started ===");
  Serial.print("Initial weight: ");
  Serial.print(current_weight);
  Serial.println(" kg");
  
  tote.tote_kg = current_weight;
  controller.setTare();

  // Start with ice dispensing
  toteState = ToteState::DISPENSING_ICE;
  Serial.println("Transitioning to DISPENSING_ICE");
}

void onStop() {
  Serial.println("\n=== STOP pressed ===");
  
  // Stop all pumps
  stopICEPump();
  controller.writeDigitalOutput(WATER_PUMP, LOW);
  auto_stop_ice_routine.cancel();
  stop_water_routine.cancel();
  
  // Cancel current process
  toteState = ToteState::CANCELED;
}

void onManualIce() {
  Serial.println("Manual Ice");
  startICEPump();
  auto_stop_ice_routine.restartDelayed(5000);
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

bool setToteIdFromUI(const String& toteId) {
  if (toteState != ToteState::WAITING_TOTE_ID) {
    Serial.println("Cannot set ID, not in WAITING_TOTE_ID state");
    return false;
  }

  // Copy ID to tote struct
  memset(tote.id, 0, sizeof(tote.id));
  toteId.substring(0, sizeof(tote.id)-1).toCharArray(tote.id, sizeof(tote.id));

  Serial.print("Tote ID set to: ");
  Serial.println(tote.id);

  // Send validation via WebSocket
  wsClient.sendToteValidated(tote.id);

  // Transition to COMPLETED
  toteState = ToteState::COMPLETED;
  wsClient.sendStateChange("COMPLETED");
  return true;
}

// ==================== New Functions ====================

void onIDLE() {
  // In IDLE we wait for the START button
  // Nothing to do here, START button calls onStart()
}

void onWaitingToteID() {
  static uint32_t lastPrompt = 0;

  // Show prompt every 3 seconds
  if (millis() - lastPrompt > 3000) {
    const bool bleReady = bleQRClient.isConnected();
    Serial.println("\n╔════════════════════════════════════╗");
    Serial.println("║   WAITING FOR TOTE ID              ║");
    Serial.println("╠════════════════════════════════════╣");
    Serial.println("║ Tote:  " + String(tote.tote_kg) + " kg");
    Serial.println("║ Ice:   " + String(tote.ice_kg) + " kg");
    Serial.println("║ Water: " + String(tote.water_kg) + " kg");
    Serial.println("╠════════════════════════════════════╣");
    if (bleReady) {
      Serial.println("║ [BLE]  QR-Reader conectado ✓       ║");
      Serial.println("║        Leyendo QR automáticamente  ║");
    } else {
      Serial.println("║ [BLE]  QR-Reader no conectado      ║");
    }
    Serial.println("║ [WEB]  Captura con cámara del tel  ║");
    Serial.println("╚════════════════════════════════════╝\n");

    // If BLE reader is connected, request the buffered QR every 3 s
    if (bleReady) {
      bleQRClient.requestQR();
    }

    lastPrompt = millis();
  }

  // Transition to COMPLETED is handled by setToteIdFromUI() (BLE or web path)
}

void onCanceled() {
  Serial.println("Tote canceled, cleaning up...");
  
  // Stop pumps
  stopICEPump();
  controller.writeDigitalOutput(WATER_PUMP, LOW);
  
  // Clear data
  tote = {0, 0, 0, 0, 0};
  controller.setTare();
  
  // Reset stages
  stage_1.destroy();
  stage_2.destroy();
  stage_3.destroy();
  
  // Return to IDLE
  toteState = ToteState::IDLE;
  Serial.println("Returned to IDLE");
}

// ==================== Backend API Functions ====================

bool createToteInBackend(const char* toteId, uint32_t tote_kg, uint32_t water_kg, uint32_t ice_kg, uint32_t raw_kg) {
  if (!controller.isWiFiConnected()) {
    Serial.println("WiFi not connected, cannot create tote in backend");
    return false;
  }

  HTTPClient http;
  String url = String(BACKEND_URL) + "/api/totes";
  
  Serial.println("\n=== Creating Tote in Backend ===");
  Serial.print("POST: ");
  Serial.println(url);
  
  // Create JSON payload
  DynamicJsonDocument doc(512);
  doc["tote_id"] = toteId;
  doc["tote_kg"] = tote_kg;
  doc["water_kg"] = water_kg;
  doc["ice_kg"] = ice_kg;
  doc["raw_kg"] = raw_kg;
  
  String jsonPayload;
  serializeJson(doc, jsonPayload);
  
  Serial.print("Payload: ");
  Serial.println(jsonPayload);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000); // 10 seconds timeout
  
  int httpCode = http.POST(jsonPayload);
  
  if (httpCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpCode);
    
    String response = http.getString();
    Serial.print("Response: ");
    Serial.println(response);
    
    if (httpCode == 201) {
      Serial.println("Tote created in backend successfully!");
      http.end();
      return true;
    }
    else if (httpCode == 409) {
      Serial.println("Tote ID already exists in backend (409 Conflict)");
      http.end();
      return false;
    }
  }
  else {
    Serial.print("HTTP POST failed, error: ");
    Serial.println(http.errorToString(httpCode).c_str());
  }
  
  http.end();
  return false;
}

// WebSocket message handler
void onWebSocketMessage(String type, JsonDocument& doc) {
  Serial.printf("WebSocket message received: %s\n", type.c_str());
  
  if (type == "qr_scanned") {
    const char* toteId = doc["toteId"];
    if (toteId && strlen(toteId) > 0) {
      Serial.printf("QR scanned from browser: %s\n", toteId);
      
      // Use setToteIdFromUI to properly handle state transition
      if (setToteIdFromUI(String(toteId))) {
        Serial.println("Tote ID set successfully from browser QR scan");
      } else {
        Serial.println("Failed to set Tote ID - may not be in WAITING_TOTE_ID state");
        // If not in WAITING_TOTE_ID state, just save the ID for later use
        setToteID(String(toteId));
      }
    }
  }
  else if (type == "command") {
    const char* command = doc["command"];
    Serial.printf("Command received: %s\n", command);
    
    // Handle commands from backend/browser
    if (strcmp(command, "start") == 0) {
      if (toteState == ToteState::IDLE) {
        toteState = ToteState::DISPENSING_ICE;
        wsClient.sendStateChange("DISPENSING_ICE");
      }
    }
    else if (strcmp(command, "stop") == 0) {
      toteState = ToteState::CANCELED;
      wsClient.sendStateChange("CANCELED");
    }
  }
}
