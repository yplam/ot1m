#include "at_system.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(at_sys, LOG_LEVEL_INF);
#include <power/reboot.h>

extern struct at_param_list at_param_list;
extern char rsp_buf[CONFIG_APP_AT_RESPONSE_MAX_LEN];

/**@brief List of supported AT commands. */
enum at_system_type {
    AT_SYSTEM_AT,
    AT_SYSTEM_ATE0,
    AT_SYSTEM_ATE1,
    AT_SYSTEM_RST,
    AT_SYSTEM_GMR,
    AT_SYSTEM_MAX
};

static int handle_at_system_at(enum at_cmd_type cmd_type);
static int handle_at_system_ate0(enum at_cmd_type cmd_type);
static int handle_at_system_ate1(enum at_cmd_type cmd_type);
static int handle_at_system_reset(enum at_cmd_type cmd_type);
static int handle_at_system_gmr(enum at_cmd_type cmd_type);

static at_cmd_list_t m_at_system_list[AT_SYSTEM_MAX] = {
        {AT_SYSTEM_AT, "AT", handle_at_system_at},
        {AT_SYSTEM_ATE0, "ATE0", handle_at_system_ate0},
        {AT_SYSTEM_ATE1, "ATE1", handle_at_system_ate1},
        {AT_SYSTEM_RST, "AT+RST", handle_at_system_reset},
        {AT_SYSTEM_GMR, "AT+GMR", handle_at_system_gmr},
};

int at_modem_system_parser(const char *at_cmd){
    int ret = -ENOTSUP;
    enum at_cmd_type type;

    for (int i = 0; i < AT_SYSTEM_MAX; i++) {
        if (app_at_modem_cmd_casecmp(at_cmd, m_at_system_list[i].string)) {
            ret = at_parser_params_from_str(at_cmd, NULL,
                                            &at_param_list);
            if (ret) {
                return -EINVAL;
            }
            type = at_parser_cmd_type_get(at_cmd);
            ret = m_at_system_list[i].handler(type);
            break;
        }
    }

    return ret;
}

static int handle_at_system_at(enum at_cmd_type cmd_type) {
    return 0;
}

static int handle_at_system_ate0(enum at_cmd_type cmd_type){
    app_at_set_ate(false);
    return 0;
}

static int handle_at_system_ate1(enum at_cmd_type cmd_type){
    app_at_set_ate(true);
    return 0;
}

static int handle_at_system_reset(enum at_cmd_type cmd_type) {
    sys_reboot(SYS_REBOOT_WARM);
    return 0;
}

static int handle_at_system_gmr(enum at_cmd_type cmd_type) {
    const char sVersion[] = "ot1m 2.4.0"
#if defined(__DATE__)
" " __DATE__ " " __TIME__
#endif
    ;
    sprintf(rsp_buf, "+GMR:\"%s\"\r\n",
            sVersion);
    app_at_modem_rsp_send(rsp_buf, strlen(rsp_buf));
    return 0;
}