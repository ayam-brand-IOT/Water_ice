#ifndef MODBUS_DEVICE_H
#define MODBUS_DEVICE_H

#include "config.h"
#include "Logging.h"
#include <HardwareSerial.h>
#include "ModbusClientRTU.h"

#define OFFSET 3
#define DATA_SIZE 2
// #define RXPIN2 27  M5 SHIT
// #define TXPIN2 19. M5 SHIT


#define RXPIN2 RS_485_RX
#define TXPIN2 RS_485_TX

// ModbusClientRTU MB(Serial1);

class ModbusDevice {
  private:
    uint16_t NUM_VALUES = 0;
    uint16_t BAUDRATE2 = 0;
    uint16_t FIRST_REGISTER = 0;

  public:
    uint32_t request_time;
    ModbusDevice(uint8_t no_values, uint16_t baudrate, uint16_t first_reg);

    void init();
    void setCallback(std::function<void (ModbusMessage response, uint32_t token)> callback);
    void setOnError(std::function<void (Error error, uint32_t token)> callback);
    void requestMultipleRegisters();
};
#endif
