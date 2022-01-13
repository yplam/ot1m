//
// Created by yplam on 14/12/2021.
//

#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(bat, LOG_LEVEL_INF);
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include "bat.h"

#include <drivers/adc.h>
#include <hal/nrf_saadc.h>

#define ADC_DEVICE_NAME		DT_LABEL(DT_NODELABEL(adc))
#define ADC_RESOLUTION		12
#define ADC_OVERSAMPLING	4 /* 2^ADC_OVERSAMPLING samples are averaged */
#define ADC_MAX 		4096
#define ADC_GAIN		ADC_GAIN_1_5
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_REF_INTERNAL_MV	600UL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define ADC_CHANNEL_ID		0
#define ADC_CHANNEL_INPUT	NRF_SAADC_INPUT_AIN4

static const struct device *gpio0_dev, *gpio1_dev, *adc_dev;

static int16_t channel_0_data;  // This will hold the adc result

bool bat_is_charge(void) {
    return !gpio_pin_get(gpio1_dev, 11);
}

bool bat_is_standby(void) {
    return !gpio_pin_get(gpio1_dev, 10);
}

int16_t get_bat_val(void) {
    return channel_0_data;
}

static int init_adc(void)
{
    adc_dev = device_get_binding(ADC_DEVICE_NAME);
    if (!adc_dev) {
        LOG_ERR("Cannot get ADC device");
        return -ENXIO;
    }

    static const struct adc_channel_cfg channel_cfg = {
            .gain             = ADC_GAIN,
            .reference        = ADC_REFERENCE,
            .acquisition_time = ADC_ACQUISITION_TIME,
            .channel_id       = ADC_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
            .input_positive   = ADC_CHANNEL_INPUT,
#endif
    };
    int err = adc_channel_setup(adc_dev, &channel_cfg);
    if (err) {
        LOG_ERR("Setting up the ADC channel failed");
        return err;
    }

    return 0;
}


int bat_val_read(void)
{
    int ret;
    struct adc_sequence sequence = {
            /* This is a bitmask that tells the driver which channels to convert : bit n = 1 for channel n */
            .channels    = (1 << 0),
            /* Where will the data be stored (could be an array if there are multiple channels to convert */
            .buffer      = &channel_0_data,
            /* buffer size in bytes, not number of samples */
            .buffer_size = sizeof(channel_0_data),
            /* 12 bit resolution */
            .resolution  = ADC_RESOLUTION,
            /* nulls for the rest of the fields */
            .options = NULL,
            .calibrate = 0,
            .oversampling = ADC_OVERSAMPLING,
    };
    ret = adc_read(adc_dev, &sequence);
    return channel_0_data;
}
void bat_init(void) {
    gpio0_dev = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));
    if (!gpio0_dev) {
        LOG_ERR("Cannot get GPIO device");
    }

    gpio1_dev = device_get_binding(DT_LABEL(DT_NODELABEL(gpio1)));
    if (!gpio1_dev) {
        LOG_ERR("Cannot get GPI1 device");
    }

    int err = gpio_pin_configure(gpio1_dev,
                                 11,
                                 GPIO_INPUT | GPIO_PULL_UP);

    if (err) {
        LOG_ERR("Cannot configure charge pin");
    }

    err = gpio_pin_configure(gpio1_dev,
                                 10,
                                 GPIO_INPUT | GPIO_PULL_UP);

    if (err) {
        LOG_ERR("Cannot configure standby pin");
    }

    gpio_pin_configure(gpio0_dev,
                       3,
                       GPIO_OUTPUT);
    gpio_pin_set_raw(gpio0_dev,
                     3,
                     1);
    init_adc();
}