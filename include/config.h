#pragma once

#include <Arduino.h>
#include "EdgeBox_ESP_100.h"

#define MIN_WEIGHT 5

// #################### DISPENSING TARGETS ####################
#define TARGET_ICE_KG 2.0    // Peso objetivo de hielo en kg
#define TARGET_WATER_KG 2.0  // Peso objetivo de agua en kg

// ###################### INPUTS ######################
#define START_IO                DI_0
#define STOP_IO                 DI_1

#define MANUAL_ICE_IO           DI_3
#define MANUAL_WATER_IO         DI_2

// ###################### OUTPUTS ######################
#define ICE_PUMP                DO_3
#define ICE_STOP                DO_4
#define WATER_PUMP              DO_2
#define INDICATOR_1             DO_0
#define INDICATOR_2             DO_1


//Configuración de red
#define HAS_STATIC_IP                               //TURN ON THE STATIC IP
#define IP_ADDRESS { 192, 168, 100, 21 }
#define GATEWAY_ADDRESS { 192, 168, 100, 254 }
#define SUBNET_ADDRESS { 255, 255, 255, 0 }
//TURN ON THE STATIC IP

// #define U_SSID "MFP-Guest24"
// #define U_PASS "testing123"

// #define U_SSID "Pez Gordo"
// #define U_PASS "SardinaMacarena2021"

#define U_SSID "CFPP-Iot"
#define U_PASS ""

// ##################### BACKEND API #####################
#define BACKEND_HOST "10.20.30.100"  // Cambiar a la IP del backend
// #define BACKEND_HOST "192.168.99.58"  // Cambiar a la IP del backend

#define BACKEND_PORT 3000  // Puerto directo del backend Node.js (sin nginx)
#define BACKEND_WS_PORT 3001
#define BACKEND_URL "http://" BACKEND_HOST ":3000"  // Conexión directa al backend

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

