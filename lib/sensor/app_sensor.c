#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(app_sensor, LOG_LEVEL_INF);
#include "app_sensor.h"
#include <drivers/sensor.h>
#include <device.h>

static struct sensor_value sensors[2];

struct sensor_value *get_sensor(int index) {
    if (index >= 2) {
        return NULL;
    }
    return &sensors[index];
}

void app_sensor(void) {
    const struct device *dev = device_get_binding("SHT3XD");
    int rc;
    LOG_INF("sensor starting");
    if (dev == NULL) {
        LOG_WRN("can not find device");
        return;
    }
    while (true) {
        rc = sensor_sample_fetch(dev);
        if (rc == 0) {
            rc = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP,
                                    get_sensor(0));
        }
        if (rc == 0) {
            rc = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY,
                                    get_sensor(1));
        }
        if (rc == 0) {
            LOG_INF("fetch sensor OK");
        }
        k_sleep(K_MSEC(60000));
    }
}

K_THREAD_DEFINE(app_sensor_id, 1024, app_sensor, NULL, NULL, NULL,
                7, 0, 100);