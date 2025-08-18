#include "Stage.h"
#include <TaskScheduler.h>
#include "hardware/Controller.h"


typedef struct {
  char id[ID_SIZE];
  uint32_t water_weight;
  uint32_t ice_weight;
  uint32_t tote_weight;
  uint32_t raw_weight;
} tote_data;

enum button_type {
    NONE,
    START,
    STOP,
    MANUAL_ICE,
    MANUAL_WATER,
    BTN_COUNT
};

struct button_action{
  button_type type;
  Button button;
  void (*handler)();

  button_action(button_type t, uint8_t io, void (*h)())
    : type(t), button(io), handler(h)
  {}
};
void onStop();
void onStart();
void onManualIce();
void onManualWater();
void onButtonPressed();
void setToteID(const String& id);
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