#ifndef OT_AT_APP_AT_MODEM_H
#define OT_AT_APP_AT_MODEM_H
#include <stdio.h>
#include <zephyr.h>
#include <modem/at_cmd_parser.h>

typedef int (*at_cmd_parser_t)(const char *at_buf);

/**@brief AT command handler type. */
typedef int (*at_cmd_handler_t) (enum at_cmd_type);

/**@brief AT command list item type. */
typedef struct at_cmd_list {
    uint8_t type;
    char *string;
    at_cmd_handler_t handler;
} at_cmd_list_t;

void app_at_modem_init(void);
void app_at_set_ate(bool new_ate);
void app_at_modem_rsp_send(const uint8_t *str, size_t len);
bool app_at_modem_cmd_casecmp(const char *cmd_buf, const char *cmd_target);

#endif //OT_AT_APP_AT_MODEM_H
