//
// Created by yplam on 27/1/2022.
//

#include "senseair_s8.h"
#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(s8, LOG_LEVEL_INF);
#include <drivers/gpio.h>
#include <modbus/modbus.h>

static int client_iface;

const static struct modbus_iface_param client_param = {
        .mode = MODBUS_MODE_RTU,
        .rx_timeout = 500000,
        .serial = {
                .baud = 9600,
                .parity = UART_CFG_PARITY_NONE,
        },
};

int senseair_s8_init(void) {
    const char iface_name[] = {DT_PROP(DT_INST(0, zephyr_modbus_serial), label)};

    client_iface = modbus_iface_get_by_name(iface_name);

    return modbus_init_client(client_iface, client_param);
}

uint16_t senseair_s8_read(void) {
    uint16_t sensor_value[1];
    modbus_read_input_regs(
        client_iface,
        0xFE,
        3,
        sensor_value,
        1);
    return sensor_value[0];
}