
# set(COMPONENT_SRCDIRS ".")
# set(COMPONENT_ADD_INCLUDEDIRS "include")
# register_component()

idf_component_register(SRCS "wifi.c" 
                       INCLUDE_DIRS "." "include" "$ENV{IDF_PATH}/examples/common_components/protocol_examples_common/include"
                       PRIV_REQUIRES  nvs_flash 
                       )