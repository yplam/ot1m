
#ifndef APP_SENSOR_H
#define APP_SENSOR_H

#include <drivers/sensor.h>

struct sensor_value *app_sensor_get_value(int index);
struct k_sem *app_sensor_get_sem(void);
#endif //APP_SENSOR_H
