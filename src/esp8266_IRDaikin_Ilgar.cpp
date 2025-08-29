#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "version.h"
// #include "controls/switches.h"
// #include "controls/toggles.h"
#include "fs/filesystem.h"
#include "mqtt/config.h"
// #include "mqtt/mqtt.h"
#include "mqtt/mqtt_publish.h"
// #include "web/pages.h"
#include "web/server.h"
#include "wifi/wifi.h"
#include "device_config.h"
#include "system.h"
#include "device_defaults.h"

const uint32_t kBaudRate = 115200;
// настройка пинов
uint8_t blinkPin = D1;  // Кнопка (GPIO8)
uint8_t configPin = D2; // morgalka
bool mqtt_connection_state = false;
bool mqtt_connection_initiation = false;
bool mqtt_config_published = false;
bool mqtt_availability_published = false;
bool wifi_connection_state = false;
bool wifi_config_present = false;
bool wifi_connection_initiation = false;
bool wifi_ap_running = false;
// uint32_t wifi_attempts = 12;
uint8_t wifi_attempts = 2;
String mqttTopicList =  "<html><head>"
                        "<meta charset=\"UTF-8\" name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                        "<title>Mqtt topic configuration list</title>"
                        "<p><center><h3>Mqtt topic configuration list</h3></center></p>";

String mode;
String fan_speed;
uint8_t temperature;
bool boost;
bool swing;   
String dev_uid;                     

// не общие переменные
bool web_server_running = false;
uint8_t wifi_search_delay_m = 1; // время в минутах для повторного поиска ssid
uint32_t wifi_search_delay_s;
bool relay_restart_delay = true;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;

// led blink
ezOutput ledConfig(blinkPin);

void setup()
{
  // psramInit();
  ledConfig.low();
  Serial.begin(kBaudRate);
  delay(1000);
  Serial.println("Starting ESP32-S3 setup...");
  // Проверка PSRAM - Диагностика
  // Serial.printf("Total PSRAM: %d bytes\n", ESP.getPsramSize());
  // Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());

  // Инициализация LittleFS
  if (!LittleFS.begin())
  {
    Serial.println("LittleFS Mount Failed, Formatting...");
    LittleFS.format();
    if (!LittleFS.begin())
    {
      Serial.println("LittleFS Mount Failed after formatting!");
      while (true)
        ;
    }
  }
  Serial.println("LittleFS Mounted Successfully");
  // Чтение данных
  FSPrintAllJson();

  
  dev_uid = FSReadJsonString("dev_uid");

  // инициализация модуля при первом запуске
  setFlashDefaults();

  // Инициализация пинов
  pinMode(configPin, INPUT_PULLUP);

  mqttInit();

  // WiFi.onEvent(onWifiEvent);
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  // проверка конфигурации WiFi
  if (FSReadJsonString("ssid") != "" && FSReadJsonString("ssid_pw") != "" && digitalRead(configPin))
    wifi_config_present = true;
  wifi_search_delay_s = wifi_search_delay_m * 60; // конвертация минут в секунды
  Serial.println("Module setup complete!");
}

void loop()
{
  static uint32_t lastMsg = 0;
  static uint32_t ap_initiation_timestamp = 0;
  static uint32_t mqtt_reconnect_timestamp = 0;
  uint32_t now = secondsRunned();
  uint32_t every_minute = 0;

  // server.handleClient();

  rebootLoop();
  ledConfig.loop();

  // // задержка восстановления состояния каналов изза таймеров C005
  // if (now > 10 && relay_restart_delay)
  // {
  //   for (uint8_t i = 1; i <= channel_count; i++)
  //     updateChannel(i);
  //   relay_restart_delay = false;
  // }

  if ((now > 10) && mqtt_connection_state && mqtt_config_published && !mqtt_availability_published){
    // публикация доступности устройства
    publishAvailability();
    mqtt_availability_published = true;
  }

  // публикация аптайма
  if ((now - lastMsg > 60) && mqtt_connection_state)
  {
    lastMsg = now;
    publishUptimeState();
  }

  // слежение за (пере)подключением к ssid
  if (wifi_config_present && !wifi_connection_state && !wifi_connection_initiation)
  {
    // ledConfig.blink(400, 400);
    connectToWifi();
  }
  else if (wifi_config_present && !wifi_connection_state && !wifi_ap_running)
  {
    // ledConfig.blink(150, 150);
    ap_initiation_timestamp = secondsRunned();
    startAP();
  }
  else if (wifi_config_present && wifi_ap_running && WiFi.softAPgetStationNum() != 0 && (now - every_minute > 3))
    ap_initiation_timestamp = secondsRunned();
  else if (!wifi_config_present && !wifi_ap_running)
  {
    // ledConfig.blink(150, 150);
    startAP();
  }

  // единоразовый запуск web сервера
  if ((!web_server_running && wifi_ap_running) || (!web_server_running && wifi_connection_state))
  {
    Serial.println("Starting web server...");
    // startServer();
    web_server_running = true;
  }

  // разрешение на переподключение к ssid
  if (wifi_config_present && wifi_ap_running && ((wifi_search_delay_s - (secondsRunned() - ap_initiation_timestamp)) <= 0))
  {
    wifi_connection_initiation = false;
    wifi_ap_running = false;
  }

  // слежение за отвалом mqtt
  if (wifi_connection_state && !mqtt_connection_state && !mqtt_connection_initiation && (now - mqtt_reconnect_timestamp > 5)){
  // if (wifi_connection_state && !mqtt_connection_state && !mqtt_connection_initiation){
    mqtt_reconnect_timestamp = secondsRunned();
    connectMqtt();
  }

  if (wifi_connection_state && mqtt_connection_state && !mqtt_config_published){
    publishMQTTConfig();
  }

    // delay(1);
}