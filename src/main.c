/*
 * Copyright (c) 2024 Ryan McClelland
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/*
 * for this to work short PC0 to PA0 and PC1 to PA1
 * Pin CN8 1 to Pin CN5 2 and Pin CN8 2 to Pin CN5 1
 */
static const struct gpio_dt_spec capture_tester_gpios[2] = {
	GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), capture_tester_gpios, 0),
	GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), capture_tester_gpios, 1),
};

void capture_cb(const struct device *dev, uint8_t chan,
		counter_capture_flags_t flags, uint32_t ticks, void *user_data)
{
	counter_reset(dev);
	printk("Capture callback: channel %d, flags %d, ticks %u, %lluus\n",
		chan, flags, ticks, counter_ticks_to_us(dev, ticks));
}

int main(void)
{
	int ret;
	bool led_state = true;

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}
#ifdef CONFIG_COUNTER
	const struct device *timer_dev = DEVICE_DT_GET(DT_NODELABEL(capture));

	if (!device_is_ready(timer_dev)) {
		printk("Timer device not ready\n");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&capture_tester_gpios[0], GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		printk("Failed to configure capture tester pin\n");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&capture_tester_gpios[1], GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		printk("Failed to configure capture tester1 pin\n");
		return -ENODEV;
	}

	printk("Capture enabling on channel 0, freq=%uHz\n",
		counter_get_frequency(timer_dev));
	counter_start(timer_dev);
	counter_capture_callback_set(timer_dev, 0, COUNTER_CAPTURE_BOTH_EDGES,
				      capture_cb, NULL);
	counter_capture_callback_set(timer_dev, 1, COUNTER_CAPTURE_RISING_EDGE,
				      capture_cb, NULL);
	counter_capture_enable(timer_dev, 0);
	counter_capture_enable(timer_dev, 1);
	printk("Capture enabled on channel 0\n");
#endif
	while (1) {
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return 0;
		}

		led_state = !led_state;
		// printf("LED state: %s\n", led_state ? "ON" : "OFF");
		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}
