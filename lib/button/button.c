//
// Created by yplam on 13/1/2021.
//

#include "button.h"
#include <zephyr.h>
#include <devicetree.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(button, LOG_LEVEL_INF);

#include <nrfx_gpiote.h>


#define BTN_NODE	DT_ALIAS(btnapp)
#define BTN_PORT	DT_GPIO_LABEL(BTN_NODE, gpios)
#define BTN_GPIO_PIN	DT_GPIO_PIN(BTN_NODE, gpios)

static struct k_delayed_work confirm_btn_state_work;

enum button_state {
    BTN_INIT,
    BTN_FIRST_PRESS,
    BTN_SINGLE_CLICK,
    BTN_SECOND_PRESS,
    BTN_DOUBLE_CLICK,
};

static enum button_state btn_state = BTN_INIT;
static int64_t updatetime=0;
static bool btn_value = 0U;
static app_button_event_cb_t on_single_click = NULL;
static app_button_event_cb_t on_double_click = NULL;
static app_button_event_cb_t on_long_press = NULL;
static app_button_event_cb_t on_click_and_press = NULL;
static nrfx_gpiote_pin_t btn_pin;

void button_state_change_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    bool val = !nrfx_gpiote_in_is_set(btn_pin);
    int curtime = k_uptime_get();
    if(curtime - updatetime < 10){
        return;
    }
    if(val == btn_value){
        return;
    }
    LOG_INF("sta %d", val);
    btn_value = val;
    updatetime = curtime;
    switch(btn_state) {
        case BTN_INIT:
            if(val){
                btn_state = BTN_FIRST_PRESS;
                // 检测长按
                (void)k_delayed_work_submit(&confirm_btn_state_work, K_MSEC(3000));
            }
            break;
        case BTN_FIRST_PRESS:
            if(!val){
                btn_state = BTN_SINGLE_CLICK;
                // 释放后如果没有再次点击，则确认
                (void)k_delayed_work_submit(&confirm_btn_state_work, K_MSEC(500));
            }
            break;
        case BTN_SINGLE_CLICK:
            if(val) {
                btn_state = BTN_SECOND_PRESS;
                (void)k_delayed_work_submit(&confirm_btn_state_work, K_MSEC(3000));
            }
            break;
        case BTN_SECOND_PRESS:
            if(!val) {
                btn_state = BTN_DOUBLE_CLICK;
                // 释放后如果没有再次点击，则确认
                (void)k_delayed_work_submit(&confirm_btn_state_work, K_MSEC(600));
            }
            break;
        default:
            break;
    }
}

static void confirm_btn_state_work_handler(struct k_work *work) {
    LOG_INF("state: %d", btn_state);
    switch (btn_state) {
        case BTN_SINGLE_CLICK: {
            if(on_single_click){
                on_single_click();
            }
            break;
        }
        case BTN_FIRST_PRESS: {
            if(on_long_press){
                on_long_press();
            }
            break;
        }
        case BTN_DOUBLE_CLICK: {
            if(on_double_click) {
                on_double_click();
            }
            break;
        }
        case BTN_SECOND_PRESS: {
            if(on_click_and_press) {
                on_click_and_press();
            }
            break;
        }
        default:
            break;
    }
    btn_state = BTN_INIT;
}

void app_button_init(app_button_event_cb_t single_click_cb, app_button_event_cb_t double_click_cb,
                     app_button_event_cb_t long_press_cb, app_button_event_cb_t click_and_press_cb) {
    nrfx_err_t err;
    if(strcmp(BTN_PORT, "GPIO_1") ==0){
        btn_pin = BTN_GPIO_PIN + 32;
    }
    else {
        btn_pin = BTN_GPIO_PIN;
    }
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
    err = nrfx_gpiote_in_init(btn_pin, &in_config, button_state_change_handler);
    if (err != NRFX_SUCCESS) {
        LOG_ERR("nrfx_gpiote_in_init error: %d, %08x", btn_pin, err);
        return;
    }
    else {
        LOG_INF("GPIO INIT OK: %d", btn_pin);
    }
    nrfx_gpiote_in_event_enable(btn_pin, true);
    k_delayed_work_init(&confirm_btn_state_work, confirm_btn_state_work_handler);

    on_single_click = single_click_cb;
    on_double_click = double_click_cb;
    on_long_press = long_press_cb;
    on_click_and_press = click_and_press_cb;
}