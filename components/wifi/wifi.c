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
#include "nvs.h"
#include "nvs_flash.h"

#include "include/wifi.h"
#include "../gpio/include/gpio.h"

static int stateLink = 0;
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

char strNewSSID[MAX_SSID_PASS_LEN];
char strNewPASS[MAX_SSID_PASS_LEN];

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// static const char *TAG = "wifi softAP";
static const char *TAG = "wifi sta";
static int s_retry_num = 0;

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
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
      stateLink = 0;
      //turn off the led link
      off_led_link();
    }
    else
    {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);

      ESP_LOGI(TAG, "To restart after 15 seconds");
      vTaskDelay(15000 / portTICK_PERIOD_MS);
      esp_restart();
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

    stateLink = 1;
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
void init_staap()
{
  int result = 0;
  load_new_SSID_PASS();

  s_wifi_event_group = xEventGroupCreate();
  tcpip_adapter_init();
  // to use static IP
  tcpip_adapter_ip_info_t static_IP_info;
  IP4_ADDR(&static_IP_info.ip, 192, 168, 34, 108);
  IP4_ADDR(&static_IP_info.gw, 192, 168, 34, 1);
  IP4_ADDR(&static_IP_info.netmask, 255, 255, 255, 0);

  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // stop dhcp
  tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
  tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &static_IP_info);

  // wifi_init
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

  wifi_config_t wifi_config = {
      .sta = {
          .ssid = EXAMPLE_ESP_WIFI_SSID,
          .password = EXAMPLE_ESP_WIFI_PASS},
  };
  //   EXAMPLE_ESP_WIFI_PASS},
  if (validNewSSIDPASS() == 0)
  {
    sprintf((char *)wifi_config.sta.ssid, "%s", getNewSSID());
    sprintf((char *)wifi_config.sta.password, "%s", getNewPASS());
  }
  // ap
  wifi_config_t ap_config = {
      .ap = {
          .ssid = ESP_AP_SSID,
          .ssid_len = strlen(ESP_AP_SSID),
          .password = ESP_AP_PASS,
          .max_connection = 5,
          .authmode = WIFI_AUTH_WPA_WPA2_PSK},
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_sta config method finished.");

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
    if (validNewSSIDPASS() == 0)
    {
      ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
               getNewSSID(), getNewPASS());
    }
    else
    {
      ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
               EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    }
  }
  else if (bits & WIFI_FAIL_BIT)
  {
    if (validNewSSIDPASS() == 0)
    {
      ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
               getNewSSID(), getNewPASS());
    }
    else
    {
      ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
               EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    }
    result = -1;
  }
  else
  {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
    result = -1;
  }

  if (result == -1)
  {
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);

    // connect failure
    // delay 2 minutes then
    if (validNewSSIDPASS() == 0)
    { // try to do some reconnect
      esp_wifi_stop();
      esp_wifi_deinit();
      ESP_LOGI(TAG, "stop wifi , wait 5 seconds and do reconnect()");
      vTaskDelay(5000 / portTICK_PERIOD_MS);
      reconnect_with_old_ssidpass();
    }
    else
    {
      printf("wait 120 s to restart\n");
      vTaskDelay(TIME_BEFORE_RESTART / portTICK_PERIOD_MS);
      printf("Restarting now.\n");
      fflush(stdout);
      esp_restart();
    }
  }
}
void reconnect_with_old_ssidpass()
{
  int result = 0;
  s_wifi_event_group = xEventGroupCreate();

  // wifi_init
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
          .max_connection = 5,
          .authmode = WIFI_AUTH_WPA_WPA2_PSK},
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "reconnect - wifi_init_sta finished.");

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

    result = -1;
  }
  else
  {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
    result = -1;
  }

  if (result == -1)
  {
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);

    printf("wait 120 s to restart\n");
    vTaskDelay(TIME_BEFORE_RESTART / portTICK_PERIOD_MS);
    printf("reconnect - Restarting now.\n");
    fflush(stdout);
    esp_restart();
  }
}

char *getOriginalSSID()
{
  return EXAMPLE_ESP_WIFI_SSID;
}
char *getOriginalPASS()
{
  return EXAMPLE_ESP_WIFI_PASS;
}
char *getNewSSID()
{
  return strNewSSID;
}
char *getNewPASS()
{
  return strNewPASS;
}

int getWifiLink()
{
  return stateLink;
}
int setNewSSID(char *str)
{
  if (strlen(str) >= MAX_SSID_PASS_LEN)
  {
    return -1;
  }
  sprintf(strNewSSID, "%s", str);
  return 0;
}
int setNewPASS(char *str)
{
  if (strlen(str) >= MAX_SSID_PASS_LEN)
  {
    return -1;
  }
  sprintf(strNewPASS, "%s", str);
  return 0;
}
int validNewSSIDPASS()
{
  if (strlen(strNewPASS) == 0)
  {
    return -1;
  }
  if (strlen(strNewSSID) == 0)
  {
    return -1;
  }
  return 0;
}
void load_new_SSID_PASS()
{
  esp_err_t err;
  nvs_handle_t my_handle;
  size_t len;
  err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "load new open storage failed");
  }
  else
  {
    err = nvs_get_str(my_handle, "newssid", strNewSSID, &len);

    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "read newssid failed");
    }
    printf("newssid read out :%d\n", len);

    err = nvs_get_str(my_handle, "newpass", strNewPASS, &len);

    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "read newpass failed");
    }
    printf("newpass read out :%d\n", len);
    nvs_close(my_handle);

    printf("print newssid len: %d\n", strlen(strNewSSID));
    printf("print newpass len: %d\n", strlen(strNewPASS));
  }
}
int save_new_SSID_PASS(char *ssid, char *pass)
{
  esp_err_t err;
  nvs_handle_t my_handle;

  int result = 0;
  err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "open storage failed %x", err);
    result = -1;
  }
  else
  {
    err = nvs_set_str(my_handle, "newssid", ssid);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "save newssid failed");
      result = -1;
    }
    err = nvs_set_str(my_handle, "newpass", pass);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "save newpass failed");
      result = -1;
    }

    nvs_close(my_handle);
  }
  if (result == 0)
  {
    setNewSSID(ssid);
    setNewPASS(pass);
  }
  return result;
}