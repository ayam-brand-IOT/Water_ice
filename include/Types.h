#pragma once
#include <Arduino.h>
#include <Button.h>
#include "config.h"

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

