//
// Created by yplam on 26/11/2020.
//

#include "app_at_modem.h"

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(at_modem, LOG_LEVEL_INF);

#include <drivers/uart.h>
#include <sys/ring_buffer.h>
#include <ctype.h>
#include <net/net_event.h>
#include <nrfx_gpiote.h>
#include <modem/at_params.h>
#include "at_modem/at_system.h"
#include "at_modem/at_openthread.h"
#if CONFIG_APP_COAP
#include "at_modem/at_coap.h"
#endif
#if CONFIG_APP_LWM2M
#include "at_modem/at_lwm2m.h"
#endif

K_KERNEL_STACK_DEFINE(app_at_modem_stack_area, 512);
static struct k_thread app_at_modem_thread;

#ifdef CONFIG_PRINTK

extern void __printk_hook_install(int (*fn)(int));

#endif

#define AT_CMD_PARSER_SIZE 4
static at_cmd_parser_t at_cmd_parsers[AT_CMD_PARSER_SIZE] = {
        at_openthread_parser,
#if CONFIG_APP_COAP
        at_coap_parser,
#endif
#if CONFIG_APP_LWM2M
        at_lwm2m_parser,
#endif
        at_modem_system_parser
};
#define OK_STR        "OK\r\n"
#define ERROR_STR    "ERROR\r\n"
#define FATAL_STR    "FATAL ERROR\r\n"
#define AT_SYNC_STR    "Ready\r\n"

static const struct device *at_dev;
struct at_param_list at_param_list; // 外部可以访问
static uint8_t at_buf[CONFIG_APP_AT_CMD_MAX_LEN]; // 外部可以访问
char rsp_buf[CONFIG_APP_AT_RESPONSE_MAX_LEN];
static size_t at_buf_len;
static struct k_work at_cmd_work;
static struct k_delayed_work turn_off_uart_work;
static bool ate=true;

uint8_t tx_buff[1024];
struct ring_buf tx_ring_buf;
K_SEM_DEFINE(tx_sem, 0, 1);

volatile uint32_t uart_power_state;

#define EN_INPUT_PIN    DT_GPIO_PIN(DT_ALIAS(btnen), gpios)
#define CONFIG_AT_SLEEP_DELAY 100

void app_at_set_ate(bool new_ate){
    ate = new_ate;
}

static bool is_enable(void) {
    if(IS_ENABLED(CONFIG_APP_MODEM_EN_PIN)){
        return (!nrfx_gpiote_in_is_set(EN_INPUT_PIN));
    }
    return true;
}

static void disable_at_console(void) {
    if (is_enable()) {
        return;
    }
    if (uart_power_state == DEVICE_PM_ACTIVE_STATE) {
        if (ring_buf_is_empty(&tx_ring_buf)) {
            LOG_INF("at off");
            uart_power_state = DEVICE_PM_SUSPEND_STATE;
            (void)device_set_power_state(at_dev, DEVICE_PM_SUSPEND_STATE, NULL, NULL);
        } else {
            (void)k_delayed_work_submit(&turn_off_uart_work, K_MSEC(CONFIG_AT_SLEEP_DELAY));
        }
    }
}

static void turn_off_uart_work_handler(struct k_work *work) {
    disable_at_console();
}

static void enable_at_console(void) {
    if (uart_power_state != DEVICE_PM_ACTIVE_STATE) {
        LOG_INF("at on");
        uart_power_state = DEVICE_PM_ACTIVE_STATE;
        (void)device_set_power_state(at_dev, DEVICE_PM_ACTIVE_STATE, NULL, NULL);
        uart_irq_rx_enable(at_dev);
        (void)k_delayed_work_submit(&turn_off_uart_work, K_MSEC(CONFIG_AT_SLEEP_DELAY));
    }
}


/**
 * 系统 printk 调用
 * @param c
 * @return
 */
static int at_console_out(int c) {
    (void)ring_buf_put(&tx_ring_buf, (uint8_t *) &c, 1);
    k_sem_give(&tx_sem);
    return c;
}

void app_at_modem_rsp_send(const uint8_t *str, size_t len) {
    (void)ring_buf_put(&tx_ring_buf, str, len);
    k_sem_give(&tx_sem);
}

bool app_at_modem_cmd_casecmp(const char *cmd_buf, const char *cmd_target) {
    int i;
    int cmd_target_len = strlen(cmd_target);

    if (strlen(cmd_buf) < cmd_target_len) {
        return false;
    }

    for (i = 0; i < cmd_target_len; i++) {
        if (toupper((int) *(cmd_buf + i)) != toupper((int) *(cmd_target + i))) {
            return false;
        }
    }
//    if (strlen(cmd_buf) > (cmd_target_len + 2)) {
    if (strlen(cmd_buf) > (cmd_target_len + 1)) {
        char ch = *(cmd_buf + i);
        /* With parameter, SET TEST, "="; READ, "?" */
        return ((ch == '=') || (ch == '?'));
    }

    return true;
}


static void at_data_handler(char character) {
    static bool inside_quotes = false;
    static size_t cmd_len = 0;
    size_t pos;

    cmd_len += 1;
    pos = cmd_len - 1;
//    LOG_INF("%c", character);
    /* Handle special characters. */
    switch (character) {
        case 0x08: /* Backspace. */
            /* Fall through. */
        case 0x7F: /* DEL character */
            pos = pos ? pos - 1 : 0;
            at_buf[pos] = 0;
            cmd_len = (cmd_len <= 1) ? 0 : (cmd_len - 2);
            break;
        case '"':
            inside_quotes = !inside_quotes;
            /* Fall through. */
        default:
            /* Detect AT command buffer overflow or zero length */
            if (cmd_len >= CONFIG_APP_AT_CMD_MAX_LEN) {
                LOG_ERR("Buffer overflow, dropping '%c'\n", character);
                cmd_len = CONFIG_APP_AT_CMD_MAX_LEN - 1;
                return;
            } else if (cmd_len < 1) {
                LOG_ERR("Invalid AT command length: %d", cmd_len);
                cmd_len = 0;
                return;
            }

            at_buf[pos] = character;
            break;
    }
    if(ate){
        printk("%c", character);
    }
    if (inside_quotes) {
        return;
    }

//    LOG_INF("n %d, %d, %d", pos, at_buf[pos - 1], character);
    /** in minicom use Enter */
//    if ((at_buf[pos - 1] == '\r') && (character == '\n')) {
    if (at_buf[pos] == '\r') {
        LOG_INF("New cmd");
        if(ate){
            printk("\n");
        }
        uart_irq_rx_disable(at_dev);
        k_work_submit(&at_cmd_work);
        at_buf_len = cmd_len;
        cmd_len = 0;
    }
}

/**
 * 串口中断处理函数
 * 将接收数据保存到ringbuffer，等待线程处理
 * 如果有数据待发送，则写入串口fifo
 * @param dev
 */

static void uart_interrupt_handler(const struct device *dev, void *user_data) {
    int rx_tx_len;
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);
    (void)uart_irq_update(at_dev);

    if (uart_irq_tx_ready(at_dev)) {
        uint8_t tx_char;
        size_t len_to_send = 0;
        len_to_send = ring_buf_get(&tx_ring_buf, &tx_char, 1);
        if (!len_to_send) {
            uart_irq_tx_disable(at_dev);
        } else {
            rx_tx_len = uart_fifo_fill(at_dev, &tx_char, len_to_send);
            if (rx_tx_len < len_to_send) {
                LOG_ERR("TX ERROR");
            }
        }
    }

    if (uart_irq_rx_ready(at_dev)) {
        char rx_char;
        while (uart_fifo_read(at_dev, &rx_char, 1)) {
            at_data_handler(rx_char);
        }
    }

}


/**
 * handle at command
 */
static void at_cmd_handler(struct k_work *work) {
    int err;
    ARG_UNUSED(work);

    /* Make sure the string is 0-terminated */
    at_buf[MIN(at_buf_len, CONFIG_APP_AT_CMD_MAX_LEN - 1)] = 0;
//    LOG_HEXDUMP_INF(at_buf, at_buf_len, "RX");
//    k_msleep(10);
    for (int i = 0; i < AT_CMD_PARSER_SIZE; i++) {
        if (!at_cmd_parsers[i]) {
            break;
        }
        err = at_cmd_parsers[i](at_buf);
        if (err == 0) {
            app_at_modem_rsp_send(OK_STR, sizeof(OK_STR) - 1);
            goto done;
        } else if (err != -ENOTSUP) {
            app_at_modem_rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
            LOG_ERR("ERR: %d", err);
            goto done;
        }
    }
    // empty
    app_at_modem_rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
    done:
    uart_irq_rx_enable(at_dev);
}

static void en_pin_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
    LOG_INF("GPIO input event callback");
    if (!is_enable()) {
        disable_at_console();
    } else {
        enable_at_console();
    }
}

/**
 * 初始化GPIO，设置EN引脚中断响应
 */
static void app_at_modem_init_gpio(void) {
    nrfx_err_t err;

    nrfx_gpiote_in_config_t const in_config = {
            .sense = NRF_GPIOTE_POLARITY_TOGGLE,
            .pull = NRF_GPIO_PIN_PULLUP,
            .is_watcher = false,
            .hi_accuracy = false,
            .skip_gpio_setup = false,
    };

    /* Initialize input pin to generate event on high to low transition
     * (falling edge) and call button_handler()
     */
    err = nrfx_gpiote_in_init(EN_INPUT_PIN, &in_config, en_pin_handler);
    if (err != NRFX_SUCCESS) {
        LOG_ERR("nrfx_gpiote_in_init error: %08x", err);
        return;
    }
}

static void app_at_modem_init_uart(void) {
    int err;
    err = at_params_list_init(&at_param_list, CONFIG_APP_AT_MAX_PARAM);
    if (err) {
        LOG_ERR("Failed to init AT Parser: %d", err);
        return;
    }
    /* Initialize AT Parser */
    ring_buf_init(&tx_ring_buf, sizeof(tx_buff), tx_buff);
    k_work_init(&at_cmd_work, at_cmd_handler);
    k_delayed_work_init(&turn_off_uart_work, turn_off_uart_work_handler);

    at_dev = device_get_binding(CONFIG_APP_AT_DEV_NAME);
    if (!at_dev) {
        LOG_ERR("AT CMD device not found");
        return;
    }
    uart_irq_rx_disable(at_dev);
    uart_irq_tx_disable(at_dev);
    uart_irq_callback_set(at_dev, uart_interrupt_handler);
#ifdef CONFIG_PRINTK
    __printk_hook_install(at_console_out);
#endif
}

static void app_at_modem_process(void *arg1, void *arg2, void *arg3) {
    while (true) {
        k_sem_take(&tx_sem, K_FOREVER);
        enable_at_console();
        uart_irq_tx_enable(at_dev);
    }
}

void app_at_modem_init(void) {
    LOG_INF("Starting...");
    app_at_modem_init_gpio();
    app_at_modem_init_uart();

    if (is_enable()) {
        uart_power_state = DEVICE_PM_ACTIVE_STATE;
        (void)device_set_power_state(at_dev, DEVICE_PM_ACTIVE_STATE, NULL, NULL);
        uart_irq_rx_enable(at_dev);
    } else {
        uart_power_state = DEVICE_PM_SUSPEND_STATE;
        (void)device_set_power_state(at_dev, DEVICE_PM_SUSPEND_STATE, NULL, NULL);
    }
    nrfx_gpiote_in_event_enable(EN_INPUT_PIN, true);

    k_thread_create(&app_at_modem_thread, app_at_modem_stack_area,
                    K_KERNEL_STACK_SIZEOF(app_at_modem_stack_area),
                    app_at_modem_process,
                    NULL, NULL, NULL,
                    K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);
    k_thread_name_set(&app_at_modem_thread, "app_at");
}
