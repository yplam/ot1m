//
// Created by yplam on 12/1/2021.
//
#include <zephyr.h>
#include <init.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/pwm.h>
#include <drivers/gpio.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(led, LOG_LEVEL_INF);

#define PWM_LED0_NODE	DT_ALIAS(pwm_led0)
#define PWM_LABEL	DT_PWMS_LABEL(PWM_LED0_NODE)
#define PWM_CHANNEL	DT_PWMS_CHANNEL(PWM_LED0_NODE)
#define PWM_FLAGS	DT_PWMS_FLAGS(PWM_LED0_NODE)

K_THREAD_STACK_DEFINE(led_stack, 1024);
struct k_thread led_thread;

#define PERIOD_USEC	20000U
#define NUM_STEPS	50U
#define STEP_USEC	(PERIOD_USEC / NUM_STEPS)
#define SLEEP_MSEC	25U

static void led_process(void *arg1, void *arg2, void *arg3) {
    const struct device *pwm;
    uint32_t pulse_width = 0U;
    uint8_t dir = 1U;
    int ret;

    LOG_INF("starting pwm");
    pwm = device_get_binding(PWM_LABEL);
    if (!pwm) {
        LOG_ERR("no pwm");
        return;
    }
    while (1) {
        ret = pwm_pin_set_usec(pwm, PWM_CHANNEL, PERIOD_USEC,
                               pulse_width, PWM_FLAGS);
        if (ret) {
            return;
        }
        if (dir) {
            pulse_width += STEP_USEC;
            if (pulse_width >= PERIOD_USEC) {
                pulse_width = PERIOD_USEC - STEP_USEC;
                dir = 0U;
            }
        } else {
            if (pulse_width >= STEP_USEC) {
                pulse_width -= STEP_USEC;
            } else {
                pulse_width = STEP_USEC;
                dir = 1U;
            }
        }

        k_sleep(K_MSEC(SLEEP_MSEC));
    }
}

static int led_startup(const struct device *dev) {
    k_thread_create(&led_thread, led_stack,
                    K_KERNEL_STACK_SIZEOF(led_stack),
                    led_process,
                    NULL, NULL, NULL,
                    K_HIGHEST_APPLICATION_THREAD_PRIO, 0, K_MSEC(1000));
    return 0;
}

SYS_INIT(led_startup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
