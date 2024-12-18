#pragma once

#include <Arduino.h>
#include "EdgeBox_ESP_100.h"


// ###################### INPUTS ######################
#define PRESENCE_SENSOR         DI_0
#define ICE_READY               DI_1
#define WATER_READY             DI_2

// ###################### OUTPUTS ######################
#define WATER_PUMP              DO_0
#define ICE_PUMP                DO_1

// #################### MAREL - INFO ####################
// Datos del servidor Marel
#define SERVER_PORT 52200
#define SERVER_IP   "192.168.1.7"

//Configuración de red
#define MAC_ADDRESS { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }
#define IP_ADDRESS { 192, 168, 1, 5 }
#define GATEWAY_ADDRESS { 192, 168, 1, 1 }
#define SUBNET_ADDRESS { 255, 255, 255, 0 }

// // Configuración de red
// extern byte mac[];
// extern IPAddress ip;
// extern IPAddress gateway;
// extern IPAddress subnet;


