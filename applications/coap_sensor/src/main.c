/*
 * main 线程，进行公共的初始化操作，然后启动其他线程
 */

#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <devicetree.h>
#include <drivers/gpio.h>


/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define LED_PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#define LED_FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)

void main(void) {
    LOG_INF("OT1M Starting...");
    int ret;
    const struct device *dev = device_get_binding("SHT3XD");

    if (dev == NULL) {
        LOG_INF("Could not get dev");
        return;
    }

    while (true) {
        struct sensor_value temp, hum;

        ret = sensor_sample_fetch(dev);
        if (ret == 0) {
            ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP,
                                    &temp);
        }
        if (ret == 0) {
            ret = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY,
                                    &hum);
        }
        if (ret != 0) {
            LOG_INF("SHT3XD: failed: %d\n", ret);
            break;
        }
        LOG_INF("SHT3XD: %d,%d Cel ; %d,%d %%RH\n",
               temp.val1, temp.val2,
               hum.val1, hum.val2);

        k_sleep(K_MSEC(2000));
    }
    LOG_ERR("NEVER RUN!");
}

