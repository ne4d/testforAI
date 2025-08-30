#include "config.h"

String generateMQTTConfig(String dev_type) {
    String mac = FSReadJsonString("mac");
    String entity_name = FSReadJsonString("entity_name");
    // String dev_name = FSReadJsonString("dev_name");
    String dev_uid = FSReadJsonString("dev_uid");
    // String version = FIRMWARE_VERSION;
    String version = "FIRMWARE_VERSION";
    String ip = WiFi.localIP().toString();
    JsonDocument cfg;
    if (dev_type == "uptime"){
        dev_type = "sensor";
        cfg["dev_cla"] = "duration";
        cfg["unit_of_meas"] = "m";
        cfg["stat_cla"] = "total_increasing";
        // cfg["name"] = ent_name;
        cfg["name"] = "uptime";
        cfg["ic"] = "mdi:timer-outline";
        cfg["ent_cat"] = "diagnostic";
        cfg["avty_t"] = dev_uid + "/status";
        cfg["stat_t"] = dev_uid + "/" + dev_type + "/uptime/state";
        cfg["uniq_id"] = dev_uid + "_uptime";
    } else if (dev_type == "reboot"){
        dev_type = "button";
        cfg["dev_cla"] = "restart";
        cfg["ent_cat"] = "config";
        // cfg["name"] = ent_name;
        cfg["name"] = "reboot";
        cfg["avty_t"] = dev_uid + "/status";
        cfg["cmd_t"] = dev_uid + "/" + dev_type + "/reboot/command";
        cfg["uniq_id"] = dev_uid + "_reboot";
    } else if (dev_type == "climate") {
        dev_type = "climate";
        cfg["name"] = "AC control";
        cfg["send_if_off"] = true;
        cfg["unique_id"] = dev_uid + "_ac";
        cfg["avty_t"] = dev_uid + "/status";
        cfg["current_temperature_topic"] = dev_uid + "/" + dev_type + "/inside_temperature/state";
        cfg["fan_mode_command_topic"] = dev_uid + "/" + dev_type + "/fan_mode/set";
        cfg["fan_mode_state_topic"] = dev_uid + "/" + dev_type + "/fan_mode/state";
        JsonArray fan_modes = cfg["fan_modes"].to<JsonArray>();
            fan_modes.add("auto");
            fan_modes.add("low");
            fan_modes.add("medium");
            fan_modes.add("high");
        cfg["mode_command_topic"] = dev_uid + "/" + dev_type + "/hvac_mode/set";
        cfg["mode_state_topic"] = dev_uid + "/" + dev_type + "/hvac_mode/state";
        JsonArray modes = cfg["modes"].to<JsonArray>();
            modes.add("auto");
            modes.add("cool");
            modes.add("heat");
            modes.add("dry");
            modes.add("fan_only");
            modes.add("off");
        cfg["preset_mode_command_topic"] = dev_uid + "/" + dev_type + "/hvac_preset_mode/set";
        cfg["preset_mode_state_topic"] = dev_uid + "/" + dev_type + "/hvac_preset_mode/state";
        JsonArray preset_modes = cfg["preset_modes"].to<JsonArray>();
            preset_modes.add("boost");
        cfg["swing_mode_command_topic"] = dev_uid + "/" + dev_type + "/hvac_swing_mode/set";
        cfg["swing_mode_state_topic"] = dev_uid + "/" + dev_type + "/hvac_swing_mode/state";
        JsonArray swing_modes = cfg["swing_modes"].to<JsonArray>();
            swing_modes.add("on");
            swing_modes.add("off");
        cfg["min_temp"] = 16;
        cfg["max_temp"] = 32;
        cfg["precision"] = 1;
        cfg["retain"] = true;
        cfg["temperature_command_topic"] = dev_uid + "/" + dev_type + "/temperature/set";
        cfg["temperature_state_topic"] = dev_uid + "/" + dev_type + "/temperature/state";
        cfg["temp_step"] = 1;
    }

    // Создаем вложенный объект dev
    JsonObject dev = cfg["dev"].to<JsonObject>();
        dev["ids"] = mac;
        // dev["name"] = dev_name;
        dev["name"] = entity_name;
        dev["sw"] = version;
        dev["mdl"] = "esp32-s3";
        dev["mf"] = "Nechaevskii Dmitrii";
        // Создаем вложенный массив x2 cns внутри dev
        JsonArray cns = dev["cns"].to<JsonArray>();
            JsonArray cns_entry = cns.add<JsonArray>();
                cns_entry.add("mac");
                cns_entry.add(mac);
                cns_entry.add("ip");
                cns_entry.add(ip);

    String json;
    serializeJson(cfg, json);
    return json;
}

String generateMQTTcfgTopic(String dev_type){
    String dev_uid = FSReadJsonString("dev_uid");
    String mqtt;
    if (dev_type == "uptime"){
        dev_type = "sensor";
        mqtt = "homeassistant/" + dev_type + "/" + dev_uid + "/uptime/config";
    } else if (dev_type == "reboot"){
        dev_type = "button";
        mqtt = "homeassistant/" + dev_type + "/" + dev_uid + "/reboot/config";
    } else if (dev_type == "climate"){
        dev_type = "climate";
        mqtt = "homeassistant/" + dev_type + "/" + dev_uid + "/config";
    }
    return mqtt;
}