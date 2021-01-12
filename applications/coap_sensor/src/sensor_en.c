//
// Created by yplam on 12/1/2021.
//
#include <zephyr.h>
#include <init.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(app_sensor, LOG_LEVEL_INF);
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#define EN_NODE DT_ALIAS(btnen)

#define EN	DT_GPIO_LABEL(EN_NODE, gpios)
#define PIN	DT_GPIO_PIN(EN_NODE, gpios)
#define FLAGS	DT_GPIO_FLAGS(EN_NODE, gpios)

static int app_en_sensor_startup(const struct device *dev){
    const struct device *en_dev =device_get_binding(EN);
    int ret;

    if (en_dev == NULL) {
        LOG_INF("Could not get en dev");
        return 0;
    }
    ret = gpio_pin_configure(en_dev, PIN, GPIO_OUTPUT | GPIO_ACTIVE_HIGH);
    if (ret < 0) {
        LOG_INF("Can not set pin");
        return 0;
    }
    gpio_pin_set(en_dev, PIN, 1);
    LOG_INF("sensor enabled");
    return 0;
}

SYS_INIT(app_en_sensor_startup, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY-10);
