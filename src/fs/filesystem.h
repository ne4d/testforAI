#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

void FSWriteJsonBool(const char* key, bool value);
bool FSReadJsonBool(const char* key, bool defaultValue = false);

void FSWriteJsonUint64(const char* key, uint32_t value);
uint32_t FSReadJsonUint64(const char* key, uint32_t defaultValue = 0);

void FSWriteJsonString(const char* key, const String& value);
String FSReadJsonString(const char* key, const String& defaultValue = "");

void FSPrintAllJson();
const char* convertToConstChar(const String& inputString);

#endif