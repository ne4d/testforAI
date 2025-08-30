#include "system.h"

uint8_t reboot_delay = 5;

// Время в минутах
uint32_t minutesRunned() {
  return millis() / 60000;
}

// Время в секундах
uint32_t secondsRunned() {
  return millis() / 1000;
}

// Время в милисекундах
uint32_t millisecondsRunned() {
  return millis();
}

// reboot
void rebootLoop(bool trigger){
    static uint32_t timestamp = 0;
    static bool restart_flag = false;
    if (trigger) {
        timestamp = secondsRunned(); 
        restart_flag = true;
        // ledConfig.blink(25,25);
    }
    if (restart_flag && (secondsRunned() - timestamp > reboot_delay)){ 
        Serial.println("reboot"); 
        ESP.restart();
    }
}