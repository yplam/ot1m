//
// Created by yplam on 4/3/2021.
//

#include "e0102.h"
#include <zephyr.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(e0102, LOG_LEVEL_INF);

#include <device.h>
#include <stdio.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>

const struct device *spi;
const struct device *gpio0;
const struct device *gpio1;

#define EN_PIN  0
#define CS_PIN  15
#define DC_PIN  17
#define RST_PIN 20
#define BUSY_PIN    22

struct spi_config cfg = {
        .frequency = 4000000,
        .operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8),
};

uint32_t size;
uint8_t HRES, VRES_0, VRES_1;

static void e0102_spi_init(void) {
    int ret;

    spi = device_get_binding("SPI_0");
    if (!spi) {
        LOG_ERR("SPI not found");
    }
    gpio0 = device_get_binding("GPIO_0");
    if (!gpio0) {
        LOG_ERR("GPIO_0 not found");
    }
    gpio1 = device_get_binding("GPIO_1");
    if (!gpio1) {
        LOG_ERR("GPIO_1 not found");
    }

    ret = gpio_pin_configure(gpio1, EN_PIN, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("EN_PIN error");
    }

    ret = gpio_pin_configure(gpio0, CS_PIN, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("CS_PIN error");
    }

    ret = gpio_pin_configure(gpio0, RST_PIN, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("RST_PIN error");
    }

    ret = gpio_pin_configure(gpio0, BUSY_PIN, GPIO_INPUT | GPIO_PULL_UP);
    if (ret < 0) {
        LOG_ERR("RST_PIN error");
    }
}

static void e0102_cmd(uint8_t cmd) {
    uint8_t buf[2];
    buf[0] = cmd;
    const struct spi_buf tx_buf[1] = {
            {
                    .buf = buf,
                    .len = 2,
            }
    };
    const struct spi_buf_set tx = {
            .buffers = tx_buf,
            .count = 1
    };
    gpio_pin_set(gpio0, CS_PIN, 0);
    k_msleep(1);
    gpio_pin_set(gpio0, DC_PIN, 0);
    int ret = spi_write(spi, &cfg, &tx);
    if (ret != 0) {
        LOG_ERR("SPI CMD ERROR %d", ret);
    } else {
        LOG_INF("CMD OK");
    }
    gpio_pin_set(gpio0, CS_PIN, 1);
    k_msleep(1);
}

static void e0102_data(uint8_t data) {
    const struct spi_buf tx_buf[1] = {
            {
                    .buf = &data,
                    .len = 1,
            }
    };
    const struct spi_buf_set tx = {
            .buffers = tx_buf,
            .count = 1
    };
    gpio_pin_set(gpio0, CS_PIN, 0);
    k_msleep(1);
    gpio_pin_set(gpio0, DC_PIN, 1);
    int ret = spi_write(spi, &cfg, &tx);
    if (ret != 0) {
        LOG_ERR("SPI DATA ERROR %d", ret);
    }
    gpio_pin_set(gpio0, CS_PIN, 1);
    k_msleep(1);
}

static void e0102_wait_busy(void) {
    int busy;
    do {
        e0102_cmd(0x71);
        busy = gpio_pin_get_raw(gpio0, BUSY_PIN);
    } while (!busy);
}

void e0102_init(void) {
    LOG_INF("INIT E0102");
//    LOG_INF("%d, %d", DT_IRQN(DT_NODELABEL(spi0)), DT_IRQ(spi0, priority));

//    IRQ_CONNECT(DT_IRQN(SPI(idx)), DT_IRQ(SPI(idx), priority),     \
//			    nrfx_isr, nrfx_spi_##idx##_irq_handler, 0);	       \

    e0102_spi_init();
    k_msleep(100);
    LOG_INF("INIT ASD");
    gpio_pin_set(gpio0, RST_PIN, 0);
    k_msleep(100);
    gpio_pin_set(gpio0, RST_PIN, 1);
    k_msleep(100);

    HRES = 0x98;                        //152
    VRES_0 = 0x01;                //296
    VRES_1 = 0x28;
    LOG_INF("BCMD");
    e0102_cmd(0x00);
    LOG_INF("CMD");
    e0102_data(0x5f);
    LOG_INF("DATA");

    e0102_cmd(0x2A);
    e0102_data(0x00);
    e0102_data(0x00);

    e0102_cmd(0x04);
    LOG_INF("wait");

    e0102_wait_busy();

    LOG_INF("INIT OK");

//    EPD_W21_Init();//Electronic paper IC reset
//
//    EPD_W21_WriteCMD(0x00);
//    EPD_W21_WriteDATA (0x5f);	// otp
//
//    EPD_W21_WriteCMD(0x2A);
//    EPD_W21_WriteDATA(0x00);
//    EPD_W21_WriteDATA(0x00);
//    EPD_W21_WriteCMD(0x04);     		//power on
//    lcd_chkstatus();//waiting for the electronic paper IC to release the idle signal
}
