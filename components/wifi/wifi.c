#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "include/wifi.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// static const char *TAG = "wifi softAP";
static const char *TAG = "wifi sta";
static int s_retry_num = 0;
// static void wifi_event_handler(void *arg, esp_event_base_t event_base,
//                                int32_t event_id, void *event_data)
// {
//   if (event_id == WIFI_EVENT_AP_STACONNECTED)
//   {
//     wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
//     ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
//              MAC2STR(event->mac), event->aid);
//   }
//   else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
//   {
//     wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
//     ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
//              MAC2STR(event->mac), event->aid);
//   }
// }
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
  {
    esp_wifi_connect();
  }
  else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
  {
    if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
    {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
    }
    else
    {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(TAG, "connect to the AP fail");
  }
  else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "got ip:%s",
             ip4addr_ntoa(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
  else if (event_id == WIFI_EVENT_AP_STACONNECTED)
  {
    wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
             MAC2STR(event->mac), event->aid);
  }
  else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
  {
    wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
             MAC2STR(event->mac), event->aid);
  }
  else if (event_id == WIFI_EVENT_AP_START)
  {
    ESP_LOGI(TAG, "AP Started");
    // open http server
  }
  else if (event_id == WIFI_EVENT_AP_STOP)
  {
    ESP_LOGI(TAG, "AP Stopped");
    // close http server
  }
}
void init_sta()
{

  s_wifi_event_group = xEventGroupCreate();
  tcpip_adapter_init();
  // to use static IP
  tcpip_adapter_ip_info_t static_IP_info;
  IP4_ADDR(&static_IP_info.ip, 192, 168, 32, 108);
  IP4_ADDR(&static_IP_info.gw, 192, 168, 32, 1);
  IP4_ADDR(&static_IP_info.netmask, 255, 255, 255, 0);
  // tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &static_IP_info);

  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // stop dhcp
  tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
  tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &static_IP_info);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

  wifi_config_t wifi_config = {
      .sta = {
          .ssid = EXAMPLE_ESP_WIFI_SSID,
          .password = EXAMPLE_ESP_WIFI_PASS},
  };

  // ap
  wifi_config_t ap_config = {
      .ap = {
          .ssid = ESP_AP_SSID,
          .ssid_len = strlen(ESP_AP_SSID),
          .password = ESP_AP_PASS,
          .max_connection = 2,
          .authmode = WIFI_AUTH_WPA_WPA2_PSK},
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_sta finished.");

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE,
                                         pdFALSE,
                                         portMAX_DELAY);

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
  if (bits & WIFI_CONNECTED_BIT)
  {
    ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
  }
  else if (bits & WIFI_FAIL_BIT)
  {
    ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
  }
  else
  {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
  }

  ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
  ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
  vEventGroupDelete(s_wifi_event_group);
}

// void init_softap()
// {
//   ESP_LOGI(TAG, "Soft App ");
//   tcpip_adapter_init();
//   ESP_ERROR_CHECK(esp_event_loop_create_default());

//   wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//   ESP_ERROR_CHECK(esp_wifi_init(&cfg));

//   ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

//   wifi_config_t wifi_config = {
//       .ap = {
//           .ssid = EXAMPLE_ESP_WIFI_SSID,
//           .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
//           .password = EXAMPLE_ESP_WIFI_PASS,
//           .max_connection = EXAMPLE_MAX_STA_CONN,
//           .authmode = WIFI_AUTH_WPA_WPA2_PSK},
//   };

//   if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0)
//   {
//     wifi_config.ap.authmode = WIFI_AUTH_OPEN;
//   }

//   ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
//   ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
//   ESP_ERROR_CHECK(esp_wifi_start());

//   ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s",
//            EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
// }