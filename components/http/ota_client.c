#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "protocol_examples_common.h"

#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"

#include "./include/http.h"
#include "../util/include/util.h"

#define BUFFSIZE 1024
#define HASH_LEN 32 /* SHA-256 digest length */
#define BIT_0 (1 << 0)

static const char *TAG = "ota_client";
/*an ota data write buffer ready to write to the flash*/
static char ota_write_data[BUFFSIZE + 1] = {0};
static char OTA_URL[64] = {0};
static uint8_t OTA_STATUS = 0;

// Declare a variable to hold the created event group.
EventGroupHandle_t xCreatedEventGroup;

int set_ota_url(char *p)
{
  sprintf(OTA_URL, "%s", p);
  return 0;
}

uint8_t get_ota_status()
{
  return OTA_STATUS;
}

int trigure_ota_event()
{
  ESP_LOGI(TAG, "trigure_ota_event");
  xEventGroupSetBits(xCreatedEventGroup, BIT_0);
  return 0;
}

// static bool diagnostic(void)
// {
//   gpio_config_t io_conf;
//   io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
//   io_conf.mode = GPIO_MODE_INPUT;
//   io_conf.pin_bit_mask = (1ULL << CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);
//   io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
//   io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
//   gpio_config(&io_conf);

//   ESP_LOGI(TAG, "Diagnostics (5 sec)...");
//   vTaskDelay(5000 / portTICK_PERIOD_MS);

//   bool diagnostic_is_ok = gpio_get_level(CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);

//   gpio_reset_pin(CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);
//   return diagnostic_is_ok;
// }

static void print_sha256(const uint8_t *image_hash, const char *label)
{
  char hash_print[HASH_LEN * 2 + 1];
  hash_print[HASH_LEN * 2] = 0;
  for (int i = 0; i < HASH_LEN; ++i)
  {
    sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
  }
  ESP_LOGI(TAG, "%s: %s", label, hash_print);
}
void upgrade()
{
  ESP_LOGI(TAG, "Start upgrade ...");
  OTA_STATUS = UPGRADE_STATUS_GOING;
  printf("url:%s\n", OTA_URL);
}
static void ota_example_task(void *pvParameter)
{
  EventBits_t uxBits;
  esp_err_t err;

  ESP_LOGI(TAG, "Starting OTA task");
  while (1)
  {
    uxBits = xEventGroupWaitBits(
        xCreatedEventGroup,
        BIT_0,
        pdTRUE,
        pdFALSE,
        portMAX_DELAY);
    if ((uxBits & BIT_0) == BIT_0)
    {
      ESP_LOGI(TAG, "received OTA command");
      upgrade();
    }
    else
    {
      ESP_LOGI(TAG, "received Strange command");
    }
  }
}

void init_ota_client()
{

  uint8_t sha_256[HASH_LEN] = {0};
  esp_partition_t partition;

  ESP_LOGI(TAG, "init OTA Client");
  // get sha256 digest for the partition table
  partition.address = ESP_PARTITION_TABLE_OFFSET;
  partition.size = ESP_PARTITION_TABLE_MAX_LEN;
  partition.type = ESP_PARTITION_TYPE_DATA;
  esp_partition_get_sha256(&partition, sha_256);
  print_sha256(sha_256, "SHA-256 for the partition table: ");

  // get sha256 digest for bootloader
  partition.address = ESP_BOOTLOADER_OFFSET;
  partition.size = ESP_PARTITION_TABLE_OFFSET;
  partition.type = ESP_PARTITION_TYPE_APP;
  esp_partition_get_sha256(&partition, sha_256);
  print_sha256(sha_256, "SHA-256 for bootloader: ");

  // get sha256 digest for running partition
  esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
  print_sha256(sha_256, "SHA-256 for current firmware: ");

  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t ota_state;
  if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK)
  {
    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
    {
      // run diagnostic function ...
      // bool diagnostic_is_ok = diagnostic();
      bool diagnostic_is_ok = false;
      if (diagnostic_is_ok)
      {
        ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
        esp_ota_mark_app_valid_cancel_rollback();
      }
      else
      {
        ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
        esp_ota_mark_app_invalid_rollback_and_reboot();
      }
    }
  }

  // Attempt to create the event group.
  xCreatedEventGroup = xEventGroupCreate();

  // Was the event group created successfully?
  if (xCreatedEventGroup == NULL)
  {
    // The event group was not created because there was insufficient
    // FreeRTOS heap available.
    ESP_LOGE(TAG, "EventGroup creation failed");
  }
  else
  {
    // The event group was created.
    ESP_LOGI(TAG, "EventGroup creation succeed");
  }

  xTaskCreate(&ota_example_task, "ota_task", 8192, NULL, 5, NULL);
}