#include "at_coap.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(at_coap, LOG_LEVEL_INF);
#include <modem/at_cmd_parser.h>
#include "app_coap.h"
#include <net/coap.h>

extern struct at_param_list at_param_list;
extern char rsp_buf[CONFIG_APP_AT_RESPONSE_MAX_LEN];

#define PEER_PORT 5683

/**@brief List of supported AT commands. */
enum at_coap_type {
    AT_COAP_GET,
    AT_COAP_POST,
    AT_TB_TELEMETRY,
    AT_COAP_MAX
};

static int handle_at_coap_get(enum at_cmd_type cmd_type);
static int handle_at_coap_post(enum at_cmd_type cmd_type);
static int handle_at_coap_tb_telemetry(enum at_cmd_type cmd_type);

static at_cmd_list_t m_at_cmd_coap_list[AT_COAP_MAX] = {
        {AT_COAP_GET, "AT+COAPGET", handle_at_coap_get},
        {AT_COAP_POST, "AT+COAPPOST", handle_at_coap_post},
        {AT_TB_TELEMETRY, "AT+COAPTB", handle_at_coap_tb_telemetry},
};


int at_coap_parser(const char *at_cmd){
    int ret = -ENOTSUP;
    enum at_cmd_type type;

    for (int i = 0; i < AT_COAP_MAX; i++) {
        if (app_at_modem_cmd_casecmp(at_cmd, m_at_cmd_coap_list[i].string)) {
            ret = at_parser_params_from_str(at_cmd, NULL,
                                            &at_param_list);
            if (ret) {
                return -EINVAL;
            }
            type = at_parser_cmd_type_get(at_cmd);
            ret = m_at_cmd_coap_list[i].handler(type);
            break;
        }
    }

    return ret;
}

static void at_coap_send_cb(const char *data, size_t data_len){
    struct coap_packet reply;
    const uint8_t *payload;
    uint16_t payload_len;
    int ret;
    ret = coap_packet_parse(&reply, (uint8_t *)data, data_len, NULL, 0);
    if (ret < 0) {
        LOG_ERR("Invalid data received");
    }
    else{
        payload = coap_packet_get_payload(&reply, &payload_len);
        sprintf(rsp_buf, "+COAP:\"%.*s\"\r\n", payload_len, payload);
        app_at_modem_rsp_send(rsp_buf, strlen(rsp_buf));
    }
}

static int handle_at_coap_request(enum coap_method method, enum at_cmd_type cmd_type){
    int ret = 0;
    struct sockaddr_in6 addr6;
    char ip_str[40];
    uint8_t *payload;
    uint8_t uri_path[CONFIG_APP_COAP_MAX_URI_LEN+1];
    size_t len;
    // 分配内存
    payload = (uint8_t *)k_malloc(CONFIG_APP_MAX_COAP_MSG_LEN);
    if (!payload) {
        ret = -ENOMEM;
        goto exit;
    }

    // IP
    len = 39;
    ret = at_params_string_get(&at_param_list, 1, ip_str, &len);
    if (ret < 0) {
        LOG_ERR("wrong ip input");
        ret = -EINVAL;
        goto exit;
    }
    ip_str[len] = '\0';

    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons(PEER_PORT);
    addr6.sin6_scope_id = 0U;
    if (net_addr_pton(AF_INET6, ip_str, &addr6.sin6_addr) < 0) {
        ret = -EINVAL;
        LOG_ERR("wrong ip format");
        goto exit;
    }

    // URI
    len = CONFIG_APP_COAP_MAX_URI_LEN;
    ret = at_params_string_get(&at_param_list, 2, uri_path, &len);
    if (ret < 0) {
        LOG_ERR("wrong uri");
        ret = -EINVAL;
        goto exit;
    }
    uri_path[len] = '\0';

    if(at_params_valid_count_get(&at_param_list) > 3){
        len = CONFIG_APP_MAX_COAP_MSG_LEN-1;
        ret = at_params_string_get(&at_param_list, 3, payload, &len);
        if (ret < 0) {
            LOG_ERR("wrong payload");
            ret = -EINVAL;
            goto exit;
        }
    }
    else{
        payload[0] = '\0';
    }
    ret = app_coap_send_request(&addr6, method, uri_path, payload, COAP_TYPE_CON, at_coap_send_cb);
    if (ret < 0) {
        LOG_ERR("coap request error %d", ret);
        goto exit;
    }
exit:
    k_free(payload);
    return ret;
}

static int handle_at_coap_get(enum at_cmd_type cmd_type) {
    LOG_INF("coap get");
    if (at_params_valid_count_get(&at_param_list) < 3) {
        return -EINVAL;
    }
    return handle_at_coap_request(COAP_METHOD_GET, cmd_type);
}

static int handle_at_coap_post(enum at_cmd_type cmd_type) {
    LOG_INF("coap post");
    if (at_params_valid_count_get(&at_param_list) < 4) {
        return -EINVAL;
    }
    return handle_at_coap_request(COAP_METHOD_POST, cmd_type);
}

static int handle_at_coap_tb_telemetry(enum at_cmd_type cmd_type) {
    LOG_INF("thingsboard tel");
    int ret = 0;
    struct sockaddr_in6 addr6;
    char ip_str[40];
    char tel_key[40];
     char tel_value[40];
    uint8_t *payload;
    uint8_t uri_path[CONFIG_APP_COAP_MAX_URI_LEN+1];
    size_t len;
    // 分配内存

    if (at_params_valid_count_get(&at_param_list) < 5U) {
        return -EINVAL;
    }

    payload = (uint8_t *)k_malloc(CONFIG_APP_MAX_COAP_MSG_LEN);
    if (!payload) {
        ret = -ENOMEM;
        goto exit;
    }

    // IP
    len = 39;
    ret = at_params_string_get(&at_param_list, 1, ip_str, &len);
    if (ret < 0) {
        LOG_ERR("wrong ip input");
        ret = -EINVAL;
        goto exit;
    }
    ip_str[len] = '\0';
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons(PEER_PORT);
    addr6.sin6_scope_id = 0U;
    if (net_addr_pton(AF_INET6, ip_str, &addr6.sin6_addr) < 0) {
        ret = -EINVAL;
        LOG_ERR("wrong ip format");
        goto exit;
    }

    // URI
    len = CONFIG_APP_COAP_MAX_URI_LEN;
    ret = at_params_string_get(&at_param_list, 2, uri_path, &len);
    if (ret < 0) {
        LOG_ERR("wrong uri");
        ret = -EINVAL;
        goto exit;
    }
    uri_path[len] = '\0';

    len = 39;
    ret = at_params_string_get(&at_param_list, 3, tel_key, &len);
    if (ret < 0) {
        LOG_ERR("wrong tel key");
        ret = -EINVAL;
        goto exit;
    }
    tel_key[len] = '\0';

    len = 39;
    ret = at_params_string_get(&at_param_list, 4, tel_value, &len);
    if (ret < 0) {
        LOG_ERR("wrong tel value");
        ret = -EINVAL;
        goto exit;
    }
    tel_value[len] = '\0';

    len = sprintf(payload, "{\"%s\":\"%s\"}\n", tel_key, tel_value);
    payload[len] = '\0';

    ret = app_coap_send_request(&addr6, COAP_METHOD_POST, uri_path, payload, COAP_TYPE_CON, NULL);
    if (ret < 0) {
        LOG_ERR("coap request error %d", ret);
        goto exit;
    }
    ret = 0;
exit:
    k_free(payload);
    return ret;
}
