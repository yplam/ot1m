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
#include <pm/pm.h>
#include <math.h>
#include "app_settings.h"
#include "app_openthread.h"
#include "senseair_s8.h"
#include <net/openthread.h>
#include <openthread/link.h>
#include <openthread/thread.h>
#include <openthread/ping_sender.h>
#include <app_lwm2m.h>
#include <modbus/modbus.h>
#include <sys/ring_buffer.h>
#include <usb/usb_device.h>
#include <logging/log_backend.h>
//#include <drivers/flash.h>


#define FLASH_DEVICE DT_LABEL(DT_INST(0, nordic_qspi_nor))
#define FLASH_NAME "JEDEC QSPI-NOR"
#define FLASH_TEST_REGION_OFFSET 0xff000
#define FLASH_SECTOR_SIZE        4096

static struct k_work start_network;
static struct k_work_delayable send_ping;
static struct k_work_delayable start_lwm2m;
static struct k_work_delayable manage_lwm2m;
static struct k_work_delayable read_co2;
static bool lwm2m_setup=false;

RING_BUF_ITEM_DECLARE_POW2(server_ring_buf, 5);

void printIp6Address(const otIp6Address * addr){
    char buf[40];
    net_addr_ntop(AF_INET6, addr, buf, 40);
    LOG_INF("+IPADDR:%s", log_strdup(buf));
}

void handlePingReply(const otPingSenderReply *aReply, void *aContext) {
    LOG_INF("ping response");
    printIp6Address(&aReply->mSenderAddress);
    ring_buf_put(&server_ring_buf, (uint8_t *)&aReply->mSenderAddress, sizeof(aReply->mSenderAddress));
}

/**
 * 检查网络启动，并不断尝试 JOIN
 * 多次不成功则进入低功耗模式
 * @param work
 */
void startNetwork(struct k_work *work) {
    int static retry = 0;
    for(int i=0; i< 10; i++){
        if(app_ot_is_connected()){
            LOG_INF("thread network connected");
            break;
        }
        k_sleep(K_MSEC(1000));
    }
    if (!app_ot_is_connected()) {
        app_ot_start_join();
        retry += 1;
        k_work_submit(&start_network);
    } else {
        retry = 0;
        otInstance * instance = openthread_get_default_instance();
        otError  error        = OT_ERROR_NONE;
        __ASSERT(instance, "OT instance is NULL");
        LOG_INF("thread network ip:");
        const otNetifAddress *unicastAddrs = otIp6GetUnicastAddresses(instance);
        for (const otNetifAddress *addr = unicastAddrs; addr; addr = addr->mNext)
        {
            printIp6Address(&addr->mAddress);
        }
        LOG_INF("start ping");
        k_work_schedule(&send_ping, K_SECONDS(1));
    }

}

void sendPing(struct k_work *work) {
    if(!app_ot_is_connected()) {
        k_work_schedule(&send_ping, K_SECONDS(30));
//        k_work_submit(&start_network);
        return;
    }
    otError  error        = OT_ERROR_NONE;
    otInstance * instance = openthread_get_default_instance();
    __ASSERT(instance, "OT instance is NULL");
    otPingSenderConfig config;
    memset(&config, 0, sizeof(config));
    LOG_INF("sending ping");
    error = otIp6AddressFromString("ff03::fd", &config.mDestination);
    if(error != OT_ERROR_NONE){
        LOG_ERR("parse address error");
    }
    config.mReplyCallback      = handlePingReply;
    config.mCount = 2;
    otPingSenderPing(instance, &config);

    k_work_schedule(&start_lwm2m, K_SECONDS(4));
}

void startLWM2M(struct k_work *work) {
    if(!app_ot_is_connected()) {
        k_work_schedule(&start_lwm2m, K_SECONDS(30));
        return;
    }
    otIp6Address serverAddress;
    int ret;
    ret = (int)ring_buf_get(&server_ring_buf, (uint8_t *) &serverAddress, sizeof(serverAddress));
    if(ret != sizeof(serverAddress)){
        LOG_WRN("empty address already, re ping");
        ring_buf_reset(&server_ring_buf);
        k_work_submit(&send_ping);
        return;
    }
    LOG_INF("connecting to lwm2m server:");
    printIp6Address(&serverAddress);
    app_lwm2m_settings lwm2m_settings = {
            .enable_psk = 0U,
            .server_addr = {0x00},
            .server_is_bootstrap = false,
    };
    net_addr_ntop(AF_INET6, &serverAddress, lwm2m_settings.server_addr, NET_IPV6_ADDR_LEN);
    if(!lwm2m_setup) {
        ret = app_lwm2m_client_setup(&lwm2m_settings);
        if(ret ==0){
            (void)lwm2m_engine_create_obj_inst("3300/0");
            (void)lwm2m_engine_set_string("3300/0/5701", "ppm");
            (void)lwm2m_engine_set_string("3300/0/5751", "carbon_dioxide");
            lwm2m_setup = true;
            app_lwm2m_client_start();
        } else {
            LOG_ERR("lwm2m setup error %d", ret);
        }
    } else {
        app_lwm2m_client_stop();
        char *server_url;
        uint16_t server_url_len;
        uint8_t server_url_flags;
        /* Server URL */
        ret = lwm2m_engine_get_res_data("0/0/0",
                                        (void **)&server_url, &server_url_len,
                                        &server_url_flags);
        if (ret < 0) {
            LOG_ERR("can not read server url");
            return;
        }

        (void)snprintk(server_url, server_url_len, "coap%s//%s%s%s",
                       lwm2m_settings.enable_psk ? "s:" : ":",
                       strchr(lwm2m_settings.server_addr, ':') ? "[" : "", lwm2m_settings.server_addr,
                       strchr(lwm2m_settings.server_addr, ':') ? "]" : "");
        k_sleep(K_MSEC(100));
        app_lwm2m_client_start();
    }
    k_work_schedule(&manage_lwm2m, K_SECONDS(30));
//    int pos = 0;
//    ret=0;
//    char buf[40];
//    otExtAddress extAddress;
//    otInstance * instance = openthread_get_default_instance();
//    __ASSERT(instance, "OT instance is NULL");
//
//    app_lwm2m_settings lwm2m_settings = {
//            .enable_psk = 0U,
//            .server_addr = {0x00},
//            .server_is_bootstrap = false,
//    };
//    net_addr_ntop(AF_INET6, &serverAddress, lwm2m_settings.server_addr, NET_IPV6_ADDR_LEN);
//    ret = app_lwm2m_client_setup(&lwm2m_settings);
//
//    if(ret !=0){
//        LOG_ERR("lwmwm setup error %d", ret);
//    } else {
//        app_lwm2m_client_start();
//    }
}

void manageLWM2M(struct k_work *work) {
    static enum app_lwm2m_status status = APP_LWM2M_CONNECT;
    enum app_lwm2m_status new_status = app_lwm2m_get_status();
    if(new_status == APP_LWM2M_DISCONNECT && status == APP_LWM2M_DISCONNECT) {
        status = APP_LWM2M_CONNECT;
        k_work_schedule(&start_lwm2m, K_SECONDS(1));
        return;
    }
    status = new_status;
    k_work_schedule(&manage_lwm2m, K_SECONDS(30));
}

void readCO2(struct k_work *work) {
    struct float32_value v;
    v.val1 = senseair_s8_read();
    v.val2 = 0;
    if(app_lwm2m_get_status() == APP_LWM2M_CONNECT) {
        lwm2m_engine_set_float32("3300/0/5700", &v);
    }
    k_work_schedule(&read_co2, K_SECONDS(60));
}

void main(void) {
    const struct log_backend * log_backend_current = log_backend_get(0);
    log_backend_deactivate(log_backend_current);
    LOG_INF("OT1M Starting...");
    int ret;
    app_init_settings();

    k_work_init(&start_network, startNetwork);
    k_work_init_delayable(&send_ping, sendPing);
    k_work_init_delayable(&start_lwm2m, startLWM2M);
    k_work_init_delayable(&manage_lwm2m, manageLWM2M);
    k_work_init_delayable(&read_co2, readCO2);

    k_work_submit(&start_network);

    ret = senseair_s8_init();
    if(ret){
        LOG_ERR("init modbus error");
    } else {
        k_work_schedule(&read_co2, K_SECONDS(5));
    }
    const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    uint32_t dtr = 0;

    if (usb_enable(NULL)) {
        return;
    }
    /* Poll if the DTR flag was set */
    while (!dtr) {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        /* Give CPU resources to low priority threads. */
        k_sleep(K_SECONDS(1));
    }
    log_backend_activate(log_backend_current, log_backend_current->cb->ctx);
    while (true) {
        k_sleep(K_SECONDS(10));
    }
}