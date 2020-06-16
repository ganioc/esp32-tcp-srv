
# This example uses an extra component for common functions such as Wi-Fi and Ethernet connection.
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)

# set(COMPONENT_SRCS "http.c")
# set(COMPONENT_ADD_INCLUDEDIRS ". include")
# set(COMPONENT_PRIV_REQUIRES  nvs_flash )
# register_component()

idf_component_register(SRCS "http.c" "home.c" "info.c" "setting.c" "ota_client.c"
                       INCLUDE_DIRS "." "include" "$ENV{IDF_PATH}/examples/common_components/protocol_examples_common/include"
                       PRIV_REQUIRES  nvs_flash esp_http_server esp_http_client bootloader_support app_update
                       )
