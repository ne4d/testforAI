#include <Arduino.h>
#include <LittleFS.h>
// #include <ESPAsyncWebServer.h>
#include <ESP8266WebServer.h>
#include <AsyncMqttClient.h>
// #include <ESP8266WiFi.h>
#include <ESP8266WiFi.h>
// #include <Update.h>
#include <ArduinoOTA.h>
#include <UptimeString.h>
#include <Uptime.h>    
#include <FileData.h>
// #include <ArduinoJson.h>

#include <IRsend.h>
#include <ir_Electra.h>
String used_lib = "ir_Electra.h";

// В начале файла
#define MQTT_RECONNECT_INTERVAL 10000
static uint32_t lastMqttReconnect = 0;

const uint32_t kBaudRate = 115200;
const uint8_t configPin = 38;     // morgalka
const uint16_t kIrLed = 6;  // ESP8266 GPIO пин ИК светодиода

IRElectraAc ac(kIrLed);

AsyncMqttClient mqttClient;
// AsyncWebServer server(80);
ESP8266WebServer server(80);

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;

// Структура данных
struct Data {
  char data_my_mac[20];
  char data_ssid[40];
  char data_ssid_pass[20];
  char data_webUI_pass[20];
  char data_mqtt_ip[20];
  uint16_t data_mqtt_port;
  char data_mqtt_user[20];
  char data_mqtt_pass[20];
  char data_dev_name[30] = "IR";
  char data_entity_name[30] = "AC control";
  char data_mode[20];
  char data_fan_speed[20];
  uint8_t data_temperature;
  bool data_boost;
  bool data_swing;
};

Data mydata;
FileData data(&LittleFS, "/data.dat", 'A', &mydata, sizeof(mydata));

String mode;
uint8_t temperature;
String fan_speed;
uint32_t last_commandsend_time;
uint32_t last_commandsend_passedtime;
bool boost = true;
bool swing = false;

uint32_t lastMsg = 0;
uint32_t lastBlink = 0;
uint32_t sendDelayStart = 0;
bool sendDelayActive = false;
bool publishFlag = false;
uint32_t boot_delay;
uint32_t wifi_reconnect_timer = 0;
bool ap_started = false;
uint8_t wifi_search_delay_m = 10;
uint32_t wifi_search_delay_s;
uint8_t wifi_attempts;
uint8_t wifi_attempts_max = 10;
bool config_present = false;
bool mqtt_connected = false;
bool reconnectMQTT_allow;

// Устройство
String dev_uid, config_topicUT, config_topicReboot, clientId, config_topic;
String entity_name, dev_name, dev_model;
String sw_ver = "1.0.0";

// uptime
// String _uptime;
UptimeString uptimeString;
Uptime newuptime;

// Веб-сервер
String SERPASS;
String WIRELESSNAME;
#define MAX_UID 8
String _rndcookie;

// reboot
uint64_t flag_time;
bool restart_flag;

// Флаг для отслеживания статуса обновления
bool updateFinished = false;
bool updateSuccess = false;

String generateUID() {
  const char possible[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  String uid = "";
  for (int i = 0; i < MAX_UID; i++) {
    int r = random(0, strlen(possible));
    uid += possible[r];
  }
  return uid;
}

bool isAuthenticated() {
if (server.hasHeader("Cookie")) {
  String cookie = server.header("Cookie");
  return (cookie.equals("key=" + _rndcookie)); //ok3
}
  return false;
}

void InitCookie() {
  _rndcookie = generateUID();
}

// Функции для работы с LittleFS
void FSWriteSwing(bool value) { mydata.data_swing = value; data.update(); }
bool FSReadSwing() { return mydata.data_swing; }
void FSWriteTurbo(bool value) { mydata.data_boost = value; data.update(); }
bool FSReadTurbo() { return mydata.data_boost; }

void FSWriteWifiName(String value) { snprintf(mydata.data_ssid, sizeof(mydata.data_ssid), "%s", value.c_str()); data.update(); }
String FSReadWifiName() { return mydata.data_ssid; }
void FSWriteDeviceName(String value) { snprintf(mydata.data_dev_name, sizeof(mydata.data_dev_name), "%s", value.c_str()); data.update(); }
String FSReadDeviceName() { return mydata.data_dev_name; }
void FSWriteEntityName(String value) { snprintf(mydata.data_entity_name, sizeof(mydata.data_entity_name), "%s", value.c_str()); data.update(); }
String FSReadEntityName() { return mydata.data_entity_name; }
void FSWriteWifiPass(String value) { snprintf(mydata.data_ssid_pass, sizeof(mydata.data_ssid_pass), "%s", value.c_str()); data.update(); }
String FSReadWifiPass() { return mydata.data_ssid_pass; }
void FSWriteMAC(String value) { snprintf(mydata.data_my_mac, sizeof(mydata.data_my_mac), "%s", value.c_str()); data.update(); }
String FSReadMAC() { return mydata.data_my_mac; }
String FSReadWebUIPass() { return mydata.data_webUI_pass; }
void FSWriteWebUIPass(String value) { snprintf(mydata.data_webUI_pass, sizeof(mydata.data_webUI_pass), "%s", value.c_str()); data.update(); }

void FSWriteMQTTIP(const char* value) {
  if (strlen(value) > 79) {
    Serial.println("MQTT IP too long (>79 chars), ignored");
    return;
  }
  strncpy(mydata.data_mqtt_ip, value, sizeof(mydata.data_mqtt_ip));
  mydata.data_mqtt_ip[sizeof(mydata.data_mqtt_ip) - 1] = '\0'; // Гарантируем нулевой терминатор
  data.update();
  Serial.println("Wrote MQTT IP: " + String(mydata.data_mqtt_ip));
}

const char* FSReadMQTTIP() {
  return mydata.data_mqtt_ip;
}

void FSWriteMQTTPort(uint16_t value) { mydata.data_mqtt_port = value; data.update(); }
uint32_t FSReadMQTTPort() { return mydata.data_mqtt_port; }

void FSWriteMQTTUser(const char* value) {
  if (strlen(value) > 19) {
    Serial.println("MQTT User too long (>19 chars), ignored");
    return;
  }
  strncpy(mydata.data_mqtt_user, value, sizeof(mydata.data_mqtt_user));
  mydata.data_mqtt_user[sizeof(mydata.data_mqtt_user) - 1] = '\0'; // Гарантируем нулевой терминатор
  data.update();
  Serial.println("Wrote MQTT User: " + String(mydata.data_mqtt_user));
}

const char* FSReadMQTTUser() {
  return mydata.data_mqtt_user;
}

void FSWriteMQTTPass(const char* value) {
  if (strlen(value) > 19) {
    Serial.println("MQTT Pass too long (>19 chars), ignored");
    return;
  }
  strncpy(mydata.data_mqtt_pass, value, sizeof(mydata.data_mqtt_pass));
  mydata.data_mqtt_pass[sizeof(mydata.data_mqtt_pass) - 1] = '\0';
  data.update();
  Serial.println("Wrote MQTT Pass: " + String(mydata.data_mqtt_pass));
}

const char* FSReadMQTTPass() {
  return mydata.data_mqtt_pass;
}

void FSWriteTemp(uint8_t value) {
  mydata.data_temperature = temperature;
  data.update();
}

uint8_t FSReadTemp() {
  uint8_t value = mydata.data_temperature;
  return value;
}

void FSWriteMode(String value) {
  snprintf(mydata.data_mode, sizeof(mydata.data_mode), "%s", value.c_str());
  data.update();
}

String FSReadMode() {
  String value = mydata.data_mode;
  return value;
}

void FSWriteFanSpeed(String value) {
  snprintf(mydata.data_fan_speed, sizeof(mydata.data_fan_speed), "%s", value.c_str());
  data.update();
}

String FSReadFanSpeed() {
  String value = mydata.data_fan_speed;
  return value;
}

void FSDataRead() {
  Serial.println("=== FS Data ===");
  Serial.print("temp: ");
  Serial.println(FSReadTemp());
  Serial.print("mode: ");
  Serial.println(FSReadMode());
  Serial.print("fan speed: ");
  Serial.println(FSReadFanSpeed());
  Serial.println("SSID: " + FSReadWifiName());
  Serial.println("SSID Password: " + FSReadWifiPass());
  Serial.println("MQTT IP: " + String(FSReadMQTTIP()));
  Serial.println("MQTT Port: " + String(FSReadMQTTPort()));
  Serial.println("MQTT User: " + String(FSReadMQTTUser()));
  Serial.println("MQTT Password: " + String(FSReadMQTTPass()));
  Serial.println("WebUI Password: " + FSReadWebUIPass());
  Serial.println("Device UID: " + dev_uid);
}

String loginPage() {
  String page = "<html><head>";
  page += "<meta charset=\"UTF-8\" name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  page += "<title>Авторизация</title>";
  page += "<style>";
  page += "body { text-align: center; font-family: Arial, sans-serif; font-size: 20px;}";
  page += ".container { width: 250px; display: inline-block; text-align: left; padding: 10px; border: 2px solid #3498db; border-radius: 10px; }";
  page += "p, input { margin: 5px 0; }";
  page += "input[type='password'] { width: 250px; background-color: #FFFFFF; color: black; padding: 10px; border: 3px solid #3498db; border-radius: 5px; cursor: pointer; }";
  page += "input[type='submit'] { width: 250px; background-color: #3498db; color: white; padding: 10px; border: none; border-radius: 5px; cursor: pointer; }";
  page += "</style>";
  page += "</head><body>";
  page += "<center>";
  page += "<h3>" + entity_name + "(" + dev_uid + ")</h3>";
  page += "<form action='/login' method='POST'>";
  page += "Пароль:<br><input type='password' name='password'><br>";
  page += "<input type='submit' value='Войти'>";
  page += "</form>";
  page += "</center>";
  page += "</body></html>";
  return page;
}


String serverIndex() {
  String page = "<html><head>";
  page += "<meta charset=\"UTF-8\" name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  page += "<style>";
  page += "body { text-align: center; font-family: Arial, sans-serif; font-size: 20px;}";
  page += ".container { width: 250px; display: inline-block; text-align: left; padding: 10px; border: 2px solid #3498db; border-radius: 10px; }";
  page += "p, input { margin: 5px 0; }";
  page += "input[type='password'], input[type='text'] { width: 250px; background-color: #FFFFFF; color: black; padding: 10px; border: 3px solid #3498db; border-radius: 5px; cursor: pointer; }";
  page += "input[type='submit'] { width: 100%; background-color: #3498db; color: white; padding: 10px; border: none; border-radius: 5px; cursor: pointer; }";
  page += "#btn_reset { background-color: red; }";
  page += "#btn_info { background-color: green; }";
  page += "#btn_reboot { background-color: orange; }";
  page += "#btn_on { background-color: green; width: 120px; display: inline-block; color: white; padding: 10px; border: none; border-radius: 5px; cursor: pointer; margin: 3px;}"; // Добавлен display: inline-block
  page += "#btn_off { background-color: red; width: 120px; display: inline-block; color: white; padding: 10px; border: none; border-radius: 5px; cursor: pointer; margin: 3px;}"; // Добавлен display: inline-block
  page += ".ts16 { font-size: 16px; }";
  page += ".button-container { display: flex; justify-content: center; gap: 10px; }"; // Новый стиль для контейнера кнопок
  page += ".status-container { margin-top: 20px; padding: 10px; border: 1px solid #ccc; border-radius: 5px; }"; // Новый стиль для контейнера состояния
  page += "progress { width: 100%; height: 20px; }";
  page += "</style>";

  page += "<script>";
  page += "function sendPost(url) {";
  page += "  fetch(url, { method: 'POST' })";
  page += "    .then(response => { if (response.ok) { console.log('Action successful'); } })"; // Обработка успеха (без перезагрузки)
  page += "    .catch(error => console.error('Error:', error));";
  page += "}";

  page += "document.addEventListener('DOMContentLoaded', function() {";
  page += "  const otaForm = document.querySelector('form[action=\"/update\"]');";
  page += "  if (otaForm) {";
  page += "    otaForm.addEventListener('submit', function(event) {";
  page += "      event.preventDefault();";
  page += "      const formData = new FormData(otaForm);";
  page += "      const xhr = new XMLHttpRequest();";
  page += "      xhr.open('POST', '/update', true);";
  page += "      xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest');";
  page += "      xhr.upload.onprogress = function(e) {";
  page += "        if (e.lengthComputable) {";
  page += "          const percent = Math.round((e.loaded / e.total) * 100);";
  page += "          document.getElementById('progressBar').value = percent;";
  page += "          document.getElementById('progressPercent').textContent = percent + '%';";
  page += "        }";
  page += "      };";
  page += "      xhr.onloadstart = function() {";
  page += "        document.getElementById('ota-progress').style.display = 'block';";
  page += "        setTimeout(pollForReboot, 30000);"; // 30 секунд и перезагрузка
  page += "      };";
  page += "      xhr.send(formData);";
  page += "    });";
  page += "  }";
  page += "});";

  page += "function pollForReboot() {";
  page += "  fetch('/', { credentials: 'include' })";
  page += "    .then(res => {";
  page += "      if (res.ok) {";
  page += "        location.reload();";
  page += "      } else {";
  page += "        setTimeout(pollForReboot, 1000);";
  page += "      }";
  page += "    })";
  page += "    .catch(() => setTimeout(pollForReboot, 1000));";
  page += "}";
  page += "</script>";

  page += "</head><body>";
  page += "<p><center><h3>(" + dev_uid + " v" + sw_ver + ")</h3></center></p>";
  page += "<p><center><h3>Network config</h3></center></p>";
  page += "<center><div class='container'>";
  page += "<p>Uptime: <span id='uptime'>" + String(uptimeString.getUptime2()) + "</span></p>";
  page += "<br>";
  page += "<form action='/newwifi' method='POST'>";
  page += "ssid:<br><input type='text' name='wifiname' value='" + String(FSReadWifiName()) + "'><br>";
  page += "ssid password:<br><input type='text' name='wifipwd' value='" + String(FSReadWifiPass()) + "'><br>";
  page += "<input type='submit' value='Обновить настройки сети'>";
  page += "</form>";
  page += "<form action='/newmqtt' method='POST'>";
  page += "mqtt ip:<br><input type='text' name='mqttip' value='" + String(FSReadMQTTIP()) + "'><br>";
  page += "mqtt port:<br><input type='text' name='mqttport' value='" + String(FSReadMQTTPort()) + "'><br>";
  page += "mqtt login:<br><input type='text' name='mqttlogin' value='" + String(FSReadMQTTUser()) + "'><br>";
  page += "mqtt password:<br><input type='text' name='mqttpassword' value='" + String(FSReadMQTTPass()) + "'><br>";
  page += "<input type='submit' value='Обновить настройки mqtt'>";
  page += "</form>";
  page += "<form action='/newwebpass' method='POST'>";
  page += "webUI password:<br><input type='text' name='webpass' value='" + String(FSReadWebUIPass()) + "'><br>";
  page += "<input type='submit' value='Обновить пароль WebUI'>";
  page += "</form>";
  page += "<form method='POST' action='/setDeviceCred'>";
  page += "<label for='deviceName'>Device name:</label><br>";
  page += "<input type='text' name='deviceName' value='" + String(dev_name) + "'><br>";
  page += "<label for='entName'>Entity name:</label><br>";
  page += "<input type='text' name='entName' value='" + String(entity_name) + "'><br>";
  page += "<input type='submit' value='Apply'>";
  page += "</form>";
  page += "<form action='/reboot' method='POST'>";
  page += "<input id='btn_reboot' type='submit' value='Reboot'>";
  page += "</form>";
  page += "</div></center>";
  page += "<br>";
  
  page += "<p><center><h3>Firmware</h3></center></p>";
  page += "<center><div class='container'>";
  page += "<center><p>OTA:</p>";
  page += "<form method='POST' action='/update' enctype='multipart/form-data'>";
  page += "<input type='file' name='update'><br>";
  page += "<input type='submit' value='Update'>";
  page += "<div id='ota-progress' style='display:none;'>";
  page += "<progress id='progressBar' value='0' max='100'></progress>";
  page += "</div>";
  page += "</form>";

  page += "<form action='/hardreset' method='POST'>";
  page += "<input id='btn_reset' type='submit' value='Hard reset'>";
  page += "</form></div></center>";
  page += "<br>";

  page += "<p><center><h3>Command test</h3></center></p>";
  page += "<center><div class='container'>";
  
  page += "<div class='button-container'>"; // Контейнер для кнопок в одной строке
  page += "<button id='btn_on' onclick=\"sendPost('/acon')\">AC Auto mode</button>";
  page += "<button id='btn_off' onclick=\"sendPost('/acoff')\">OFF</button>";
  page += "</div>";
  
  page += "<div class='status-container'>"; // Контейнер для состояний
  page += "<p>lib(dev): " + used_lib + "</p>";
  page += "<p>Mode: " + mode + "</p>";
  page += "<p>Temp: " + String(temperature) + "</p>";
  page += "<p>Fan: " + fan_speed + "</p>";
  page += "<p>Swing: " + String(swing) + "</p>";
  page += "<p>Turbo: " + String(boost) + "</p>";
  page += "<p>Last command: " + String((millis() - last_commandsend_time) / 1000) + "s ago</p>";
  page += "</div>";
  page += "</div></center>";

  page += "</body></html>";
  return page;
}

void publishUptime(){
  newuptime.calculateUptime();
  mqttClient.publish((dev_uid + "/uptime/state").c_str(), 0, true, String(newuptime.getTotalSeconds()/60).c_str());
}

void publishAvailability() {
  mqttClient.publish(String(dev_uid + "/status").c_str(), 0, true, "online");
}

void publishMode(){
  mqttClient.publish(String(dev_uid + "/_hvac_mode/state").c_str(), 0, true, mode.c_str());
}

void publishTemp(){
  mqttClient.publish(String(dev_uid + "/_temperature/state").c_str(), 0, true, String(temperature).c_str());
}

void publishFanSpeed(){
  mqttClient.publish(String(dev_uid + "/_fan_mode/state").c_str(), 0, true, String(fan_speed).c_str());
}

void publishSwing() {
  mqttClient.publish((dev_uid + "/_hvac_swing_mode/state").c_str(), 0, true, swing ? "on" : "off");
}

void publishBoost() {
  mqttClient.publish((dev_uid + "/_hvac_preset_mode/state").c_str(), 0, true, boost ? "boost" : "none");

}

void publishMQTTConfig() {
  String configUT = "{\"icon\": \"mdi:timer-outline\",\"availability\":{\"topic\":\"" + dev_uid + "/status\",\"payload_available\":\"online\",\"payload_not_available\":\"offline\"},\"name\": \"" + entity_name + " uptime\",\"unit_of_measurement\": \"m\",\"device\":{\"name\":\"" + dev_name + "\",\"model\":\"" + dev_model + "\",\"manufacturer\":\"Dmitrii Nechaevskii\",\"sw_version\":\"" + sw_ver + "\",\"identifiers\":\"" + dev_uid + "\",\"connections\":[[\"mac\",\"" + FSReadMAC() + "\", \"ip\",\"" + WiFi.localIP().toString() + "\"]]},\"unique_id\": \"" + dev_uid + "_uptime\",\"device_class\": \"duration\",\"state_topic\": \"~state\",\"state_class\": \"total_increasing\",\"entity_category\": \"diagnostic\",\"~\":\"" + dev_uid + "/uptime/\"}";

  String configReboot = "{\"availability\":{\"topic\":\"" + dev_uid + "/status\",\"payload_available\":\"online\",\"payload_not_available\":\"offline\"},\"name\": \"" + entity_name + " reboot\",\"device\":{\"name\":\"" + dev_name + "\",\"model\":\"" + dev_model + "\",\"manufacturer\":\"Dmitrii Nechaevskii\",\"sw_version\":\"" + sw_ver + "\",\"identifiers\":\"" + dev_uid + "\",\"connections\":[[\"mac\",\"" + FSReadMAC() + "\", \"ip\",\"" + WiFi.localIP().toString() + "\"]]},\"unique_id\": \"" + dev_uid + "_reboot\",\"command_topic\": \"~command\",\"payload_press\": \"restart\",\"entity_category\": \"diagnostic\",\"~\":\"" + dev_uid + "/reboot/\"}";

  String config;
  config += "{";
  config += "\"name\": \"" + entity_name + "\",";
  config += "\"send_if_off\": true,";
  config += "\"unique_id\": \"" + dev_uid + "\",";
  config += "\"availability\": {";
  config += "\"topic\": \"" + dev_uid + "/status\",";
  config += "\"payload_available\": \"online\",";
  config += "\"payload_not_available\": \"offline\",";
  config += "\"value_template\": \"{{ value_json.status }}\"";
  config += "},";
  config += "\"current_temperature_topic\": \"" + dev_uid + "/_inside_temperature/state\",";
  config += "\"fan_mode_command_topic\": \"" + dev_uid + "/_fan_mode/set\",";
  config += "\"fan_mode_state_topic\": \"" + dev_uid + "/_fan_mode/state\",";
  config += "\"fan_modes\": [";
  config += "\"auto\",";
  config += "\"low\",";
  config += "\"medium\",";
  config += "\"high\"";
  config += "],";
  config += "\"mode_command_topic\": \"" + dev_uid + "/_hvac_mode/set\",";
  config += "\"mode_state_topic\": \"" + dev_uid + "/_hvac_mode/state\",";
  config += "\"modes\": [";
  config += "\"auto\",";
  config += "\"cool\",";
  config += "\"heat\",";
  config += "\"dry\",";
  config += "\"fan_only\",";
  config += "\"off\"";
  config += "],";

  config += "\"preset_mode_command_topic\": \"" + dev_uid + "/_hvac_preset_mode/set\",";
  config += "\"preset_mode_state_topic\": \"" + dev_uid + "/_hvac_preset_mode/state\",";
  config += "\"preset_modes\": [";
  config += "\"boost\"";
  config += "],";

  config += "\"swing_mode_command_topic\": \"" + dev_uid + "/_hvac_swing_mode/set\",";
  config += "\"swing_mode_state_topic\": \"" + dev_uid + "/_hvac_swing_mode/state\",";
  config += "\"swing_modes\": [";
  config += "\"on\",";
  config += "\"off\"";
  config += "],";

  config += "\"min_temp\": 16,";
  config += "\"max_temp\": 32,";
  config += "\"precision\": 1,";
  config += "\"retain\": true,";
  config += "\"temperature_command_topic\": \"" + dev_uid + "/_temperature/set\",";
  config += "\"temperature_state_topic\": \"" + dev_uid + "/_temperature/state\",";
  config += "\"temp_step\": 1,";
  config += "\"device\": {";
  config += "\"manufacturer\": \"Dmitrii Nechaevskii\",";
  config += "\"identifiers\": \"" + dev_uid + "\",";
  config += "\"name\": \"" + dev_name + "\",";
  config += "\"model\": \"" + dev_model + "\",";
  config += "\"sw_version\": \"" + sw_ver + "\",";
  config += "\"connections\": [[";
  config += "\"mac\",\"" + FSReadMAC() + "\",";
  config += "\"ip\",\"" + WiFi.localIP().toString() + "\"";
  config += "]]";
  config += "}";
  config += "}";

  mqttClient.publish(config_topic.c_str(), 0, true, config.c_str());
  boot_delay = millis();
  
  mqttClient.publish(config_topicUT.c_str(), 0, true, configUT.c_str());
  Serial.println("Config UT Published: " + config_topicUT);
  mqttClient.publish(config_topicReboot.c_str(), 0, true, configReboot.c_str());
  Serial.println("Config Reboot Published: " + config_topicReboot);

  publishUptime();
  publishAvailability();
  
  // publishTemp();
  // publishMode();
  // publishFanSpeed();
  
  sendDelayStart = millis();
  sendDelayActive = true;
  publishFlag = true;
  Serial.println("State on boot Published");
}


void ac_command(){

  ac.setTemp(temperature);
  ac.setTurbo(boost);
  ac.setSwingV(swing);
  ac.setSwingH(swing);
  
  Serial.println("temp." + String(temperature));

  if (mode == "off"){
    ac.off();
    Serial.println("mode.off");
  }
  else if (mode == "cool"){
    ac.setPower(true);
    ac.setMode(kElectraAcCool);
    Serial.println("mode.cool");
  }
  else if (mode == "heat"){
    ac.setPower(true);
    ac.setMode(kElectraAcHeat);
    Serial.println("mode.heat");
  }
  else if (mode == "auto"){
    ac.setPower(true);
    ac.setMode(kElectraAcAuto);
    Serial.println("mode.auto");
  }
  else if (mode == "dry"){
    ac.setPower(true);
    ac.setMode(kElectraAcDry);
    Serial.println("mode.dry");
  }
  else if (mode == "fan_only"){
    ac.setPower(true);
    ac.setMode(kElectraAcFan);
    Serial.println("mode.fan_only");
  }

  if (fan_speed == "low"){
      ac.setFan(kElectraAcFanLow);
      Serial.println("fan.low");
  }
  else if (fan_speed == "medium"){
      ac.setFan(kElectraAcFanMed);
      Serial.println("fan.med");
  }
  else if (fan_speed == "high"){
      ac.setFan(kElectraAcFanHigh);
      Serial.println("fan.hgh");
  }
  else if (fan_speed == "auto"){
      ac.setFan(kElectraAcFanAuto);
      Serial.println("fan.auto");
  }

  sendDelayStart = millis();
  sendDelayActive = true;
  publishFlag = true;
  if (millis() - boot_delay > 5000){
    ac.send();
  }
  Serial.println("----------");
}

void testDefaultValues(){
  if (FSReadTemp() > 0) temperature = FSReadTemp(); else temperature = 22;
  if (FSReadMode() != "") mode = FSReadMode(); else mode = "auto";
  if (FSReadFanSpeed() != "") fan_speed = FSReadFanSpeed(); else fan_speed = "auto";
}

String getFormattedMAC() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  String formattedMAC = "";
  for (int i = 0; i < 6; ++i) {
    if (mac[i] < 16) formattedMAC += '0';
    formattedMAC += String(mac[i], HEX);
  }
  formattedMAC.toLowerCase();
  return formattedMAC;
}

void connectToWifi() {
  if (config_present) {
    if (wifi_attempts < wifi_attempts_max) {
      WiFi.mode(WIFI_STA);
      Serial.println("Connecting to Wi-Fi: " + FSReadWifiName());
      WiFi.begin(FSReadWifiName().c_str(), FSReadWifiPass().c_str());
      wifi_attempts++;
    } else {
      if (!ap_started) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(dev_uid.c_str());
        Serial.println("Hotspot started: " + dev_uid);
        wifi_attempts = 0;
        wifi_reconnect_timer = millis();
        ap_started = true;
      }
    }
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(dev_uid.c_str());
    Serial.println("Hotspot started: " + dev_uid);
    ap_started = true;
  }
}

void reconnectMQTT() {
  if (!WiFi.isConnected()) {
    Serial.println("WiFi not connected, skipping MQTT reconnect.");
    return;
  }

  String mqttHost = String(FSReadMQTTIP());
  uint16_t mqttPort = FSReadMQTTPort();
  Serial.println("Reconnecting to MQTT...");
  Serial.println("MQTT Host: " + mqttHost);
  Serial.println("MQTT Port: " + String(mqttPort));

  if (mqttHost.length() == 0 || mqttPort == 0) {
    Serial.println("Invalid MQTT settings: Host or Port empty/invalid.");
    return;
  }

  IPAddress mqttIP;
  if (mqttIP.fromString(mqttHost)) {
    Serial.println("MQTT IP resolved to: " + mqttIP.toString());
    mqttClient.setServer(mqttIP, mqttPort);
  } else {
    Serial.println("Attempting DNS resolution for: " + mqttHost);
    if (WiFi.hostByName(mqttHost.c_str(), mqttIP)) {
      Serial.println("MQTT IP resolved to: " + mqttIP.toString());
      mqttClient.setServer(mqttIP, mqttPort);
    } else {
      Serial.println("DNS resolution failed for: " + mqttHost);
      return;
    }
  }

  const char* mqttUser = FSReadMQTTUser();
  const char* mqttPass = FSReadMQTTPass();
  Serial.print("MQTT User (raw): '"); Serial.print(mqttUser); Serial.println("'");
  Serial.print("MQTT Pass (raw): '"); Serial.print(mqttPass); Serial.println("'");
  Serial.println("MQTT User length: " + String(strlen(mqttUser)));
  Serial.println("MQTT Pass length: " + String(strlen(mqttPass)));
  Serial.print("MQTT User bytes: ");
  for (size_t i = 0; i < strlen(mqttUser); i++) {
    Serial.print((uint8_t)mqttUser[i], HEX); Serial.print(" ");
  }
  Serial.println();
  Serial.print("MQTT Pass bytes: ");
  for (size_t i = 0; i < strlen(mqttPass); i++) {
    Serial.print((uint8_t)mqttPass[i], HEX); Serial.print(" ");
  }
  Serial.println();

  if (strlen(mqttUser) == 0 || strlen(mqttPass) == 0 || strlen(mqttUser) > 19 || strlen(mqttPass) > 19) {
    Serial.println("Invalid MQTT credentials, using default: local, aYCMeT");
    mqttClient.setCredentials("local", "aYCMeT");
  } else {
    Serial.println("Setting MQTT credentials: " + String(mqttUser) + ", " + String(mqttPass));
    mqttClient.setCredentials(mqttUser, mqttPass);
  }

  mqttClient.setClientId(clientId.c_str());
  mqttClient.setKeepAlive(60);
  
  mqttClient.connect();
  Serial.println("MQTT connect initiated.");
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
      wifi_attempts = 0;
      Serial.println("Connected to Wi-Fi: " + WiFi.localIP().toString());
      // delay(5000);
      // if (FSReadMQTTIP() != "" && FSReadMQTTPort() != 0) {
      if (FSReadMQTTIP() && FSReadMQTTPort() != 0) {
        reconnectMQTT();
      } else {
        Serial.println("Invalid MQTT settings: IP or Port empty/invalid.");
      }
      lastMqttReconnect = millis();
      reconnectMQTT_allow = true;
    }
void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
      
      Serial.println("Disconnected from Wi-Fi.");
      connectToWifi();
}

void onMqttConnect(bool sessionPresent) {
  
  uint16_t packetIdSub0 = mqttClient.subscribe(String(dev_uid + "/_pause").c_str(), 0); // QoS 0
  Serial.print("Subscribing at QoS 0, packetId: ");
  Serial.println(packetIdSub0);
  uint16_t packetIdSub1 = mqttClient.subscribe(String(dev_uid + "/_hvac_mode/set").c_str(), 0); // QoS 0
  Serial.print("Subscribing at QoS 0, packetId: ");
  Serial.println(packetIdSub1);
  uint16_t packetIdSub2 = mqttClient.subscribe(String(dev_uid + "/_temperature/set").c_str(), 0);
  Serial.print("Subscribing at QoS 0, packetId: ");
  Serial.println(packetIdSub2);
  uint16_t packetIdSub3 = mqttClient.subscribe(String(dev_uid + "/_fan_mode/set").c_str(), 0);
  Serial.print("Subscribing at QoS 0, packetId: ");
  Serial.println(packetIdSub3);
  uint16_t packetIdSub4 = mqttClient.subscribe(String(dev_uid + "/_hvac_preset_mode/set").c_str(), 0);
  Serial.print("Subscribing at QoS 0, packetId: ");
  Serial.println(packetIdSub4);
  uint16_t packetIdSub5 = mqttClient.subscribe(String(dev_uid + "/_hvac_swing_mode/set").c_str(), 0);
  Serial.print("Subscribing at QoS 0, packetId: ");
  Serial.println(packetIdSub5);
  uint16_t packetIdSub6 = mqttClient.subscribe(String(dev_uid + "/reboot/command").c_str(), 0);
  Serial.print("Subscribing at QoS 0, packetId: ");
  Serial.println(packetIdSub6);
  
  publishMQTTConfig();
  
  mqttClient.publish(String(dev_uid + "/status").c_str(), 0, false, "online");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT. Reason: " + String((uint8_t)reason));
  switch (reason) {
    case AsyncMqttClientDisconnectReason::TCP_DISCONNECTED:
      Serial.println("TCP Disconnected. Check network or server availability.");
      break;
    case AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION:
      Serial.println("MQTT Protocol Version not supported.");
      break;
    case AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED:
      Serial.println("Client ID rejected. Check Client ID.");
      break;
    case AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE:
      Serial.println("MQTT Server unavailable.");
      break;
    case AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS:
      Serial.println("Authentication failed. Malformed credentials.");
      break;
    case AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED:
      Serial.println("Authentication failed. Check username/password.");
      break;
    default:
      Serial.println("Unknown MQTT disconnect reason.");
      break;
  }
}

void onMqttMessage(char *in_topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {

  if (String(in_topic) == dev_uid + "/_hvac_mode/set") {
    String messageMQTT;
    for (size_t i = 0; i < len; i++) {
      messageMQTT += (char)payload[i];
    }
    if (messageMQTT == "off"){
      mode = "off";
    }
    else if (messageMQTT == "cool") {
      mode = "cool";
    }
    else if (messageMQTT == "heat") {
      mode = "heat";
    }
    else if (messageMQTT == "auto") {
      mode = "auto";
    }
    else if (messageMQTT == "dry") {
      mode = "dry";
    }
    else if (messageMQTT == "fan_only") {
      mode = "fan_only";
    }
  }

  if (String(in_topic) == dev_uid + "/_fan_mode/set") {
    String messageMQTT;
    for (size_t i = 0; i < len; i++) {
      messageMQTT += (char)payload[i];
    }
    if (messageMQTT == "low"){
      fan_speed = "low";
    }
    else if (messageMQTT == "medium") {
      fan_speed = "medium";
    }
    else if (messageMQTT == "high") {
      fan_speed = "high";
    }
    else if (messageMQTT == "auto") {
      fan_speed = "auto";
    }
  }

  if (String(in_topic) == dev_uid + "/_temperature/set") {
    String messageMQTT;
    for (size_t i = 0; i < len; i++) {
      messageMQTT += (char)payload[i];
    }
    temperature = messageMQTT.toInt();
  }

  if (String(in_topic) == dev_uid + "/_hvac_preset_mode/set") {
    String messageMQTT;
    for (size_t i = 0; i < len; i++) {
      messageMQTT += (char)payload[i];
    }
    if (messageMQTT == "boost"){
      boost = true;
    }
    if (messageMQTT == "none"){
      boost = false;
    }
  }

  if (String(in_topic) == dev_uid + "/_hvac_swing_mode/set") {
    String messageMQTT;
    for (size_t i = 0; i < len; i++) {
      messageMQTT += (char)payload[i];
    }
    if (messageMQTT == "on"){
      swing = true;
    }
    if (messageMQTT == "off"){
      swing = false;
    }
  }

  ac_command();
  
  if (String(in_topic) == dev_uid + "/reboot/command") {
    String messageMQTT;
    for (size_t i = 0; i < len; i++) {
      messageMQTT += (char)payload[i];
    }
    if (messageMQTT == "restart"){
      Serial.println("reboot mqtt message accepted");
      flag_time = millis();
      restart_flag = true;
    }
  }

}

void startServer() {
  server.on("/", HTTP_GET, []() {
    if (isAuthenticated()) {
      server.send(200, "text/html", serverIndex());
    } else {
      server.send(200, "text/html", loginPage());
    }
  });

  server.on("/login", HTTP_POST, []() {
    if (server.hasArg("password")) {
      String password = server.arg("password");
      if (password == SERPASS) {
        server.sendHeader("Location", "/");
        server.sendHeader("Cache-Control", "no-cache");
        server.sendHeader("Set-Cookie", "key=" + _rndcookie);
        server.send(301);
      } else {
        server.send(401, "text/plain", "Unauthorized");
      }
    }
  });

  server.on("/setDeviceCred", HTTP_POST, []() {
    if (!isAuthenticated()) {
      server.send(403, "text/plain", "Not Authorized");
      return;
    }
    if (server.hasArg("deviceName") && server.hasArg("entName")) {
      dev_name = server.arg("deviceName");
      FSWriteDeviceName(dev_name);
      entity_name = server.arg("entName");
      FSWriteEntityName(entity_name);
      if (config_present) {
        flag_time = millis();
        restart_flag = true;
        server.send(200, "text/plain", "MQTT updated, reboot in 10sec...");
      } else {
        server.send(200, "text/html", "<html><body><center><h1>MQTT configuration updated!</h1><br>No Wifi configuration found, reboot not required<br><a href='/'>Back</a></center></body></html>");
      }
    } else {
      server.send(400, "text/plain", "Bad Request: Name or Entity not provided");
    }
  });

  server.on("/newwifi", HTTP_POST, []() {
    if (!isAuthenticated()) {
      server.send(403, "text/plain", "Not Authorized");
      return;
    }
    if (server.hasArg("wifiname") && server.hasArg("wifipwd")) {
      String wifiname = server.arg("wifiname");
      String wifipwd = server.arg("wifipwd");
      if (wifiname.length() == 0 || wifiname.length() > 33 || wifipwd.length() < 8 || wifipwd.length() > 15) {
        server.send(403, "text/plain", "Wrong wifi config, some fields are wrong");
        return;
      }
      FSWriteWifiName(wifiname);
      FSWriteWifiPass(wifipwd);
      flag_time = millis();
      restart_flag = true;
      server.send(403, "text/plain", "Wifi config changed, reboot in 10sec...");
    } else {
      server.send(403, "text/plain", "Check required fields!");
    }
  });

  server.on("/newmqtt", HTTP_POST, []() {
    if (!isAuthenticated()) {
      server.send(403, "text/plain", "Not Authorized");
      return;
    }
    if (server.hasArg("mqttip") && server.hasArg("mqttport")) {
      String mqttip = server.arg("mqttip");
      String mqttport = server.arg("mqttport");
      String mqttlogin = server.hasArg("mqttlogin") ? server.arg("mqttlogin") : "";
      String mqttpass = server.hasArg("mqttpassword") ? server.arg("mqttpassword") : "";

      if (mqttip.length() == 0 || mqttport.length() == 0) {
        server.send(403, "text/plain", "Wrong mqtt config, IP or port empty");
        return;
      }
      if (mqttip.length() > 79 || mqttlogin.length() > 19 || mqttpass.length() > 19) {
        server.send(403, "text/plain", "MQTT IP (>79 chars) or login/password (>19 chars) too long");
        return;
      }

      Serial.println("Updating MQTT settings:");
      Serial.println("IP: " + mqttip);
      Serial.println("Port: " + mqttport);
      Serial.println("Login: " + mqttlogin);
      Serial.println("Password: " + mqttpass);

      FSWriteMQTTIP(mqttip.c_str());
      FSWriteMQTTPort(mqttport.toInt());
      FSWriteMQTTUser(mqttlogin.c_str());
      FSWriteMQTTPass(mqttpass.c_str());

      if (config_present) {
        flag_time = millis();
        restart_flag = true;
        server.send(200, "text/plain", "MQTT updated, reboot in 10sec...");
      } else {
        server.send(200, "text/html", "<html><body><center><h1>MQTT configuration updated!</h1><br>No Wifi configuration found, reboot not required<br><a href='/'>Back</a></center></body></html>");
      }
    } else {
      server.send(400, "text/plain", "Bad Request: MQTT fields missing");
    }
  });

  server.on("/newwebpass", HTTP_POST, []() {
    if (!isAuthenticated()) {
      server.send(403, "text/plain", "Not Authorized");
      return;
    }
    if (server.hasArg("webpass")) {
      String webpass = server.arg("webpass");
      if (webpass.length() < 4 || webpass.length() > 10) {
        server.send(403, "text/plain", "Wrong webpass config, >3 and <10");
        return;
      }
      FSWriteWebUIPass(webpass);
      SERPASS = webpass;
      InitCookie();
      server.send(403, "text/plain", "Webpass changed, login again");
    } else {
      server.send(400, "text/plain", "Bad Request: webpass not provided");
    }
  });

  server.on("/hardreset", HTTP_POST, []() {
    if (!isAuthenticated()) {
      server.send(403, "text/plain", "Not Authorized");
      return;
    }
    LittleFS.format();
    flag_time = millis();
    restart_flag = true;
    server.send(403, "text/plain", "Full config reset, memory cleaned, reboot in 10sec...");
  });

  server.on("/reboot", HTTP_POST, []() {
    if (!isAuthenticated()) {
      server.send(403, "text/plain", "Not Authorized");
      return;
    }
    flag_time = millis();
    restart_flag = true;
    server.send(403, "text/plain", "Reboot in 10sec...");
  });

  server.on("/acon", HTTP_POST, []() {
    if (!isAuthenticated()) {
      server.send(403, "text/plain", "Not Authorized");
      return;
    }
    mode = "auto";
    ac_command();
    server.send(200, "text/plain", "OK"); // Пустой ответ для AJAX
  });
  
  server.on("/acoff", HTTP_POST, []() {
    if (!isAuthenticated()) {
      server.send(403, "text/plain", "Not Authorized");
      return;
    }
    mode = "off";
    ac_command();
    server.send(200, "text/plain", "OK"); // Пустой ответ для AJAX
  });

  // Настройка маршрута для обновления
//   server.on("/update", HTTP_POST, []() {
//   if (!isAuthenticated(request)) {
//     server.send(403, "text/plain", "Not Authorized");
//     return;
//   }
//   server.send(200, "text/plain", "Update started, wait for completion...");
// }, [](, String filename, size_t index, uint8_t *data, size_t len, bool final) {
//   if (!index) {
//     Serial.printf("Update Start: %s\n", filename.c_str());
//     updateFinished = false;
//     updateSuccess = false;
//     if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
//       Update.printError(Serial);
//       return;
//     }
//   }

//   if (Update.write(data, len) != len) {
//     Update.printError(Serial);
//     return;
//   }

//   if (final) {
//     if (Update.end(true)) {
//       Serial.printf("Update Success: %u\n", index + len);
//       updateSuccess = true;
//     } else {
//       Update.printError(Serial);
//       updateSuccess = false;
//     }
//     updateFinished = true;

//     AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", updateSuccess ? "OK" : "FAIL");
//     server.sendHeader("Connection", "close");
//     server.send(response);

//     // Увеличенная задержка для надежной отправки ответа
//     delay(1000); // 1 секунда
//     ESP.restart();
//   }
// });

  InitCookie();
  server.begin();
  Serial.println("AsyncWebServer started");
}

void setup() {
  // psramInit();
  Serial.begin(kBaudRate);
  // delay(1000);
  Serial.println("Starting ESP32-S3 setup...");

  // Проверка PSRAM - Диагностика
  // Serial.printf("Total PSRAM: %d bytes\n", ESP.getPsramSize());
  // Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());

  // Инициализация LittleFS
  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Failed, Formatting...");
    LittleFS.format();
    if (!LittleFS.begin()) {
      Serial.println("LittleFS Mount Failed after formatting!");
      while (true);
    }
  }
  Serial.println("LittleFS Mounted Successfully");

  // Чтение данных
  FDstat_t stat = data.read();
  Serial.println("Data read status: " + String(stat));
  FSDataRead();

  // uint32_t timestamp = millis();
  if (String(FSReadMAC()).length() < 12){
    WiFi.mode(WIFI_STA);
    WiFi.begin("init", "12345678");
    delay(3000);
    FSWriteMAC(getFormattedMAC());
    Serial.println("MAC added to memory, reboot in 10 sec..");
    // while (millis() - timestamp < 10000){
    //   if (data.tick() == FD_WRITE) {
    //     Serial.println("Data updated!");
    //     FSDataRead();
    //   }
    // }
    data.updateNow();
    delay(1500);
    Serial.println("Restart");
    ESP.restart();
  }

  // Инициализация переменных
  dev_uid = "ne4d_ac_" + FSReadMAC();
  dev_model = "HVAC Control: " + dev_uid;
  clientId = "ac_" + dev_uid;
  config_topicUT = "homeassistant/sensor/" + dev_uid + "/_ut/config";
  config_topicReboot = "homeassistant/button/" + dev_uid + "/_reboot/config";
  
  config_topic = "homeassistant/climate/" + dev_uid + "/config";
  
  SERPASS = FSReadWebUIPass() == "" ? "admin" : FSReadWebUIPass();
  wifi_search_delay_s = wifi_search_delay_m * 60 * 1000;

  testDefaultValues();

  // Инициализация пинов
  pinMode(configPin, INPUT_PULLUP);
  dev_name = FSReadDeviceName();
  entity_name = FSReadEntityName();

  // Настройка MQTT
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setMaxTopicLength(1024);
  // SUPERIMPORTANT!!!
  static String willTopic = dev_uid + "/status";
  mqttClient.setWill(willTopic.c_str(), 0, true, "offline");
  mqttClient.setKeepAlive(30);
  // WiFi.onEvent(onWifiEvent);
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  
  // Инициализация Wi-Fi
  WiFi.setHostname(clientId.c_str());
  // if (FSReadWifiName() != "" && FSReadWifiPass() != "" && digitalRead(configPin)) {
  //   config_present = true;
  //   connectToWifi();
  //   unsigned long wifi_timeout = millis();
  //   while (WiFi.status() != WL_CONNECTED && millis() - wifi_timeout < 10000) {
  //     delay(1000);
  //     Serial.print(".");
  //   }
  //   if (WiFi.status() == WL_CONNECTED) {
  //     Serial.println("WiFi connected: " + WiFi.localIP().toString());
  //   } else {
  //     Serial.println("WiFi connection failed!");
  //   }
  // } else {
  //   Serial.println("No WiFi credentials, starting AP...");
  //   WiFi.mode(WIFI_AP);
  //   WiFi.softAP(dev_uid.c_str());
  //   ap_started = true;
  //   Serial.println("AP started: " + dev_uid);
  // }
  
  ac.begin();
  // AUX defaults
  ac.setSwingV(false);
  ac.setSwingH(false);
  ac.setLightToggle(false);
  ac.setClean(false);
  ac.setIFeel(false);
  ac.setTurbo(true);
}

void loop() {

  // _uptime = uptimeString.getUptime2();

  uint64_t now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    publishUptime();
  }
  
  if (WiFi.status() == WL_CONNECTED && !mqttClient.connected() && (now - lastMqttReconnect > MQTT_RECONNECT_INTERVAL && reconnectMQTT_allow) && !restart_flag) {
    lastMqttReconnect = now;
    reconnectMQTT();
  }

  if (data.tick() == FD_WRITE) {
    Serial.println("Data updated!");
    FSDataRead();
  }

  if (restart_flag && (millis() - flag_time > 10000)) {
    ESP.restart();
  }

  if (config_present && ap_started && (millis() - wifi_reconnect_timer > wifi_search_delay_s)) {
    Serial.println("Trying to connect to stored WiFi...");
    ap_started = false;
    connectToWifi();
  }
  
  if (sendDelayActive) {
    if (now - sendDelayStart >= 500) { //задержка перед ответом
      sendDelayActive = false;
    }
  }

  if (WiFi.status() == WL_CONNECTED){
  Serial.println("Starting server...");
  startServer();

    if (sendDelayActive && publishFlag && mqttClient.connected()){
      publishMode();
      publishTemp();
      publishFanSpeed();
      publishSwing();
      publishBoost();
      FSWriteMode(mode);
      FSWriteTemp(temperature);
      FSWriteFanSpeed(fan_speed);
      FSWriteTurbo(boost);
      FSWriteSwing(swing);
      publishFlag = false;
      last_commandsend_time = millis();
    }
  }
  
  // delay(10);
}