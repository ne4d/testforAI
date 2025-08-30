#include "server.h"
#define MAX_UID 8

String _rndcookie;

ESP8266WebServer server(80);

String generateUID()
{
  const char possible[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  String uid = "";
  for (int i = 0; i < MAX_UID; i++)
  {
    int r = random(0, strlen(possible));
    uid += possible[r];
  }
  return uid;
}

bool isAuthenticated()
{
  if (server.hasHeader("Cookie"))
  {
    String cookie = server.header("Cookie");
    return (cookie.equals("key=" + _rndcookie)); // ok3
  }
  return false;
}

void InitCookie()
{
  _rndcookie = generateUID();
}

void startServer()
{
  server.on("/", HTTP_GET, []()
            {
    if (isAuthenticated()) {
      server.send(200, "text/html", serverIndex());
    } else {
      server.send(200, "text/html", loginPage());
    } });

  server.on("/login", HTTP_POST, []()
            {
    if (server.hasArg("password")) {
      String password = server.arg("password");
      // if (password == (("web_pw") == "" ? "admin" : FSReadJsonString("web_pw"))) {
      if (password == (FSReadJsonString("web_pw") == "" ? "admin" : FSReadJsonString("web_pw"))) {
        server.sendHeader("Location", "/");
        server.sendHeader("Cache-Control", "no-cache");
        server.sendHeader("Set-Cookie", "key=" + _rndcookie);
        server.send(301);
      } else {
        server.send(401, "text/plain", "Unauthorized");
      }
    } });

  server.on("/setDeviceCred", HTTP_POST, []()
            {
    if (!isAuthenticated()) {
      server.send(403, "text/plain", "Not Authorized");
      return;
    }
    if (server.hasArg("entName")) {
      FSWriteJsonString("entity_name", server.arg("entName"));
      if (FSReadJsonString("ssid") != "" && FSReadJsonString("ssid_pw") != "") {
        server.send(200, "text/plain", "MQTT updated, reboot in 5sec...");
        rebootLoop(true);
      } else {
        server.send(200, "text/html", "<html><body><center><h1>MQTT configuration updated!</h1><br>No Wifi configuration found, reboot not required<br><a href='/'>Back</a></center></body></html>");
      }
    } else {
      server.send(400, "text/plain", "Bad Request: Entity not provided");
    } });

  server.on("/newwifi", HTTP_POST, []()
            {
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
      FSWriteJsonString("ssid", wifiname);
      FSWriteJsonString("ssid_pw", wifipwd);
    
      rebootLoop(true);
      server.send(403, "text/plain", "Wifi config changed, reboot in 5sec...");
    } else {
      server.send(403, "text/plain", "Check required fields!");
    } });

  server.on("/newmqtt", HTTP_POST, []()
            {
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

      FSWriteJsonString("mqtt_server", mqttip);
      FSWriteJsonUint64("mqtt_port", mqttport.toInt());
      FSWriteJsonString("mqtt_un", mqttlogin);
      FSWriteJsonString("mqtt_pw", mqttpass);

      if (FSReadJsonString("ssid") != "" && FSReadJsonString("ssid_pw") != "") {
        rebootLoop(true);
        server.send(200, "text/plain", "MQTT updated, reboot in 5sec...");
      } else {
        server.send(200, "text/html", "<html><body><center><h1>MQTT configuration updated!</h1><br>No Wifi configuration found, reboot not required<br><a href='/'>Back</a></center></body></html>");
      }
    } else {
      server.send(400, "text/plain", "Bad Request: MQTT fields missing");
    } });

  server.on("/mqttInfo", HTTP_POST, []()
            {
    if (!isAuthenticated()) {
      server.send(403, "text/plain", "Not Authorized");
      return;
    }
    server.send(200, "text/html", mqttTopicList + "<center><a href='/'>Back</a></center></body></html>"); });

  server.on("/newwebpass", HTTP_POST, []()
            {
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
      FSWriteJsonString("web_pw", webpass);
      InitCookie();
      server.send(403, "text/plain", "Webpass changed, login again");
    } else {
      server.send(400, "text/plain", "Bad Request: webpass not provided");
    } });

  server.on("/ch1on", HTTP_POST, []()
            {
              if (!isAuthenticated())
              {
                server.send(403, "text/plain", "Not Authorized");
                return;
              }

    // mode = "auto";
    // ac_command();
    server.send(200, "text/plain", "OK"); // Пустой ответ для AJAX
            });

  server.on("/ch1off", HTTP_POST, []()
            {
              if (!isAuthenticated())
              {
                server.send(403, "text/plain", "Not Authorized");
                return;
              }

    // mode = "off";
    // ac_command();
              server.send(200, "text/plain", "OK"); // Пустой ответ для AJAX
            });

  server.on("/hardreset", HTTP_POST, []()
            {
    if (!isAuthenticated()) {
      server.send(403, "text/plain", "Not Authorized");
      return;
    }
    LittleFS.format();
    rebootLoop(true);
    server.send(403, "text/plain", "Full config reset, memory cleaned, reboot in 5sec..."); });

  server.on("/reboot", HTTP_POST, []()
            {
    if (!isAuthenticated()) {
      server.send(403, "text/plain", "Not Authorized");
      return;
    }
    rebootLoop(true);
    server.send(403, "text/plain", "Reboot in 5sec..."); });

  // Настройка маршрута для обновления

  server.on("/update", HTTP_POST, []() {
    if (!isAuthenticated()) {
      server.send(403, "text/plain", "Not Authorized");
      return;
    }
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.setDebugOutput(true);
      WiFiUDP::stopAll();
      Serial.printf("Update: %s\n", upload.filename.c_str());
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        server.send(403, "text/plain", "Update success, REbooting...");
      } else {
        Update.printError(Serial);
      }
      Serial.setDebugOutput(false);
    }
    yield();
  });

  InitCookie();
  server.begin();
  Serial.println("AsyncWebServer started");
}