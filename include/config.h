#pragma once

#include <Arduino.h>
#include "EdgeBox_ESP_100.h"

#define MIN_WEIGHT 5

// ###################### INPUTS ######################
#define START_IO                DI_0
#define STOP_IO                 DI_1

#define MANUAL_ICE_IO           DI_3
#define MANUAL_WATER_IO         DI_2

// ###################### OUTPUTS ######################
#define WATER_PUMP              DO_0
#define ICE_PUMP                DO_1


// #################### MAREL - INFO ####################
// Datos del servidor Marel
#define SERVER_PORT 52200
#define SERVER_IP   "192.168.1.7"

// #define SERVER_IP   "169.254.207.197"

//Configuración de red
#define MAC_ADDRESS { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }
#define IP_ADDRESS { 192, 168, 1, 5 }
#define GATEWAY_ADDRESS { 192, 168, 1, 1 }
#define SUBNET_ADDRESS { 255, 255, 255, 0 }

#define U_SSID "MFP-Guest24"
#define U_PASS "testing123"

// #define U_SSID "CFPP (Test)"
// #define U_PASS "cfpptest"

// ##################### WEB SERVER #####################

#define SSID_SIZE 32
#define ID_SIZE 32
#define PASSWORD_SIZE 64
#define HOSTNAME_SIZE 32
#define IP_ADDRESS_SIZE 16

#define www_username "admin"
#define www_password "admin"
#define VERSION "1.0.0"
#define SERVER_INDEX_HTML INDEX_HTML

