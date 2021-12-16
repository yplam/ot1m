#include "app_settings.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(app_settings, LOG_LEVEL_INF);
#include "settings/settings.h"

/**
 * 配置读取上下文定义
 */
struct app_setting_read_ctx {
    /* 数据缓冲区指针 */
    uint8_t *value;

    /* 缓冲区长度，或者读到的数据长度 */
    uint16_t *length;

    /* 操作结果 */
    int status;
};

/**
 * setting 子系统初始化
 */
void app_init_settings(void) {
    int err;
    err = settings_subsys_init();
    if (err) {
        LOG_ERR("settings subsys initialization: fail (err %d)\n", err);
        return;
    }
}

static int ot_setting_read_cb(const char *key, size_t len,
                              settings_read_cb read_cb, void *cb_arg,
                              void *param) {
    int ret;
    struct app_setting_read_ctx *ctx = (struct app_setting_read_ctx *) param;

    if ((ctx->value == NULL) || (ctx->length == NULL)) {
        goto out;
    }

    if (*(ctx->length) < len) {
        len = *(ctx->length);
    }

    ret = read_cb(cb_arg, ctx->value, len);
    if (ret <= 0) {
        LOG_ERR("Failed to read the setting, ret: %d", ret);
        ctx->status = -EIO;
        return 1;
    }

out:
    if (ctx->length != NULL) {
        *(ctx->length) = len;
    }
    ctx->status = 0;

    return 1;
}

/**
 * 读取 key 的配置项，存到 value 指向的缓冲区，缓冲区大小以 value_length 指定
 * @param key
 * @param value
 * @param value_length 输入缓冲区大小，返回读到的数据长度
 * @return
 */
int app_settings_get(enum app_settings_key key, uint8_t *value, uint16_t *value_length) {
    int ret;
    char path[16];
    struct app_setting_read_ctx read_ctx = {
            .value = value,
            .length = (uint16_t *) value_length,
            .status = -ENOENT,
    };

    ret = snprintk(path, sizeof(path), "app/%x", (uint16_t) key);
    __ASSERT(ret < sizeof(path), "Setting path buffer too small.");

    ret = settings_load_subtree_direct(path, ot_setting_read_cb, &read_ctx);
    if (ret != 0) {
        LOG_ERR("Failed to load app setting key %d, ret %d",
                key, ret);
    }

    if (read_ctx.status != 0) {
        LOG_INF("key %u not found", key);
        return -ENOENT;
    }

    return 0;
}

/**
 * 将 value 指向的长度为 value_lenght 的内容写入配置的 key
 * @param key
 * @param value
 * @param value_length
 * @return
 */
int app_settings_set(enum app_settings_key key, uint8_t *value, uint16_t value_length) {
    int ret;
    char path[16];
    ret = snprintk(path, sizeof(path), "app/%x", (uint16_t) key);
    __ASSERT(ret < sizeof(path), "Setting path buffer too small.");

    ret = settings_save_one(path, value, value_length);
    if (ret != 0) {
        LOG_ERR("Failed to store setting %d, ret %d", key, ret);
        return -ENOSPC;
    }
    return 0;
}