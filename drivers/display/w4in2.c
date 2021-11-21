/*
 * Copyright (c) 2018-2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT waveshare_4in2

#define LOG_LEVEL CONFIG_DISPLAY_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(w4in2);

#include <string.h>
#include <device.h>
#include <drivers/display.h>
#include <init.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <sys/byteorder.h>

#include <display/cfb.h>

/* time constants in ms */
#define W4IN2_RESET_DELAY			20
#define W4IN2_BUSY_DELAY			1

#define W4IN2_SPI_FREQ DT_INST_PROP(0, spi_max_frequency)
#define W4IN2_BUS_NAME DT_INST_BUS_LABEL(0)
#define W4IN2_DC_PIN DT_INST_GPIO_PIN(0, dc_gpios)
#define W4IN2_DC_FLAGS DT_INST_GPIO_FLAGS(0, dc_gpios)
#define W4IN2_DC_CNTRL DT_INST_GPIO_LABEL(0, dc_gpios)
#define W4IN2_CS_PIN DT_INST_SPI_DEV_CS_GPIOS_PIN(0)
#define W4IN2_CS_FLAGS DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0)
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
#define W4IN2_CS_CNTRL DT_INST_SPI_DEV_CS_GPIOS_LABEL(0)
#endif
#define W4IN2_BUSY_PIN DT_INST_GPIO_PIN(0, busy_gpios)
#define W4IN2_BUSY_CNTRL DT_INST_GPIO_LABEL(0, busy_gpios)
#define W4IN2_BUSY_FLAGS DT_INST_GPIO_FLAGS(0, busy_gpios)

#define W4IN2_RESET_PIN DT_INST_GPIO_PIN(0, reset_gpios)
#define W4IN2_RESET_CNTRL DT_INST_GPIO_LABEL(0, reset_gpios)
#define W4IN2_RESET_FLAGS DT_INST_GPIO_FLAGS(0, reset_gpios)

#define W4IN2_POWER_PIN DT_INST_GPIO_PIN(0, power_gpios)
#define W4IN2_POWER_CNTRL DT_INST_GPIO_LABEL(0, power_gpios)
#define W4IN2_POWER_FLAGS DT_INST_GPIO_FLAGS(0, power_gpios)

#define EPD_PANEL_WIDTH			400
#define EPD_PANEL_HEIGHT		300
#define EPD_PANEL_NUMOF_COLUMS		EPD_PANEL_WIDTH
#define EPD_PANEL_NUMOF_ROWS_PER_PAGE	8
#define EPD_PANEL_NUMOF_PAGES		(EPD_PANEL_HEIGHT / \
					 EPD_PANEL_NUMOF_ROWS_PER_PAGE)

#define W4IN2_PANEL_FIRST_PAGE	0
#define W4IN2_PANEL_LAST_PAGE		(EPD_PANEL_NUMOF_PAGES - 1)
#define W4IN2_PANEL_FIRST_GATE	0
#define W4IN2_PANEL_LAST_GATE		(EPD_PANEL_NUMOF_COLUMS - 1)

#define W4IN2_PIXELS_PER_BYTE		8
#define W4IN2_DEFAULT_TR_VALUE	25U
#define W4IN2_TR_SCALE_FACTOR		256U

struct w4in2_data {
    const struct device *power;
    const struct device *reset;
    const struct device *dc;
    const struct device *busy;
    const struct device *spi_dev;
    struct spi_config spi_config;
#if defined(W4IN2_CS_CNTRL)
    struct spi_cs_control cs_ctrl;
#endif
    const void *buffer;
    uint8_t scan_mode;
    uint8_t update_cmd;

};

static const struct device *led_dev;

#define LED0_NODE DT_NODELABEL(led0)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define LED_PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#define LED_FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)


static inline int w4in2_write_cmd(struct w4in2_data *driver,
                                    uint8_t cmd, uint8_t *data, size_t len)
{
    int err;
    struct spi_buf buf = {.buf = &cmd, .len = sizeof(cmd)};
    struct spi_buf_set buf_set = {.buffers = &buf, .count = 1};

    gpio_pin_set(driver->dc, W4IN2_DC_PIN, 1);
    err = spi_write(driver->spi_dev, &driver->spi_config, &buf_set);
    if (err < 0) {
        return err;
    }

    if (data != NULL) {
        buf.buf = data;
        buf.len = len;
        gpio_pin_set(driver->dc, W4IN2_DC_PIN, 0);
        err = spi_write(driver->spi_dev, &driver->spi_config, &buf_set);
        if (err < 0) {
            return err;
        }
    }

    return 0;
}

static inline void w4in2_busy_wait(struct w4in2_data *driver)
{
    int pin = gpio_pin_get(driver->busy, W4IN2_BUSY_PIN);
    w4in2_write_cmd(driver, 0x71, NULL, 0);
    while (pin > 0) {
//	    LOG_INF("x");
        __ASSERT(pin >= 0, "Failed to get pin level");
        k_msleep(W4IN2_BUSY_DELAY);
        w4in2_write_cmd(driver, 0x71, NULL, 0);
        pin = gpio_pin_get(driver->busy, W4IN2_BUSY_PIN);
    }
//    LOG_INF("y");
}

static int w4in2_blanking_off(const struct device *dev)
{
    int err;
    uint8_t cmd = 0x10;
    struct spi_buf buf;
    struct spi_buf_set buf_set = {.buffers = &buf, .count = 1};
    struct w4in2_data *driver = dev->data;
    uint8_t data[50];
    memset(data, 0xFF, 50);
    cmd = 0x10;
    buf.buf = &cmd;
    buf.len = 1;
    gpio_pin_set(driver->dc, W4IN2_DC_PIN, 1);
    err = spi_write(driver->spi_dev, &driver->spi_config, &buf_set);
    if (err < 0) {
        return err;
    }
    buf.buf = data;
    buf.len = 50;
    gpio_pin_set(driver->dc, W4IN2_DC_PIN, 0);
    for (int j = 0; j < 300; j++) {
        err = spi_write(driver->spi_dev, &driver->spi_config, &buf_set);
        if (err < 0) {
            return err;
        }
    }
    err = w4in2_write_cmd(driver, 0x13, driver->buffer, 15000);
    if (err < 0) {
        LOG_WRN("ERR %d", err);
        return err;
    }

    err = w4in2_write_cmd(driver, 0x12, NULL, 0);
    if (err < 0) {
        LOG_WRN("ERR %d", err);
        return err;
    }
    k_msleep(4000);
    w4in2_busy_wait(driver);
    return 0;
}

// clear screen
static int w4in2_blanking_on(const struct device *dev)
{
    int err;
    uint8_t cmd = 0x10;
    struct spi_buf buf;
    struct spi_buf_set buf_set = {.buffers = &buf, .count = 1};
    struct w4in2_data *driver = dev->data;
    uint8_t data[50];
    memset(data, 0xFF, 50);
    for(int i=0; i<2; i++) {
        cmd = 0x10 + 3*i;
        buf.buf = &cmd;
        buf.len = 1;
        gpio_pin_set(driver->dc, W4IN2_DC_PIN, 1);
        err = spi_write(driver->spi_dev, &driver->spi_config, &buf_set);
        if (err < 0) {
            return err;
        }
        buf.buf = data;
        buf.len = 50;
        gpio_pin_set(driver->dc, W4IN2_DC_PIN, 0);
        for (int j = 0; j < 300; j++) {
            err = spi_write(driver->spi_dev, &driver->spi_config, &buf_set);
            if (err < 0) {
                return err;
            }
        }
    }
    err = w4in2_write_cmd(driver, 0x12, NULL, 0);
    if (err < 0) {
        LOG_WRN("ERR %d", err);
        return err;
    }
    k_msleep(4000);
    w4in2_busy_wait(driver);
    return 0;
}

static int w4in2_update_display(const struct device *dev)
{
    return -ENOTSUP;
}

static int w4in2_write(const struct device *dev, const uint16_t x,
                         const uint16_t y,
                         const struct display_buffer_descriptor *desc,
                         const void *buf)
{
    return -ENOTSUP;
}

static int w4in2_read(const struct device *dev, const uint16_t x,
                        const uint16_t y,
                        const struct display_buffer_descriptor *desc,
                        void *buf)
{
    LOG_ERR("not supported read");
    return -ENOTSUP;
}

static void *w4in2_get_framebuffer(const struct device *dev)
{
    struct w4in2_data *driver = dev->data;
    return driver->buffer;
}

static int w4in2_set_brightness(const struct device *dev,
                                  const uint8_t brightness)
{
    LOG_WRN("not supported");
    return -ENOTSUP;
}

static int w4in2_set_contrast(const struct device *dev, uint8_t contrast)
{
    LOG_WRN("not supported");
    return -ENOTSUP;
}

static void w4in2_get_capabilities(const struct device *dev,
                                     struct display_capabilities *caps)
{
    memset(caps, 0, sizeof(struct display_capabilities));
    caps->x_resolution = EPD_PANEL_WIDTH;
    caps->y_resolution = EPD_PANEL_HEIGHT -
                         EPD_PANEL_HEIGHT % EPD_PANEL_NUMOF_ROWS_PER_PAGE;
    caps->supported_pixel_formats = PIXEL_FORMAT_MONO10;
    caps->current_pixel_format = PIXEL_FORMAT_MONO10;
    caps->screen_info = SCREEN_INFO_MONO_VTILED |
                        SCREEN_INFO_MONO_MSB_FIRST |
                        SCREEN_INFO_EPD |
                        SCREEN_INFO_DOUBLE_BUFFER;
}

static int w4in2_set_orientation(const struct device *dev,
                                   const enum display_orientation
                                   orientation)
{
    LOG_ERR("Unsupported");
    return -ENOTSUP;
}

static int w4in2_set_pixel_format(const struct device *dev,
                                    const enum display_pixel_format pf)
{
    if (pf == PIXEL_FORMAT_MONO10) {
        return 0;
    }

    LOG_ERR("not supported");
    return -ENOTSUP;
}

static const unsigned char EPD_4IN2_lut_vcom0[] = {
        0x00, 0x17, 0x00, 0x00, 0x00, 0x02,
        0x00, 0x17, 0x17, 0x00, 0x00, 0x02,
        0x00, 0x0A, 0x01, 0x00, 0x00, 0x01,
        0x00, 0x0E, 0x0E, 0x00, 0x00, 0x02,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const unsigned char EPD_4IN2_lut_ww[] = {
        0x40, 0x17, 0x00, 0x00, 0x00, 0x02,
        0x90, 0x17, 0x17, 0x00, 0x00, 0x02,
        0x40, 0x0A, 0x01, 0x00, 0x00, 0x01,
        0xA0, 0x0E, 0x0E, 0x00, 0x00, 0x02,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const unsigned char EPD_4IN2_lut_bw[] = {
        0x40, 0x17, 0x00, 0x00, 0x00, 0x02,
        0x90, 0x17, 0x17, 0x00, 0x00, 0x02,
        0x40, 0x0A, 0x01, 0x00, 0x00, 0x01,
        0xA0, 0x0E, 0x0E, 0x00, 0x00, 0x02,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const unsigned char EPD_4IN2_lut_wb[] = {
        0x80, 0x17, 0x00, 0x00, 0x00, 0x02,
        0x90, 0x17, 0x17, 0x00, 0x00, 0x02,
        0x80, 0x0A, 0x01, 0x00, 0x00, 0x01,
        0x50, 0x0E, 0x0E, 0x00, 0x00, 0x02,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const unsigned char EPD_4IN2_lut_bb[] = {
        0x80, 0x17, 0x00, 0x00, 0x00, 0x02,
        0x90, 0x17, 0x17, 0x00, 0x00, 0x02,
        0x80, 0x0A, 0x01, 0x00, 0x00, 0x01,
        0x50, 0x0E, 0x0E, 0x00, 0x00, 0x02,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static int w4in2_init_lut(struct w4in2_data *driver)
{
    int err;
    err = w4in2_write_cmd(driver, 0x20, EPD_4IN2_lut_vcom0, 44);
    if (err < 0) {
        LOG_WRN("ERR %d", err);
        return err;
    }

    err = w4in2_write_cmd(driver, 0x21, EPD_4IN2_lut_ww, 42);
    if (err < 0) {
        LOG_WRN("ERR %d", err);
        return err;
    }

    err = w4in2_write_cmd(driver, 0x22, EPD_4IN2_lut_bw, 42);
    if (err < 0) {
        LOG_WRN("ERR %d", err);
        return err;
    }

    err = w4in2_write_cmd(driver, 0x23, EPD_4IN2_lut_wb, 42);
    if (err < 0) {
        LOG_WRN("ERR %d", err);
        return err;
    }

    err = w4in2_write_cmd(driver, 0x24, EPD_4IN2_lut_bb, 42);
    if (err < 0) {
        LOG_WRN("ERR %d", err);
        return err;
    }
    return err;
}

static int w4in2_controller_init(const struct device *dev)
{
    int err;
    uint8_t tmp[3];
    size_t len;
    struct w4in2_data *driver = dev->data;

    LOG_DBG("controller init");
//    gpio_pin_set(driver->reset, 2, 1);

    gpio_pin_set(driver->reset, W4IN2_RESET_PIN, 1);
    k_msleep(W4IN2_RESET_DELAY);
    gpio_pin_set(driver->reset, W4IN2_RESET_PIN, 0);
    k_msleep(W4IN2_RESET_DELAY);
    gpio_pin_set(driver->reset, W4IN2_RESET_PIN, 1);
    k_msleep(W4IN2_RESET_DELAY);
    gpio_pin_set(driver->reset, W4IN2_RESET_PIN, 0);
    k_msleep(W4IN2_RESET_DELAY);
    w4in2_busy_wait(driver);

    const uint8_t power_setting[] = {0x03, 0x00, 0x2b, 0x2b};
    err = w4in2_write_cmd(driver, 0x01, power_setting, 4);
    if (err < 0) {
        LOG_WRN("ERR %d", err);
        return err;
    }

    const uint8_t boost[] = {0x17, 0x17, 0x17};
    err = w4in2_write_cmd(driver, 0x06, boost, 3);
    if (err < 0) {
        LOG_WRN("ERR %d", err);
        return err;
    }

    err = w4in2_write_cmd(driver, 0x04, NULL, 0);
    if (err < 0) {
        LOG_WRN("ERR %d", err);
        return err;
    }
    w4in2_busy_wait(driver);

    const uint8_t panel[] = {0xbf, 0x0d};
    err = w4in2_write_cmd(driver, 0x00, panel, 2);
    if (err < 0) {
        LOG_WRN("ERR %d", err);
        return err;
    }

    const uint8_t pll[] = {0x3c};
    err = w4in2_write_cmd(driver, 0x30, pll, 1);
    if (err < 0) {
        LOG_WRN("ERR %d", err);
        return err;
    }

    const uint8_t res[] = {0x01, 0x90, 0x01, 0x2c};
    err = w4in2_write_cmd(driver, 0x61, res, 4);
    if (err < 0) {
        LOG_WRN("ERR %d", err);
        return err;
    }

    const uint8_t dc[] = {0x28};
    err = w4in2_write_cmd(driver, 0x82, dc, 1);
    if (err < 0) {
        LOG_WRN("ERR %d", err);
        return err;
    }

    const uint8_t vcom[] = {0x97};
    err = w4in2_write_cmd(driver, 0X50, vcom, 1);
    if (err < 0) {
        LOG_WRN("ERR %d", err);
        return err;
    }

    err = w4in2_init_lut(driver);
    if (err < 0) {
        LOG_WRN("ERR %d", err);
        return err;
    }

    LOG_WRN("FINISH %d", err);

    return 0;
}

static int w4in2_init(const struct device *dev)
{
    struct w4in2_data *driver = dev->data;

    LOG_DBG("w4in2 start");

    driver->power = device_get_binding(W4IN2_POWER_CNTRL);
    if (driver->power == NULL) {
        LOG_ERR("Could not get GPIO port for W4IN2 power");
        return -EIO;
    }
    gpio_pin_configure(driver->power, W4IN2_POWER_PIN,
                       GPIO_OUTPUT_ACTIVE | W4IN2_POWER_FLAGS);

    driver->spi_dev = device_get_binding(W4IN2_BUS_NAME);
    if (driver->spi_dev == NULL) {
        LOG_ERR("Could not get SPI device for W4IN2");
        return -EIO;
    }

    driver->spi_config.frequency = W4IN2_SPI_FREQ;
    driver->spi_config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
    driver->spi_config.slave = DT_INST_REG_ADDR(0);
    driver->spi_config.cs = NULL;

    driver->reset = device_get_binding(W4IN2_RESET_CNTRL);
    if (driver->reset == NULL) {
        LOG_ERR("Could not get GPIO port for W4IN2 reset");
        return -EIO;
    }

    gpio_pin_configure(driver->reset, W4IN2_RESET_PIN,
                       GPIO_OUTPUT_INACTIVE | W4IN2_RESET_FLAGS);

    driver->dc = device_get_binding(W4IN2_DC_CNTRL);
    if (driver->dc == NULL) {
        LOG_ERR("Could not get GPIO port for W4IN2 DC signal");
        return -EIO;
    }

    gpio_pin_configure(driver->dc, W4IN2_DC_PIN,
                       GPIO_OUTPUT_INACTIVE | W4IN2_DC_FLAGS);

    driver->busy = device_get_binding(W4IN2_BUSY_CNTRL);
    if (driver->busy == NULL) {
        LOG_ERR("Could not get GPIO port for W4IN2 busy signal");
        return -EIO;
    }

    gpio_pin_configure(driver->busy, W4IN2_BUSY_PIN,
                       GPIO_INPUT | W4IN2_BUSY_FLAGS);

#if defined(W4IN2_CS_CNTRL)
    driver->cs_ctrl.gpio_dev = device_get_binding(W4IN2_CS_CNTRL);
	if (!driver->cs_ctrl.gpio_dev) {
		LOG_ERR("Unable to get SPI GPIO CS device");
		return -EIO;
	}

	driver->cs_ctrl.gpio_pin = W4IN2_CS_PIN;
	driver->cs_ctrl.gpio_dt_flags = W4IN2_CS_FLAGS;
	driver->cs_ctrl.delay = 0U;
	driver->spi_config.cs = &driver->cs_ctrl;
#endif

	driver->buffer = k_malloc(EPD_PANEL_WIDTH*EPD_PANEL_HEIGHT/8);
	if(!driver->buffer){
        LOG_ERR("Unable to malloc memory");
        return -ENOMEM;
	}
    return w4in2_controller_init(dev);
}

static struct w4in2_data w4in2_driver;

static struct display_driver_api w4in2_driver_api = {
        .blanking_on = w4in2_blanking_on,
        .blanking_off = w4in2_blanking_off,
        .write = w4in2_write,
        .read = w4in2_read,
        .get_framebuffer = w4in2_get_framebuffer,
        .set_brightness = w4in2_set_brightness,
        .set_contrast = w4in2_set_contrast,
        .get_capabilities = w4in2_get_capabilities,
        .set_pixel_format = w4in2_set_pixel_format,
        .set_orientation = w4in2_set_orientation,
};


DEVICE_DT_INST_DEFINE(0, w4in2_init, device_pm_control_nop,
                      &w4in2_driver, NULL,
                      POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,
                      &w4in2_driver_api);
