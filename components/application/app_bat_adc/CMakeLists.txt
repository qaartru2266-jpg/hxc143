idf_component_register(
    SRCS "app_bat_adc.c" "app_bat_adc_globals.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_adc ui
    PRIV_REQUIRES application
)
