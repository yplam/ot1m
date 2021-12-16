#include <zephyr.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(app_display, LOG_LEVEL_INF);

#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <devicetree.h>
#include <drivers/display.h>
#include "epdpaint.h"
#include "app_display.h"

const struct device *display_dev;

static struct sensor_value *sensors[2] = {0, 0};

Paint paint;
#define COLORED      1
#define UNCOLORED    0

void set_sensor(int index, struct sensor_value *sensor) {
    if (index >= 2) {
        return;
    }
    sensors[index] = sensor;
}

void app_display(void) {
    uint8_t buff[50];
    int ret;

    LOG_INF("starting display");
    display_dev = device_get_binding("WaveShare_4IN2");

    if (display_dev == NULL) {
        LOG_ERR("device not found.  Aborting test.");
        return;
    }
    Paint_Init(&paint, display_get_framebuffer(display_dev), 400, 300);
    Paint_Clear(&paint, UNCOLORED);
    const char *sensorNames[] = {"Indoor", "Outdoor", "PM2.5", "GM"};

    while (1) {
        LOG_INF("LOOP");
        Paint_Clear(&paint, UNCOLORED);
//        Paint_DrawHorizontalLine(&paint, 2, 185, 248, COLORED);
        Paint_DrawVerticalLine(&paint, 250, 2, 296, COLORED);

        for (int i = 0; i < 4; i++) {
            if (i > 0) {
                Paint_DrawHorizontalLine(&paint, 250, 75 * i, 148, COLORED);
            }
            Paint_DrawStringAt(&paint, 255, 75 * i + 5, sensorNames[i], &Font815, COLORED);
        }

        if (sensors[0]) {
            ret = sprintf(buff, "%d", (int) sensors[0]->val1);
            buff[ret] = '\0';
            Paint_DrawStringAt(&paint, 270, 30, buff, &Font1632, COLORED);
            ret = sprintf(buff, "%d", (int) sensors[0]->val2 / 100000);
            buff[ret] = '\0';
            Paint_DrawStringAt(&paint, 303, 45, buff, &Font815, COLORED);
            Paint_DrawCircle(&paint, 306, 41, 2, COLORED);
            Paint_DrawCircle(&paint, 306, 41, 3, COLORED);
        }
        if (sensors[1]) {
            ret = sprintf(buff, "%d%%", (int) sensors[1]->val1);
            buff[ret] = '\0';
            Paint_DrawStringAt(&paint, 340, 30, buff, &Font1632, COLORED);
        }
        display_blanking_off(display_dev);
        k_sleep(K_MSEC(10000));
    }
}

K_THREAD_DEFINE(app_display_id, 1024, app_display, NULL, NULL, NULL,
                7, 0, 100);