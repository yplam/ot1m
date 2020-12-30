/*
 * main 线程，进行公共的初始化操作，然后启动其他线程
 */

#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#ifdef CONFIG_NRFX_GPIOTE
#include <drivers/include/nrfx_gpiote.h>
#endif

#ifdef CONFIG_APP_AT_MODEM
#include "app_at_modem.h"
#endif

#ifdef CONFIG_APP_TESTING
#include "app_testing.h"
#endif

#include "app_settings.h"

K_SEM_DEFINE(main_forever, 0, 1);

static void app_init_gpio(void) {
    nrfx_err_t err;
    if (IS_ENABLED(CONFIG_NRFX_GPIOTE)) {
        /* Connect GPIOTE_0 IRQ to nrfx_gpiote_irq_handler */
        IRQ_CONNECT(DT_IRQN(DT_NODELABEL(gpiote)),
                    DT_IRQ(DT_NODELABEL(gpiote), priority),
                    nrfx_isr, nrfx_gpiote_irq_handler, 0);

        /* Initialize GPIOTE (the interrupt priority passed as the parameter
         * here is ignored, see nrfx_glue.h).
         */
        err = nrfx_gpiote_init(0);
        if (err != NRFX_SUCCESS) {
            LOG_ERR("nrfx_gpiote_init error: %08x", err);
        }
    }
}

void main(void) {
    LOG_INF("OT1M Starting...");
    app_init_settings();
    app_init_gpio();
    if (IS_ENABLED(CONFIG_APP_AT_MODEM)) {
        app_at_modem_init();
    }
#ifdef CONFIG_APP_TESTING
    app_testing();
#endif
    while (true) {
        k_sem_take(&main_forever, K_FOREVER);
    }
    LOG_ERR("NEVER RUN!");
}

