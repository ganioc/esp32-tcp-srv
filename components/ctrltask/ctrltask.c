#include <stdio.h>
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

GlobalStatus_t gStatus = {
    .enable = 1,
    .mode = 1};



Msg_t msg;

void handle_msg(Msg_t *msg)
{
  if (msg->buf[0] == CMD_REQUEST && msg->buf[1] == TARGET_ULTRA_SONIC && msg->buf[2] == UT_WORKING_MODE)
  {
    if (msg->buf[3] == UT_MODE_1)
    {
      printf("To set UT_MODE_1\n");
    }
    else if (msg->buf[3] == UT_MODE_2)
    {
      printf("To set UT_MODE_2\n");
    }
    else
    {
      printf("Unrecognized UT_Mode\n");
    }
  }

  else if (msg->buf[0] == CMD_REQUEST && msg->buf[1] == TARGET_ULTRA_SONIC && msg->buf[2] == ESP32_RESET)
  {
    printf("ESP32 reset\n");
  }

  else if (msg->buf[0] == CMD_REQUEST && msg->buf[1] == TARGET_ESP32 && msg->buf[2] == ESP32_SET_TIMESTAMP)
  {
    printf("ESP32 set timestamp\n");
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
          printf("msg send\n");
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
          void handle_msg(&msg);
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