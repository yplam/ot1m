//
// Created by yplam on 14/1/2021.
//

#ifndef COAP_SENSOR_SHT30D_H
#define COAP_SENSOR_SHT30D_H
#include <drivers/sensor.h>
void app_sht30d_init(void);
int app_sht30d_fetch(float * temp, float * hum);
int app_sht30d_fetch_sensor(struct sensor_value * temp, struct sensor_value * hum);
#endif //COAP_SENSOR_SHT30D_H
