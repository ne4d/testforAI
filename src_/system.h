#ifndef SYSTEM_H
#define SYSTEM_H

#include <Arduino.h>
#include "device_config.h"

uint32_t millisecondsRunned(); // Время в милисекундах
uint32_t secondsRunned(); // Время в секундах
uint32_t minutesRunned(); // Время в минутах

void rebootLoop(bool trigger = false); // reboot

#endif