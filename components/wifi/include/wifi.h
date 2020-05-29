/**
 * 
 * 
 */

#pragma once

#define EXAMPLE_ESP_WIFI_SSID "nanchao_test"
#define EXAMPLE_ESP_WIFI_PASS "nanchao.org"
#define EXAMPLE_MAX_STA_CONN 3
#define EXAMPLE_ESP_MAXIMUM_RETRY 10

#define EXAMPLE_STATIC_IP "192.168.32.108"
#define LONG_STATIC_IP ((u32_t)0xC0A8206CUL)

#define ESP_AP_SSID "ruff-hub-sensor"
#define ESP_AP_PASS "nanchao123"

#define MAX_SSID_PASS_LEN 24

// Prototypes
// wifi init
// void init_softap();
void init_sta();

char *getSSID();
char *getPASS();
char *getOriginalSSID();
char *getOriginalPASS();
char *getNewSSID();
char *getNewPASS();

int getWifiLink();

int validNewSSIDPASS();

int setNewSSID(char *str);
int setNewPASS(char *str);

void load_new_SSID_PASS();
int save_new_SSID_PASS(char *ssid, char *pass);
