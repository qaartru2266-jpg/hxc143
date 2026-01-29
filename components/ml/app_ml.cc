#include "app_ml.h"
#include "ml_runner.h"
#include "esp_log.h"

static const char* TAG = "app_ml";

bool app_ml_init(void) {
    bool ok = ml_init();
    if (ok) {
        ESP_LOGI(TAG, "ml_init OK");
    } else {
        ESP_LOGE(TAG, "ml_init FAIL");
    }
    return ok;
}