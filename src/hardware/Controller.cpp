#include "Controller.h"

byte mac[] = MAC_ADDRESS;
IPAddress ip(IP_ADDRESS);
IPAddress gateway(GATEWAY_ADDRESS);
IPAddress subnet(SUBNET_ADDRESS);

MarelClient marel(SERVER_IP, SERVER_PORT, mac, ip, gateway, subnet);


void Controller::init(){
    setUpIOS();
    // setUpI2C();
    setUpDevice();
    // setUpRTC();
}

void Controller::setUpIOS(){
    Serial.begin(115200);

    setUpDigitalInputs();
    setUpDigitalOutputs();
}

void Controller::setUpI2C(){

}

void Controller::loopOTA() {
  wifi.loopOTA();
}

void Controller::WiFiLoop() {
  if (!isWiFiConnected()) {
    reconnectWiFi();
    vTaskDelay(500 / portTICK_PERIOD_MS);
    return;
  }
}

void Controller::reconnectWiFi() {
  wifi.reconnect();
}


bool Controller::isWiFiConnected() {
  return wifi.isConnected();
}

void Controller::setUpWiFi(const char* ssid, const char* password, const char* hostname) {
  wifi.init(ssid, password, hostname);
}

void Controller::connectToWiFi(bool web_server, bool web_serial, bool OTA) {
  wifi.connectToWiFi();
  if(OTA) wifi.setUpOTA();
  if(web_server) wifi.setUpWebServer(web_serial);
}

void Controller::setState(ControllerState state){
    this->state = state;
}

ControllerState Controller::getState(){
    return this->state;
}

void Controller::setUpDevice(){
#ifndef DEBUG
    marel.begin();
#endif
}

bool Controller::setTare(){
#ifndef DEBUG
    marel.setTare();
    return true;
#else
    DEBUG_M("Tare set");
    return true;
#endif
}
uint32_t Controller::getWeight(){
#ifndef DEBUG
    const String weight = marel.getWeight();
    return weight.toInt();
#else
    //return a random number between 100 and 1000
    return random(500, 1000);
#endif
}

void Controller::setUpDigitalInputs(){
    for(int i = 0; i < num_inputs; i++) pinMode(inputs[i], INPUT_PULLUP);
}

void Controller::setUpDigitalOutputs(){
    for(int i = 0; i < num_outputs; i++) pinMode(outputs[i], OUTPUT);
}

void Controller::setUpRTC(){
    
}

bool Controller::isRTCConnected(){
    return true;
}

bool Controller::readDigitalInput(uint8_t input){
    return digitalRead(input);
}

void Controller::writeDigitalOutput(uint8_t output, uint8_t value){
    return digitalWrite(output, value);
}

bool Controller::hasIntervalPassed(uint32_t &previousMillis, uint32_t interval, bool to_min) {
    if(to_min) interval *= 60000UL;
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      return true;
    }
    return false; 
}