#pragma once

#include "esp_ota_ops.h"

char *getVersion();

char *getVersion()
{
  const esp_app_desc_t *app_desc = esp_ota_get_app_description();
  return app_desc->version;
}