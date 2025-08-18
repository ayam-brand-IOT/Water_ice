#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <FS.h>
#include "config.h"
#include <SPIFFS.h>
#include <Button.h>
#include <Arduino.h>
#include "marel.h"
#include "WIFI.h"
#include <Preferences.h>
#include "EdgeBox_ESP_100.h"

enum ControllerState {
    IDLE,
    FEED_TOTE,
    WATER_FILLING,
    ICE_FILLING,
    TOTE_READY
};


class Controller {
private:
    EdgeBox_ESP_100 edgebox;
    ControllerState state = IDLE;

    const uint8_t outputs[2] = {WATER_PUMP, ICE_PUMP};

    void setUpIOS();
    void setUpI2C();
    void setUpDevice();
    void setUpDigitalInputs();
    void setUpDigitalOutputs();


public:
    // ~Controller();
    // Controller(/* args */);
    WIFI wifi;

    void init();
    bool setTare();
    void setUpRTC();
    
    void loopOTA();
    void WiFiLoop();
    void reconnectWiFi();
    bool isWiFiConnected();
    uint32_t getWeight();
    bool isRTCConnected();
    ControllerState getState();
    void setState(ControllerState state);
    bool readDigitalInput(uint8_t input);
    void writeDigitalOutput(uint8_t output, uint8_t value);
    void connectToWiFi(bool web_server, bool web_serial, bool OTA);
    void setUpWiFi(const char* ssid, const char* password, const char* hostname);
    bool hasIntervalPassed(uint32_t &previousMillis, uint32_t interval, bool to_min);
    void broadcastWeight(uint32_t weight);

    void DEBUG_M(const char *message) {
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "[Controller]: %s", message);
        Serial.println(buffer);
    }

};

#endif