#pragma once

void init_http();

void init_ota_client();

int set_ota_url(char *p);

uint8_t get_ota_status();

int trigure_ota_event();
