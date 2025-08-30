#include "server.h"
#define MAX_UID 8

String _rndcookie;

AsyncWebServer server(80);

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

bool isAuthenticated(AsyncWebServerRequest *request)
{
  if (request->hasHeader("Cookie"))
  {
    String cookie = request->header("Cookie");
    return cookie.equals("key=" + _rndcookie);
  }
  return false;
}

void InitCookie()
{
  _rndcookie = generateUID();
}

void startServer()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (isAuthenticated(request)) {
      request->send(200, "text/html", serverIndex());
    } else {
      request->send(200, "text/html", loginPage());
    } });

  server.on("/login", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    if (request->hasParam("password", true)) {
      String password = request->getParam("password", true)->value();
      // if (password == (("web_pw") == "" ? "admin" : FSReadJsonString("web_pw"))) {
      if (password == (FSReadJsonString("web_pw") == "" ? "admin" : FSReadJsonString("web_pw"))) {
        AsyncWebServerResponse *response = request->beginResponse(301);
        response->addHeader("Location", "/");
        response->addHeader("Cache-Control", "no-cache");
        response->addHeader("Set-Cookie", "key=" + _rndcookie);
        request->send(response);
      } else {
        request->send(401, "text/plain", "Unauthorized");
      }
    } });

  server.on("/setDeviceCred", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    if (!isAuthenticated(request)) {
      request->send(403, "text/plain", "Not Authorized");
      return;
    }
    if (request->hasParam("entName", true)) {
      FSWriteJsonString("entity_name", request->getParam("entName", true)->value());
      if (FSReadJsonString("ssid") != "" && FSReadJsonString("ssid_pw") != "") {
        request->send(200, "text/plain", "MQTT updated, reboot in 5sec...");
        rebootLoop(true);
      } else {
        request->send(200, "text/html", "<html><body><center><h1>MQTT configuration updated!</h1><br>No Wifi configuration found, reboot not required<br><a href='/'>Back</a></center></body></html>");
      }
    } else {
      request->send(400, "text/plain", "Bad Request: Entity not provided");
    } });

  server.on("/newwifi", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    if (!isAuthenticated(request)) {
      request->send(403, "text/plain", "Not Authorized");
      return;
    }
    if (request->hasParam("wifiname", true) && request->hasParam("wifipwd", true)) {
      String wifiname = request->getParam("wifiname", true)->value();
      String wifipwd = request->getParam("wifipwd", true)->value();
      if (wifiname.length() == 0 || wifiname.length() > 33 || wifipwd.length() < 8 || wifipwd.length() > 15) {
        request->send(403, "text/plain", "Wrong wifi config, some fields are wrong");
        return;
      }
      FSWriteJsonString("ssid", wifiname);
      FSWriteJsonString("ssid_pw", wifipwd);
    
      rebootLoop(true);
      request->send(403, "text/plain", "Wifi config changed, reboot in 5sec...");
    } else {
      request->send(403, "text/plain", "Check required fields!");
    } });

  server.on("/newmqtt", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    if (!isAuthenticated(request)) {
      request->send(403, "text/plain", "Not Authorized");
      return;
    }
    if (request->hasParam("mqttip", true) && request->hasParam("mqttport", true)) {
      String mqttip = request->getParam("mqttip", true)->value();
      String mqttport = request->getParam("mqttport", true)->value();
      String mqttlogin = request->hasParam("mqttlogin", true) ? request->getParam("mqttlogin", true)->value() : "";
      String mqttpass = request->hasParam("mqttpassword", true) ? request->getParam("mqttpassword", true)->value() : "";

      if (mqttip.length() == 0 || mqttport.length() == 0) {
        request->send(403, "text/plain", "Wrong mqtt config, IP or port empty");
        return;
      }
      if (mqttip.length() > 79 || mqttlogin.length() > 19 || mqttpass.length() > 19) {
        request->send(403, "text/plain", "MQTT IP (>79 chars) or login/password (>19 chars) too long");
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
        request->send(200, "text/plain", "MQTT updated, reboot in 5sec...");
      } else {
        request->send(200, "text/html", "<html><body><center><h1>MQTT configuration updated!</h1><br>No Wifi configuration found, reboot not required<br><a href='/'>Back</a></center></body></html>");
      }
    } else {
      request->send(400, "text/plain", "Bad Request: MQTT fields missing");
    } });

  server.on("/mqttInfo", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) {
      request->send(403, "text/plain", "Not Authorized");
      return;
    }
    request->send(200, "text/html", mqttTopicList + "<center><a href='/'>Back</a></center></body></html>");
  });

  server.on("/newwebpass", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    if (!isAuthenticated(request)) {
      request->send(403, "text/plain", "Not Authorized");
      return;
    }
    if (request->hasParam("webpass", true)) {
      String webpass = request->getParam("webpass", true)->value();
      if (webpass.length() < 4 || webpass.length() > 10) {
        request->send(403, "text/plain", "Wrong webpass config, >3 and <10");
        return;
      }
      FSWriteJsonString("web_pw", webpass);
      InitCookie();
      request->send(403, "text/plain", "Webpass changed, login again");
    } else {
      request->send(400, "text/plain", "Bad Request: webpass not provided");
    } });

    
  // server.on("/setCH1Type", HTTP_POST, [](AsyncWebServerRequest *request)
  //           {
  //   if (!isAuthenticated(request)) {
  //     request->send(403, "text/plain", "Not Authorized");
  //     return;
  //   }
  //   if (request->hasParam("CH1Type", true)) {
  //     String ch_type = request->getParam("CH1Type", true)->value();
  //     FSWriteJsonString("channel1Type", ch_type);
    
  //     if (FSReadJsonString("ssid") != "" && FSReadJsonString("ssid_pw") != "") {
  //       rebootLoop(true);
  //       request->send(200, "text/plain", "CH1 type changed, reboot in 5sec...");
  //     } else {
  //       request->send(200, "text/html", "<html><body><center><h1>CH1 type changed!</h1><br>No Wifi configuration found, reboot not required<br><a href='/'>Back</a></center></body></html>");
  //     }
  //   } else {
  //     request->send(403, "text/plain", "Check required fields!");
  //   } });
  // server.on("/setCH2Type", HTTP_POST, [](AsyncWebServerRequest *request)
  //           {
  //   if (!isAuthenticated(request)) {
  //     request->send(403, "text/plain", "Not Authorized");
  //     return;
  //   }
  //   if (request->hasParam("CH2Type", true)) {
  //     String ch_type = request->getParam("CH2Type", true)->value();
  //     FSWriteJsonString("channel2Type", ch_type);
    
  //     if (FSReadJsonString("ssid") != "" && FSReadJsonString("ssid_pw") != "") {
  //       rebootLoop(true);
  //       request->send(200, "text/plain", "CH2 type changed, reboot in 5sec...");
  //     } else {
  //       request->send(200, "text/html", "<html><body><center><h1>CH2 type changed!</h1><br>No Wifi configuration found, reboot not required<br><a href='/'>Back</a></center></body></html>");
  //     }
  //   } else {
  //     request->send(403, "text/plain", "Check required fields!");
  //   } });

  server.on("/ch1on", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              if (!isAuthenticated(request))
              {
                request->send(403, "text/plain", "Not Authorized");
                return;
              }

              // channelStates[1] = true;
              // updateChannel(1);
              request->send(200, "text/plain", "OK"); // Пустой ответ для AJAX
            });

  server.on("/ch1off", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              if (!isAuthenticated(request))
              {
                request->send(403, "text/plain", "Not Authorized");
                return;
              }

              // channelStates[1] = false;
              // updateChannel(1);
              request->send(200, "text/plain", "OK"); // Пустой ответ для AJAX
            });

  // server.on("/ch2on", HTTP_POST, [](AsyncWebServerRequest *request)
  //           {
  //             if (!isAuthenticated(request))
  //             {
  //               request->send(403, "text/plain", "Not Authorized");
  //               return;
  //             }

  //             // channelStates[2] = true;
  //             // updateChannel(2);
  //             request->send(200, "text/plain", "OK"); // Пустой ответ для AJAX
  //           });

  // server.on("/ch2off", HTTP_POST, [](AsyncWebServerRequest *request)
  //           {
  //             if (!isAuthenticated(request))
  //             {
  //               request->send(403, "text/plain", "Not Authorized");
  //               return;
  //             }

  //             // channelStates[2] = false;
  //             // updateChannel(2);
  //             request->send(200, "text/plain", "OK"); // Пустой ответ для AJAX
  //           });

  // server.on("/switch_state", HTTP_GET, [](AsyncWebServerRequest *request)
  //           {
  //   String json = getSwitchState();
  //   request->send(200, "application/json", json); });

  // server.on("/updateT1", HTTP_POST, [](AsyncWebServerRequest *request)
  //           {
  //     if (!isAuthenticated(request)) {
  //       request->send(403, "text/plain", "Not Authorized");
  //       return;
  //     }
  //     if (request->hasParam("T1mode", true)) {
  //       FSWriteJsonString("toggler1Type", request->getParam("T1mode", true)->value());
  //       delay(200);
  //       initInputs();
  //     }
  //     request->send(200, "text/html", "<html><body><center><h1>T1 configuration updated!</h1><br>You can go back now<br><a href='/'>Back</a></center></body></html>"); });

  // server.on("/updateT2", HTTP_POST, [](AsyncWebServerRequest *request)
  //           {
  //       if (!isAuthenticated(request)) {
  //         request->send(403, "text/plain", "Not Authorized");
  //         return;
  //       }
  //       if (request->hasParam("T2mode", true)) {
  //         FSWriteJsonString("toggler2Type", request->getParam("T2mode", true)->value());
  //         delay(200);
  //       initInputs();
  //     }
  //     request->send(200, "text/html", "<html><body><center><h1>T2 configuration updated!</h1><br>You can go back now<br><a href='/'>Back</a></center></body></html>"); });

  // server.on("/setDebounce", HTTP_POST, [](AsyncWebServerRequest *request)
  //           {
  //       if (!isAuthenticated(request)) {
  //         request->send(403, "text/plain", "Not Authorized");
  //         return;
  //       }
  //       if (request->hasParam("debounce", true)) {
  //         uint16_t value = (request->getParam("debounce", true)->value()).toInt();
  //         FSWriteJsonUint64("debounceMS", value);
  //         debounceMS = value;
  //     }
  //     request->send(200, "text/html", "<html><body><center><h1>T2 configuration updated!</h1><br>You can go back now<br><a href='/'>Back</a></center></body></html>"); });

  server.on("/hardreset", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    if (!isAuthenticated(request)) {
      request->send(403, "text/plain", "Not Authorized");
      return;
    }
    LittleFS.format();
    rebootLoop(true);
    request->send(403, "text/plain", "Full config reset, memory cleaned, reboot in 5sec..."); });

  server.on("/reboot", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    if (!isAuthenticated(request)) {
      request->send(403, "text/plain", "Not Authorized");
      return;
    }
    rebootLoop(true);
    request->send(403, "text/plain", "Reboot in 5sec..."); });

  // Настройка маршрута для обновления
  // server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request)
  //           {
  //   if (!isAuthenticated(request)) {
  //     request->send(403, "text/plain", "Not Authorized");
  //     return;
  //   }
  //   // Отправляем начальный ответ, чтобы клиент знал, что обновление началось
  //   request->send(200, "text/plain", "Update started, wait for completion..."); }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
  //           {
  //   if (!index) {
  //     Serial.printf("Update Start: %s\n", filename.c_str());
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
  //     } else {
  //       Update.printError(Serial);
  //     }

  //     // Отправляем ответ клиенту
  //     AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Update finished!");
  //     response->addHeader("Connection", "close");
  //     request->send(response);

  //     // Перезагружаем устройство через небольшой таймаут, чтобы ответ успел отправиться
  //     rebootLoop(true);
  //   } });

  InitCookie();
  server.begin();
  Serial.println("AsyncWebServer started");
}