
#include "app_openthread.h"
#include <zephyr.h>
#include <init.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(app_display, LOG_LEVEL_INF);
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <devicetree.h>
#include <net/openthread.h>
#include <openthread/ip6.h>
#include <openthread/thread.h>
#include "app_settings.h"
#include <drivers/display.h>
#include "epdpaint.h"

const struct device *display_dev;

Paint paint;
#define COLORED      1
#define UNCOLORED    0

static int app_display_start(const struct device *dev)
{
    uint8_t buff[50];
    int ret;
    struct sensor_value temp, hum;
    LOG_INF("starting display");
    display_dev = device_get_binding("WaveShare_4IN2");

    if (display_dev == NULL) {
        LOG_ERR("device not found.  Aborting test.");
        return -1;
    }
    Paint_Init(&paint, display_get_framebuffer(display_dev),400,300);
    Paint_Clear(&paint, UNCOLORED);
    const char *sensorNames[] = {"Indoor","Outdoor","PM2.5","GM"};

    Paint_Clear(&paint, UNCOLORED);
//        Paint_DrawHorizontalLine(&paint, 2, 185, 248, COLORED);
    Paint_DrawVerticalLine(&paint, 250, 2, 296, COLORED);

    for(int i=0; i<4; i++){
        if(i>0){
            Paint_DrawHorizontalLine(&paint, 250, 75*i, 148, COLORED);
        }
        Paint_DrawStringAt(&paint, 255, 75*i+5, sensorNames[i], &Font815, COLORED);
    }
    ret = sprintf(buff, "%d",temp.val1);
    buff[ret] = '\0';
    Paint_DrawStringAt(&paint, 270, 30, buff, &Font1632, COLORED);
    ret = sprintf(buff, "%d",temp.val2/100000);
    buff[ret] = '\0';
    Paint_DrawStringAt(&paint, 303, 45, buff, &Font815, COLORED);
    Paint_DrawCircle(&paint, 306, 41, 2, COLORED);
    Paint_DrawCircle(&paint, 306, 41, 3, COLORED);

    ret = sprintf(buff, "%d%%",hum.val1);
    buff[ret] = '\0';
    Paint_DrawStringAt(&paint, 340, 30, buff, &Font1632, COLORED);
    display_blanking_off(display_dev);
    return 0;
}

SYS_INIT(app_display_start, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT+1);
