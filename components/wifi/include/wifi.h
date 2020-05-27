/**
 * 
 * 
 */

#pragma once

#define EXAMPLE_ESP_WIFI_SSID "nanchao_test"
#define EXAMPLE_ESP_WIFI_PASS "nanchao.org"
#define EXAMPLE_MAX_STA_CONN 3
#define EXAMPLE_ESP_MAXIMUM_RETRY 5

#define EXAMPLE_STATIC_IP "192.168.32.108"

// Prototypes
// wifi init
// void init_softap();
void init_sta();
