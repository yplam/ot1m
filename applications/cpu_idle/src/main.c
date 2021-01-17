/*
 * main 线程，进行公共的初始化操作，然后启动其他线程
 */

#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);


void main(void) {
    LOG_INF("OT1M Starting...");

    while (true) {
        k_cpu_idle();
    }
    LOG_ERR("NEVER RUN!");
}

