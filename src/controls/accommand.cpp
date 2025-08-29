#include "accommand.h"

// const uint16_t kIrLed = 13;  // ESP8266 GPIO пин ИК светодиода
IRDaikinESP ac(kIrLed);

// void ac_command(int temp, String(mode), String(fan_speed)){
void ac_command(){

  ac.setTemp(temperature);
  Serial.println("temp." + String(temperature));

  if (mode == "off"){
    ac.off();
    Serial.println("mode.off");
  }
  else if (mode == "cool"){
    ac.setPower(true);
    ac.setMode(kDaikinCool);
    Serial.println("mode.cool");
  }
  else if (mode == "heat"){
    ac.setPower(true);
    ac.setMode(kDaikinHeat);
    Serial.println("mode.heat");
  }
  else if (mode == "auto"){
    ac.setPower(true);
    ac.setMode(kDaikinAuto);
    Serial.println("mode.auto");
  }
  else if (mode == "dry"){
    ac.setPower(true);
    ac.setMode(kDaikinDry);
    Serial.println("mode.dry");
  }
  else if (mode == "fan_only"){
    ac.setPower(true);
    ac.setMode(kDaikinFan);
    Serial.println("mode.fan_only");
  }

  if (fan_speed == "low"){
      ac.setFan(kDaikinFanMin);
      Serial.println("fan.low");
      FSWriteJsonString("fan_speed", "low");
  }
  else if (fan_speed == "medium"){
      ac.setFan(kDaikinFanMed);
      Serial.println("fan.med");
      FSWriteJsonString("fan_speed", "medium");
    }
    else if (fan_speed == "high"){
      ac.setFan(kDaikinFanMax);
      Serial.println("fan.hgh");
      FSWriteJsonString("fan_speed", "high");
    }
    else if (fan_speed == "auto"){
      ac.setFan(kDaikinFanAuto);
      Serial.println("fan.auto");
      FSWriteJsonString("fan_speed", "auto");
  }

  ac.send();
  Serial.println("----------");
  publishAvailability();
  publishMode();
  publishTemp();
  publishFanSpeed();
  // publishSwing();
  // publishBoost();
}