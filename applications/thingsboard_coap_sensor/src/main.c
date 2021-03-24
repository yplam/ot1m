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
#include <power/power.h>
#include <math.h>
#include <app_openthread.h>
#include <nrfx_gpiote.h>
#include "button.h"
#include "app_coap.h"
#include "sht30d.h"

struct sockaddr_in6 addr6;
K_SEM_DEFINE(main_forever, 0, 1);
void sensor_timer_handler(struct k_timer *dummy);
K_TIMER_DEFINE(sensor_timer, sensor_timer_handler, NULL);
void sensor_work_handler(struct k_work *work);
K_WORK_DEFINE(sensor_work, sensor_work_handler);

float pre_temp=0.0f;
float pre_hum=0.0f;

void sensor_timer_handler(struct k_timer *dummy){
    if(app_ot_is_connected()){
        k_work_submit(&sensor_work);
    }
}

void sensor_work_handler(struct k_work *work) {
    int ret;
    char payload[256];
    char uri[128];
    float temp, hum;
    size_t len;
    if(0 != app_sht30d_fetch(&temp, &hum)){
        LOG_ERR("fetch sensor error");
        return;
    }
    if(fabsf(pre_temp-temp) < 0.1 && fabsf(pre_hum-hum) < 0.1 ) {
        return;
    }
    pre_temp = temp;
    pre_hum = hum;
    len = sprintf(payload, "{\"temperature\":\"%.1f\", \"humidity\":\"%.1f\"}\n",
                  temp, hum);
    payload[len] = '\0';
    len = sprintf(uri, "api/v1/%s/telemetry", CONFIG_APP_THINGS_BOARD_TOKEN);
    uri[len] = '\0';
    ret = app_coap_send_request(&addr6, COAP_METHOD_POST,
                                uri,
                                payload, COAP_TYPE_NON_CON, NULL);
    if (ret < 0) {
        LOG_ERR("coap request error %d", ret);
    }
}


void main(void) {
    LOG_INF("OT1M Starting...");
    /* Connect GPIOTE_0 IRQ to nrfx_gpiote_irq_handler */
    IRQ_CONNECT(DT_IRQN(DT_NODELABEL(gpiote)),
                DT_IRQ(DT_NODELABEL(gpiote), priority),
                nrfx_isr, nrfx_gpiote_irq_handler, 0);
    (void)nrfx_gpiote_init(NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
    app_button_init();
    app_sht30d_init();

    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons(5683);
    addr6.sin6_scope_id = 0U;
    if (net_addr_pton(AF_INET6, CONFIG_APP_THINGS_BOARD_SERVER, &addr6.sin6_addr) < 0) {
        LOG_ERR("COAP server error");
        return;
    }
    k_timer_start(&sensor_timer, K_SECONDS(60), K_SECONDS(60));
    (void)k_sem_take(&main_forever, K_FOREVER);
    LOG_ERR("NEVER RUN!");
}

