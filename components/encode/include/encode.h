#pragma once
#include <stdio.h>
#include "../../queue/include/queue.h"

#define CMD_REQUEST 0x01
#define CMD_RESPONSE 0x02
#define CMD_NOTIFICATION 0x03

#define TARGET_ULTRA_SONIC 0x01
#define TARGET_SWITCH_1 0x02
#define TARGET_SWITCH_2 0x03
#define TARGET_ESP32 0x10
#define TARGET_HOST 0x11

int encodeSensorNotification(Msg_t *msg, char *data, int64_t timestamp);
int64_t getstamp();
