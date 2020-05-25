#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"

#include "../queue/include/queue.h"
#include "../ctrltask/include/ctrltask.h"
#include "../util/include/util.h"

#include "./include/usart.h"

static const char *TAG = "usart";

extern QueueHandle_t uartQueue;
extern GlobalStatus_t gStatus;

uint8_t datausart[BUF_SIZE];
static Msg_t msg;
static int ENLOG = 0;
int counter = 0;

static int is_valid(int counter, int mode)
{

  if (mode == 1)
  {
    return 0;
  }
  else if (mode == 2)
  {
    if (counter % 2 == 0)
    {
      if (ENLOG == 1)
      {
        printf("mode %d , counter %d\n", mode, counter);
      }
      return 0;
    }
  }
  else
  {
    if (counter % mode == 0)
    {
      if (ENLOG == 1)
      {
        printf("mode %d , counter %d\n", mode, counter);
      }
      return 0;
    }
  }

  return -1;
}
/**
 * How can we know the xQueues from the conn_task? 
 * 
 */
/////////////////////////
// check uart
static void usart_task()
{

  while (1)
  {

    int len = uart_read_bytes(UART_NUM_1, datausart, BUF_SIZE, 130 / portTICK_RATE_MS);

    if (len >= 7)
    {
      counter++;

      if (ENLOG)
      {
        printf("Usart rx %d\n", len);
        for (int i = 0; i < 7; i++)
        {
          printf("0x%2x  ", datausart[i]);
        }
        printf("\n");
      }

      // 0xcb  0x55  0x4
      if (datausart[0] == 0xcb && datausart[1] == 0x55 && datausart[2] == 0x4)
      {
        // send a msg to
        if (gStatus.enable == 1 && is_valid(counter, gStatus.mode) == 0)
        {
          // create notification msg
          int64_t iTime = getstamp64();
          encodeSensorN(&msg, (char *)&datausart[3], iTime);

          if (xQueueSend(uartQueue, &msg, 100 / portTICK_RATE_MS))
          {
            printf("msg send from uart\n");
            ESP_LOGI(TAG, "msg send from uart");
          }
        }
      }
    }

    // check switch 1, DIN1
    

    // check switch 2, DIN2


  }
  vTaskDelete(NULL);
}
void init_usart()
{

  /* Configure parameters of an UART driver,
     * communication pins and install the driver */
  uart_config_t uart_config = {
      .baud_rate = 9600,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

  uart_param_config(UART_NUM_1, &uart_config);

  uart_set_pin(UART_NUM_1, UT_TXD, UT_RXD, -1, -1);
  uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);

  printf("\nCreate uart queue\n");
  uartQueue = create_queue();

  xTaskCreate(usart_task, "usart_task", 2048, NULL, 4, NULL);
}