/*
 * Copyright (c) 2024 Ryan McClelland
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>

/*
 * for this to work short PC0 to PA0 and PC1 to PA1
 * Pin CN8 1 to Pin CN5 2 and Pin CN8 2 to Pin CN5 1
 */
static const struct gpio_dt_spec capture_tester_gpios[2] = {
	GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), capture_tester_gpios, 0),
	GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), capture_tester_gpios, 1),
};

const struct device *timer_dev = DEVICE_DT_GET(DT_NODELABEL(capture));

K_SEM_INIT(capture_sem, 0, 1);

void capture_cb(const struct device *dev, uint8_t chan,
		counter_capture_flags_t flags, uint32_t ticks, void *user_data)
{
	printk("Capture callback: channel %d, flags %d, ticks %u, %lluus\n",
		chan, flags, ticks, counter_ticks_to_us(dev, ticks));

	k_sem_give(&capture_sem);
}

ZTEST(counter_capture, test_rising_edge_capture)
{
	int ret;

	ret = counter_capture_callback_set(timer_dev, 1, COUNTER_CAPTURE_RISING_EDGE,
					   capture_cb, NULL);
	if (ret == -ENOTSUP) {
		TC_PRINT("rising edge capture type not supported\n");
		ztest_test_skip();
	} else {
		zassert_equal(ret, 0, "failed to set capture callback");
	}

	ret = counter_capture_enable(timer_dev, 1);
	zassert_equal(ret, 0, "failed to enable capture");

	/* Set GPIO pin to high to trigger rising edge */
	ret = gpio_pin_set_dt(&capture_tester_gpios[1], 1);
	zassert_equal(ret, 0, "failed to set GPIO pin");

	ret = k_sem_take(&capture_sem, K_SECONDS(1));
	zassert_equal(ret, 0, "capture callback not called");
}

ZTEST(counter_capture, test_falling_edge_capture)
{
	int ret;

	ret = counter_capture_callback_set(timer_dev, 1, COUNTER_CAPTURE_FALLING_EDGE,
					   capture_cb, NULL);
	if (ret == -ENOTSUP) {
		TC_PRINT("falling edge capture type not supported\n");
		ztest_test_skip();
	} else {
		zassert_equal(ret, 0, "failed to set capture callback");
	}

	ret = counter_capture_enable(timer_dev, 1);
	zassert_equal(ret, 0, "failed to enable capture");

	/* Set GPIO pin to low to trigger falling edge */
	ret = gpio_pin_set_dt(&capture_tester_gpios[1], 0);
	zassert_equal(ret, 0, "failed to set GPIO pin");

	ret = k_sem_take(&capture_sem, K_SECONDS(1));
	zassert_equal(ret, 0, "capture callback not called");
}

ZTEST(counter_capture, test_both_edges_capture)
{
	int ret;

	ret = counter_capture_callback_set(timer_dev, 1, COUNTER_CAPTURE_BOTH_EDGES,
					   capture_cb, NULL);
	if (ret == -ENOTSUP) {
		TC_PRINT("both edges capture type not supported\n");
		ztest_test_skip();
	} else {
		zassert_equal(ret, 0, "failed to set capture callback");
	}

	ret = counter_capture_enable(timer_dev, 1);
	zassert_equal(ret, 0, "failed to enable capture");

	/* Toggle GPIO pin to trigger both edges */
	for (int i = 0; i < 2; i++) {
		ret = gpio_pin_toggle_dt(&capture_tester_gpios[1]);
		zassert_equal(ret, 0, "failed to toggle GPIO pin");

		ret = k_sem_take(&capture_sem, K_SECONDS(1));
		zassert_equal(ret, 0, "capture callback not called");
	}
}

static void counter_before(void)
{
	int ret;

	counter_reset(timer_dev);
	counter_start(timer_dev);
}

static void counter_after(void)
{
	counter_stop(timer_dev);
}

static void counter_setup(void)
{
	int ret;

	zassert_true(device_is_ready(timer_dev), "counter device is not ready");

	ret = gpio_pin_configure_dt(&capture_tester_gpios[0], GPIO_OUTPUT_LOW);
	ret = gpio_pin_configure_dt(&capture_tester_gpios[1], GPIO_OUTPUT_LOW);
}

ZTEST_SUITE(counter_capture, NULL, counter_setup, counter_before, counter_after, NULL);