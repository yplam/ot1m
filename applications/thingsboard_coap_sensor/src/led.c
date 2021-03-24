//
// Created by yplam on 12/1/2021.
//
#include <zephyr.h>
#include <init.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(led, LOG_LEVEL_INF);
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#define LED0_NODE DT_NODELABEL(led0)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define LED_PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#define LED_FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)

static const struct device *led_dev;

static int app_led_startup(const struct device *dev){
    led_dev =device_get_binding(LED0);
    int ret;
    LOG_INF("config led");
    if (led_dev == NULL) {
        LOG_INF("Could not get led_dev");
        return 0;
    }
    ret = gpio_pin_configure(led_dev, LED_PIN, GPIO_OUTPUT | LED_FLAGS);
    if (ret < 0) {
        LOG_INF("Can not set pin");
        return 0;
    }
    gpio_pin_set(led_dev, LED_PIN, 1);
    return 0;
}

//SYS_INIT(app_led_startup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
