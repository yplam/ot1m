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

#include <net/openthread.h>
#include <openthread/link.h>
#include <openthread/thread.h>

void main(void) {
    LOG_INF("OT1M Starting...");
    uint8_t buff[50];
    int ret;

    const struct device *dev = device_get_binding("SHT3XD");
    int rc;

    if (dev == NULL) {
        LOG_WRN("Could not get SHT3XD device\n");
        return;
    }

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

