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
static struct k_work_delayable bat_work;

static bool serverFound=false;
static otIp6Address lwm2mServer;

struct k_poll_event events[1];

struct float32_value outdoor_temp, outdoor_humi;

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

void processBatAdc(struct k_work *work) {
    LOG_INF("read bat");
    bat_val_read();
    k_work_schedule(&bat_work, K_SECONDS(120));
}

static void *temperature_get_buf(uint16_t obj_inst_id, uint16_t res_id,
                                 uint16_t res_inst_id, size_t *data_len)
{
    struct float32_value *v = (struct float32_value *) app_sensor_get_value(0);
    /* echo the value back through the engine to update min/max values */
    lwm2m_engine_set_float32("3303/0/5700", v);
    *data_len = sizeof(v);
    return v;
}

static void *humidity_get_buf(uint16_t obj_inst_id, uint16_t res_id,
                                 uint16_t res_inst_id, size_t *data_len)
{
    struct float32_value *v = (struct float32_value *) app_sensor_get_value(1);
    /* echo the value back through the engine to update min/max values */
    lwm2m_engine_set_float32("3304/0/5700", v);
    *data_len = sizeof(v);
    return v;
}

static void *outdoor_temperature_get_buf(uint16_t obj_inst_id, uint16_t res_id,
                                 uint16_t res_inst_id, size_t *data_len)
{
    *data_len = sizeof(outdoor_temp);
    return &outdoor_temp;
}

static void *outdoor_humidity_get_buf(uint16_t obj_inst_id, uint16_t res_id,
                              uint16_t res_inst_id, size_t *data_len)
{
    *data_len = sizeof(outdoor_humi);
    return &outdoor_humi;
}

static void *lwm2m_pre_write(uint16_t obj_inst_id, uint16_t res_id,
                             uint16_t res_inst_id, size_t *data_len)
{
    LOG_INF("pre write %d, %d", obj_inst_id, res_id);
    *data_len = sizeof(outdoor_temp);
    return &outdoor_temp;
}

static int lwm2m_post_write (uint16_t obj_inst_id,
                             uint16_t res_id, uint16_t res_inst_id,
                             uint8_t *data, uint16_t data_len,
                             bool last_block, size_t total_size)
{
    LOG_INF("post write %d, %d", obj_inst_id, res_id);
    return 0;
}


void startLWM2M(struct k_work *work) {
    if(!(app_ot_is_connected() && serverFound)) {
        return ;
    }
    LOG_INF("connecting lwm2m");
    int pos = 0;
    int ret=0;
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
#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
            .enable_psk = 1U,
#else
            .enable_psk = 0U,
#endif
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
    (void)lwm2m_engine_create_obj_inst("3304/0");
    (void)lwm2m_engine_create_obj_inst("32769/0");
//    lwm2m_engine_register_pre_write_callback("32769/0/26241", lwm2m_pre_write);
    lwm2m_engine_register_read_callback("3303/0/5700", temperature_get_buf);
    lwm2m_engine_register_read_callback("3304/0/5700", humidity_get_buf);
//    lwm2m_engine_register_read_callback("3304/1/5700", outdoor_humidity_get_buf);
    k_msleep(1000);
    ret = app_lwm2m_client_setup(&lwm2m_settings);
    if(ret !=0){
        LOG_ERR("lwmwm setup error %d", ret);
    } else {
        app_lwm2m_client_start();
    }
}

void main(void) {
    LOG_INF("OT1M Starting...");
    int ret;
    app_init_settings();

    k_work_init(&ping_work, sendPing);
    k_work_init(&lwm2m_work, startLWM2M);
    k_work_init_delayable(&bat_work, processBatAdc);

    bat_init();
    if(bat_is_charge()){
        LOG_INF("bat is charge");
    }
    if(bat_is_standby()){
        LOG_INF("bat is standby");
    }
    int val = bat_val_read();
    k_work_schedule(&bat_work, K_SECONDS(120));
    app_display_init();
    LOG_INF("bat is %d", val);


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

    k_poll_event_init(&events[0],
                      K_POLL_TYPE_SEM_AVAILABLE,
                      K_POLL_MODE_NOTIFY_ONLY,
                      app_sensor_get_sem());
    struct float32_value * sensor_val;
    while (true) {
        ret = k_poll(events, 1, K_FOREVER);
        if (events[0].state == K_POLL_STATE_SEM_AVAILABLE) {
            k_sem_take(events[0].sem, K_NO_WAIT);
            if(app_lwm2m_get_status() == APP_LWM2M_CONNECT) {
                sensor_val = (struct float32_value *) app_sensor_get_value(0);
                LOG_INF("temp %d, %d", sensor_val->val1, sensor_val->val2);
                lwm2m_engine_set_float32("3303/0/5700", (struct float32_value *) app_sensor_get_value(0));
                lwm2m_engine_set_float32("3304/0/5700", (struct float32_value *) app_sensor_get_value(1));
            }
            app_display_update();
            events[0].state = K_POLL_STATE_NOT_READY;
        }
    }
}


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