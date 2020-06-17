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
  esp_err_t err;
  /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
  esp_ota_handle_t update_handle = 0;
  const esp_partition_t *update_partition = NULL;

  const esp_partition_t *configured = esp_ota_get_boot_partition();
  const esp_partition_t *running = esp_ota_get_running_partition();

  ESP_LOGI(TAG, "Start upgrade ...");
  OTA_STATUS = UPGRADE_STATUS_GOING;
  printf("url:%s\n", OTA_URL);

  if (configured != running)
  {
    ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
             configured->address, running->address);
    ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
  }
  ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
           running->type, running->subtype, running->address);

  esp_http_client_config_t config = {
      .url = OTA_URL};
  // clear client in the end or met error
  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == NULL)
  {
    ESP_LOGE(TAG, "Failed to initialise HTTP connection");
    OTA_STATUS = UPGRADE_STATUS_ERROR_INIT_HTTP_CLIENT;
    return;
  }

  err = esp_http_client_open(client, 0);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    OTA_STATUS = UPGRADE_STATUS_ERROR_OPEN_CLIENT;
    return;
  }
  esp_http_client_fetch_headers(client);

  update_partition = esp_ota_get_next_update_partition(NULL);
  ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
           update_partition->subtype, update_partition->address);

  if (update_partition == NULL)
  {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    ESP_LOGW(TAG, "update partion error");
    OTA_STATUS = UPGRADE_STATUS_NEXT_PARTITION_NULL;
    return;
  }

  int binary_file_length = 0;
  bool image_header_was_checked = false;

  while (1)
  {
    ESP_LOGI(TAG, "read from socket %d", BUFFSIZE);
    int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
    if (data_read < 0)
    {
      ESP_LOGE(TAG, "Error: SSL data read error");
      OTA_STATUS = UPGRADE_STATUS_ERROR_READ_CLIENT;
      break;
    }
    else if (data_read > 0)
    {
      ESP_LOGI(TAG, "data_read: %d", data_read);
      if (image_header_was_checked == false)
      {
        esp_app_desc_t new_app_info;
        if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
        {
          // check current version with downloading
          memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
          ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

          esp_app_desc_t running_app_info;
          if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
          {
            ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
          }

          const esp_partition_t *last_invalid_app = esp_ota_get_last_invalid_partition();
          esp_app_desc_t invalid_app_info;

          if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK)
          {
            ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
          }
          // check current version with last invalid partition
          if (last_invalid_app != NULL)
          {
            if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0)
            {
              ESP_LOGW(TAG, "New version is the same as invalid version.");
              ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
              ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
              OTA_STATUS = UPGRADE_STATUS_ERROR_VERSION_SAME;
              break;
            }
          }

          if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0)
          {
            ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
            OTA_STATUS = UPGRADE_STATUS_ERROR_VERSION_SAME;
            break;
          }

          image_header_was_checked = true;

          err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
          if (err != ESP_OK)
          {
            ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
            OTA_STATUS = UPGRADE_STATUS_ERROR_OTA_BEGIN;
            break;
          }
          ESP_LOGI(TAG, "esp_ota_begin succeeded");
        }
        else
        {
          ESP_LOGE(TAG, "received package is not fit len");
          OTA_STATUS = UPGRADE_STATUS_ERROR_LENGTH_OVERLIMIT;
          break;
        }
      }
      err = esp_ota_write(update_handle, (const void *)ota_write_data, data_read);
      if (err != ESP_OK)
      {
        ESP_LOGW(TAG, "Write image error");
        OTA_STATUS = UPGRADE_STATUS_ERROR_WRITE_FLASH;
        break;
      }
      binary_file_length += data_read;
      ESP_LOGI(TAG, "Write image length %d", binary_file_length);
    }
    else if (data_read == 0)
    {
      ESP_LOGI(TAG, "Connection closed,all data received");
      OTA_STATUS = UPGRADE_STATUS_FINISHED_OK;
      break;
    }
  }

  ESP_LOGI(TAG, "Total Write binary data length : %d", binary_file_length);

  esp_http_client_close(client);
  esp_http_client_cleanup(client);

  if (OTA_STATUS == UPGRADE_STATUS_FINISHED_OK)
  {
    if (esp_ota_end(update_handle) != ESP_OK)
    {
      ESP_LOGE(TAG, "esp_ota_end failed!");
      OTA_STATUS = UPGRADE_STATUS_ERROR_OTA_END;
    }
    else
    {
      err = esp_ota_set_boot_partition(update_partition);
      if (err != ESP_OK)
      {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        OTA_STATUS = UPGRADE_STATUS_ERROR_SET_BOOT;
      }
      else
      {
        ESP_LOGI(TAG, "Prepare to restart system after 3 s!");
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        esp_restart();
      }
    }
  }

  ESP_LOGI(TAG, "out of upgrade ...");
}
static void ota_example_task(void *pvParameter)
{
  EventBits_t uxBits;

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