#include <zephyr.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(app_display, LOG_LEVEL_INF);

#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <devicetree.h>
#include <drivers/display.h>
#include <app_sensor.h>
#include <net/lwm2m.h>
#include <app_lwm2m.h>
#include "epdpaint.h"
#include "app_display.h"
#include "bat.h"

const struct device *display_dev;

Paint paint;
#define COLORED      1
#define UNCOLORED    0

void app_display_update(void) {
    uint8_t buff[50];
    int ret;
    struct float32_value sensor_val;
    LOG_INF("display update");
    const char *sensorNames[] = {"Indoor", "Outdoor", "PM2.5", "GM"};
    Paint_Clear(&paint, UNCOLORED);
//        Paint_DrawHorizontalLine(&paint, 2, 185, 248, COLORED);
    Paint_DrawVerticalLine(&paint, 250, 2, 296, COLORED);

    for (int i = 0; i < 4; i++) {
        if (i > 0) {
            Paint_DrawHorizontalLine(&paint, 250, 75 * i, 148, COLORED);
        }
        Paint_DrawStringAt(&paint, 255, 75 * i + 5, sensorNames[i], &Font815, COLORED);
    }

    if(app_lwm2m_get_status() == APP_LWM2M_CONNECT){
        if (0 == lwm2m_engine_get_float32("3303/0/5700", &sensor_val)) {
            ret = sprintf(buff, "%d", (int)sensor_val.val1);
            buff[ret] = '\0';
            Paint_DrawStringAt(&paint, 270, 30, buff, &Font1632, COLORED);
            ret = sprintf(buff, "%d", (int)sensor_val.val2 / 100000);
            buff[ret] = '\0';
            Paint_DrawStringAt(&paint, 303, 45, buff, &Font815, COLORED);
            Paint_DrawCircle(&paint, 306, 41, 2, COLORED);
            Paint_DrawCircle(&paint, 306, 41, 3, COLORED);
        }
        if (0 == lwm2m_engine_get_float32("3304/0/5700", &sensor_val)) {
            ret = sprintf(buff, "%d%%", (int) sensor_val.val1);
            buff[ret] = '\0';
            Paint_DrawStringAt(&paint, 340, 30, buff, &Font1632, COLORED);
        }
        if (0 == lwm2m_engine_get_float32("32769/0/26241", &sensor_val)) {
            LOG_INF("sensor outdoor %d, %d", (int)sensor_val.val1, (int)sensor_val.val2);
            ret = sprintf(buff, "%d", (int)sensor_val.val1);
            buff[ret] = '\0';
            Paint_DrawStringAt(&paint, 270, 30+75, buff, &Font1632, COLORED);
            ret = sprintf(buff, "%d", (int)sensor_val.val2 / 100000);
            buff[ret] = '\0';
            Paint_DrawStringAt(&paint, 303, 45+75, buff, &Font815, COLORED);
            Paint_DrawCircle(&paint, 306, 41+75, 2, COLORED);
            Paint_DrawCircle(&paint, 306, 41+75, 3, COLORED);
        }
    }

//    if (0 == lwm2m_engine_get_float32("3303/1/5700", &sensor_val)) {
//        ret = sprintf(buff, "%d", (int)sensor_val.val1);
//        buff[ret] = '\0';
//        Paint_DrawStringAt(&paint, 270, 30+75, buff, &Font1632, COLORED);
//        ret = sprintf(buff, "%d", (int)sensor_val.val2 / 100000);
//        buff[ret] = '\0';
//        Paint_DrawStringAt(&paint, 303, 45+75, buff, &Font815, COLORED);
//        Paint_DrawCircle(&paint, 306, 41+75, 2, COLORED);
//        Paint_DrawCircle(&paint, 306, 41+75, 3, COLORED);
//    }
//    if (0 == lwm2m_engine_get_float32("3304/1/5700", &sensor_val)) {
//        ret = sprintf(buff, "%d%%", (int) sensor_val.val1);
//        buff[ret] = '\0';
//        Paint_DrawStringAt(&paint, 340, 30+75, buff, &Font1632, COLORED);
//    }




    int bat = (int)get_bat_val();
    LOG_INF("bat %d", bat);
    buff[0] = ' ';
    buff[1] = '\0';
    if(!bat_is_charge()){
        int vm100 = (bat*600/4096);
        if(vm100 >= 400) {
            buff[0] += 5;
        } else if (vm100 >= 387) {
            buff[0] += 4;
        } else if (vm100 >= 379) {
            buff[0] += 3;
        } else if (vm100 >= 370) {
            buff[0] += 2;
        } else {
            buff[0] += 1;
        }
    }
    Paint_DrawStringAt(&paint, 380, 0, buff, &FontICON, COLORED);

    display_blanking_off(display_dev);
    k_sleep(K_MSEC(10000));
}
void app_display_init(void) {

    LOG_INF("starting display");
    display_dev = device_get_binding("WaveShare_4IN2");

    if (display_dev == NULL) {
        LOG_ERR("device not found.  Aborting test.");
        return;
    }
    Paint_Init(&paint, display_get_framebuffer(display_dev), 400, 300);


}
