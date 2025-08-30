#include "mqtt.h"

AsyncMqttClient mqttClient;

void onMqttMessage(char *in_topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
    // void onMqttMessage(char *in_topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len) {
    String messageMQTT = String(payload).substring(0, len);
    String topic = String(in_topic);
    analyzeMqttMessage(topic, messageMQTT);
}

void mqttInit()
{
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onMessage(onMqttMessage);
    // mqttClient.setMaxTopicLength(1024);
    // SUPERIMPORTANT!!!
    // static String willTopic = dev_uid + "/status";
    // mqttClient.setWill(willTopic.c_str(), 0, true, "offline");
    mqttClient.setWill(String(dev_uid + "/status").c_str(), 0, true, "offline");
    mqttClient.setClientId(FSReadJsonString("mac").c_str());
    mqttClient.setKeepAlive(20);
}

void connectMqtt()
{
    mqtt_connection_initiation = true;
    String mqttHost = FSReadJsonString("mqtt_server");
    uint16_t mqttPort = FSReadJsonUint64("mqtt_port");
    Serial.println("Connecting to MQTT...");
    Serial.println("MQTT Host: " + mqttHost);
    Serial.println("MQTT Port: " + String(mqttPort));

    if (mqttHost.length() == 0 || mqttPort == 0)
    {
        Serial.println("Invalid MQTT settings: Host or Port empty/invalid.");
        return;
    }

    //   попытка извлечь IP из доменного имени
    IPAddress mqttIP;
    if (mqttIP.fromString(mqttHost))
    {
        Serial.println("MQTT IP resolved to: " + mqttIP.toString());
        mqttClient.setServer(mqttIP, mqttPort);
    }
    else
    {
        Serial.println("Attempting DNS resolution for: " + mqttHost);
        if (WiFi.hostByName(mqttHost.c_str(), mqttIP))
        {
            Serial.println("MQTT IP resolved to: " + mqttIP.toString());
            mqttClient.setServer(mqttIP, mqttPort);
        }
        else
        {
            Serial.println("DNS resolution failed for: " + mqttHost);
            return;
        }
    }

    const char *mqttUser = convertToConstChar(FSReadJsonString("mqtt_un"));
    const char *mqttPass = convertToConstChar(FSReadJsonString("mqtt_pw"));

    // [DEBUG BLOCK]
    // Serial.print("MQTT User (raw): '");
    // Serial.print(mqttUser);
    // Serial.println("'");
    // Serial.print("MQTT Pass (raw): '");
    // Serial.print(mqttPass);
    // Serial.println("'");
    // Serial.println("MQTT User length: " + String(strlen(mqttUser)));
    // Serial.println("MQTT Pass length: " + String(strlen(mqttPass)));
    // Serial.print("MQTT User bytes: ");
    // for (size_t i = 0; i < strlen(mqttUser); i++)
    // {
    //     Serial.print((uint8_t)mqttUser[i], HEX);
    //     Serial.print(" ");
    // }
    // Serial.println();
    // Serial.print("MQTT Pass bytes: ");
    // for (size_t i = 0; i < strlen(mqttPass); i++)
    // {
    //     Serial.print((uint8_t)mqttPass[i], HEX);
    //     Serial.print(" ");
    // }
    // Serial.println();
    // [DEBUG BLOCK END]

    if (strlen(mqttUser) == 0 || strlen(mqttPass) == 0 || strlen(mqttUser) > 19 || strlen(mqttPass) > 19)
    {
        Serial.println("Invalid MQTT credentials, using default: local, local");
        mqttClient.setCredentials("local", "local");
    }
    else
    {
        Serial.println("Setting MQTT credentials: " + String(mqttUser) + ", " + String(mqttPass));
        mqttClient.setCredentials(mqttUser, mqttPass);
    }

    // Serial.println("Client ID: " + FSReadJsonString("mac"));
    // mqttClient.setClientId(FSReadJsonString("mac").c_str());
    // mqttClient.setKeepAlive(60);

    Serial.println("MQTT connect initiated.");
    mqttClient.connect();
    
}

void onMqttConnect(bool sessionPresent)
{
    Serial.println("Connected to MQTT. Session present: " + String(sessionPresent));
    mqtt_connection_state = true;
    // ledConfig.blink(2500, 200);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
    // mqtt_connection_state = false;
    // mqtt_connection_initiation = false;
    // mqtt_config_published = false;
    // mqtt_availability_published = false;
    // ledConfig.blink(400, 400);

    Serial.println("Disconnected from MQTT. Reason: " + String((uint8_t)reason));
    switch (reason)
    {
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
    if (WiFi.isConnected()) {
        mqttReconnectTimer.once(2, connectMqtt);
    }
}

void publishMqttString(String topic, String msg, uint8_t qos, bool retain)
{
    mqttClient.publish(topic.c_str(), qos, retain, msg.c_str());
    // Serial.println("Publishing : \"" + topic + " (" + msg + ")"); // [DEBUG]
}

void publishMqttBool(String topic, bool msg, uint8_t qos, bool retain)
{
    mqttClient.publish(topic.c_str(), qos, retain, msg ? "on" : "off");
    // Serial.println("Publishing : \"" + topic + " (" + (msg ? "ON" : "OFF") + ")"); // [DEBUG]
}

void subscribeMqttTopic(String topic, uint8_t qos)
{
    mqttClient.subscribe(topic.c_str(), qos);
    Serial.println("Subscribed on topic: \"" + topic + "\"");
}

void publishAvailability(uint8_t qos, bool retain)
{
    publishMqttString(dev_uid + "/status", "online", qos, retain);
    // Serial.println(dev_uid + "/status"); // [DEBUG]
}

void publishUptimeState(uint8_t qos, bool retain)
{
    publishMqttString(dev_uid + "/sensor/uptime/state", String(minutesRunned()).c_str(), qos, retain);
}

void publishMode(uint8_t qos, bool retain){
    publishMqttString(dev_uid + "/climate/hvac_mode/state", mode.c_str(), qos, retain);
}

void publishTemp(uint8_t qos, bool retain){
    publishMqttString(dev_uid + "/climate/temperature/state", String(temperature).c_str(), qos, retain);
}

void publishFanSpeed(uint8_t qos, bool retain){
    publishMqttString(dev_uid + "/climate/fan_mode/state", mode.c_str(), qos, retain);
}

void publishSwing(uint8_t qos, bool retain){
    publishMqttBool(dev_uid + "/climate/hvac_swing_mode/state", swing, qos, retain);
}

void publishBoost(uint8_t qos, bool retain){
    publishMqttString(dev_uid + "/climate/hvac_preset_mode/state", boost ? "boost" : "none", qos, retain);
}