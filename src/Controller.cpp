#include "Controller.h"

ModbusDevice scaime(SCAIME_NUM_VALUES, DEVICE_BAUDRATE, SCAIME_FIRST_REGISTER);

void Controller::init(){
    setUpIOS();
    // setUpI2C();
    setUpModbusDevices();
    // setUpRTC();
}

void Controller::setUpIOS(){
    Serial.begin(115200);

    setUpDigitalInputs();
    setUpDigitalOutputs();
}

void Controller::setUpI2C(){

}

void Controller::setState(ControllerState state){
    this->state = state;
}

ControllerState Controller::getState(){
    return this->state;
}

void Controller::setUpModbusDevices(){
    scaime.init();
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

