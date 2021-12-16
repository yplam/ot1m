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
#include "app_display.h"
#include "app_sensor.h"
#include "app_openthread.h"
#include <net/openthread.h>
#include <openthread/link.h>
#include <openthread/thread.h>
#include <openthread/ping_sender.h>
#include <app_lwm2m.h>
#include "bat.h"
//#include <drivers/flash.h>


#define FLASH_DEVICE DT_LABEL(DT_INST(0, nordic_qspi_nor))
#define FLASH_NAME "JEDEC QSPI-NOR"
#define FLASH_TEST_REGION_OFFSET 0xff000
#define FLASH_SECTOR_SIZE        4096

static struct k_work ping_work;
static struct k_work lwm2m_work;

static bool serverFound=false;
static otIp6Address lwm2mServer;

void printIp6Address(const otIp6Address * addr){
    char buf[40];
    net_addr_ntop(AF_INET6, addr, buf, 40);
    LOG_INF("+IPADDR:%s", log_strdup(buf));
}

void handlePingReply(const otPingSenderReply *aReply, void *aContext) {
    LOG_INF("ping response");
    printIp6Address(&aReply->mSenderAddress);
    if(serverFound == false){
        lwm2mServer = aReply->mSenderAddress;
        serverFound = true;
        k_work_submit(&lwm2m_work);
    }
}

void sendPing(struct k_work *work) {
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
    config.mCount = 4;
    otPingSenderPing(instance, &config);
}

void startLWM2M(struct k_work *work) {
    if(!(app_ot_is_connected() && serverFound)) {
        return ;
    }
    LOG_INF("connecting lwm2m");
    int pos = 0;
    char buf[40];
    otExtAddress extAddress;
    otInstance * instance = openthread_get_default_instance();
    __ASSERT(instance, "OT instance is NULL");

    LOG_INF("connecting to lwm2m server:");
    printIp6Address(&lwm2mServer);
    app_lwm2m_settings lwm2m_settings = {
            .psk_id = {0x00},
            .psk_key = {
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
            },
            .psk_key_length = 16,
            .enable_psk = 1U,
            .server_addr = {0x00},
            .server_is_bootstrap = false,
    };
    net_addr_ntop(AF_INET6, &lwm2mServer, lwm2m_settings.server_addr, NET_IPV6_ADDR_LEN);
    otLinkGetFactoryAssignedIeeeEui64(instance, &extAddress);
    pos = 0;
    for(int i=0; i<OT_EXT_ADDRESS_SIZE; i++){
        pos += sprintf(&buf[pos], "%02x", extAddress.m8[i]);
    }
    buf[pos] = '\0';
    memcpy(lwm2m_settings.psk_id, buf, pos);
    (void)lwm2m_engine_create_obj_inst("3303/0");
    k_msleep(1000);
    app_lwm2m_client_start(&lwm2m_settings);
}

void main(void) {
    LOG_INF("OT1M Starting...");
    app_init_settings();
    set_sensor(0, get_sensor(0));
    set_sensor(1, get_sensor(1));

    k_work_init(&ping_work, sendPing);
    k_work_init(&lwm2m_work, startLWM2M);

    bat_init();
    if(bat_is_charge()){
        LOG_INF("bat is charge");
    }
    if(bat_is_standby()){
        LOG_INF("bat is standby");
    }
    int val = bat_val_read();
    LOG_INF("bat is %d", val);
//    const struct device *flash_dev;
//    flash_dev = device_get_binding(FLASH_DEVICE);
//
//    if (!flash_dev) {
//        LOG_WRN("SPI flash driver %s was not found!\n",
//               FLASH_DEVICE);
//        return;
//    }
//    LOG_INF("\nTest 1: Flash erase\n");
//    flash_write_protection_set(flash_dev, false);
//
//    ret = flash_erase(flash_dev, FLASH_TEST_REGION_OFFSET,
//                     FLASH_SECTOR_SIZE);
//    if (ret != 0) {
//        LOG_WRN("Flash erase failed! %d\n", ret);
//    } else {
//        LOG_INF("Flash erase succeeded!\n");
//    }



//    const struct device *dev = device_get_binding("LIS3DH");
//    int rc;
//
//    if (dev == NULL) {
//        LOG_WRN("Could not get LIS3DH device\n");
//        return;
//    }
//
//    while (true) {
//        struct sensor_value x, y, z;
//
//        rc = sensor_sample_fetch(dev);
//        if (rc == 0) {
//            rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X,
//                                    &x);
//        }
//        if (rc == 0) {
//            rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Y,
//                                    &y);
//        }
//        if (rc == 0) {
//            rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z,
//                                    &z);
//        }
//        if (rc != 0) {
//            LOG_INF("SHT3XD: failed: %d\n", rc);
//            break;
//        }
//        LOG_INF("x: %d, %d, y: %d, %d, z: %d, %d", x.val1, x.val2, y.val1, y.val2, z.val1, z.val2);
//
//        k_sleep(K_MSEC(2000));
//    }
//
//    const struct device *dev = device_get_binding("SHT3XD");
//    int rc;
//
//    if (dev == NULL) {
//        LOG_WRN("Could not get SHT3XD device\n");
//        return;
//    }
    for(int i=0; i< 10; i++){
        if(app_ot_is_connected()){
            LOG_INF("thread network connected");
            break;
        }
        k_sleep(K_MSEC(3000));
    }
    if(!app_ot_is_connected()){
        app_ot_start_join();
        for(int i=0; i< 10; i++){
            if(app_ot_is_connected()){
                LOG_INF("join thread network");
                break;
            }
            k_sleep(K_MSEC(3000));
        }
    }
    otInstance * instance = openthread_get_default_instance();
    otError  error        = OT_ERROR_NONE;
    __ASSERT(instance, "OT instance is NULL");
    const otNetifAddress *unicastAddrs = otIp6GetUnicastAddresses(instance);
    for (const otNetifAddress *addr = unicastAddrs; addr; addr = addr->mNext)
    {
        printIp6Address(&addr->mAddress);
    }
    LOG_INF("start ping");
    k_work_submit(&ping_work);

    /* Connect GPIOTE_0 IRQ to nrfx_gpiote_irq_handler */
//    IRQ_CONNECT(DT_IRQN(DT_NODELABEL(gpiote)),
//                DT_IRQ(DT_NODELABEL(gpiote), priority),
//                nrfx_isr, nrfx_gpiote_irq_handler, 0);
//    (void)nrfx_gpiote_init(NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
//    app_button_init(on_btn_single_click, NULL, on_btn_long_press, NULL);
//    app_sht30d_init();
//    led_init();
//    k_msleep(100);
//    otInstance * instance = openthread_get_default_instance();
//    otError  error        = OT_ERROR_NONE;
//    __ASSERT(instance, "OT instance is NULL");
//    const otNetifAddress *unicastAddrs = otIp6GetUnicastAddresses(instance);
//    char buf[40];
//    for (const otNetifAddress *addr = unicastAddrs; addr; addr = addr->mNext)
//    {
//        net_addr_ntop(AF_INET6, &addr->mAddress, buf, 40);
//        LOG_INF("+IPADDR:%s", log_strdup(buf));
//    }
//    int pos = 0;
//    otExtAddress extAddress;
//    otLinkGetFactoryAssignedIeeeEui64(instance, &extAddress);
//    for(int i=0; i<OT_EXT_ADDRESS_SIZE; i++){
//        pos += sprintf(&buf[pos], "%02x", extAddress.m8[i]);
//    }
//    buf[pos] = '\0';
//    LOG_INF("PSK:%s", log_strdup(buf));
//    app_lwm2m_settings lwm2m_settings = {
//            .psk_id = {0x00},
//            .psk_key = {
//                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
//                    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
//            },
//            .psk_key_length = 16,
//            .enable_psk = 1U,
//            .server_addr = "2001:db8:1:ffff::c0a8:223",
//            .server_is_bootstrap = false,
//    };
//    memcpy(lwm2m_settings.psk_id, buf, pos);
//    (void)lwm2m_engine_create_obj_inst("3303/0");
//
//    for(int i=0; i<30; i++){
//        if (app_ot_is_connected()){
//            break;
//        }
//        (void)k_msleep(1000);
//    }
//    k_msleep(60000);
//    ret = app_lwm2m_client_start(&lwm2m_settings);
//    if (ret < 0) {
//        LOG_ERR("Cannot setup LWM2M fields (%d)", ret);
//    }

    while (true) {
        k_sleep(K_MSEC(60000));
    }
}
