#ifndef WIFI_H
#define WIFI_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "../system.h"
#include "../device_config.h"
#include "../fs/filesystem.h"
#include "../mqtt/mqtt.h"
#include "../web/server.h"

String getFormattedMAC();
void connectToWifi();
void startAP();
// void onWifiEvent(WiFiEvent_t event);
void onWifiConnect(const WiFiEventStationModeGotIP& event);
void onWifiDisconnect(const WiFiEventStationModeDisconnected& event);

#endif