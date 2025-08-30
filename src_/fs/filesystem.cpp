#include "filesystem.h"

// Функция для чтения JSON из файла
JsonDocument readJsonFromFile() {
  JsonDocument doc;
  if (LittleFS.exists("/config.json")) {
    File file = LittleFS.open("/config.json", "r");
    if (file) {
      DeserializationError error = deserializeJson(doc, file);
      if (error) {
        Serial.println("Failed to parse config file");
      }
      file.close();
    }
  }
  return doc;
}

// Функция для записи JSON в файл
void writeJsonToFile(const JsonDocument& doc) {
  File file = LittleFS.open("/config.json", "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
  } else {
    Serial.println("Failed to open config file for writing");
  }
}

void FSWriteJsonBool(const char* key, bool value) {
  JsonDocument doc = readJsonFromFile();
  doc[key] = value;
  writeJsonToFile(doc);
}

bool FSReadJsonBool(const char* key, bool defaultValue) {
  JsonDocument doc = readJsonFromFile();
  // Заменяем containsKey на is<bool>()
  if (doc[key].is<bool>()) {
    return doc[key].as<bool>();
  }
  return defaultValue;
}

void FSWriteJsonUint64(const char* key, uint32_t value) {
  JsonDocument doc = readJsonFromFile();
  doc[key] = value;
  writeJsonToFile(doc);
}

uint32_t FSReadJsonUint64(const char* key, uint32_t defaultValue) {
  JsonDocument doc = readJsonFromFile();
  if (doc[key].is<uint32_t>()) {
    return doc[key].as<uint32_t>();
  }
  return defaultValue;
}

void FSWriteJsonString(const char* key, const String& value) {
  JsonDocument doc = readJsonFromFile();
  doc[key] = value;
  writeJsonToFile(doc);
}

String FSReadJsonString(const char* key, const String& defaultValue) {
  JsonDocument doc = readJsonFromFile();
  if (doc[key].is<String>()) {
    return doc[key].as<String>();
  }
  return defaultValue;
}

void FSPrintAllJson() {
  JsonDocument doc = readJsonFromFile();
  Serial.println("=== All JSON Key-Value Pairs ===");
  for (JsonPair kv : doc.as<JsonObject>()) {
    const char* key = kv.key().c_str();
    JsonVariant value = kv.value();
    if (value.is<bool>()) {
      Serial.print(key);
      Serial.print(": ");
      Serial.println(value.as<bool>());
    } else if (value.is<uint32_t>()) {
      Serial.print(key);
      Serial.print(": ");
      Serial.println(value.as<uint32_t>());
    } else if (value.is<String>()) {
      Serial.print(key);
      Serial.print(": ");
      Serial.println(value.as<String>());
    } else {
      Serial.print(key);
      Serial.print(": ");
      Serial.println("Unknown type");
    }
  }
  Serial.println("==============================");
}

const char* convertToConstChar(const String& inputString) {
  // Выделяем память для копии строки + нулевой терминатор
  char* result = new char[inputString.length() + 1];
  // Копируем строку
  inputString.toCharArray(result, inputString.length() + 1);
  return result;
}