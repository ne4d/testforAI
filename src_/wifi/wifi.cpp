#include "wifi.h"

String getFormattedMAC()
{
  uint8_t mac[6];
  WiFi.macAddress(mac);
  String formattedMAC = "";
  for (int i = 0; i < 6; ++i)
  {
    if (mac[i] < 16)
      formattedMAC += '0';
    formattedMAC += String(mac[i], HEX);
  }
  formattedMAC.toLowerCase();
  return formattedMAC;
}

// void connectToWifi()
// {
//   wifi_connection_initiation = true;
//   uint8_t attempt = 1;
//   while (attempt < wifi_attempts)
//   {
//     WiFi.mode(WIFI_STA);
//     Serial.println("Connecting to SSID \"" + FSReadJsonString("ssid") + "\" attempt " + String(attempt));
//     WiFi.begin(FSReadJsonString("ssid").c_str(), FSReadJsonString("ssid_pw").c_str());
//     uint32_t wifi_timeout = secondsRunned();
//     while (WiFi.status() != WL_CONNECTED && secondsRunned() - wifi_timeout < 10)
//     {
//       delay(1000);
//       Serial.print(".");
//     }
//     if (WiFi.status() == WL_CONNECTED)
//     {
//       wifi_connection_state = true;
//       return;
//     }
//     else
//     {
//       attempt++;
//       WiFi.disconnect();
//     }
//   }
// }
void webServerStart(){
  static bool web_server_running = false;
  if (!web_server_running)
  {
    Serial.println("Starting web server...");
    startServer();
    web_server_running = true;
  }
}

void connectToWifi() {
  Serial.println("Connecting to SSID " + FSReadJsonString("ssid"));
  WiFi.mode(WIFI_STA);
  WiFi.begin(FSReadJsonString("ssid").c_str(), FSReadJsonString("ssid_pw").c_str());
}

void startAP()
{
  wifi_ap_running = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(FSReadJsonString("dev_uid").c_str());
  Serial.println("Hotspot started: \"" + FSReadJsonString("dev_uid") + "\"");
  webServerStart();
}

// void onWifiEvent(WiFiEvent_t event)
// {
//   switch (event)
//   {
//   case WIFI_EVENT_STAMODE_GOT_IP:
//     Serial.println("Connected to Wi-Fi: " + WiFi.localIP().toString());
//     break;
//   case WIFI_EVENT_STAMODE_DISCONNECTED:
//     wifi_connection_state = false;
//     break;
//   default:
//     break;
//   }
// }

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi: " + WiFi.localIP().toString());
  // wifi_connection_state = true;
  webServerStart();
  connectMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  static uint8_t attempts = 0;
  Serial.println("Disconnected from Wi-Fi.");
  // wifi_connection_state = false;
  // mqtt_connection_state = false;
  // mqtt_config_published = false;
  // mqtt_availability_published = false;
  if (attempts <= wifi_attempts){
    wifiReconnectTimer.once(2, connectToWifi);
    mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
    attempts ++;
  } else {
    wifiReconnectTimer.detach();
    startAP();
  }
}