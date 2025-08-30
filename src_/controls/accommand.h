#ifndef ACCOMMANDS_H
#define ACCOMMANDS_H

#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Daikin.h>
#include "../fs/filesystem.h"
#include "../device_config.h"
// #include "../mqtt/mqtt_publish.h"

// void ac_command(int temp, String(ac_mode), String(f_spd));
void ac_command();

#endif