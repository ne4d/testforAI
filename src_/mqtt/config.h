#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../fs/filesystem.h"
#include "../../include/version.h"
#include <ESP8266WiFi.h>

String generateMQTTConfig(String dev_type);
String generateMQTTcfgTopic(String dev_type);

#endif