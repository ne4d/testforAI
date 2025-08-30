#ifndef PAGES_H
#define PAGES_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../fs/filesystem.h"
#include "../../include/version.h"
#include "../device_config.h"
#include "../mqtt/mqtt_publish.h"

String loginPage();
String serverIndex();
String getSwitchState();
void addToMqttList(String header, String config);

#endif