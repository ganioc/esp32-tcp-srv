#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "./include/gpio.h"

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
  uint32_t gpio_num = (uint32_t)arg;
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void print_gpio()
{
  printf("I am gpio\n");
}

void config_key_2()
{
}
void config_key_3()
{
}

void conf_gpio()
{
  // set output pins
  gpio_config_t io_conf;
  // disable interrupt
  io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  off_led_link();
  off_led_stat();
  off_ut_pwr();

  // set input pins
  //interrupt of rising edge
  io_conf.intr_type = GPIO_PIN_INTR_ANYEDGE;
  //bit mask of the pins, use GPIO4/5 here
  io_conf.pin_bit_mask = GPIO_INPUT_PIN_TRIG_SEL;
  //set as input mode
  io_conf.mode = GPIO_MODE_INPUT;
  //enable pull-up mode
  io_conf.pull_up_en = 1;
  gpio_config(&io_conf);

  //create a queue to handle gpio event from isr
  gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

  //install gpio isr service
  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  //hook isr handler for specific gpio pin
  gpio_isr_handler_add(GPIO_DIN_1, gpio_isr_handler, (void *)GPIO_DIN_1);
  //hook isr handler for specific gpio pin
  gpio_isr_handler_add(GPIO_DIN_2, gpio_isr_handler, (void *)GPIO_DIN_2);
}
void on_ut_pwr()
{
  gpio_set_level(GPIO_UT_PWR_EN, 1);
}
void off_ut_pwr()
{
  gpio_set_level(GPIO_UT_PWR_EN, 0);
}
void on_led_link()
{
  gpio_set_level(GPIO_LED_LINK, 0);
}
void off_led_link()
{
  gpio_set_level(GPIO_LED_LINK, 1);
}
void on_led_stat()
{
  gpio_set_level(GPIO_LED_STAT, 0);
}
void off_led_stat()
{
  gpio_set_level(GPIO_LED_STAT, 1);
}