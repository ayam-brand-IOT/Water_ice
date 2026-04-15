#include "main.h"
#include "Settings.h"
#include "Debug.h"

Scheduler runner;
Controller controller;
TaskHandle_t detached_task;
ToteWebSocketClient wsClient;  // WebSocket client instance
BLEQRClient bleQRClient;       // BLE Central – connects to QR-Reader peripheral
extern bool early_init_ran;


// Function prototypes
void startICEPump();
void stopICEPump();
void onSettlingIce();

// Stages
Stage stage_1(2, initStage1, destroyStage1);
Stage stage_2(2, initStage2, destroyStage2);
Stage stage_3(2, initStage3, destroyStage3);

Task buttons_routine(20, TASK_FOREVER, &onButtonPressed);
Task ice_start_pulse(200, TASK_ONCE, []() {
  controller.writeDigitalOutput(ICE_PUMP, LOW);
  LOG_MAIN("Ice pump start pulse ended\n");
});
Task ice_stop_pulse(200, TASK_ONCE, []() {
  controller.writeDigitalOutput(ICE_STOP, LOW);
  LOG_MAIN("Ice pump stop pulse ended\n");
});
Task auto_stop_ice_routine(100, TASK_ONCE, []() {
  stopICEPump();
  LOG_MAIN("Ice pump turned off\n");
});
Task stop_water_routine(100, TASK_ONCE, []() {
  controller.writeDigitalOutput(WATER_PUMP, LOW);
  LOG_MAIN("Water pump turned off\n");
});

Task broadcast_weight_routine(200, TASK_FOREVER, []() {
  static float last_weight = 0;
  static uint32_t last_broadcast = 0;
  const uint32_t now = millis();

  const float current_weight = controller.getWeight();
  if (isnan(current_weight)) {
    LOG_MAIN("Weight reading is NaN, skipping broadcast\n");
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

// ── Indicator tasks ────────────────────────────────────────────────────────────
// Ticks cada 250 ms; las distintas tasas de parpadeo se obtienen con módulo.
//   INDICATOR_1 = estado del sistema   INDICATOR_2 = estado BLE QR reader
Task indicator_task(250, TASK_FOREVER, []() {
  static uint8_t tick = 0;
  tick++;

  // ── INDICATOR_1: Estado del sistema ───────────────────────────────────
  uint8_t ind1 = LOW;
  switch (toteState) {
    case ToteState::IDLE:
      ind1 = LOW;                              // Apagado
      break;
    case ToteState::DISPENSING_ICE:
    case ToteState::DISPENSING_WATER:
      ind1 = (tick % 2) ? HIGH : LOW;          // Parpadeo rápido 500 ms
      break;
    case ToteState::SETTLING_ICE:
      ind1 = (tick % 4 < 2) ? HIGH : LOW;     // Parpadeo medio 1 s (bomba apagada, asentando)
      break;
    case ToteState::WAITING_TOTE_ID:
      ind1 = (tick % 8 < 4) ? HIGH : LOW;     // Parpadeo lento 1 s
      break;
    case ToteState::COMPLETED:
      ind1 = HIGH;                             // Encendido fijo
      break;
    case ToteState::CANCELED:
    case ToteState::ERROR: {                   // Triple flash + 2 s OFF
      uint8_t p = tick % 20;                  // Ciclo 5 s
      ind1 = (p < 6 && p % 2 == 0) ? HIGH : LOW;
      break;
    }
  }
  controller.writeDigitalOutput(INDICATOR_1, ind1);

  // ── INDICATOR_2: Estado BLE QR reader ─────────────────────────────
  uint8_t ind2 = LOW;
  switch (bleQRClient.getState()) {
    case BLEQRState::CONNECTED:
      ind2 = HIGH;                             // Encendido fijo: lector listo
      break;
    case BLEQRState::SCANNING:
    case BLEQRState::CONNECTING:
    case BLEQRState::LOST:
      ind2 = (tick % 8 < 4) ? HIGH : LOW;     // Parpadeo lento: buscando
      break;
    default:                                   // IDLE: apagado
      ind2 = LOW;
      break;
  }
  controller.writeDigitalOutput(INDICATOR_2, ind2);
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
  Settings::load();  // Load persisted ice/water/min-weight targets from NVS
  
  for (auto &b : buttons) b.button.begin();
  controller.setUpWiFi(U_SSID, U_PASS, "tote-inbound");
  controller.connectToWiFi(/* web_server */ true, /* web_serial */ true, /* OTA */ true);
  
  // Initialize WebSocket client
  wsClient.begin(BACKEND_HOST, BACKEND_WS_PORT, "/esp32");
  wsClient.setMessageCallback(onWebSocketMessage);

  // Initialize BLE QR client – scans for "QR-Reader" peripheral
  bleQRClient.begin([](const String& qr) -> bool {
    if (qr == "NO_QR") {
      LOG_BLE("[BLE-QR] No QR in reader buffer yet\n");
      return false;
    }
    LOG_BLE("[BLE-QR] QR received via BLE: %s\n", qr.c_str());
    return setToteIdFromUI(qr);  // true = procesado → BLEQRClient enviará ACK
  });

  controller.setUpIOS();

  xTaskCreatePinnedToCore(communicationTask, "communicationTask", 12000, NULL, 1, &detached_task, 0);

  runner.init();
  runner.addTask(buttons_routine);
  runner.addTask(ice_start_pulse);
  runner.addTask(ice_stop_pulse);
  runner.addTask(auto_stop_ice_routine);
  runner.addTask(stop_water_routine);
  runner.addTask(broadcast_weight_routine);
  runner.addTask(indicator_task);
  buttons_routine.enable();
  broadcast_weight_routine.enable();
  indicator_task.enable();

  controller.setupPinMode(AO_0, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)AO_0, HIGH);

  controller.writeDigitalOutput(ICE_STOP, HIGH);
  delay(500);
  controller.writeDigitalOutput(ICE_STOP, LOW);

  LOG_MAIN("Starting...\n");

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

    case ToteState::SETTLING_ICE:
      onSettlingIce();
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
    const float target_total = tote.ice_kg + Settings::getTargetWaterKg();
    
    if (weight_delta < target_total) {
      // Target weight not reached yet
      static uint32_t lastPrint = 0;
      if (millis() - lastPrint > 500) {
        LOG_MAIN("Water: %.2f / %.2f kg (Total: %.2f)\r", weight_delta - tote.ice_kg, Settings::getTargetWaterKg(), weight_delta);
        lastPrint = millis();
      }
      return;
    }
    LOG_MAIN("\u2713 Target water weight reached: %.2f kg (Total: %.2f kg)\n", weight_delta - tote.ice_kg, weight_delta);
    stage_2.nextStep();
  }

  if (stage_2.getCurrentStep() == 2) {
    stage_2.destroy();
    // After water, wait for tote ID
    toteState = ToteState::WAITING_TOTE_ID;
    wsClient.sendStateChange("WAITING_TOTE_ID");
    LOG_MAIN("Transitioning to WAITING_TOTE_ID\n");
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
    
    if (weight_delta < Settings::getTargetIceKg()) {
      // Target weight not reached yet
      static uint32_t lastPrint = 0;
      if (millis() - lastPrint > 500) {
        LOG_MAIN("Ice: %.2f / %.2f kg\r", weight_delta, Settings::getTargetIceKg());
        lastPrint = millis();
      }
      return;
    }
    LOG_MAIN("\u2713 Target ice weight reached: %.2f kg\n", weight_delta);
    stage_1.nextStep();
  }

  if (stage_1.getCurrentStep() == 2) {
    stage_1.destroy();
    // Pause before water: let residual ice finish falling
    toteState = ToteState::SETTLING_ICE;
    LOG_MAIN("Transitioning to SETTLING_ICE (8 s debounce)\n");
  }
}

void onSettlingIce() {
  static uint32_t settleStart = 0;
  if (settleStart == 0) {
    settleStart = millis();
    wsClient.sendStateChange("SETTLING_ICE");
    LOG_MAIN("Settling: waiting 8 s for residual ice to stop falling...\n");
  }
  if (millis() - settleStart >= 8000UL) {
    settleStart = 0;  // reset for next cycle
    const float ice_settled = controller.getWeight();
    LOG_MAIN("Ice dispensed: %.2f kg\n", ice_settled);

    tote.ice_kg = ice_settled - tote.initial_weight;  // Solo el hielo (TARE ya eliminó el peso del tote)

    // Send ice dispensed value to frontend
    wsClient.sendIceDispensed(tote.ice_kg);
    toteState = ToteState::DISPENSING_WATER;
    LOG_MAIN("Transitioning to DISPENSING_WATER\n");
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
    LOG_MAIN("Tote completed and sent!\n");
    stage_3.nextStep();
  }

  if (stage_3.getCurrentStep() == 2) {
    stage_3.destroy();
    // Return to IDLE to wait for next tote
    toteState = ToteState::IDLE;
    wsClient.sendStateChange("IDLE");
    LOG_MAIN("\n=== Ready for next tote ===\n");
  }
}

void onButtonPressed() {
  handleInputs();
  readButtonTypeFromSerial(); // Read button type from serial input
}

void initStage1() {
  LOG_MAIN("\n=== Stage 1: Dispensing Ice ===\n");
  // Save initial weight to calculate delta (workaround if TARE doesn't work)
  tote.initial_weight = controller.getWeight();
  LOG_MAIN("Initial weight saved: %.2f kg\n", tote.initial_weight);
  controller.setTare();  // Try TARE anyway
  startICEPump();
}

void initStage2() {
  LOG_MAIN("\n=== Stage 2: Filling Water ===\n");
  // Do NOT setTare here - we want to measure cumulative weight (ice + water)
  controller.writeDigitalOutput(WATER_PUMP, HIGH);
}

void initStage3() {
  LOG_MAIN("Stage 3 Tote Ready\n");
 
}

void destroyStage1() {
  stopICEPump();

  LOG_MAIN("Ice dispensing completed\n");
  LOG_MAIN("Stage 1 destroyed\n");
}

void destroyStage2() {
  const float water_out_kg = controller.getWeight();

  LOG_MAIN("Water filled: %.2f kg\n", water_out_kg);

  tote.water_kg = water_out_kg - tote.initial_weight - tote.ice_kg;  // Solo restar el hielo

  controller.writeDigitalOutput(WATER_PUMP, LOW);

  // Send water dispensed value to frontend
  wsClient.sendWaterDispensed(tote.water_kg);

  LOG_MAIN("Water filling completed\n");
  LOG_MAIN("Stage 2 destroyed\n");
} 

void destroyStage3() {
  // Show all completed tote data
  LOG_MAIN("\n=== Tote Summary ===\n");
  LOG_MAIN("ID: %s\n",    tote.id);
  LOG_MAIN("Tote:  %.2f kg\n", (float)tote.tote_kg);
  LOG_MAIN("Ice:   %.2f kg\n", (float)tote.ice_kg);
  LOG_MAIN("Water: %.2f kg\n", (float)tote.water_kg);
  LOG_MAIN("Raw:   %.2f kg\n", (float)controller.getWeight());
  LOG_MAIN("===================\n\n");
  
  // Calculate raw_kg and water_kg
  float raw_kg = controller.getWeight();
  
  // Send data to backend with POST
  bool success = createToteInBackend(
    tote.id,
    tote.tote_kg,
    tote.water_kg,
    tote.ice_kg,
    raw_kg
  );
  
  if (success) {
    LOG_MAIN("\u2713 Tote created in backend successfully!\n");
  } else {
    LOG_MAIN("\u2717 Failed to create tote in backend\n");
    LOG_MAIN("  Please check backend connection or try again.\n");
  }
  
  // Reset the tare
  controller.setTare();
  
  // Clear data for next tote
  tote = {0, 0, 0, 0, 0};
  LOG_MAIN("Stage 3 destroyed\n");
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
    LOG_MAIN("Already in process\n");
    return;
  }

  const float current_weight = controller.getWeight();

  if(current_weight < Settings::getMinWeight()) {
    LOG_MAIN("Weight is too low to start\n");
    return;
  }

  LOG_MAIN("\n=== System Started ===\n");
  LOG_MAIN("Initial weight: %.2f kg\n", current_weight);
  
  tote.tote_kg = current_weight;
  controller.setTare();

  // Start with ice dispensing
  toteState = ToteState::DISPENSING_ICE;
  LOG_MAIN("Transitioning to DISPENSING_ICE\n");
}

void onStop() {
  LOG_MAIN("\n=== STOP pressed ===\n");
  
  // Stop all pumps
  stopICEPump();
  controller.writeDigitalOutput(WATER_PUMP, LOW);
  auto_stop_ice_routine.cancel();
  stop_water_routine.cancel();
  
  // Cancel current process
  toteState = ToteState::CANCELED;
}

void onManualIce() {
  LOG_MAIN("Manual Ice\n");
  startICEPump();
  auto_stop_ice_routine.restartDelayed(5000);
}

void onManualWater() {
  LOG_MAIN("Manual Water\n");
  controller.writeDigitalOutput(WATER_PUMP, HIGH);
  stop_water_routine.restartDelayed(5000);
  
}

void readButtonTypeFromSerial() {
  if (Serial.available()) {
    const uint8_t buttonType = Serial.parseInt();

    if (buttonType >= 0 && buttonType < BTN_COUNT) { // Valid button types are 0 to 5
      LOG_MAIN("Button type received: %d\n", buttonType);
      handleInputs(static_cast<button_type>(buttonType));
    }
    else {
      LOG_MAIN("Invalid button type. Please enter a number between 0 and 5.\n");
    }
  }
}


void setToteID(const String& id) {
  if (id.length() >= ID_SIZE) {
    LOG_MAIN("Tote ID is too long\n");
    return;
  }

  strncpy(tote.id, id.c_str(), ID_SIZE);
  tote.id[ID_SIZE - 1] = '\0'; // Ensure null termination
  LOG_MAIN("Tote ID set to: %s\n", tote.id);
}

bool setToteIdFromUI(const String& toteId) {
  if (toteState != ToteState::WAITING_TOTE_ID) {
    LOG_MAIN("Cannot set ID, not in WAITING_TOTE_ID state\n");
    return false;
  }

  // Copy ID to tote struct
  memset(tote.id, 0, sizeof(tote.id));
  toteId.substring(0, sizeof(tote.id)-1).toCharArray(tote.id, sizeof(tote.id));

  LOG_MAIN("Tote ID set to: %s\n", tote.id);

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
    LOG_MAIN("\n╔════════════════════════════════════╗\n");
    LOG_MAIN("║   WAITING FOR TOTE ID              ║\n");
    LOG_MAIN("╠════════════════════════════════════╣\n");
    LOG_MAIN("║ Tote:  %.2f kg\n", tote.initial_weight);
    LOG_MAIN("║ Ice:   %.2f kg\n", tote.ice_kg);
    LOG_MAIN("║ Water: %.2f kg\n", tote.water_kg);
    LOG_MAIN("╠════════════════════════════════════╣\n");
    if (bleReady) {
      LOG_MAIN("║ [BLE]  QR-Reader conectado ✓       ║\n");
      LOG_MAIN("║        Leyendo QR automáticamente  ║\n");
    } else {
      LOG_MAIN("║ [BLE]  QR-Reader no conectado      ║\n");
    }
    LOG_MAIN("║ [WEB]  Captura con cámara del tel  ║\n");
    LOG_MAIN("╚════════════════════════════════════╝\n\n");

    // If BLE reader is connected, request the buffered QR every 3 s
    if (bleReady) {
      bleQRClient.requestQR();
    }

    lastPrompt = millis();
  }

  // Transition to COMPLETED is handled by setToteIdFromUI() (BLE or web path)
}

void onCanceled() {
  LOG_MAIN("Tote canceled, cleaning up...\n");
  
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
  LOG_MAIN("Returned to IDLE\n");
}

// ==================== Backend API Functions ====================

bool createToteInBackend(const char* toteId, float tote_kg, float water_kg, float ice_kg, float raw_kg) {
  if (!controller.isWiFiConnected()) {
    LOG_ERR("WiFi not connected, cannot create tote in backend\n");
    return false;
  }

  HTTPClient http;
  String url = String(BACKEND_URL) + "/api/totes";
  
  LOG_MAIN("\n=== Creating Tote in Backend ===\n");
  LOG_MAIN("POST: %s\n", url.c_str());
  
  // Create JSON payload
  DynamicJsonDocument doc(512);
  doc["tote_id"] = toteId;
  doc["tote_kg"] = tote_kg;
  doc["water_kg"] = water_kg;
  doc["ice_kg"] = ice_kg;
  doc["raw_kg"] = raw_kg;
  
  String jsonPayload;
  serializeJson(doc, jsonPayload);
  
  LOG_MAIN("Payload: %s\n", jsonPayload.c_str());
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000); // 10 seconds timeout
  
  int httpCode = http.POST(jsonPayload);
  
  if (httpCode > 0) {
    LOG_MAIN("HTTP Response code: %d\n", httpCode);
    
    String response = http.getString();
    LOG_MAIN("Response: %s\n", response.c_str());
    
    if (httpCode == 201) {
      LOG_MAIN("Tote created in backend successfully!\n");
      http.end();
      return true;
    }
    else if (httpCode == 409) {
      LOG_MAIN("Tote ID already exists in backend (409 Conflict)\n");
      http.end();
      return false;
    }
  }
  else {
    LOG_ERR("HTTP POST failed: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
  return false;
}

// WebSocket message handler
void onWebSocketMessage(String type, JsonDocument& doc) {
  LOG_MAIN("WebSocket message received: %s\n", type.c_str());
  
  if (type == "qr_scanned") {
    const char* toteId = doc["toteId"];
    if (toteId && strlen(toteId) > 0) {
      LOG_MAIN("QR scanned from browser: %s\n", toteId);
      
      // Use setToteIdFromUI to properly handle state transition
      if (setToteIdFromUI(String(toteId))) {
        LOG_MAIN("Tote ID set successfully from browser QR scan\n");
      } else {
        LOG_MAIN("Failed to set Tote ID - may not be in WAITING_TOTE_ID state\n");
        // If not in WAITING_TOTE_ID state, just save the ID for later use
        setToteID(String(toteId));
      }
    }
  }
  else if (type == "command") {
    const char* command = doc["command"];
    LOG_MAIN("Command received: %s\n", command);
    
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
  else if (type == "update_settings") {
    const float ice   = doc["ice_kg"]   | Settings::getTargetIceKg();
    const float water = doc["water_kg"] | Settings::getTargetWaterKg();
    const float minW  = doc["min_w"]    | Settings::getMinWeight();
    Settings::save(ice, water, minW);
    // Echo back the saved values so the browser panel can confirm
    wsClient.sendSettingsCurrent(ice, water, minW);
  }
  else if (type == "get_settings") {
    wsClient.sendSettingsCurrent(
      Settings::getTargetIceKg(),
      Settings::getTargetWaterKg(),
      Settings::getMinWeight()
    );
  }
}
