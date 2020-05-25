#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "../queue/include/queue.h"
#include "./include/ctrltask.h"
#include "../util/include/util.h"

extern QueueManager_t xManager[];
extern SemaphoreHandle_t xSemaphore;
extern QueueHandle_t uartQueue;

static const char *TAG = "ctrltask";

GlobalStatus_t gStatus = {
    .enable = 0,
    .mode = 20};

Msg_t msg;

void handle_msg(QueueHandle_t queue, Msg_t *msg)
{
  for (int i = 0; i < msg->len; i++)
  {
    printf("%x ", msg->buf[i]);
  }
  printf("\n");

  if (msg->buf[0] == CMD_REQUEST && msg->buf[1] == TARGET_ULTRA_SONIC && msg->buf[2] == UT_WORKING_MODE)
  {
    if (msg->buf[3] == UT_MODE_1)
    {
      printf("To set UT_MODE_1\n");
      gStatus.enable = 1;
      gStatus.mode = 1;
    }
    else if (msg->buf[3] == UT_MODE_2)
    {
      printf("To set UT_MODE_2\n");
      gStatus.enable = 1;
      gStatus.mode = 2;
    }
    else if (msg->buf[3] > 5 && msg->buf[3] < 30)
    {
      printf("Unrecognized UT_Mode\n");

      gStatus.enable = 1;
      gStatus.mode = msg->buf[3];
    }
  }
  else if (msg->buf[0] == CMD_REQUEST && msg->buf[1] == TARGET_ULTRA_SONIC && msg->buf[2] == UT_RESET)
  {
    printf("UT reset\n");
  }
  else if (msg->buf[0] == CMD_REQUEST && msg->buf[1] == TARGET_ESP32 && msg->buf[2] == ESP32_RESET)
  {
    printf("ESP32 reset\n");
  }
  else if (msg->buf[0] == CMD_REQUEST && msg->buf[1] == TARGET_ESP32 && msg->buf[2] == ESP32_SET_TIMESTAMP)
  {
    int64_t tm = 0;
    int err = 0;
    int index = 0;
    int i;
    char buffer[32];

    printf("ESP32 set timestamp\n");
    if (msg->len >= 12)
    {
      tm = stamp64FromBuffer((char *)&msg->buf[3], msg->len - 3);
      printf("tm is %lld\n", tm);
      setstamp64(tm / 1000, (tm % 1000) * 1000);
    }
    else
    {
      err = 1;
      printf("Wrong packet length: %d\n", msg->len);
    }
    tm = getstamp64();
    printf("\ntm is:%lld\n", tm);
    stamp64ToBuffer(tm, buffer);

    printf("buffer length: %d\n", strlen(buffer));

    index = 0;
    msg->buf[index++] = 0x02;
    msg->buf[index++] = TARGET_ESP32;
    msg->buf[index++] = err;
    msg->buf[index++] = ESP32_SET_TIMESTAMP;
    for (i = 0; i < strlen(buffer); i++)
    {
      msg->buf[index++] = buffer[i];
      printf("%x ", buffer[i]);
    }
    printf("\n");
    msg->len = index;
    // send back response
    if (xQueueSend(queue, msg, 50 / portTICK_RATE_MS))
    {
      printf("set timestamp msg send ->\n");
    }
    else
    {
      printf("set timestamp msg send  Failed ->\n");
    }
  }
  else
  {
    printf("Unrecognized cmd\n");
  }
}

void broadcast(Msg_t *mMsg)
{
  if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
  {
    for (int i = 0; i < QUEUE_LENGTH; i++)
    {
      if (xManager[i].flag == 1)
      {
        if (xQueueSend(xManager[i].tx_queue, mMsg, 50 / portTICK_RATE_MS))
        {
          printf("msg send: \n");
          ESP_LOGI(TAG, "msg send %d", mMsg->len);
        }
      }
    }
    xSemaphoreGive(xSemaphore);
  }
}
void check_rx_queues()
{
  if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
  {
    // printf("Check rx queue\n");
    for (int i = 0; i < QUEUE_LENGTH; i++)
    {
      if (xManager[i].flag == 1)
      {
        if (xQueueReceive(xManager[i].rx_queue, &msg, 100 / portTICK_RATE_MS))
        {
          printf("-> %d rx msg %d\n", i, msg.len);
          handle_msg(xManager[i].tx_queue, &msg);
        }
      }
    }
    xSemaphoreGive(xSemaphore);
    // vTaskDelay(500 / portTICK_RATE_MS);
  }
}
/**
 * this task is used for sending uart data
 * and checking switch data
 */
static void ctrl_task()
{

  while (1)
  {
    ////////////////////////
    // to check rx queue, change global status
    // check_rx_queues();
    check_rx_queues();

    if (xQueueReceive(uartQueue, &msg, 200 / portTICK_RATE_MS))
    {
      printf("-> rx msg %d\n", msg.len);
      // send out uart data
      ///////////////////////////
      // send sth. out to tx queue
      broadcast(&msg);

      // send out switch data
    }
  }
}
void init_ctrltask()
{
  xTaskCreate(ctrl_task, "ctrl_task", 2048, NULL, 6, NULL);
}