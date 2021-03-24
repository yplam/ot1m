/*
 * main 线程，进行公共的初始化操作，然后启动其他线程
 */

#include <zephyr.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include <devicetree.h>
#include <device.h>
#include <display/cfb.h>
#include <drivers/gpio.h>

static const struct device *epd_dev;


void main(void) {
    LOG_INF("E0102 Starting...");
    int ret;

    epd_dev = device_get_binding(DT_LABEL(DT_INST(0, gooddisplay_uc8175)));
    if (epd_dev == NULL) {
        printk("UC8175 device not found\n");
    }

//    if (cfb_framebuffer_init(epd_dev)) {
//        printk("Framebuffer initialization failed\n");
//    }
//
//    cfb_framebuffer_clear(epd_dev, true);
//    cfb_framebuffer_set_font(epd_dev, 1);
//
//    ret = cfb_print(epd_dev, "HELLO", 0, 0);
//
//    cfb_framebuffer_finalize(epd_dev);

//    if(ret != 0){
//        LOG_ERR("ERR print");
//    }
    while (1) {
        k_msleep(1000);
        LOG_INF("hello...");
    }
    LOG_ERR("NEVER RUN!");
}

