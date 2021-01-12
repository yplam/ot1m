#ifndef OT1M_APP_SETTINGS_H
#define OT1M_APP_SETTINGS_H

#include <zephyr.h>

// 注意：只能在后面添加，不能更改顺序
enum app_settings_key {
    APP_SETTINGS_NULL,      // 不使用
    APP_SETTINGS_OT,    // openthread setting
    APP_SETTINGS_LWM2M,    // lwm2m setting
};

void app_init_settings(void);
int app_settings_get(enum app_settings_key key, uint8_t *value, uint16_t *value_length);
int app_settings_set(enum app_settings_key key, uint8_t *value, uint16_t value_length);

#endif //OT1M_APP_SETTINGS_H
