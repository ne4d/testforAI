#include "device_defaults.h"

void setFlashDefaults()
{

    if (String(FSReadJsonString("mac")).length() < 12)
    {
        WiFi.mode(WIFI_STA);
        WiFi.begin("init", "12345678");
        delay(3000);
        //
        // FSWriteJsonString("ssid", "Popov_D");
        // FSWriteJsonString("ssid_pw", "WlHE5E4K");
        // FSWriteJsonString("mqtt_server", "10.10.99.240");
        // FSWriteJsonUint64("mqtt_port", 1883);
        // FSWriteJsonString("mqtt_un", "local");
        // FSWriteJsonString("mqtt_pw", "aYCMeT");

        // FSWriteJsonString("ssid", "Norris Service");
        // FSWriteJsonString("ssid_pw", "norrispower");
        // FSWriteJsonString("mqtt_server", "10.10.1.254");
        // FSWriteJsonUint64("mqtt_port", 1883);
        // FSWriteJsonString("mqtt_un", "local");
        // FSWriteJsonString("mqtt_pw", "local");

        FSWriteJsonString("ssid", "Anna");
        FSWriteJsonString("ssid_pw", "4488625sk");
        FSWriteJsonString("mqtt_server", "10.10.1.240");
        FSWriteJsonUint64("mqtt_port", 1883);
        FSWriteJsonString("mqtt_un", "local");
        FSWriteJsonString("mqtt_pw", "local");

        FSWriteJsonString("mac", getFormattedMAC());
        FSWriteJsonString("web_pw", "admin");
        FSWriteJsonString("dev_uid", "ne4d_hvac_" + FSReadJsonString("mac"));
        FSWriteJsonString("dev_model", "HVAC Control: " + FSReadJsonString("dev_uid"));
        FSWriteJsonString("clientId", FSReadJsonString("dev_uid"));
        FSWriteJsonString("entity_name", FSReadJsonString("HVAC 8266"));

        FSWriteJsonString("mode", "off");
        FSWriteJsonString("fan_speed", "auto");
        FSWriteJsonUint64("temperature", 22);
        FSWriteJsonBool("boost", false);
        FSWriteJsonBool("swing", false);
        Serial.println("MAC added to memory, reboot in 5 sec..");
        ESP.restart();
    }
}