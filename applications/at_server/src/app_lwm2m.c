
#include "app_lwm2m.h"
#include <zephyr.h>
#include "../../zephyr/subsys/net/lib/lwm2m/lwm2m_engine.h"
#include "app_settings.h"
#include <logging/log.h>
#include <logging/log_ctrl.h>
LOG_MODULE_REGISTER(app_lwm2m, LOG_LEVEL_INF);
#include <power/reboot.h>
#include <init.h>
#include <net/openthread.h>
#include <openthread/platform/radio.h>
#include <openthread/link.h>
#include <stdio.h>

#define TLS_TAG			1
static struct lwm2m_ctx client;
uint32_t flags;
enum lwm2m_rd_client_event client_event;
char eui[17];
enum lwm2m_rd_client_event app_lwm2m_get_client_event(void)
{
    return client_event;
}

static void rd_client_event(struct lwm2m_ctx *client,
                            enum lwm2m_rd_client_event rd_client_event)
{
    ARG_UNUSED(client);
    client_event = rd_client_event;
    switch (client_event) {

        case LWM2M_RD_CLIENT_EVENT_NONE:
            /* do nothing */
            break;

        case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE:
            LOG_INF("Bootstrap registration failure!");
            break;

        case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE:
            LOG_INF("Bootstrap registration complete");
            break;

        case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_TRANSFER_COMPLETE:
            LOG_INF("Bootstrap transfer complete");
            break;

        case LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE:
            LOG_INF("Registration failure!");
            break;

        case LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE:
            LOG_INF("Registration complete");
            break;

        case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_FAILURE:
            LOG_INF("Registration update failure!");
            break;

        case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE:
            LOG_INF("Registration update complete");
            break;

        case LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE:
            LOG_INF("Deregister failure!");
            break;

        case LWM2M_RD_CLIENT_EVENT_DISCONNECT:
            LOG_INF("Disconnected");
            break;

        case LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF:
            LOG_INF("Queue mode RX window closed");
            break;
    }
}

static int device_reboot_cb(uint16_t obj_inst_id)
{
    ARG_UNUSED(obj_inst_id);
    LOG_INF("Device rebooting.");
    LOG_PANIC();
    sys_reboot(SYS_REBOOT_WARM);
    return 0; /* wont reach this */
}

static int device_reset_error_code_cb(uint16_t obj_inst_id)
{
    LOG_INF("Device reset error code.");
    return 0; /* wont reach this */
}

int app_lwm2m_client_start(app_lwm2m_settings * lwm2m_settings){
    int32_t ret;
    char *server_url;
    uint16_t server_url_len;
    uint8_t server_url_flags;
    flags = lwm2m_settings->server_is_bootstrap ?
                     LWM2M_RD_CLIENT_FLAG_BOOTSTRAP : 0U;
    (void)memset(&client, 0x0, sizeof(client));
    LOG_INF("connecting to lwm2m");
    if(!strlen(lwm2m_settings->server_addr)){
        return -EINVAL;
    }
    if(lwm2m_settings->enable_psk){
        if(!strlen(lwm2m_settings->psk_id)){
            LOG_ERR("no psk_id");
            return -EINVAL;
        }
        if(!lwm2m_settings->psk_key_length){
            LOG_ERR("wrong psk key len");
            return -EINVAL;
        }
    }
    /* Server URL */
    ret = lwm2m_engine_get_res_data("0/0/0",
                                    (void **)&server_url, &server_url_len,
                                    &server_url_flags);
    if (ret < 0) {
        return ret;
    }

    (void)snprintk(server_url, server_url_len, "coap%s//%s%s%s",
             lwm2m_settings->enable_psk ? "s:" : ":",
             strchr(lwm2m_settings->server_addr, ':') ? "[" : "", lwm2m_settings->server_addr,
             strchr(lwm2m_settings->server_addr, ':') ? "]" : "");

    /* Security Mode */
    (void)lwm2m_engine_set_u8("0/0/2",
                        lwm2m_settings->enable_psk ? 0 : 3);
    if(lwm2m_settings->enable_psk){
        (void)lwm2m_engine_set_string("0/0/3", (char *)lwm2m_settings->psk_id);
        (void)lwm2m_engine_set_opaque("0/0/5",
                                (void *)lwm2m_settings->psk_key, lwm2m_settings->psk_key_length);
        client.tls_tag = TLS_TAG;
    }
    if(lwm2m_settings->server_is_bootstrap){
        /* Mark 1st instance of security object as a bootstrap server */
        (void)lwm2m_engine_set_u8("0/0/1", 1);

        /* Create 2nd instance of security object needed for bootstrap */
        (void)lwm2m_engine_create_obj_inst("0/1");
    }
    else{
        /* Match Security object instance with a Server object instance with
     * Short Server ID.
     */
        (void)lwm2m_engine_set_u16("0/0/10", 101);
        (void)lwm2m_engine_set_u16("1/0/0", 101);
    }
    otInstance * instance = openthread_get_default_instance();
    char pos = 0U;
    otExtAddress extAddress;
    otLinkGetFactoryAssignedIeeeEui64(instance, &extAddress);
    for(int i=0; i<OT_EXT_ADDRESS_SIZE; i++){
        pos += sprintf(eui+pos, "%02x", extAddress.m8[i]);
    }
    eui[16] = '\0';

    (void)lwm2m_engine_set_res_data("3/0/0", CONFIG_APP_MANUFACTURER,
                              sizeof(CONFIG_APP_MANUFACTURER),
                              LWM2M_RES_DATA_FLAG_RO);
    (void)lwm2m_engine_set_res_data("3/0/1", CONFIG_APP_MODEL_NUMBER,
                              sizeof(CONFIG_APP_MODEL_NUMBER),
                              LWM2M_RES_DATA_FLAG_RO);
    (void)lwm2m_engine_set_res_data("3/0/2", eui,
                              strlen(eui),
                              LWM2M_RES_DATA_FLAG_RO);
    (void)lwm2m_engine_set_res_data("3/0/3", CONFIG_APP_FIRMWARE_VER,
                              sizeof(CONFIG_APP_FIRMWARE_VER),
                              LWM2M_RES_DATA_FLAG_RO);
    (void)lwm2m_engine_set_res_data("3/0/18", CONFIG_APP_HW_VER,
                                    sizeof(CONFIG_APP_HW_VER),
                                    LWM2M_RES_DATA_FLAG_RO);

    /* Reboot resource of Device object = 3/0/4 */
    (void)lwm2m_engine_register_exec_callback("3/0/4", device_reboot_cb);
    (void)lwm2m_engine_register_exec_callback("3/0/12", device_reset_error_code_cb);

//    struct lwm2m_engine_obj_inst *obj_inst = NULL;
//    ret = lwm2m_create_obj_inst(IPSO_OBJECT_TEMP_SENSOR_ID, 0, &obj_inst);
//    if (ret < 0) {
//        LOG_DBG("Create LWM2M instance 0 error: %d", ret);
//    }

    lwm2m_rd_client_start(&client, eui, flags, rd_client_event);
    return ret;
}

static int app_lwm2m_init_client(const struct device *dev)
{
    app_lwm2m_settings lwm2m_settings;
    uint16_t setting_len = sizeof(lwm2m_settings);
    int ret = app_settings_get(APP_SETTINGS_LWM2M, (uint8_t *)&lwm2m_settings, &setting_len);
    if(ret == 0){
        ret = app_lwm2m_client_start(&lwm2m_settings);
        if(ret != 0){
            LOG_WRN("bootstrap error %d", ret);
        }
    }
    else{
        LOG_INF("no lwm2m setting config, skip");
    }
    return 0;
}
SYS_INIT(app_lwm2m_init_client, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT+1);
