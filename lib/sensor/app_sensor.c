#include <zephyr.h>
#include <logging/log.h>
#include <kernel.h>
LOG_MODULE_REGISTER(app_sensor, LOG_LEVEL_INF);
#include "app_sensor.h"
#include <drivers/sensor.h>
#include <device.h>

#define APP_SENSOR_STACK_SIZE 1024
#define APP_SENSOR_PRIORITY 7

static struct sensor_value sensors[2];
K_SEM_DEFINE(sensor_sem, 0, 1);

struct k_sem *app_sensor_get_sem(void) {
    return &sensor_sem;
}

struct sensor_value *app_sensor_get_value(int index) {
    if (index >= 2) {
        return NULL;
    }
    return &sensors[index];
}

void app_sensor(void *arg1, void *arg2, void *arg3) {
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
                                    app_sensor_get_value(0));
            LOG_INF("temp %d, %d", app_sensor_get_value(0)->val1, app_sensor_get_value(0)->val2);
        }
        if (rc == 0) {
            rc = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY,
                                    app_sensor_get_value(1));
            LOG_INF("humi %d, %d", app_sensor_get_value(1)->val1, app_sensor_get_value(1)->val2);
        }
        if (rc == 0) {
            LOG_INF("fetch sensor OK");
        }
        k_sem_give(&sensor_sem);
        k_sleep(K_MSEC(60000));
    }
}

K_THREAD_DEFINE(app_sensor_id, APP_SENSOR_STACK_SIZE, app_sensor, NULL, NULL, NULL,
                APP_SENSOR_PRIORITY, 0, 100);