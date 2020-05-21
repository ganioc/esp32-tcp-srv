#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "./include/usart.h"

uint8_t datausart[BUF_SIZE];

static void usart_task()
{

  while (1)
  {
    int len = uart_read_bytes(UART_NUM_1, datausart, BUF_SIZE, 100 / portTICK_RATE_MS);

    if (len > 0)
    {
      printf("Usart rx %d\n", len);
    }
    else
    {
      printf("Usart rx non\n");
    }
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

  xTaskCreate(usart_task, "usart_task", 1024, NULL, 4, NULL);
}