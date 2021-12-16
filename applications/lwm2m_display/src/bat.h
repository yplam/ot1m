//
// Created by yplam on 14/12/2021.
//

#ifndef LWM2M_DISPLAY_BAT_H
#define LWM2M_DISPLAY_BAT_H
#include <zephyr.h>

bool bat_is_charge(void);
bool bat_is_standby(void);
void bat_init(void);
int bat_val_read(void);
#endif //LWM2M_DISPLAY_BAT_H
