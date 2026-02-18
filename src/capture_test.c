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

#define NUM_OF_CAPTURES 5

#define TIMER_CHANNEL 1

/*
 * for this to work short PC0 to PA0 and PC1 to PA1
 * Pin CN8 1 to Pin CN5 2 and Pin CN8 2 to Pin CN5 1
 */
static const struct gpio_dt_spec capture_tester_gpios[2] = {
	GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), capture_tester_gpios, 0),
	GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), capture_tester_gpios, 1),
};

const struct device *timer_dev = DEVICE_DT_GET(DT_NODELABEL(capture));

K_SEM_DEFINE(capture_sem, 0, 1);

/***************************************************************
 * ISR capture callback
 ***************************************************************/

void capture_cb(const struct device *dev, uint8_t chan, counter_capture_flags_t flags,
		uint32_t ticks, void *user_data)
{
	TC_PRINT("Capture callback: channel %d, flags %d, ticks %u, %lluus\n", chan, flags, ticks,
		 counter_ticks_to_us(dev, ticks));

	k_sem_give(&capture_sem);
}

/***************************************************************
 * Helper cases
 ***************************************************************/

static int counter_capture_test_rising_edge_capture(void)
{
	int ret;

	/* Check if GPIO pin is already high, as it needs to be low initially */
	if (gpio_pin_get_dt(&capture_tester_gpios[1]) == 1) {
		ret = gpio_pin_set_dt(&capture_tester_gpios[1], 0);
		zassert_ok(ret, "failed to set GPIO pin");
		/* give it some settling time */
		k_msleep(20);
	}

	/* Set GPIO pin to high to trigger rising edge */
	ret = gpio_pin_set_dt(&capture_tester_gpios[1], 1);
	zassert_ok(ret, "failed to set GPIO pin");

	ret = k_sem_take(&capture_sem, K_SECONDS(1));
	if (ret != 0) {
		return ret;
	}

	/* Check GPIO pin state */
	ret = gpio_pin_get_dt(&capture_tester_gpios[1]);
	if (ret == 0) {
		zassert_equal(ret, 1, "GPIO pin state does not match expected value of high");
	} else if (ret != 1) {
		zassert_equal(ret, 0, "failed to get GPIO pin state");
	}

	return ret;
}

static int counter_capture_test_falling_edge_capture(void)
{
	int ret;

	/* Check if GPIO pin is already low, as it needs to be high initially */
	if (gpio_pin_get_dt(&capture_tester_gpios[1]) == 0) {
		ret = gpio_pin_set_dt(&capture_tester_gpios[1], 1);
		zassert_ok(ret, "failed to set GPIO pin");
		/* give it some settling time */
		k_msleep(20);
	}

	/* Set GPIO pin to low to trigger falling edge */
	ret = gpio_pin_set_dt(&capture_tester_gpios[1], 0);
	zassert_ok(ret, "failed to set GPIO pin");

	ret = k_sem_take(&capture_sem, K_SECONDS(1));
	if (ret != 0) {
		return ret;
	}

	/* Check GPIO pin state, expected to be low */
	ret = gpio_pin_get_dt(&capture_tester_gpios[1]);
	if (ret == 1) {
		zassert_equal(ret, 0, "GPIO pin state does not match expected value of low");
	} else if (ret != 0) {
		zassert_equal(ret, 0, "failed to get GPIO pin state");
	}

	return ret;
}

/***************************************************************
 * Test cases
 ***************************************************************/

ZTEST(counter_capture, test_rising_edge_continous_capture)
{
	int ret;

	ret = counter_capture_callback_set(timer_dev, TIMER_CHANNEL,
					   COUNTER_CAPTURE_RISING_EDGE | COUNTER_CAPTURE_CONTINUOUS,
					   capture_cb, NULL);
	if (ret == -ENOTSUP) {
		TC_PRINT("rising edge capture type not supported\n");
		ztest_test_skip();
	} else {
		zassert_ok(ret, "failed to set capture callback");
	}

	ret = counter_enable_capture(timer_dev, TIMER_CHANNEL);
	zassert_ok(ret, "failed to enable capture");

	for (uint8_t i = 0; i < NUM_OF_CAPTURES; i++) {
		/* Set GPIO pin to high to trigger rising edge */
		ret = counter_capture_test_rising_edge_capture();
		zassert_ok(ret, "rising edge capture test failed");
	}
}

ZTEST(counter_capture, test_rising_edge_single_capture)
{
	int ret;

	ret = counter_capture_callback_set(
		timer_dev, TIMER_CHANNEL, COUNTER_CAPTURE_RISING_EDGE | COUNTER_CAPTURE_SINGLE_SHOT,
		capture_cb, NULL);
	if (ret == -ENOTSUP) {
		TC_PRINT("rising edge capture type not supported\n");
		ztest_test_skip();
	} else {
		zassert_ok(ret, "failed to set capture callback");
	}

	ret = counter_enable_capture(timer_dev, TIMER_CHANNEL);
	zassert_ok(ret, "failed to enable capture");

	/* Set GPIO pin to high to trigger rising edge */
	ret = counter_capture_test_rising_edge_capture();
	zassert_ok(ret, "rising edge capture test failed");

	/* Verify that capture callback is not called again after single shot capture */
	ret = counter_capture_test_rising_edge_capture();
	zassert_equal(ret, -ETIMEDOUT,
		      "capture callback should not be called after single shot capture");
}

ZTEST(counter_capture, test_falling_edge_continous_capture)
{
	int ret;

	ret = counter_capture_callback_set(
		timer_dev, TIMER_CHANNEL, COUNTER_CAPTURE_FALLING_EDGE | COUNTER_CAPTURE_CONTINUOUS,
		capture_cb, NULL);
	if (ret == -ENOTSUP) {
		TC_PRINT("falling edge capture type not supported\n");
		ztest_test_skip();
	} else {
		zassert_ok(ret, "failed to set capture callback");
	}

	ret = counter_enable_capture(timer_dev, TIMER_CHANNEL);
	zassert_ok(ret, "failed to enable capture");

	for (uint8_t i = 0; i < NUM_OF_CAPTURES; i++) {
		/* Set GPIO pin to low to trigger falling edge */
		ret = counter_capture_test_falling_edge_capture();
		zassert_ok(ret, "falling edge capture test failed");
	}
}

ZTEST(counter_capture, test_falling_edge_single_capture)
{
	int ret;

	ret = counter_capture_callback_set(
		timer_dev, TIMER_CHANNEL,
		COUNTER_CAPTURE_FALLING_EDGE | COUNTER_CAPTURE_SINGLE_SHOT, capture_cb, NULL);
	if (ret == -ENOTSUP) {
		TC_PRINT("falling edge capture type not supported\n");
		ztest_test_skip();
	} else {
		zassert_ok(ret, "failed to set capture callback");
	}

	ret = counter_enable_capture(timer_dev, TIMER_CHANNEL);
	zassert_ok(ret, "failed to enable capture");

	/* Set GPIO pin to low to trigger falling edge */
	ret = counter_capture_test_falling_edge_capture();
	zassert_ok(ret, "falling edge capture test failed");

	/* Verify that capture callback is not called again after single shot capture */
	ret = counter_capture_test_falling_edge_capture();
	zassert_equal(ret, -ETIMEDOUT,
		      "capture callback should not be called after single shot capture");
}

ZTEST(counter_capture, test_both_edges_continous_capture)
{
	int ret;

	ret = counter_capture_callback_set(timer_dev, TIMER_CHANNEL,
					   COUNTER_CAPTURE_BOTH_EDGES | COUNTER_CAPTURE_CONTINUOUS,
					   capture_cb, NULL);
	if (ret == -ENOTSUP) {
		TC_PRINT("both edges capture type not supported\n");
		ztest_test_skip();
	} else {
		zassert_ok(ret, "failed to set capture callback");
	}

	ret = counter_enable_capture(timer_dev, TIMER_CHANNEL);
	zassert_ok(ret, "failed to enable capture");

	for (uint8_t i = 0; i < NUM_OF_CAPTURES; i++) {
		/* if gpio is already low, then test rising edge first, otherwise falling edge first
		 */
		if (gpio_pin_get_dt(&capture_tester_gpios[1]) == 0) {
			/* Set GPIO pin to high to trigger rising edge */
			ret = counter_capture_test_rising_edge_capture();
			zassert_ok(ret, "rising edge capture test failed");

			/* Set GPIO pin to low to trigger falling edge */
			ret = counter_capture_test_falling_edge_capture();
			zassert_ok(ret, "falling edge capture test failed");
		} else {
			/* Set GPIO pin to low to trigger falling edge */
			ret = counter_capture_test_falling_edge_capture();
			zassert_ok(ret, "falling edge capture test failed");

			/* Set GPIO pin to high to trigger rising edge */
			ret = counter_capture_test_rising_edge_capture();
			zassert_ok(ret, "rising edge capture test failed");
		}
	}
}

ZTEST(counter_capture, test_both_edges_single_capture)
{
	int ret;

	ret = counter_capture_callback_set(timer_dev, TIMER_CHANNEL,
					   COUNTER_CAPTURE_BOTH_EDGES | COUNTER_CAPTURE_SINGLE_SHOT,
					   capture_cb, NULL);
	if (ret == -ENOTSUP) {
		TC_PRINT("both edges capture type not supported\n");
		ztest_test_skip();
	} else {
		zassert_ok(ret, "failed to set capture callback");
	}

	ret = counter_enable_capture(timer_dev, TIMER_CHANNEL);
	zassert_ok(ret, "failed to enable capture");

	/* Set GPIO pin to high to trigger rising edge */
	ret = counter_capture_test_rising_edge_capture();
	zassert_ok(ret, "rising edge capture test failed");

	/* Verify that capture callback is not called again after single shot capture */
	ret = counter_capture_test_falling_edge_capture();
	zassert_equal(ret, -ETIMEDOUT,
		      "capture callback should not be called after single shot capture");
}

static void counter_before(void* f)
{
	ARG_UNUSED(f);

	counter_reset(timer_dev);
	counter_start(timer_dev);
}

static void counter_after(void* f)
{
	ARG_UNUSED(f);

	counter_disable_capture(timer_dev, TIMER_CHANNEL);
	counter_stop(timer_dev);
}

static void *counter_setup(void)
{
	int ret;

	zassert_true(device_is_ready(timer_dev), "counter device is not ready");
	zassert_true(gpio_is_ready_dt(&capture_tester_gpios[0]),
		     "capture tester GPIO device is not ready");
	zassert_true(gpio_is_ready_dt(&capture_tester_gpios[1]),
		     "capture tester GPIO device is not ready");

	ret = gpio_pin_configure_dt(&capture_tester_gpios[0], GPIO_OUTPUT_LOW);
	zassert_ok(ret, "failed to configure GPIO pin");

	ret = gpio_pin_configure_dt(&capture_tester_gpios[1], GPIO_OUTPUT_LOW);
	zassert_ok(ret, "failed to configure GPIO pin");

	return NULL;
}

ZTEST_SUITE(counter_capture, NULL, counter_setup, counter_before, counter_after, NULL);
