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
//

#define UT_WORKING_MODE 0x01
// mode=1, Send every packet
// mode=2,  Send every other packet
#define UT_MODE_1 0x1
#define UT_MODE_2 0x2

#define UT_RESET 0x02

#define ESP32_RESET 0x1
#define ESP32_SET_TIMESTAMP 0x2
#define ESP32_GET_TIMESTAMP 0x3

#define STATE_IDLE 0x0
#define STATE_SEQ1 0x01
#define STATE_SEQ2 0x02
#define STATE_LEN1 0x03
#define STATE_LEN2 0x04
#define STATE_CONTENT 0x05

int64_t getstamp64();
int setstamp64(int64_t sec, int64_t usec);
int encodeSensorN(Msg_t *msg, char *data, int64_t timestamp);
// uint16_t calCRC16(const uint8_t *data, uint32_t size);
int check_crc(char *buf);
int create_frame(char *buf, Msg_t msg);
int64_t stamp64FromBuffer(char *buf, int len);
int stamp64ToBuffer(int64_t tm, char *buf);
int encodeUTReset(Msg_t *msg, uint8_t err, int64_t timestamp);
