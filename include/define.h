#include <Arduino.h>

#ifndef WIFI_AP_PASSWORD
#define WIFI_AP_PASSWORD "password"
#endif

// Inputs
#define GPIO_WIFI_RESET     A0

// Outputs
#define GPIO_HEARTBEAT      D0
#define GPIO_RESET_STM32    D1
#define GPIO_BOOT0_STM32    D3
