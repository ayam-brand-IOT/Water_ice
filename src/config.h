#include <Arduino.h>
#include <WS_V2.h>


// ###################### INPUTS ######################
#define PRESENCE_SENSOR         PORT_B0
#define ICE_READY               PORT_B1
#define WATER_READY             PORT_B2

// ###################### OUTPUTS ######################
#define WATER_PUMP              PORT_C0
#define ICE_PUMP                PORT_C1

// ################## RS-485 REGISTERS ##################
#define SCAIME_NUM_VALUES       1
#define DEVICE_BAUDRATE         9600
#define SCAIME_FIRST_REGISTER   0x0082


