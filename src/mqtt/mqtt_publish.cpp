#include "mqtt_publish.h"

void generateAndPublishDefaultsMqttConfig()
{
  // uptime
  String configUptime = generateMQTTConfig("uptime");
  String configUptimeTopic = generateMQTTcfgTopic("uptime");
  // Serial.println(configUptime);  // [DEBUG]
  // Serial.println(configUptimeTopic);  // [DEBUG]
  publishMqttString(configUptimeTopic, configUptime);
  addToMqttList("Uptime", configUptime);

  // публикация аптайма
  publishUptimeState();
  
  // reboot
  String configReboot = generateMQTTConfig("reboot");
  String configRebootTopic = generateMQTTcfgTopic("reboot");
  // Serial.println(configReboot);  // [DEBUG]
  // Serial.println(configRebootTopic);  // [DEBUG]
  publishMqttString(configRebootTopic, configReboot);
  addToMqttList("Reboot", configReboot);

  // подписка на reboot
  subscribeMqttTopic(FSReadJsonString("dev_uid") + "/button/reboot/command");
}

void publishMQTTConfig()
{
  mqtt_config_published = true;
  
  // publish uptime and reboot button config
  generateAndPublishDefaultsMqttConfig();
  
  String config = generateMQTTConfig("climate");
  String configtopic = generateMQTTcfgTopic("climate");
  Serial.println(config);  // [DEBUG]
  Serial.println(configtopic);  // [DEBUG]
  publishMqttString(configtopic, config);
  
  addToMqttList("AC Control", config);

  String dev_uid = FSReadJsonString("dev_uid");
  subscribeMqttTopic(dev_uid + "/climate/fan_mode/set");
  subscribeMqttTopic(dev_uid + "/climate/fan_mode/set");
  subscribeMqttTopic(dev_uid + "/climate/hvac_mode/set");
  subscribeMqttTopic(dev_uid + "/climate/hvac_preset_mode/set");
  subscribeMqttTopic(dev_uid + "/climate/hvac_swing_mode/set");
  subscribeMqttTopic(dev_uid + "/climate/temperature/set");

  mode = FSReadJsonString("mode");
  fan_speed = FSReadJsonString("fan_speed");
  temperature = FSReadJsonUint64("temperature");
  boost = FSReadJsonBool("boost");
  swing = FSReadJsonBool("swing");

  publishMode();
  publishTemp();
  publishFanSpeed();
  publishSwing();
  publishBoost();
}
