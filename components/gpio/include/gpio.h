// Copyright 2020 yango3

#pragma once

#define GPIO_LED_LINK 26
#define GPIO_LED_STAT 27
#define GPIO_KEY_2 34
#define GPIO_KEY_3 35
#define GPIO_DIN_1 33
#define GPIO_DIN_2 25

#define GPIO_UT_PWR_EN 14

#define GPIO_INPUT_PIN_SEL ((1ULL << GPIO_KEY_2) | (1ULL << GPIO_KEY_3))
#define GPIO_INPUT_PIN_TRIG_SEL ((1ULL << GPIO_DIN_1) | (1ULL << GPIO_DIN_2))
#define GPIO_OUTPUT_PIN_SEL ((1ULL << GPIO_LED_LINK) | (1ULL << GPIO_LED_STAT) | (1ULL << GPIO_UT_PWR_EN))

#define ESP_INTR_FLAG_DEFAULT 0

void on_led_link();
void off_led_link();
void on_led_stat();
void off_led_stat();
void on_ut_pwr();
void off_ut_pwr();
void off_led_stat();
void on_ut_pwr();
void off_ut_pwr();
void conf_gpio();
int get_din1();
int get_din2();
