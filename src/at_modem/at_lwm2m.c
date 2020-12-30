#include "at_lwm2m.h"
#include "../../../zephyr/subsys/net/lib/lwm2m/lwm2m_engine.h"
#include "../../../zephyr/subsys/net/lib/lwm2m/lwm2m_rd_client.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(at_lwm2m, LOG_LEVEL_INF);
#include <modem/at_cmd_parser.h>
#include "app_settings.h"

extern struct at_param_list at_param_list;
extern char rsp_buf[CONFIG_APP_AT_RESPONSE_MAX_LEN];

/**@brief List of supported AT commands. */
enum at_lwm2m_type {
    AT_LW_EVENT,
    AT_LW_STATE,
    AT_LW_SER,
    AT_LW_OBJ,
    AT_LW_STRING,
    AT_LW_BOOL,
    AT_LW_S8,
    AT_LW_S16,
    AT_LW_S32,
    AT_LW_S64,
    AT_LW_U8,
    AT_LW_U16,
    AT_LW_U32,
    AT_LW_U64,
    AT_LW_FLOAT32,
    AT_LW_FLOAT64,
    AT_LW_MAX
};

static int handle_at_lwm2m_state(enum at_cmd_type cmd_type);
static int handle_at_lwm2m_event(enum at_cmd_type cmd_type);
static int handle_at_lwm2m_server(enum at_cmd_type cmd_type);
static int handle_at_lwm2m_obj_inst(enum at_cmd_type cmd_type);
static int handle_at_lwm2m_string(enum at_cmd_type cmd_type);
static int handle_at_lwm2m_bool(enum at_cmd_type cmd_type);
static int handle_at_lwm2m_s8(enum at_cmd_type cmd_type);
static int handle_at_lwm2m_s16(enum at_cmd_type cmd_type);
static int handle_at_lwm2m_s32(enum at_cmd_type cmd_type);
static int handle_at_lwm2m_s64(enum at_cmd_type cmd_type);
static int handle_at_lwm2m_u8(enum at_cmd_type cmd_type);
static int handle_at_lwm2m_u16(enum at_cmd_type cmd_type);
static int handle_at_lwm2m_u32(enum at_cmd_type cmd_type);
static int handle_at_lwm2m_u64(enum at_cmd_type cmd_type);
static int handle_at_lwm2m_float32(enum at_cmd_type cmd_type);
static int handle_at_lwm2m_float64(enum at_cmd_type cmd_type);

static at_cmd_list_t m_at_cmd_lwm2m_list[AT_LW_MAX] = {
        {AT_LW_EVENT, "AT+LWEVENT", handle_at_lwm2m_event},
        {AT_LW_STATE, "AT+LWSTATE", handle_at_lwm2m_state},
        {AT_LW_SER, "AT+LWSER", handle_at_lwm2m_server},
        {AT_LW_OBJ, "AT+LWOBJ", handle_at_lwm2m_obj_inst},
        {AT_LW_STRING, "AT+LWVSTR", handle_at_lwm2m_string},
        {AT_LW_BOOL, "AT+LWVBOOL", handle_at_lwm2m_bool},
        {AT_LW_S8, "AT+LWVS", handle_at_lwm2m_s8},
        {AT_LW_S16, "AT+LWVSS", handle_at_lwm2m_s16},
        {AT_LW_S32, "AT+LWVSSS", handle_at_lwm2m_s32},
        {AT_LW_S64, "AT+LWVSSSS", handle_at_lwm2m_s64},
        {AT_LW_U8, "AT+LWVU", handle_at_lwm2m_u8},
        {AT_LW_U16, "AT+LWVUU", handle_at_lwm2m_u16},
        {AT_LW_U32, "AT+LWVUUU", handle_at_lwm2m_u32},
        {AT_LW_U64, "AT+LWVUUUU", handle_at_lwm2m_u64},
        {AT_LW_FLOAT32, "AT+LWVF", handle_at_lwm2m_float32},
        {AT_LW_FLOAT64, "AT+LWVFF", handle_at_lwm2m_float64},
};

int at_lwm2m_parser(const char *at_cmd){
    int ret = -ENOTSUP;
    enum at_cmd_type type;

    for (int i = 0; i < AT_LW_MAX; i++) {
        if (app_at_modem_cmd_casecmp(at_cmd, m_at_cmd_lwm2m_list[i].string)) {
            ret = at_parser_params_from_str(at_cmd, NULL,
                                            &at_param_list);
            if (ret) {
                return -EINVAL;
            }
            type = at_parser_cmd_type_get(at_cmd);
            ret = m_at_cmd_lwm2m_list[i].handler(type);
            break;
        }
    }

    return ret;
}

static int handle_at_lwm2m_event(enum at_cmd_type cmd_type) {
    enum lwm2m_rd_client_event client_event = app_lwm2m_get_client_event();
    sprintf(rsp_buf, "+LWEVENT:%d\r\n", client_event);
    app_at_modem_rsp_send(rsp_buf, strlen(rsp_buf));
    return 0;
}

static int handle_at_lwm2m_state(enum at_cmd_type cmd_type) {
    int ret;
    uint32_t start_stop;
    if(cmd_type == AT_CMD_TYPE_SET_COMMAND){
        if (at_params_valid_count_get(&at_param_list) < 2U) {
            return -EINVAL;
        }
        ret = at_params_int_get(&at_param_list, 1U, &start_stop);
        if (ret < 0) {
            return ret;
        }
        if(start_stop){

        }
        else{

        }
    }
    return 0;
}

static int handle_at_lwm2m_server(enum at_cmd_type cmd_type) {
    app_lwm2m_settings lwm2m_settings;
    uint16_t setting_len = sizeof(lwm2m_settings);
    uint8_t buffer[65];
    size_t param_len = NET_IPV6_ADDR_LEN;
    uint16_t server_is_bootstrap;
    int ret = app_settings_get(APP_SETTINGS_LWM2M, (uint8_t *)&lwm2m_settings, &setting_len);
    if(cmd_type == AT_CMD_TYPE_SET_COMMAND) {
        if (at_params_valid_count_get(&at_param_list) < 3U) {
            return -EINVAL;
        }
        ret = at_params_short_get(&at_param_list, 1U, &server_is_bootstrap);
        if (ret < 0) {
            return ret;
        }
        lwm2m_settings.server_is_bootstrap = (uint8_t) server_is_bootstrap;
        ret = at_params_string_get(&at_param_list, 2U, lwm2m_settings.server_addr, &param_len);
        if (ret < 0) {
            return ret;
        }
        lwm2m_settings.server_addr[param_len] = '\0';
        lwm2m_settings.enable_psk = 0;
        if (at_params_valid_count_get(&at_param_list) == 5U) {
            param_len = 31;
            ret = at_params_string_get(&at_param_list, 3U, lwm2m_settings.psk_id, &param_len);
            if (ret < 0) {
                return ret;
            }
            lwm2m_settings.psk_id[param_len] = '\0';
            param_len = 64;
            ret = at_params_string_get(&at_param_list, 4U, buffer, &param_len);
            if (ret < 0) {
                return ret;
            }
            buffer[param_len] = '\0';
            lwm2m_settings.psk_key_length = hex2bin(buffer, param_len, lwm2m_settings.psk_key, 32);
            if (!lwm2m_settings.psk_key_length) {
                return -EINVAL;
            }
            lwm2m_settings.enable_psk = 1;
        }
        (void) app_settings_set(APP_SETTINGS_LWM2M, (uint8_t *) &lwm2m_settings, setting_len);
        LOG_INF("get %d, %s, %d", lwm2m_settings.server_is_bootstrap, log_strdup(lwm2m_settings.server_addr),
                lwm2m_settings.enable_psk);
        ret = app_lwm2m_client_start(&lwm2m_settings);
    }
    else if(cmd_type == AT_CMD_TYPE_READ_COMMAND) {
        if(ret != 0){   // empty setting string
            sprintf(rsp_buf, "+LWSER:0,\"\"\r\n");
            app_at_modem_rsp_send(rsp_buf, strlen(rsp_buf));
        }
        else{
            LOG_INF("get %d, %s, %d, %d", lwm2m_settings.server_is_bootstrap, log_strdup(lwm2m_settings.server_addr),
                    lwm2m_settings.enable_psk, lwm2m_settings.psk_key_length);
            param_len = bin2hex(lwm2m_settings.psk_key, lwm2m_settings.psk_key_length, buffer, 64U);
            if(param_len == 0U){
                return -EINVAL;
            }
            buffer[param_len] = '\0';
            sprintf(rsp_buf, "+LWSER:%d,\"%s\",\"%s\",\"%s\"\r\n", lwm2m_settings.server_is_bootstrap,
                    lwm2m_settings.server_addr, lwm2m_settings.psk_id,buffer
                    );
            app_at_modem_rsp_send(rsp_buf, strlen(rsp_buf));
            return 0;
        }
    }
    return ret;
}

static int handle_at_lwm2m_obj_inst(enum at_cmd_type cmd_type) {
    int32_t ret=0;
    uint8_t buffer[32];
    size_t param_len = 31;
    if(cmd_type == AT_CMD_TYPE_SET_COMMAND) {
        if (at_params_valid_count_get(&at_param_list) < 2U) {
            return -EINVAL;
        }
        ret = at_params_string_get(&at_param_list, 1, buffer, &param_len);
        if (ret < 0) {
            return ret;
        }
        buffer[param_len] = '\0';
        ret = lwm2m_engine_create_obj_inst(buffer);
        if (ret == 0) {
            engine_trigger_update();
        }
    }
    return ret;
}

//static int handle_at_lwm2m_float32(enum at_cmd_type cmd_type) {
//    int32_t ret;
//    uint8_t path[32];
//    size_t param_len = 31;
//    float32_value_t fvalue = {0, 0};
//    uint32_t ui_value;
//    if (at_params_valid_count_get(&at_param_list) < 3U) {
//        return -EINVAL;
//    }
//    ret = at_params_string_get(&at_param_list, 1U, path, &param_len);
//    if (ret < 0) {
//        LOG_INF("ERR PATH");
//        return ret;
//    }
//    path[param_len] = '\0';
//    ret = at_params_int_get(&at_param_list, 2, &ui_value);
//    if (ret < 0) {
//        LOG_INF("ERR 2");
//        return ret;
//    }
//    fvalue.val1 = (int32_t)ui_value;
//    if(at_params_int_get(&at_param_list, 3, &ui_value) == 0){
//        fvalue.val2 = (int32_t)ui_value;
//    }
//    if(at_params_int_get(&at_param_list, 4, &ui_value) == 0){
//        if(ui_value){
//            fvalue.val1 = 0-fvalue.val1;
//        }
//    }
//    ret = lwm2m_engine_set_float32(path, &fvalue);
//    return ret;
//}

static int handle_at_lwm2m_string(enum at_cmd_type cmd_type) {
    int32_t ret;
    uint8_t path[32];
    uint8_t data[32];
    size_t param_len = 31U;
    if (at_params_valid_count_get(&at_param_list) < 3U) {
        return -EINVAL;
    }
    ret = at_params_string_get(&at_param_list, 1, path, &param_len);
    if (ret < 0) {
        return ret;
    }
    path[param_len] = '\0';
    param_len = 31U;
    ret = at_params_string_get(&at_param_list, 2, data, &param_len);
    if (ret < 0) {
        return ret;
    }
    data[param_len] = '\0';
    ret = lwm2m_engine_set_string(path, data);
    return ret;
}

enum lwm2m_value_type {
    VAL_BOOL,
    VAL_S8,
    VAL_S16,
    VAL_S32,
    VAL_S64,
    VAL_U8,
    VAL_U16,
    VAL_U32,
    VAL_U64,
    VAL_F32,
    VAL_F64,
};

static int handle_at_lwm2m_value(enum at_cmd_type cmd_type, enum lwm2m_value_type value_type) {
    int32_t ret;
    uint8_t path[32];
    size_t param_len = 31U;
    uint32_t u32_values[5] = {0U, 0U, 0U, 0U, 0U};
    uint32_t valid_count;
    // 可能使用 union 可以节省栈空间
    bool bool_val;
    int8_t int8_val;
    int16_t int16_val;
    int32_t int32_val;
    int64_t int64_val;
    uint8_t uint8_val;
    uint16_t uint16_val;
    uint32_t uint32_val;
    uint64_t uint64_val;
    float32_value_t f32_value = {0, 0};
    float64_value_t f64_value = {0, 0};

    if(cmd_type == AT_CMD_TYPE_SET_COMMAND) {
        valid_count = at_params_valid_count_get(&at_param_list);
        if (valid_count < 2U || valid_count > 7U) {
            return -EINVAL;
        }
        ret = at_params_string_get(&at_param_list, 1U, path, &param_len);
        if (ret < 0) {
            return ret;
        }
        path[param_len] = '\0';
        if(valid_count == 2U){  // READ VALUE
            switch(value_type) {
                case VAL_BOOL:
                    ret = lwm2m_engine_get_bool(path, &bool_val);
                    if(!ret){
                        sprintf(rsp_buf, "+LWVBOOL:%d\r\n", bool_val);
                    }
                    break;
                case VAL_S8:
                    ret = lwm2m_engine_get_s8(path, &int8_val);
                    if(!ret){
                        sprintf(rsp_buf, "+LWVS:%d\r\n", int8_val);
                    }
                    break;
                case VAL_S16:
                    ret = lwm2m_engine_get_s16(path, &int16_val);
                    if(!ret){
                        sprintf(rsp_buf, "+LWVSS:%d\r\n", int16_val);
                    }
                    break;
                case VAL_S32:
                    ret = lwm2m_engine_get_s32(path, &int32_val);
                    if(!ret){
                        sprintf(rsp_buf, "+LWVSSS:%d\r\n", int32_val);
                    }
                    break;
                case VAL_S64:
                    ret = lwm2m_engine_get_s64(path, &int64_val);
                    if(!ret){
                        sprintf(rsp_buf, "+LWVSSSS:%lld\r\n", int64_val);
                    }
                    break;
                case VAL_U8:
                    ret = lwm2m_engine_get_u8(path, &uint8_val);
                    if(!ret){
                        sprintf(rsp_buf, "+LWVU:%d\r\n", uint8_val);
                    }
                    break;
                case VAL_U16:
                    ret = lwm2m_engine_get_u16(path, &uint16_val);
                    if(!ret){
                        sprintf(rsp_buf, "+LWVUU:%d\r\n", uint16_val);
                    }
                    break;
                case VAL_U32:
                    ret = lwm2m_engine_get_u32(path, &uint32_val);
                    if(!ret){
                        sprintf(rsp_buf, "+LWVUUU:%u\r\n", uint32_val);
                    }
                    break;
                case VAL_U64:
                    ret = lwm2m_engine_get_u64(path, &uint64_val);
                    if(!ret){
                        sprintf(rsp_buf, "+LWVUUUU:%llu\r\n", uint64_val);
                    }
                    break;
                case VAL_F32:
                    ret = lwm2m_engine_get_float32(path, &f32_value);
                    if(!ret){
                        bool_val = f32_value.val1 > 0 ? 0 : 1;
                        f32_value.val1 = abs(f32_value.val1);
                        sprintf(rsp_buf, "+LWVF:%d,%d,%d\r\n", f32_value.val1, f32_value.val2, bool_val);
                    }
                    break;
                case VAL_F64:
                    ret = lwm2m_engine_get_float64(path, &f64_value);
                    if(!ret){
                        bool_val = f64_value.val1 > 0 ? 0 : 1;
                        f64_value.val1 = abs(f64_value.val1);
                        u32_values[0] = f64_value.val1 >> 32;
                        u32_values[1] = f64_value.val1 & 0xFFFFFFFFU;
                        u32_values[2] = f64_value.val2 >> 32;
                        u32_values[3] = f64_value.val2 & 0xFFFFFFFFU;
                        sprintf(rsp_buf, "+LWVFF:%u,%u,%u,%u,%d\r\n", u32_values[0], u32_values[1],
                                u32_values[2], u32_values[3], bool_val);
                    }
                    break;
                default:
                    return -EINVAL;
            }
            if(!ret){
                app_at_modem_rsp_send(rsp_buf, strlen(rsp_buf));
            }
            return ret;
        }
        else {
            // WRITE VALUE
            for (uint32_t i = 0U; i < (valid_count - 2U); i++) {
                ret = at_params_int_get(&at_param_list, i + 2U, &u32_values[i]);
                if (ret < 0) {
                    return ret;
                }
            }
            switch (value_type) {
                case VAL_BOOL:
                    return lwm2m_engine_set_bool(path, u32_values[0] ? true : false);
                case VAL_S8:
                    return lwm2m_engine_set_s8(path, u32_values[1] ? (int8_t) (0 - (int8_t) u32_values[0])
                                                                   : (int8_t) u32_values[0]);
                case VAL_S16:
                    return lwm2m_engine_set_s16(path, u32_values[1] ? (int16_t) (0 - (int16_t) u32_values[0])
                                                                    : (int16_t) u32_values[0]);
                case VAL_S32:
                    return lwm2m_engine_set_s32(path, u32_values[1] ? (int32_t) (0 - (int32_t) u32_values[0])
                                                                    : (int32_t) u32_values[0]);
                case VAL_S64:
                    int64_val = (((int64_t) u32_values[0]) << 32) + (int64_t) u32_values[1];
                    if (u32_values[2]) {
                        int64_val = 0 - int64_val;
                    }
                    return lwm2m_engine_set_s64(path, int64_val);
                case VAL_U8:
                    return lwm2m_engine_set_u8(path, (uint8_t) u32_values[0]);
                case VAL_U16:
                    return lwm2m_engine_set_u16(path, (uint16_t) u32_values[0]);
                case VAL_U32:
                    return lwm2m_engine_set_u32(path, (uint32_t) u32_values[0]);
                case VAL_U64:
                    return lwm2m_engine_set_u64(path,
                                                (((uint64_t) u32_values[0]) << 32) + (uint64_t) u32_values[1]);
                case VAL_F32:
                    f32_value.val1 = (int32_t) u32_values[0];
                    f32_value.val2 = (int32_t) u32_values[1];
                    if (u32_values[2]) {
                        f32_value.val1 = 0 - f32_value.val1;
                    }
                    return lwm2m_engine_set_float32(path, &f32_value);
                case VAL_F64:
                    f64_value.val1 = (((int64_t) u32_values[0]) << 32) + (int64_t) u32_values[1];
                    f64_value.val2 = (((int64_t) u32_values[2]) << 32) + (int64_t) u32_values[3];
                    if (u32_values[4]) {
                        f64_value.val1 = 0 - f64_value.val1;
                    }
                    return lwm2m_engine_set_float64(path, &f64_value);
                default:
                    return -EINVAL;
            }
        }
    }
    else{
        return 0;
    }
}

static int handle_at_lwm2m_bool(enum at_cmd_type cmd_type) {
    return handle_at_lwm2m_value(cmd_type, VAL_BOOL);
}

static int handle_at_lwm2m_s8(enum at_cmd_type cmd_type) {
    return handle_at_lwm2m_value(cmd_type, VAL_S8);
}

static int handle_at_lwm2m_s16(enum at_cmd_type cmd_type) {
    return handle_at_lwm2m_value(cmd_type, VAL_S16);
}

static int handle_at_lwm2m_s32(enum at_cmd_type cmd_type) {
    return handle_at_lwm2m_value(cmd_type, VAL_S32);
}

static int handle_at_lwm2m_s64(enum at_cmd_type cmd_type) {
    return handle_at_lwm2m_value(cmd_type, VAL_S64);
}

static int handle_at_lwm2m_u8(enum at_cmd_type cmd_type) {
    return handle_at_lwm2m_value(cmd_type, VAL_U8);
}

static int handle_at_lwm2m_u16(enum at_cmd_type cmd_type) {
    return handle_at_lwm2m_value(cmd_type, VAL_U16);
}

static int handle_at_lwm2m_u32(enum at_cmd_type cmd_type) {
    return handle_at_lwm2m_value(cmd_type, VAL_U32);
}

static int handle_at_lwm2m_u64(enum at_cmd_type cmd_type) {
    return handle_at_lwm2m_value(cmd_type, VAL_U64);
}

static int handle_at_lwm2m_float32(enum at_cmd_type cmd_type) {
    return handle_at_lwm2m_value(cmd_type, VAL_F32);
}

static int handle_at_lwm2m_float64(enum at_cmd_type cmd_type) {
    return handle_at_lwm2m_value(cmd_type, VAL_F64);
}