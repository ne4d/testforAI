#include "mqtt_onmessage.h"

void analyzeMqttMessage(String topic, String msg)
{
  Serial.println("Get message: \"" + topic + "\" (" + msg + ")");
  String dev_uid = FSReadJsonString("dev_uid");
  
  if (topic == dev_uid + "/button/reboot/command")
  {
    if (msg == "PRESS")
    {
      Serial.println("reboot mqtt message accepted");
      rebootLoop(true);
    }
  }
  else if (topic == dev_uid + "/climate/hvac_mode/set") {
    if (msg == "off"){
      mode = "off";
    }
    else if (msg == "cool") {
      mode = "cool";
    }
    else if (msg == "heat") {
      mode = "heat";
    }
    else if (msg == "auto") {
      mode = "auto";
    }
    else if (msg == "dry") {
      mode = "dry";
    }
    else if (msg == "fan_only") {
      mode = "fan_only";
    }
  }
  else if (topic == dev_uid + "/climate/fan_mode/set") {
    if (msg == "low"){
      fan_speed = "low";
    }
    else if (msg == "medium") {
      fan_speed = "medium";
    }
    else if (msg == "high") {
      fan_speed = "high";
    }
    else if (msg == "auto") {
      fan_speed = "auto";
    }
  }
  else if (topic == dev_uid + "/climate/temperature/set") {
    temperature = msg.toInt();
  }
  else if (topic == dev_uid + "/climate/hvac_preset_mode/set") {
    if (msg == "boost"){
      boost = true;
    }
    if (msg == "none"){
      boost = false;
    }
  }
  else if (topic == dev_uid + "/climate/hvac_swing_mode/set") {
    if (msg == "on"){
      swing = true;
    }
    if (msg == "off"){
      swing = false;
    }
  }

  
  if (topic != dev_uid + "/button/reboot/command") ac_command();
}