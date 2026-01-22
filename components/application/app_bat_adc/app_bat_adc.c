#include "app_bat_adc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"

#define APP_BAT_ADC_GPIO         GPIO_NUM_8
#define APP_BAT_ADC_ATTEN        ADC_ATTEN_DB_12
#define APP_BAT_ADC_BITWIDTH     ADC_BITWIDTH_12
#define APP_BAT_ADC_SAMPLE_COUNT 8
#define APP_BAT_ADC_PERIOD_MS    3000

static const char *TAG = "app_bat_adc";

static adc_oneshot_unit_handle_t s_adc_handle;
static adc_cali_handle_t s_cali_handle;
static adc_unit_t s_adc_unit;
static adc_channel_t s_adc_channel;
static bool s_calibrated;
static TaskHandle_t s_task_handle;

static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten,
                                 adc_bitwidth_t bitwidth, adc_cali_handle_t *out_handle)
{
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t curve_cfg = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = bitwidth,
    };
    if (adc_cali_create_scheme_curve_fitting(&curve_cfg, out_handle) == ESP_OK) {
        calibrated = true;
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        adc_cali_line_fitting_config_t line_cfg = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = bitwidth,
        };
        if (adc_cali_create_scheme_line_fitting(&line_cfg, out_handle) == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    return calibrated;
}

static esp_err_t bat_adc_init(void)
{
    esp_err_t ret = adc_oneshot_io_to_channel(APP_BAT_ADC_GPIO, &s_adc_unit, &s_adc_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc io map failed: %s", esp_err_to_name(ret));
        return ret;
    }

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = s_adc_unit,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ret = adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc unit init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = APP_BAT_ADC_BITWIDTH,
        .atten = APP_BAT_ADC_ATTEN,
    };
    ret = adc_oneshot_config_channel(s_adc_handle, s_adc_channel, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc channel config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    s_calibrated = adc_calibration_init(s_adc_unit, APP_BAT_ADC_ATTEN,
                                        APP_BAT_ADC_BITWIDTH, &s_cali_handle);
    if (!s_calibrated) {
        ESP_LOGW(TAG, "adc calibration not available, raw only");
    }

    return ESP_OK;
}

static int bat_adc_read_avg(void)
{
    int raw = 0;
    (void)adc_oneshot_read(s_adc_handle, s_adc_channel, &raw); // discard first read

    int sum = 0;
    int valid = 0;
    for (int i = 0; i < APP_BAT_ADC_SAMPLE_COUNT; ++i) {
        if (adc_oneshot_read(s_adc_handle, s_adc_channel, &raw) == ESP_OK) {
            sum += raw;
            ++valid;
        }
    }

    return (valid > 0) ? (sum / valid) : 0;
}

static void bat_adc_task(void *arg)
{
    (void)arg;
    for (;;) {
        int raw_avg = bat_adc_read_avg();
        if (s_calibrated) {
            int vadc_mv = 0;
            if (adc_cali_raw_to_voltage(s_cali_handle, raw_avg, &vadc_mv) == ESP_OK) {
                float vbat_v = (vadc_mv * 2.0f) / 1000.0f;
                ESP_LOGI(TAG, "raw_avg=%d, Vadc=%dmV, Vbat=%.3fV", raw_avg, vadc_mv, vbat_v);
            } else {
                ESP_LOGI(TAG, "raw_avg=%d", raw_avg);
            }
        } else {
            ESP_LOGI(TAG, "raw_avg=%d", raw_avg);
        }
        vTaskDelay(pdMS_TO_TICKS(APP_BAT_ADC_PERIOD_MS));
    }
}

esp_err_t app_bat_adc_start(void)
{
    if (s_task_handle) {
        return ESP_OK;
    }

    esp_err_t ret = bat_adc_init();
    if (ret != ESP_OK) {
        return ret;
    }

    BaseType_t ok = xTaskCreate(bat_adc_task, "bat_adc", 3072, NULL, 4, &s_task_handle);
    return (ok == pdPASS) ? ESP_OK : ESP_FAIL;
}
