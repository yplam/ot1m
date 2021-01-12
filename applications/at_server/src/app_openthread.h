#ifndef APP_OPENTHREAD_H
#define APP_OPENTHREAD_H
#include <zephyr.h>
#include <net/openthread.h>

enum app_ot_power_state {
    OT_POWER_ACTIVE,
    OT_POWER_SLEEPY,
};

typedef struct {
    uint8_t sed_enable;
    uint32_t poll_period_ms;
    uint32_t timeout_s;
} app_ot_settings;

void app_ot_set_power_state(enum app_ot_power_state state);

#endif //APP_OPENTHREAD_H
