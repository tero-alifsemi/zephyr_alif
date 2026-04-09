/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 2000
#define TIMEOUT_MS    10

// define the number of output and input gpio pins.
// OUT_CNT -> number of toggling output pins, IN_CNT -> number of input pins with interrupt
// configured.
#define OUT_CNT 4
#define IN_CNT  4

#define BLINK_WHILE_WORKING 0

#define ADC_SAMPLE_INTERVAL_MS 1

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct device *gpio14 = DEVICE_DT_GET(DT_NODELABEL(gpio14));
static uint32_t counter_out = 0;
static uint32_t counter_in = 0;
static uint32_t counter_adc = 0;
static uint32_t buffer[1];
static uint32_t average = 0;

// declarations
void adc_reader(void);

K_THREAD_DEFINE(adc_reader_thread, 1024, adc_reader, NULL, NULL, NULL, 5, 0, 0);

LOG_MODULE_REGISTER(main, LOG_LEVEL_ERR);

__maybe_unused static int set_led()
{
	int ret;

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	return 1;
}

static void gpio_interrupt_handler(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	LOG_DBG("Interrupt");
	counter_in++;
}

static void timer_callback(struct k_timer *timer)
{
	static int pin = 0;

	gpio_pin_set(gpio14, pin, !(gpio_pin_get(gpio14, pin)));

	pin++;
	if (pin >= OUT_CNT) {
		pin = 0;
	}

	counter_out++;
}

static int set_gpio()
{
	int ret;
	int i;
	static struct gpio_callback gpio_cb_data[IN_CNT];

	if (!device_is_ready(gpio14)) {
		printk("gpio14 device not ready\n");
		return 0;
	}

	// pin 0 - 3 as output
	for (i = 0; i < OUT_CNT; i++) {
		ret = gpio_pin_configure(gpio14, i, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			printk("Failed to configure pin %d as output\n", i);
			return 0;
		}
	}

	// pin 4 - 7 as input with interrupt
	for (i = 4; i < 4 + IN_CNT; i++) {
		ret = gpio_pin_configure(gpio14, i, GPIO_INPUT);
		if (ret < 0) {
			printk("Failed to configure pin %d as input\n", i);
			return 0;
		}

		ret = gpio_pin_interrupt_configure(gpio14, i, GPIO_INT_EDGE_BOTH);
		if (ret < 0) {
			printk("Failed to configure interrupt on pin %d\n", i);
			return 0;
		}

		gpio_init_callback(&gpio_cb_data[i - 4], gpio_interrupt_handler, BIT(i));
		ret = gpio_add_callback(gpio14, &gpio_cb_data[i - 4]);
		if (ret < 0) {
			printk("Failed to add callback for pin %d\n", i);
			return 0;
		}
	}

	return 1;
}

__maybe_unused enum adc_action adc_cb(const struct device *dev, const struct adc_sequence *sequence,
				      uint16_t sampling_index)
{
	// LOG_DBG("ADC callback called with sampling index %d", sampling_index);
	counter_adc++;
	average = average + (buffer[0] - average) / counter_adc;
	return ADC_ACTION_REPEAT;
}

static void check_peripheral_clk_force_states()
{
	uint32_t reg_value = sys_read32(0x4902F000);
	printf("initial value bit31: %d bit30: %d\n", (reg_value >> 31) & 0x1,
	       (reg_value >> 30) & 0x1);
	sys_write32(reg_value | (1U << 31) | (1U << 30), 0x4902F000);
}

void adc_reader(void)
{
	printk("Starting ADC reader thread\n");

	int ret;
	const struct device *adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc0));

	if (!device_is_ready(adc_dev)) {
		printf("adc device not ready\n");
		return;
	}

	struct adc_sequence_options adc_seq_options = {
		.callback = adc_cb,
		.interval_us = ADC_SAMPLE_INTERVAL_MS * 1000, // 20ms
	};

	struct adc_sequence sequence = {
		.options = &adc_seq_options,
		.buffer = (void *)buffer,
		.buffer_size = sizeof(buffer),
		.channels = (1 << 6),
	};

	struct adc_channel_cfg channel_cfg = {
		.differential = 0,
		.channel_id = 6,
	};

	/* Set the channel */
	ret = adc_channel_setup(adc_dev, &channel_cfg);
	if (ret) {
		printf("Unable set up channel\n");
		return;
	}

	adc_read(adc_dev, &sequence);

	// Never reaching this point, adc_read is blocking call. Async not supported.
	while (1) {
		k_msleep(20000);
	}
}

int main(void)
{
	int ret;
	k_timeout_t timeout = K_MSEC(TIMEOUT_MS);
	static struct k_timer timer;

	LOG_DBG("Hello from main");

#if BLINK_WHILE_WORKING
	if (!set_led()) {
		return 0;
	}
#endif
	// Set gpio
	if (!set_gpio()) {
		return 0;
	}

	// Start ADC reader thread
	k_thread_start(adc_reader_thread);

	// check peripheral clock force states and set both to enabled
	check_peripheral_clk_force_states();

	// set timer to generate interrupt to toggle output pins
	k_timer_init(&timer, timer_callback, NULL);
	k_timer_start(&timer, timeout, timeout);

	while (1) {
#if BLINK_WHILE_WORKING
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return 0;
		}
#endif
		printf("gpio out: %d, interrupts: %d adc reads: %d\n", counter_out, counter_in,
		       counter_adc);

		if (counter_in != counter_out) {
			LOG_ERR("Mismatch between output toggles and input interrupts!");
		}

		counter_out = 0;
		counter_in = 0;
		counter_adc = 0;
		average = 0;

		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
