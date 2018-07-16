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

#include <stdlib.h>
#include <string.h>

#include <libopencm3/cm3/cortex.h>

#include <libopencm3/sam/d/pm.h>
#include <libopencm3/sam/d/uart.h>
#include <libopencm3/sam/d/port.h>
#include <libopencm3/sam/d/gclk.h>
#include <libopencm3/sam/d/nvic.h>

#include <libopencm3/ushell/printf.h>
#include <libopencm3/ushell/ushell.h>

#include "queue.h"

#define USART_NUM	0
#define Q_SIZE		1024

/* input and output ring buffer */
struct {
        char buf[Q_SIZE];
        volatile size_t head, tail;
} rx, tx;

static struct gclk_hw clock = {
	.gclk0 = SRC_DFLL48M,
	.gclk1 = SRC_OSC8M,
	/* clock 1 has 8 divider, clock should be over 1khz for 1ms timer */
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

static void usart_setup(void)
{
	/* enable gpios */
	gpio_config_special(PORTA, GPIO8, SOC_GPIO_PERIPH_C); /* tx pin */
	gpio_config_special(PORTA, GPIO9, SOC_GPIO_PERIPH_C); /* rx pin */

	/* enable clocking to sercom3 */
	set_periph_clk(GCLK0, GCLK_ID_SERCOM0_CORE);
	periph_clk_en(GCLK_ID_SERCOM0_CORE, 1);
}

/* non blocking getc function */
static int usart_getc(void)
{
        int c = EOF;

        if(!qempty(rx.head, rx.tail)) {
		cm_disable_interrupts();
                c = rx.buf[rx.tail];
                rx.tail = qinc(rx.tail, Q_SIZE);
		cm_enable_interrupts();
        }

	return c;
}

/* non blocking putc function */
static void usart_putc(char c)
{
#ifndef CONSOLE_NO_AUTO_CRLF
	if (c == '\n')
		usart_putc('\r');
#endif

	if (qfull(tx.head, tx.tail, Q_SIZE))
		return;

	cm_disable_interrupts();
	tx.buf[tx.head] = c;
	tx.head = qinc(tx.head, Q_SIZE);
	cm_enable_interrupts();

	/* kick the transmitter to restart interrupts */
	usart_enable_tx_interrupt(USART_NUM);
}

/* setup our callbacks so printf and our shell works correctly */
stdout = &usart_putc;
stdin  = &usart_getc;

int main(void)
{
	gclk_init(&clock);
	usart_setup();
	usart_enable(USART_NUM, 115200);
	usart_enable_rx_interrupt(USART_NUM);
	usart_enable_tx_interrupt(USART_NUM);

	while (1) {
		/* non blocking shell loop */
		cmd_loop();
	}

}

/************************** UART Interrupt Handlers *************************/
static void uart_rx_irq(void)
{
	char c = UART(USART_NUM)->data;
		
	/* bug?, need to re-enable rx complete interrupt */
	INSERTBF(UART_INTENSET_RXC, 1, UART(USART_NUM)->intenset);

	if (!qfull(rx.head, rx.tail, Q_SIZE)) {
		rx.buf[rx.head] = c;
		rx.head = qinc(rx.head, Q_SIZE);
	}
}

static void uart_tx_irq(void)
{
	if (!qempty(tx.head, tx.tail)) {
		usart_send(USART_NUM, tx.buf[tx.tail]);
		tx.tail = qinc(tx.tail, Q_SIZE);
	} else {
		usart_disable_tx_interrupt(USART_NUM);
	}
}

void sercom0_isr(void)
{
	if (GETBF(UART_INTFLAG_RXC, UART(USART_NUM)->intflag))
		uart_rx_irq();

	if (GETBF(UART_INTFLAG_DRE, UART(USART_NUM)->intflag))
		uart_tx_irq();
}

static int version_handler(int argc, char **argv)
{
	printf("  Platform:   SAMD21\n");
	printf("  Version:\n");
	printf("    SCM:    1.0.0\n");

	if (0 == argc)
		return 0;

	if (strcmp(argv[0], "--v") == 0) {
		printf("    Branch: %s\n", BRANCH);
		printf("    HASH:   %s\n", SCMVERSION);
	} 

	return 0;
}

DECLARE_SHELL_COMMAND(version, version_handler,
	"Print the version",
	"usage: version [--v]");
