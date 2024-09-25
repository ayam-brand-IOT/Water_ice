#include "ModbusDevice.h"

ModbusClientRTU MB(Serial1);

ModbusDevice::ModbusDevice(uint8_t no_values, uint16_t baudrate, uint16_t first_reg){
  NUM_VALUES = no_values;
  BAUDRATE2 = baudrate;
  FIRST_REGISTER = first_reg;
}

void ModbusDevice::init(){
  Serial1.begin(BAUDRATE2, SERIAL_8N2, RXPIN2, TXPIN2);
  MB.setTimeout(100);
  MB.begin(Serial1);
}

void ModbusDevice::setCallback(std::function<void (ModbusMessage response, uint32_t token)> callback){
  MB.onDataHandler(callback);
}

void ModbusDevice::setOnError(std::function<void (Error error, uint32_t token)> callback){
  MB.onErrorHandler(callback);
}

void ModbusDevice::requestMultipleRegisters(){
   Error err = MB.addRequest((uint32_t)millis(), 1, READ_INPUT_REGISTER, FIRST_REGISTER, NUM_VALUES);
    if (err!=SUCCESS) {
      ModbusError e(err);
      LOG_E("Error creating request: %02X - %s\n", (int)e, (const char *)e);
    }
}