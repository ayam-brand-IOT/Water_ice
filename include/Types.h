#pragma once
#include <Arduino.h>
#include <Button.h>
#include "config.h"

typedef struct {
  char id[ID_SIZE];
  float water_kg;
  float ice_kg;
  float tote_kg;
  float raw_kg;
  float initial_weight;  // Peso inicial antes de dispensar (para calcular deltas sin TARE)
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

