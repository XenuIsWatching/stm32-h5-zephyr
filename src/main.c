/*
 * Copyright (c) 2024 Ryan McClelland
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>

void capture_cb(const struct device *dev, uint8_t chan,
		uint32_t flags, uint64_t ticks, void *user_data)
{
	printk("Capture callback: channel %d, flags %d, ticks %llu\n", chan, flags, ticks);
}

int main(void)
{
	const struct device *timer_dev = DEVICE_DT_GET(DT_NODELABEL(capture));

	counter_start(timer_dev);
	counter_capture_callback_set(timer_dev, 0, COUNTER_CAPTURE_RISING_EDGE,
				      capture_cb, NULL);
	counter_capture_enable(timer_dev, 0);
	printk("Capture enabled on channel 0\n");

	return 0;
}
