#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define QUEUE_LENGTH 3
#define MSG_LEN 32
#define MAX_MSG_NUM 4

typedef struct QueueManager
{
  int flag; // 1 valid, 0 invalid
  int sock_num;
  QueueHandle_t rx_queue; // from tcp to uart_task
  QueueHandle_t tx_queue;

} QueueManager_t;

typedef struct Msg
{
  char buf[MSG_LEN];
} Msg_t;

void init_queue();
