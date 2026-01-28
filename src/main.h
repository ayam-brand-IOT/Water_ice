#include "Stage.h"
#include "Types.h"
#include <TaskScheduler.h>
#include "hardware/Controller.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "websocket_client.h"

void onStop();
void onStart();
void onIDLE();
void onManualIce();
void onManualWater();
void onWaitingToteID();
void onCanceled();
void onButtonPressed();
void handleToteState();
void setToteID(const String& id);
bool setToteIdFromUI(const String& toteId);
button_type handleInputs(button_type override = NONE);

void handleIDLE();
void initStage1();
void initStage2();
void initStage3();

void destroyStage1();
void destroyStage2();
void destroyStage3();
void onToteReady();
void onIceFilling();
void onWaterFilling();
void readButtonTypeFromSerial();
void communicationTask(void* pvParameters);

// Backend API functions
bool createToteInBackend(const char* toteId, uint32_t tote_kg, uint32_t water_kg, uint32_t ice_kg, uint32_t raw_kg);

// WebSocket message handler
void onWebSocketMessage(String type, JsonDocument& doc);