// components/application/app_gui/app_touch.cpp
#include "app_touch.h"
#include "SensorLib.h"
#include "touch/TouchClassCST816.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "AppTouch";

// Pin mapping and I2C port for the touch controller.定义触摸 I2C 引脚、RST/INT 引脚、I2C 端口。
#define TOUCH_SDA  14
#define TOUCH_SCL  13
#define TOUCH_RST  12
#define TOUCH_INT  11
#define TOUCH_I2C_PORT  I2C_NUM_1

TouchClassCST816 touch;   //触控芯片对象（来自 SensorLib，具体芯片兼容 CST8xx 系列）。
int16_t touch_x[5], touch_y[5];

static void touch_i2c_init(void)   //配置 I2C 主机并安装驱动。
{
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)TOUCH_SDA;
    conf.scl_io_num = (gpio_num_t)TOUCH_SCL;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000;

    i2c_param_config(TOUCH_I2C_PORT, &conf);
    if (i2c_driver_install(TOUCH_I2C_PORT, conf.mode, 0, 0, 0) != ESP_OK) {
        ESP_LOGW(TAG, "Touch I2C already installed or failed to install");
    }
}

// C entry: touch init
void app_touch_init(void) {
    touch_i2c_init();  //初始化 I2C。

    touch.setPins(TOUCH_RST, TOUCH_INT);//设置触摸芯片的 RST/INT 引脚。

    // CST820 uses the CST8xx driver family (address 0x15).通过 touch.begin() 连接触控芯片（地址 0x15）。
    bool has_begun = touch.begin(TOUCH_I2C_PORT, 0x15, TOUCH_SDA, TOUCH_SCL);
    if (!has_begun) {
        ESP_LOGE(TAG, "Failed to find CST820 - check wiring!");
        return;
    }

//复位芯片，设置最大坐标 466x466，镜像设置。
    touch.reset();
    touch.setMaxCoordinates(466, 466);
    touch.setMirrorXY(false, false);

    ESP_LOGI(TAG, "Touch initialized successfully");
}

// C entry: read touch point 读取 1 个触点坐标（调用 touch.getPoint()）。
bool app_touch_read(int32_t *x, int32_t *y) {
    uint8_t touched = touch.getPoint(touch_x, touch_y, 1);
    if (touched > 0) {
        *x = touch_x[0];
        *y = touch_y[0];
        return true;  //如果有触点，就输出坐标并返回 true。
    }
    return false;
}
