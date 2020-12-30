#ifndef OT_AT_AT_CMD_COAP_H
#define OT_AT_AT_CMD_COAP_H
#include <zephyr.h>
#include "app_at_modem.h"
/**
 * COAP AT 指令
 * AT+COAPGET=<IP>,<URI>
 * AT+COAPPOST=<IP>,<URI>,<PAYLOAD>
 * AT+COAPTB=<IP>,<URI>,<KEY>,<VALUE>
 * @param at_cmd
 * @return
 */
int at_coap_parser(const char *at_cmd);
#endif //OT_AT_AT_CMD_COAP_H
