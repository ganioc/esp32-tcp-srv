#include <stdio.h>
#include "driver/gpio.h"

#include "./include/gpio.h"

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
}