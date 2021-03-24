
#include "app_openthread.h"
#include <zephyr.h>
#include <init.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(app_openthread, LOG_LEVEL_INF);
#include <net/openthread.h>
#include <openthread/ip6.h>
#include <openthread/thread.h>
#include "app_settings.h"

#if defined(CONFIG_APP_OPENTHREAD_JOINER_PSKD)
#define OT_JOINER_PSKD CONFIG_APP_OPENTHREAD_JOINER_PSKD
#else
#define OT_JOINER_PSKD "J01NME"
#endif

static bool is_connected;

static app_ot_settings ot_settings = {
        .sed_enable = 0U,
        .poll_period_ms = CONFIG_APP_OPENTHREAD_ACTIVE_POLL_PERIOD,
        .timeout_s = CONFIG_APP_OPENTHREAD_CHILD_TIMEOUT
};

bool app_ot_is_connected(void) {
    return is_connected;
}


static void handle_openthread_join_callback(otError error, void * p_context)
{
    otInstance * instance = openthread_get_default_instance();
    __ASSERT(instance, "OT instance is NULL");
    LOG_INF("joiner callback");
    if (error == OT_ERROR_NONE)
    {
        otThreadSetEnabled(instance, true);
        LOG_INF("JOIN:OK");
    }
    else
    {
        LOG_INF("JION:ERROR %d", error);
    }
}

void app_ot_start_join(void){
    otInstance * instance = openthread_get_default_instance();
    struct openthread_context * ot_context = openthread_get_default_context();
    otError  error        = OT_ERROR_NONE;
    error = otThreadSetEnabled(instance, false);
    if(error != OT_ERROR_NONE){
        return;
    }
    if(otIp6IsEnabled(instance)){
        otIp6SetEnabled(instance, false);
    }
    otIp6SetEnabled(instance, true);
    (void)otLinkSetPanId(instance, 0xFFFF);
    LOG_INF("start joiner");
    openthread_api_mutex_lock(ot_context);
    error = otJoinerStart(instance, OT_JOINER_PSKD, NULL, "OT1M",
                          "OT1M", "1.0", NULL,
                          handle_openthread_join_callback, instance);
    openthread_api_mutex_unlock(ot_context);
}


void app_ot_set_power_state(enum app_ot_power_state state) {
    otLinkModeConfig linkModeConfig;
    otInstance *instance = openthread_get_default_instance();
    linkModeConfig = otThreadGetLinkMode(instance);
    if(linkModeConfig.mRxOnWhenIdle){
        return;
    }
    switch (state) {
        case OT_POWER_ACTIVE:
            (void)otLinkSetPollPeriod(instance, CONFIG_APP_OPENTHREAD_ACTIVE_POLL_PERIOD);
            break;
        case OT_POWER_SLEEPY:
            (void)otLinkSetPollPeriod(instance, ot_settings.poll_period_ms);
            break;
        default:
            break;
    }
}


static void app_on_thread_state_changed(uint32_t flags, void *context)
{
    struct openthread_context *ot_context = context;

    if (flags & OT_CHANGED_THREAD_ROLE) {
        switch (otThreadGetDeviceRole(ot_context->instance)) {
            case OT_DEVICE_ROLE_CHILD:
            case OT_DEVICE_ROLE_ROUTER:
            case OT_DEVICE_ROLE_LEADER:
                is_connected = true;
                break;

            case OT_DEVICE_ROLE_DISABLED:
            case OT_DEVICE_ROLE_DETACHED:
            default:
                is_connected = false;
                break;
        }
    }
}

static int app_openthread_network_start(const struct device *dev)
{
    struct openthread_context * ot_context;
    LOG_INF("starting openthread network");
    ot_context = openthread_get_default_context();
    otInstance *ot_instance = ot_context->instance;
    otError error = OT_ERROR_NONE;
    uint16_t setting_len = sizeof(ot_settings);
    int ret = app_settings_get(APP_SETTINGS_OT, (uint8_t *)&ot_settings, &setting_len);
    if(ret != 0){
        ot_settings.sed_enable = 1U;
        ot_settings.poll_period_ms = CONFIG_APP_OPENTHREAD_SLEEPY_POLL_PERIOD;
        ot_settings.timeout_s = CONFIG_APP_OPENTHREAD_CHILD_TIMEOUT;
    }
    openthread_set_state_changed_cb(app_on_thread_state_changed);
    if (IS_ENABLED(CONFIG_OPENTHREAD_MANUAL_START)) {

        openthread_api_mutex_lock(ot_context);
        otLinkModeConfig ot_mode = otThreadGetLinkMode(ot_instance);
        if(ot_settings.sed_enable){
            LOG_INF("Setting sed config %d, %d", ot_settings.poll_period_ms, ot_settings.timeout_s);
            ot_mode.mRxOnWhenIdle = false;
            (void)otThreadSetLinkMode(ot_instance, ot_mode);
            (void)otLinkSetPollPeriod(ot_instance, ot_settings.poll_period_ms);
            otThreadSetChildTimeout(ot_instance, ot_settings.timeout_s);
        }
        if (otDatasetIsCommissioned(ot_instance)) {
            (void)otIp6SetEnabled(ot_context->instance, true);
            LOG_INF("OpenThread version: %s", otGetVersionString());
            LOG_INF("Network name: %s",
                    log_strdup(otThreadGetNetworkName(ot_instance)));

            /* Start the network. */
            error = otThreadSetEnabled(ot_instance, true);
            if (error != OT_ERROR_NONE) {
                LOG_ERR("Failed to start the OpenThread network [%d]", error);
            }
        }
        openthread_api_mutex_unlock(ot_context);
    }

    return error == OT_ERROR_NONE ? 0 : -EIO;
}

SYS_INIT(app_openthread_network_start, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT-1);
