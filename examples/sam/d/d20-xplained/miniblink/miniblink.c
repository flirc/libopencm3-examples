/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2018  Flirc Inc.
 * Written by Jason Kotzin <jasonkotzin@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/sam/d/port.h>
#include <libopencm3/sam/d/gclk.h>

/* User LED */
//#define LEDPORT		PORTB
//#define LED		GPIO30
#define LEDPORT		PORTA
#define LED		GPIO10

static struct gclk_hw clock = {
	.gclk0 = SRC_DFLL48M,
	.gclk1 = SRC_OSC8M,
	/* clock 1 has 8 divider */
	.gclk1_div = 100,
	.gclk2 = SRC_DFLL48M,
	.gclk3 = SRC_DFLL48M,
	.gclk3_div = 1,
	.gclk4 = SRC_OSC8M,
	.gclk4_div = 1,
	.gclk5 = SRC_DFLL48M,
	.gclk6 = SRC_DFLL48M,
	.gclk7 = SRC_DFLL48M,
};

int main(void)
{
	int i;

	gclk_init(&clock);
	
	gpio_config_output(LEDPORT, LED, GPIO_OUT_FLAG_DEFAULT_HIGH);

	while (1) {
		gpio_toggle(LEDPORT, LED);
		for (i = 0; i < 2000000; i++) /* Wait a bit. */
			__asm__("nop");
	}

	return 0;
}
