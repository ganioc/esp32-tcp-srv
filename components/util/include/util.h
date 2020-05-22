#pragma once

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "../../queue/include/queue.h"

#define CMD_REQUEST 0x01
#define CMD_RESPONSE 0x02
#define CMD_NOTIFICATION 0x03

#define TARGET_ULTRA_SONIC 0x01
#define TARGET_SWITCH_1 0x02
#define TARGET_SWITCH_2 0x03
#define TARGET_ESP32 0x10
#define TARGET_HOST 0x11

int64_t getstamp64();
int encodeSensorN(Msg_t *msg, char *data, int64_t timestamp);