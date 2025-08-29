#ifndef MQTT_ONMESSAGE_H
#define MQTT_ONMESSAGE_H

#include <Arduino.h>
#include "../fs/filesystem.h"
#include "../system.h"
#include "../controls/accommand.h"
#include "../device_config.h"

void analyzeMqttMessage (String topic, String msg);

#endif