#ifndef SERVER_H
#define SERVER_H

#include <Arduino.h>
// #include <ESP8266WebServer.h>
// #include <ESPAsyncTCP.h>
#include "ESPAsyncWebServer.h"
// #include <Update.h>
#include <ArduinoOTA.h>
#include "pages.h"
#include "../system.h"
#include "../fs/filesystem.h"
#include "../controls/accommand.h"

// extern ESP8266WebServer server;
void startServer();

#endif