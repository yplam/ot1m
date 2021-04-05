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
#include <app_lwm2m.h>
#include <net/lwm2m.h>
//#include <sht30d.h>
//#include <nrfx_gpiote.h>
//#include <button.h>

#include <net/openthread.h>
#include <openthread/link.h>
#include <openthread/thread.h>
#include <drivers/flash.h>
#include <drivers/display.h>
#include "epdpaint.h"

#define LIGHT_NAME		"Test light"

static uint32_t led_state=0;
static bool btn_state=false;
#define LED0	DT_GPIO_PIN(DT_NODELABEL(led0), gpios)
//
//static void *temperature_get_buf(uint16_t obj_inst_id, uint16_t res_id,
//                                 uint16_t res_inst_id, size_t *data_len)
//{
//    static struct float32_value temp, hum;
//
//    if(0 != app_sht30d_fetch_sensor((struct sensor_value *)&temp, (struct sensor_value *)&hum)){
//        LOG_ERR("fetch sensor error");
//    }
//    /* echo the value back through the engine to update min/max values */
//    lwm2m_engine_set_float32("3303/0/5700", &temp);
//    *data_len = sizeof(temp);
//    return &temp;
//}

void on_btn_single_click(void) {
    LOG_INF("CLICK");
    btn_state = !btn_state;
    lwm2m_engine_set_bool("3342/0/5500", btn_state);
}

void on_btn_long_press(void) {
    app_ot_start_join();
}
//
//static void led_init(void) {
//    int ret;
//    nrfx_gpiote_out_config_t const out_config = {
//            .action = NRF_GPIOTE_POLARITY_TOGGLE,
//            .init_state = 1,
//            .task_pin = true,
//    };
//
//    /* Initialize output pin. SET task will turn the LED on,
//     * CLR will turn it off and OUT will toggle it.
//     */
//    ret = nrfx_gpiote_out_init(LED0+32, &out_config);
//    if (ret != NRFX_SUCCESS) {
//        LOG_ERR("nrfx_gpiote_out_init error: %08x", ret);
//        return;
//    }
////    nrfx_gpiote_out_task_enable(LED0);
//}
//
///* TODO: Move to a pre write hook that can handle ret codes once available */
//static int led_on_off_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
//                         uint8_t *data, uint16_t data_len,
//                         bool last_block, size_t total_size)
//{
//    int ret = 0;
//    uint32_t led_val;
//
//    led_val = *(uint8_t *) data;
//    if (led_val != led_state) {
//        nrfx_gpiote_out_toggle(LED0+32);
//        led_state = led_val;
//        /* TODO: Move to be set by an internal post write function */
//        lwm2m_engine_set_s32("3311/0/5852", 0);
//    }
//
//    return ret;
//}

#define FLASH_DEVICE DT_LABEL(DT_INST(0, nordic_qspi_nor))
#define FLASH_NAME "JEDEC QSPI-NOR"
#define FLASH_TEST_REGION_OFFSET 0xff000
#define FLASH_SECTOR_SIZE        4096
Paint paint;
#define COLORED      1
#define UNCOLORED    0


void main(void) {
    LOG_INF("OT1M Starting...");
    int ret;
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

    const struct device *display_dev = device_get_binding("WaveShare_4IN2");

    if (display_dev == NULL) {
        LOG_ERR("device not found.  Aborting test.");
        return;
    }
    ret = display_blanking_on(display_dev);
    if ( ret != 0) {
        LOG_ERR("blanking error %d.", ret);
        return;
    }
    Paint_Init(&paint, display_get_framebuffer(display_dev),400,300);
    Paint_Clear(&paint, UNCOLORED);
    Paint_DrawStringAt(&paint, 5, 5, "HELLO", &Font8, COLORED);
    Paint_DrawStringAt(&paint, 5, 15, "World", &Font8, COLORED);
    ret = display_blanking_off(display_dev);
    if ( ret != 0) {
        LOG_ERR("blanking error %d.", ret);
        return;
    }

    const struct device *dev = device_get_binding("LIS3DH");
    int rc;

    if (dev == NULL) {
        LOG_WRN("Could not get LIS3DH device\n");
        return;
    }

    while (true) {
        struct sensor_value x, y, z;

        rc = sensor_sample_fetch(dev);
        if (rc == 0) {
            rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X,
                                    &x);
        }
        if (rc == 0) {
            rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Y,
                                    &y);
        }
        if (rc == 0) {
            rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z,
                                    &z);
        }
        if (rc != 0) {
            LOG_INF("SHT3XD: failed: %d\n", rc);
            break;
        }
        LOG_INF("x: %d, %d, y: %d, %d, z: %d, %d", x.val1, x.val2, y.val1, y.val2, z.val1, z.val2);

        k_sleep(K_MSEC(2000));
    }
//
//    const struct device *dev = device_get_binding("SHT3XD");
//    int rc;
//
//    if (dev == NULL) {
//        LOG_WRN("Could not get SHT3XD device\n");
//        return;
//    }
//
//    while (true) {
//        struct sensor_value temp, hum;
//
//        rc = sensor_sample_fetch(dev);
//        if (rc == 0) {
//            rc = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP,
//                                    &temp);
//        }
//        if (rc == 0) {
//            rc = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY,
//                                    &hum);
//        }
//        if (rc != 0) {
//            LOG_INF("SHT3XD: failed: %d\n", rc);
//            break;
//        }
//        LOG_INF("%d, %d", temp.val1, temp.val2);
//        LOG_INF("%d, %d", hum.val1, hum.val2);
//
//        k_sleep(K_MSEC(2000));
//    }


    /* Connect GPIOTE_0 IRQ to nrfx_gpiote_irq_handler */
//    IRQ_CONNECT(DT_IRQN(DT_NODELABEL(gpiote)),
//                DT_IRQ(DT_NODELABEL(gpiote), priority),
//                nrfx_isr, nrfx_gpiote_irq_handler, 0);
//    (void)nrfx_gpiote_init(NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
//    app_button_init(on_btn_single_click, NULL, on_btn_long_press, NULL);
//    app_sht30d_init();
//    led_init();
    k_msleep(100);
    otInstance * instance = openthread_get_default_instance();
    otError  error        = OT_ERROR_NONE;
    __ASSERT(instance, "OT instance is NULL");
    const otNetifAddress *unicastAddrs = otIp6GetUnicastAddresses(instance);
    char buf[40];
    for (const otNetifAddress *addr = unicastAddrs; addr; addr = addr->mNext)
    {
        net_addr_ntop(AF_INET6, &addr->mAddress, buf, 40);
        LOG_INF("+IPADDR:%s", log_strdup(buf));
    }
    int pos = 0;
    otExtAddress extAddress;
    otLinkGetFactoryAssignedIeeeEui64(instance, &extAddress);
    for(int i=0; i<OT_EXT_ADDRESS_SIZE; i++){
        pos += sprintf(&buf[pos], "%02x", extAddress.m8[i]);
    }
    buf[pos] = '\0';
    LOG_INF("PSK:%s", log_strdup(buf));
    app_lwm2m_settings lwm2m_settings = {
            .psk_id = {0x00},
            .psk_key = {
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
            },
            .psk_key_length = 16,
            .enable_psk = 1U,
            .server_addr = "fd40:2d04:3c20::73a",
            .server_is_bootstrap = false,
    };
    memcpy(lwm2m_settings.psk_id, buf, pos);
//    (void)lwm2m_engine_create_obj_inst("3303/0");
//    (void)lwm2m_engine_register_read_callback("3303/0/5700", temperature_get_buf);
    ret = lwm2m_engine_create_obj_inst("3342/0");
    if(ret < 0){
        LOG_ERR("create ojb 3342/0 err (%d)", ret);
    }
    ret = lwm2m_engine_create_obj_inst("3311/0");
    if(ret < 0){
        LOG_ERR("create ojb 3311/0 err (%d)", ret);
    }
//    ret = lwm2m_engine_register_post_write_callback("3311/0/5850",
//                                              led_on_off_cb);
//    if(ret < 0){
//        LOG_ERR("create resource 3311/0/5850 err (%d)", ret);
//    }
    lwm2m_engine_set_res_data("3311/0/5750",
                              LIGHT_NAME, sizeof(LIGHT_NAME),
                              LWM2M_RES_DATA_FLAG_RO);

    for(int i=0; i<30; i++){
        if (app_ot_is_connected()){
            break;
        }
        (void)k_msleep(1000);
    }
    ret = app_lwm2m_client_start(&lwm2m_settings);
    if (ret < 0) {
        LOG_ERR("Cannot setup LWM2M fields (%d)", ret);
    }
    while(1){
        k_msleep(3600000);
    }
    LOG_ERR("NEVER RUN!");
}

