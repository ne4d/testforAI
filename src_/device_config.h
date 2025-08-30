#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <Arduino.h>
#include <ezOutput.h> // ezOutput library
#include <Ticker.h>

// настройка пинов
extern uint8_t blinkPin;     // Кнопка (GPIO8)
extern uint8_t configPin;     // morgalka
// переменные
extern bool mqtt_connection_state;
extern bool mqtt_connection_initiation;
extern bool mqtt_config_published;
extern bool mqtt_availability_published;
extern bool wifi_connection_state;
extern bool wifi_connection_initiation;
extern bool wifi_config_present;
extern bool wifi_ap_running;
extern uint8_t wifi_attempts;

extern String mqttTopicList;

extern String mode;
extern String fan_speed;
extern uint8_t temperature;
extern bool boost;
extern bool swing;
const uint8_t kIrLed = D7;  // ESP8266 GPIO пин ИК светодиода
extern String dev_uid;

// led blink
//extern ezOutput ledConfig;

extern Ticker mqttReconnectTimer;
extern Ticker wifiReconnectTimer;


#endif