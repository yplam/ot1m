//
// Created by yplam on 14/1/2021.
//

#include "sht30d.h"
#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(sht30d, LOG_LEVEL_INF);
#include <device.h>
#include <devicetree.h>
#include <drivers/i2c.h>
#include <nrfx_gpiote.h>

#define EN_NODE DT_ALIAS(btnen)
#define EN_PIN	DT_GPIO_PIN(EN_NODE, gpios)

#define I2C_DEV DT_INST(0, nordic_nrf_twi)

const struct device *i2c_dev;

void app_sht30d_init(void) {
    nrfx_err_t err;

    nrfx_gpiote_out_config_t const out_config = {
            .action = NRF_GPIOTE_POLARITY_TOGGLE,
            .init_state = NRF_GPIOTE_INITIAL_VALUE_LOW,
            .task_pin = false,
    };

    err = nrfx_gpiote_out_init(EN_PIN, &out_config);
    if (err != NRFX_SUCCESS) {
        LOG_ERR("nrfx_gpiote_out_init error: %08x", err);
        return;
    }
    nrfx_gpiote_out_task_enable(EN_PIN);

    i2c_dev = device_get_binding(DT_LABEL(I2C_DEV));
    if (i2c_dev == NULL) {
        LOG_ERR("Could not get i2c dev");
        return;
    }
    (void)device_set_power_state(i2c_dev, DEVICE_PM_LOW_POWER_STATE, NULL, NULL);
    LOG_INF("i2c inited");
}

static int app_sht30d_write_command(uint16_t cmd)
{
    uint8_t tx_buf[2] = { cmd >> 8, cmd & 0xFF };

    return i2c_write(i2c_dev, tx_buf, sizeof(tx_buf), 0x44U);
}


/*
 * CRC algorithm parameters were taken from the
 * "Checksum Calculation" section of the datasheet.
 */
static uint8_t sht3xd_compute_crc(uint16_t value)
{
    uint8_t buf[2] = { value >> 8, value & 0xFF };
    uint8_t crc = 0xFF;
    uint8_t polynom = 0x31;
    int i, j;

    for (i = 0; i < 2; ++i) {
        crc = crc ^ buf[i];
        for (j = 0; j < 8; ++j) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynom;
            } else {
                crc = crc << 1;
            }
        }
    }

    return crc;
}

int app_sht30d_fetch_raw(uint16_t * temp, uint16_t * hum) {
    uint8_t rx_buf[6];
    uint16_t t_sample, rh_sample;
    nrfx_gpiote_out_set(EN_PIN);
    (void)device_set_power_state(i2c_dev, DEVICE_PM_ACTIVE_STATE, NULL, NULL);
    k_msleep(3);
    if (app_sht30d_write_command(0x240B)
        < 0) {
        LOG_DBG("Failed to set single shot measurement mode!");
        return -EIO;
    }
    k_sleep(K_MSEC(7));

    if (i2c_read(i2c_dev, rx_buf, sizeof(rx_buf), 0x44U) < 0) {
        LOG_DBG("Failed to read data sample!");
        return -EIO;
    }
    t_sample = (rx_buf[0] << 8) | rx_buf[1];
    if (sht3xd_compute_crc(t_sample) != rx_buf[2]) {
        LOG_DBG("Received invalid temperature CRC!");
        return -EIO;
    }

    rh_sample = (rx_buf[3] << 8) | rx_buf[4];
    if (sht3xd_compute_crc(rh_sample) != rx_buf[5]) {
        LOG_DBG("Received invalid relative humidity CRC!");
        return -EIO;
    }
    if(temp){
        *temp = t_sample;
    }
    if(hum){
        *hum = rh_sample;
    }
    (void)device_set_power_state(i2c_dev, DEVICE_PM_LOW_POWER_STATE, NULL, NULL);
    nrfx_gpiote_out_clear(EN_PIN);
    return 0;
}

int app_sht30d_fetch(float *temp, float *hum) {
    uint16_t t_sample, rh_sample;
    int ret;
    ret = app_sht30d_fetch_raw(&t_sample, &rh_sample);
    if(temp){
        /* val = -45 + 175 * sample / (2^16 -1) */
        *temp = -45 + 175 * ((float)t_sample) / 0xFFFF;
    }
    if(hum){
        /* val = 100 * sample / (2^16 -1) */
        *hum = 100 * ((float)rh_sample)/ 0xFFFF;
    }
    return ret;
}

int app_sht30d_fetch_sensor(struct sensor_value * temp, struct sensor_value * hum) {
    uint16_t t_sample, rh_sample;
    int ret;
    uint64_t tmp;
    ret = app_sht30d_fetch_raw(&t_sample, &rh_sample);
    if(temp) {
        /* val = -45 + 175 * sample / (2^16 -1) */
        tmp = (uint64_t)t_sample * 175U;
        temp->val1 = (int32_t)(tmp / 0xFFFF) - 45;
        temp->val2 = ((tmp % 0xFFFF) * 1000000U) / 0xFFFF;
    }
    if(hum){
        /* val = 100 * sample / (2^16 -1) */
        uint32_t tmp2 = (uint32_t)rh_sample * 100U;
        hum->val1 = tmp2 / 0xFFFF;
        /* x * 100000 / 65536 == x * 15625 / 1024 */
        hum->val2 = (tmp2 % 0xFFFF) * 15625U / 1024;
    }
    return ret;
}