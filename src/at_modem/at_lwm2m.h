#ifndef OT_AT_AT_CMD_LWM2M_H
#define OT_AT_AT_CMD_LWM2M_H
#include <zephyr.h>
#include "app_at_modem.h"
#include "app_lwm2m.h"
/**
 * COAP AT 指令
 * AT+LWSER=<IP>
 * @param at_cmd
 * @return
 */
int at_lwm2m_parser(const char *at_cmd);
#endif //OT_AT_AT_CMD_LWM2M_H
