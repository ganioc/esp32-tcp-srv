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

  off_led_link();
  off_led_stat();
  off_ut_pwr();
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