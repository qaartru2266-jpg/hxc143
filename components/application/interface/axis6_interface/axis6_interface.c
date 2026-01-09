#include <stdio.h>
#include "esp_log.h"
#include "math.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c.h"

#include "sdkconfig.h"
#include "axis6_interface.h"


static const char * TAG = "axis6";
#define LSM6DS3_ADDR_LOW   0x6A
#define LSM6DS3_ADDR_HIGH  0x6B
#define LSM6DS3_WHO_AM_I   0x0F
#define LSM6DS3_WHO_AM_I_VAL 0x69
#define LSM6DS3_WHO_AM_I_ALT 0x6A
#define LSM6DS3_CTRL1_XL   0x10
#define LSM6DS3_CTRL2_G    0x11
#define LSM6DS3_CTRL3_C    0x12
#define LSM6DS3_STATUS_REG 0x1E
#define LSM6DS3_OUTX_L_G   0x22

#define LSM6DS3_STATUS_XLDA 0x01
#define LSM6DS3_STATUS_GDA  0x02

static uint8_t s_imu_addr = LSM6DS3_ADDR_LOW;


esp_err_t i2c_master_init(void)
{
    int i2c_master_port = 0;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CONFIG_JOFTMODE_I2C_SDA,
        .scl_io_num = CONFIG_JOFTMODE_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = CONFIG_JOFTMODE_I2C_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(i2c_master_port, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: 0x%x", err);
        return err;
    }

    err = i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
    if (err == ESP_ERR_INVALID_STATE) {
        err = ESP_OK;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed: 0x%x", err);
    }

    return err;
}


/***************************  姿态传感器 QMI8658 ↓   ****************************/

// 读取QMI8658寄存器的值
esp_err_t qmi8658_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(0, s_imu_addr,  &reg_addr, 1, data, len, 1000 / portTICK_PERIOD_MS);
}

// 给QMI8658的寄存器写值
esp_err_t qmi8658_register_write_byte(uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};

    return i2c_master_write_to_device(0, s_imu_addr, write_buf, sizeof(write_buf), 1000 / portTICK_PERIOD_MS);
}

// 初始化qmi8658
void qmi8658_init(void)
{
    uint8_t id = 0;
    esp_err_t err = ESP_FAIL;

    while (1) {
        s_imu_addr = LSM6DS3_ADDR_LOW;
        err = qmi8658_register_read(LSM6DS3_WHO_AM_I, &id, 1);
        if (err == ESP_OK &&
            (id == LSM6DS3_WHO_AM_I_VAL || id == LSM6DS3_WHO_AM_I_ALT)) {
            break;
        }

        s_imu_addr = LSM6DS3_ADDR_HIGH;
        err = qmi8658_register_read(LSM6DS3_WHO_AM_I, &id, 1);
        if (err == ESP_OK &&
            (id == LSM6DS3_WHO_AM_I_VAL || id == LSM6DS3_WHO_AM_I_ALT)) {
            break;
        }

        ESP_LOGW(TAG, "LSM6DS3 not found, retry...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "LSM6DS3 OK! addr=0x%02x id=0x%02x", s_imu_addr, id);

    qmi8658_register_write_byte(LSM6DS3_CTRL3_C, 0x01);  // soft reset
    vTaskDelay(10 / portTICK_PERIOD_MS);
    qmi8658_register_write_byte(LSM6DS3_CTRL3_C, 0x44);  // BDU + IF_INC
    qmi8658_register_write_byte(LSM6DS3_CTRL1_XL, 0x38); // 52Hz, 4g
    qmi8658_register_write_byte(LSM6DS3_CTRL2_G, 0x34);  // 52Hz, 500dps
}


// 读取加速度和陀螺仪寄存器值
void qmi8658_Read_AccAndGry(t_sQMI8658 *p)
{
    uint8_t status, data_ready=0;
    int16_t buf[6];

    if (p == NULL) {
        return;
    }

    qmi8658_register_read(LSM6DS3_STATUS_REG, &status, 1);
    if (status & (LSM6DS3_STATUS_XLDA | LSM6DS3_STATUS_GDA))
        data_ready = 1;
    if (data_ready == 1){
        data_ready = 0;
        qmi8658_register_read(LSM6DS3_OUTX_L_G, (uint8_t *)buf, 12);
        p->gyr_x = buf[0];
        p->gyr_y = buf[1];
        p->gyr_z = buf[2];
        p->acc_x = buf[3];
        p->acc_y = buf[4];
        p->acc_z = buf[5];
    }
}


// 获取XYZ轴的倾角值
void qmi8658_fetch_angleFromAcc(t_sQMI8658 *p)
{
    float temp;

    qmi8658_Read_AccAndGry(p); // 读取加速度和陀螺仪的寄存器值
    // 根据寄存器值 计算倾角值 并把弧度转换成角度
    temp = (float)p->acc_x / sqrt( ((float)p->acc_y * (float)p->acc_y + (float)p->acc_z * (float)p->acc_z) );
    p->AngleX = atan(temp)*57.29578f; // 180/π=57.29578
    temp = (float)p->acc_y / sqrt( ((float)p->acc_x * (float)p->acc_x + (float)p->acc_z * (float)p->acc_z) );
    p->AngleY = atan(temp)*57.29578f; // 180/π=57.29578
    temp = sqrt( ((float)p->acc_x * (float)p->acc_x + (float)p->acc_y * (float)p->acc_y) ) / (float)p->acc_z;
    p->AngleZ = atan(temp)*57.29578f; // 180/π=57.29578
}
/***************************  姿态传感器 QMI8658 ↑  ****************************/
