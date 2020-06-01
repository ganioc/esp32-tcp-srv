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
#include "../gpio/include/gpio.h"

extern QueueManager_t xManager[];
extern SemaphoreHandle_t xSemaphore;
extern QueueHandle_t uartQueue;
extern xQueueHandle gpio_evt_queue;

extern int stateDin1;
extern int stateDin2;

static const char *TAG = "ctrltask";

GlobalStatus_t gStatus = {
    .enable = 0,
    .mode = 1};

Msg_t msg;

void handle_msg(QueueHandle_t queue, Msg_t *msg)
{
  char buffer[32];

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
      printf(" UT_Mode %d\n", msg->buf[3]);

      gStatus.enable = 1;
      gStatus.mode = msg->buf[3];
    }
    encodeUTModeRsp(msg, msg->buf[3], 0);
  }
  else if (msg->buf[0] == CMD_REQUEST && msg->buf[1] == TARGET_ULTRA_SONIC && msg->buf[2] == UT_STOP_MODE)
  {
    gStatus.enable = 0;
    gStatus.mode = 1;
    encodeUTModeStopRsp(msg, 0);
  }
  else if (msg->buf[0] == CMD_REQUEST && msg->buf[1] == TARGET_ULTRA_SONIC && msg->buf[2] == UT_READ_MODE)
  {

    encodeUTReadModeRsp(msg, gStatus.enable, gStatus.mode, 0);
  }
  else if (msg->buf[0] == CMD_REQUEST && msg->buf[1] == TARGET_ULTRA_SONIC && msg->buf[2] == UT_RESET)
  {
    printf("UT reset\n");
    off_ut_pwr();

    encodeUTReset(msg, 0);
    // send back feedback
    vTaskDelay(500 / portTICK_PERIOD_MS);
    on_ut_pwr();
  }
  else if (msg->buf[0] == CMD_REQUEST && msg->buf[1] == TARGET_ESP32 && msg->buf[2] == ESP32_RESET)
  {
    printf("ESP32 reset\n");
    ESP_LOGE(TAG, "ESP32 Restarting now.\n");
    fflush(stdout);
    esp_restart();
  }
  else if (msg->buf[0] == CMD_REQUEST && msg->buf[1] == TARGET_ESP32 && msg->buf[2] == ESP32_SET_TIMESTAMP)
  {
    int64_t tm = 0;
    int err = 0;
    int index = 0;
    int i;

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
  }
  else if (msg->buf[0] == CMD_REQUEST && msg->buf[1] == TARGET_ESP32 && msg->buf[2] == ESP32_GET_TIMESTAMP)
  {
    printf("get timestamp msg\n");
    int64_t tm = getstamp64();
    printf("\ntm is:%lld\n", tm);
    stamp64ToBuffer(tm, buffer);

    printf("buffer length: %d\n", strlen(buffer));

    int index = 0;
    msg->buf[index++] = 0x02;
    msg->buf[index++] = TARGET_ESP32;
    msg->buf[index++] = 0;
    msg->buf[index++] = ESP32_GET_TIMESTAMP;
    for (int i = 0; i < strlen(buffer); i++)
    {
      msg->buf[index++] = buffer[i];
      printf("%x ", buffer[i]);
    }
    printf("\n");
    msg->len = index;
  }
  else if (msg->buf[0] == CMD_REQUEST && msg->buf[1] == TARGET_SWITCH && msg->buf[2] == SWITCH_READ_LEVEL)
  {
    if (msg->buf[3] == TARGET_SWITCH_1 || msg->buf[3] == TARGET_SWITCH_2)
    {
      encodeSwitchRead(msg, msg->buf[3], 0);
    }
    else
    {
      printf("Unrecognized cmd\n");
      encodeSwitchRead(msg, msg->buf[3], 1);
    }
  }
  else if (msg->buf[0] == CMD_REQUEST && msg->buf[1] == TARGET_ESP32 && msg->buf[2] == ESP32_GET_VERSION)
  {
    printf("getVersion()\n");
    encodeVersionRead(msg);
  }
  else
  {
    encodeUnknowCmd(msg, msg->buf[1], msg->buf[2], 1);
    printf("Unrecognized cmd\n");
  }
  if (xQueueSend(queue, msg, 100 / portTICK_RATE_MS))
  {
    ESP_LOGI(TAG, "msg send cmd mode UT %d", msg->len);
  }
  else
  {
    ESP_LOGE(TAG, "msg send  Failed ->\n");
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
        if (xQueueReceive(xManager[i].rx_queue, &msg, 50 / portTICK_RATE_MS))
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
  int level_switch;
  while (1)
  {
    ////////////////////////
    // to check rx queue, change global status
    // check_rx_queues();
    check_rx_queues();

    if (xQueueReceive(uartQueue, &msg, 50 / portTICK_RATE_MS))
    {
      printf("-> rx msg %d\n", msg.len);
      // send out uart data
      ///////////////////////////
      // send sth. out to tx queue
      broadcast(&msg);

      // send out switch data
    }

    // check gpio interrupt
    /*
    if (xQueueReceive(gpio_evt_queue, &io_num, 50 / portTICK_RATE_MS))
    {
      int64_t iTime = getstamp64();
      level_switch = gpio_get_level(io_num);
      // printf("GPIO[%d] intr, val: %d\n", io_num, level_switch);

      if (io_num == GPIO_DIN_1)
      {
        num_switch = TARGET_SWITCH_1;
        printf("DIN1 intr, val: %d\n", level_switch);
      }
      else if (io_num == GPIO_DIN_2)
      {
        num_switch = TARGET_SWITCH_2;
        printf("DIN2 intr, val: %d\n", level_switch);
      }
      else
      {
        ESP_LOGE(TAG, "Unknown io num: %d", io_num);
        continue;
      }
      encodeSwitchN(&msg, num_switch, level_switch, iTime);
      broadcast(&msg);
    }
    */
    level_switch = get_din1();
    if (level_switch != stateDin1)
    {
      int64_t iTime = getstamp64();
      printf("DIN1 intr, val: %d\n", level_switch);
      encodeSwitchN(&msg, 1, level_switch, iTime);
      broadcast(&msg);
      stateDin1 = level_switch;
    }
    level_switch = get_din2();
    if (level_switch != stateDin2)
    {
      int64_t iTime = getstamp64();
      printf("DIN2 intr, val: %d\n", level_switch);
      encodeSwitchN(&msg, 2, level_switch, iTime);
      broadcast(&msg);
      stateDin2 = level_switch;
    }
  }
}
void init_ctrltask()
{
  xTaskCreate(ctrl_task, "ctrl_task", 2048, NULL, 6, NULL);
}