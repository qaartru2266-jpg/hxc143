idf_component_register(
    SRCS "app_antenna.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_event esp_netif bt
)
