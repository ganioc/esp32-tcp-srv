#pragma once

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "../../queue/include/queue.h"
#include "esp_task_wdt.h"

#define CMD_REQUEST 0x01
#define CMD_RESPONSE 0x02
#define CMD_NOTIFICATION 0x03

#define TARGET_ULTRA_SONIC 0x01
#define TARGET_SWITCH_1 0x02
#define TARGET_SWITCH_2 0x03
#define TARGET_ESP32 0x10
#define TARGET_HOST 0x11

//
#define UT_STOP_MODE 0x0
#define UT_WORKING_MODE 0x01
#define UT_READ_MODE 0x03

#define SWITCH_READ_LEVEL 0x01

#define UT_MODE_1 0x1
#define UT_MODE_2 0x2

#define UT_RESET 0x02

#define ESP32_RESET 0x1
#define ESP32_SET_TIMESTAMP 0x2
#define ESP32_GET_TIMESTAMP 0x3
#define ESP32_GET_VERSION 0x4
#define ESP32_SET_UPGRADE 0x05
#define ESP32_GET_UPGRADE_STATUS 0x06

#define STATE_IDLE 0x0
#define STATE_SEQ1 0x01
#define STATE_SEQ2 0x02
#define STATE_LEN1 0x03
#define STATE_LEN2 0x04
#define STATE_CONTENT 0x05

#define UPGRADE_STATUS_GOING 0x1
#define UPGRADE_STATUS_FINISHED_OK 0x2
#define UPGRADE_STATUS_ERROR_INIT_HTTP_CLIENT 0x10
#define UPGRADE_STATUS_ERROR_OPEN_CLIENT 0x11
#define UPGRADE_STATUS_ERROR_READ_CLIENT 0x12
#define UPGRADE_STATUS_ERROR_VERSION_SAME 0x13
#define UPGRADE_STATUS_ERROR_OTA_BEGIN 0x14
#define UPGRADE_STATUS_ERROR_LENGTH_OVERLIMIT 0x15
#define UPGRADE_STATUS_ERROR_OTA_END 0x16
#define UPGRADE_STATUS_ERROR_SET_BOOT 0x17
#define UPGRADE_STATUS_FINISHED_FAIL 0x18

#define CHECK_ERROR_CODE(returned, expected) ({ \
  if (returned != expected)                     \
  {                                             \
    printf("TWDT ERROR\n");                     \
    abort();                                    \
  }                                             \
})

int64_t getstamp64();
int setstamp64(int64_t sec, int64_t usec);
int encodeSensorN(Msg_t *msg, char *data, int64_t timestamp);
// uint16_t calCRC16(const uint8_t *data, uint32_t size);
int check_crc(char *buf);
int create_frame(char *buf, Msg_t msg);
int64_t stamp64FromBuffer(char *buf, int len);
int stamp64ToBuffer(int64_t tm, char *buf);
int encodeUTReset(Msg_t *msg, uint8_t err);
int encodeSwitchN(Msg_t *msg, int num_switch, int level, int64_t timestamp);
int encodeUTModeRsp(Msg_t *msg, uint8_t mode, uint8_t err);
int encodeUTReadModeRsp(Msg_t *msg, uint8_t enable, uint8_t mode, uint8_t err);
int encodeUTModeStopRsp(Msg_t *msg, uint8_t err);
int encodeSwitchRead(Msg_t *msg, uint8_t switch_num, uint8_t err);
int encodeUnknowCmd(Msg_t *msg, uint8_t target, uint8_t cmd, uint8_t err);
int encodeVersionRead(Msg_t *msg);
int encodeESP32Reset(Msg_t *msg);
int encodeESP32SetTimestamp(Msg_t *msg, int64_t tm);
int encodeESP32GetTimestamp(Msg_t *msg);

char *getVersion();