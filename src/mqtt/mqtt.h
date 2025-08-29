#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <ESP8266WiFi.h>
#include <ezOutput.h>
#include "../fs/filesystem.h"
#include "../system.h"
#include "../device_config.h"
#include "mqtt_onmessage.h"

void mqttInit();
void connectMqtt();
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void publishMqttString(String topic, String msg, uint8_t qos = 0, bool retain = true);
void publishMqttBool(String topic, bool msg, uint8_t qos = 0, bool retain = true);
void subscribeMqttTopic(String topic, uint8_t qos = 0);
void publishAvailability(uint8_t qos = 0, bool retain = true);
void publishUptimeState(uint8_t qos = 0, bool retain = true);

void publishMode(uint8_t qos = 0, bool retain = true);
void publishTemp(uint8_t qos = 0, bool retain = true);
void publishFanSpeed(uint8_t qos = 0, bool retain = true);
void publishSwing(uint8_t qos = 0, bool retain = true);
void publishBoost(uint8_t qos = 0, bool retain = true);

#endif