
#include "app_openthread.h"
#include <zephyr.h>
#include <init.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(app_openthread, LOG_LEVEL_INF);
#include <net/openthread.h>
#include <openthread/ip6.h>
#include <openthread/thread.h>
#include "app_settings.h"

static app_ot_settings ot_settings = {
        .sed_enable = 0U,
        .poll_period_ms = CONFIG_APP_OPENTHREAD_ACTIVE_POLL_PERIOD,
        .timeout_s = CONFIG_APP_OPENTHREAD_CHILD_TIMEOUT
};

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
        ot_settings.sed_enable = 0U;
        ot_settings.poll_period_ms = CONFIG_APP_OPENTHREAD_SLEEPY_POLL_PERIOD;
        ot_settings.timeout_s = CONFIG_APP_OPENTHREAD_CHILD_TIMEOUT;
    }

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
