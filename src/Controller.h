#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <FS.h>
#include "config.h"
#include <SPIFFS.h>
#include <Button.h>
#include <Arduino.h>
#include "marel.h"
#include <Preferences.h>
#include "EdgeBox_ESP_100.h"

enum ControllerState {
    IDLE,
    ICE_FILLING,
    WATER_FILLING,
};


class Controller {
private:

    ControllerState state = IDLE;
    const uint8_t inputs[3] = {PRESENCE_SENSOR, ICE_READY, WATER_READY};
    const uint8_t outputs[2] = {WATER_PUMP, ICE_PUMP};

    const size_t num_inputs = sizeof(inputs)/sizeof(inputs[0]);
    const size_t num_outputs = sizeof(outputs)/sizeof(outputs[0]);

    void setUpIOS();
    void setUpI2C();
    void setUpDevice();
    void setUpDigitalInputs();
    void setUpDigitalOutputs();

public:
    // ~Controller();
    // Controller(/* args */);

    void init();
    void setUpRTC();
    bool isRTCConnected();
    ControllerState getState();
    void setState(ControllerState state);
    bool readDigitalInput(uint8_t input);
    void writeDigitalOutput(uint8_t output, uint8_t value);

};

#endif