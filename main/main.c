/* Hello World Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "nvs_flash.h"
#include "esp_task_wdt.h"
#include "esp_ota_ops.h"

#include "../components/gpio/include/gpio.h"
#include "../components/wifi/include/wifi.h"
#include "../components/tcp/include/server.h"
#include "../components/usart/include/usart.h"
#include "../components/queue/include/queue.h"
#include "../components/ctrltask/include/ctrltask.h"
#include "../components/http/include/http.h"
#include "../components/util/include/util.h"

#define TWDT_TIMEOUT_S 8

char VERSION[32];

void app_main()
{
    const esp_app_desc_t *app_desc = esp_ota_get_app_description();
    printf("Hello world!\n");
    printf("version %s\n", app_desc->version);
    sprintf(VERSION, "%s", app_desc->version);

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Initialize TWDT\n");

    // init queue
    printf("\nConfig queue\n");
    init_queue();

    // Configuration
    printf("\nConfig GPIO");
    conf_gpio();

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // turn on Wifi
    printf("\nTurn on wifi station\n");
    init_staap();

    // tcp server on
    printf("\nTurn on tcp server\n");
    init_server();

    // http server on
    init_http();

    // ota client on
    init_ota_client();

    //Initialize or reinitialize TWDT
    CHECK_ERROR_CODE(esp_task_wdt_init(TWDT_TIMEOUT_S, true), ESP_OK);

    // turn on uart task
    printf("\ninit usart\n");
    init_usart();

    // turn on ctrl task
    printf("\ninit ctrltask\n");
    init_ctrltask();

    // turn on ultra sonic hub power
    printf("\nTurn on ultra-sonic hub\n");
    on_ut_pwr();

    // I can use this for LED status shining
    int cnt = 0;
    while (1)
    {

        cnt++;
        // printf(". %d\n", cnt++);
        if (cnt % 2 == 1)
        {
            if (getWifiLink())
            {
                on_led_link();
            }
            // if (getIsUpgrading())
            // {
            //     on_led_stat();
            // }
        }
        else
        {
            if (getWifiLink())
            {
                off_led_link();
            }
            // if (getIsUpgrading())
            // {
            //     off_led_stat();
            // }
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
