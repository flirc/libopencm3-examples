/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
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

#include <string.h>
#include <libopencm3/sam/d/gclk.h>
#include <libopencm3/sam/d/port.h>
#include <libopencm3/sam/d/nvmctrl.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/dfu.h>

#include <libopencm3/sam/d/nvic.h>
#include <libopencm3/sam/d/pm.h>
#include <libopencm3/sam/d/bitfield.h>
#include <libopencm3/sam/d/usb.h>

//#define APP_ADDRESS	0x08002000
//#define APP_ADDRESS	0x00002000
#define APP_ADDRESS	0x00002000
//#define APP_ADDRESS	0x00004000

/* Commands sent with wBlockNum == 0 as per ST implementation. */
#define CMD_SETADDR	0x21
#define CMD_ERASE	0x41

#define BUTTON_PORT PORTA
#define BUTTON_PIN GPIO27

#define BUF_SIZE 4096

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

/* We need a special large control buffer for this device: */
uint8_t usbd_control_buffer[BUF_SIZE];

static enum dfu_state usbdfu_state = STATE_DFU_IDLE;

static struct {
	uint8_t buf[sizeof(usbd_control_buffer)];
	uint16_t len;
	uint32_t addr;
	uint16_t blocknum;
} prog;

const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x1D50,
	.idProduct = 0x6017,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

const struct usb_dfu_descriptor dfu_function = {
	.bLength = sizeof(struct usb_dfu_descriptor),
	.bDescriptorType = DFU_FUNCTIONAL,
	.bmAttributes = USB_DFU_CAN_DOWNLOAD | USB_DFU_WILL_DETACH,
	.wDetachTimeout = 255,
	.wTransferSize = BUF_SIZE,
	.bcdDFUVersion = 0x011A,
};

const struct usb_interface_descriptor iface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 0,
	.bInterfaceClass = 0xFE, /* Device Firmware Upgrade */
	.bInterfaceSubClass = 1,
	.bInterfaceProtocol = 2,

	/* The ST Microelectronics DfuSe application needs this string.
	 * The format isn't documented... */
	.iInterface = 4,

	.extra = &dfu_function,
	.extralen = sizeof(dfu_function),
};

const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = &iface,
}};

const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0xC0,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char *usb_strings[] = {
	"Black Sphere Technologies",
	"DFU Demo",
	"DEMO",
	/* This string is used by ST Microelectronics' DfuSe utility. */
	//"@Internal Flash   /0x08000000/8*001Ka,56*001Kg",
	"@Internal Flash   /0x00000000/1*008Ka,15*008Kg",
	//"@Internal Flash   /0x00000000/1*0016Ka,15*0016Kg",
};

static uint8_t usbdfu_getstatus(usbd_device *usbd_dev, uint32_t *bwPollTimeout)
{
	(void)usbd_dev;

	switch (usbdfu_state) {
	case STATE_DFU_DNLOAD_SYNC:
		usbdfu_state = STATE_DFU_DNBUSY;
		*bwPollTimeout = 100;
		return DFU_STATUS_OK;
	case STATE_DFU_MANIFEST_SYNC:
		/* Device will reset when read is complete. */
		usbdfu_state = STATE_DFU_MANIFEST;
		return DFU_STATUS_OK;
	default:
		return DFU_STATUS_OK;
	}
}

static void usbdfu_getstatus_complete(usbd_device *usbd_dev, struct usb_setup_data *req)
{
	int i;
	(void)req;
	(void)usbd_dev;

	switch (usbdfu_state) {
	case STATE_DFU_DNBUSY:
		//flash_unlock();
		if (prog.blocknum == 0) {
			switch (prog.buf[0]) {
			case CMD_ERASE:
				{
					uint32_t *dat = (uint32_t *)(prog.buf + 1);
					nvmctrl_erase_row(*dat); //flash_erase_page(*dat);
				}
			case CMD_SETADDR:
				{
					uint32_t *dat = (uint32_t *)(prog.buf + 1);
					prog.addr = *dat;
				}
			}
		} else {
			//uint32_t baseaddr = prog.addr + ((prog.blocknum - 2) *
			//	       dfu_function.wTransferSize);
			uint32_t baseaddr = prog.addr;
			//for (i = 0; i < prog.len; i += 2) {
			        //uint16_t *dat = (uint16_t *)(prog.buf + i);
				//flash_program_half_word(baseaddr + i,
				//		*dat);
			for (i = 0; i < BUF_SIZE; i += 256){
				nvmctrl_erase_row(baseaddr+i);
				nvmctrl_write_row(baseaddr+i, prog.buf+i);
			}
				//}
		}
		//flash_lock();

		/* Jump straight to dfuDNLOAD-IDLE, skipping dfuDNLOAD-SYNC. */
		usbdfu_state = STATE_DFU_DNLOAD_IDLE;
		return;
	case STATE_DFU_MANIFEST:
		/* reset USB */
		INSERTBF(USB_CTRLA_SWRST, 1, USB->ctrla);
		/* jump to app */
		if ((*(volatile uint32_t *)APP_ADDRESS & 0x2FFE0000) == 0x20000000) {

			/* Set vector table base address. */
			//SCB_VTOR = APP_ADDRESS & 0xFFFF;
			SCB_VTOR = APP_ADDRESS;
			/* Initialise master stack pointer. */
			asm volatile("msr msp, %0"::"g"
				     (*(volatile uint32_t *)APP_ADDRESS));
			/* Jump to application. */
			(*(void (**)())(APP_ADDRESS + 4))();
		}

		//scb_reset_system();
		return; /* Will never return. */
	default:
		return;
	}
}

static int usbdfu_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
		uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
	if ((req->bmRequestType & 0x7F) != 0x21)
		return 0; /* Only accept class request. */

	switch (req->bRequest) {
	case DFU_DNLOAD:
		if ((len == NULL) || (*len == 0)) {
			usbdfu_state = STATE_DFU_MANIFEST_SYNC;
			return 1;
		} else {
			/* Copy download data for use on GET_STATUS. */
			prog.blocknum = req->wValue;
			prog.len = *len;
			memcpy(prog.buf, *buf, *len);
			usbdfu_state = STATE_DFU_DNLOAD_SYNC;
			return 1;
		}
	case DFU_CLRSTATUS:
		/* Clear error and return to dfuIDLE. */
		if (usbdfu_state == STATE_DFU_ERROR)
			usbdfu_state = STATE_DFU_IDLE;
		return 1;
	case DFU_ABORT:
		/* Abort returns to dfuIDLE state. */
		usbdfu_state = STATE_DFU_IDLE;
		return 1;
	case DFU_UPLOAD:
		/* Upload not supported for now. */
		return 0;
	case DFU_GETSTATUS: {
		uint32_t bwPollTimeout = 0; /* 24-bit integer in DFU class spec */
		(*buf)[0] = usbdfu_getstatus(usbd_dev, &bwPollTimeout);
		(*buf)[1] = bwPollTimeout & 0xFF;
		(*buf)[2] = (bwPollTimeout >> 8) & 0xFF;
		(*buf)[3] = (bwPollTimeout >> 16) & 0xFF;
		(*buf)[4] = usbdfu_state;
		(*buf)[5] = 0; /* iString not used here */
		*len = 6;
		*complete = usbdfu_getstatus_complete;
		return 1;
		}
	case DFU_GETSTATE:
		/* Return state with no state transision. */
		*buf[0] = usbdfu_state;
		*len = 1;
		return 1;
	}

	return 0;
}

static void usbdfu_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
	(void)wValue;

	usbd_register_control_callback(
				usbd_dev,
				USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				usbdfu_control_request);
}

static void usb_setup(void)
{
        /* Enable USB */
        INSERTBF(PM_APBBMASK_USB, 1, PM->apbbmask);

        /* enable clocking to usb */
        set_periph_clk(GCLK0, GCLK_ID_USB);
        periph_clk_en(GCLK_ID_USB, 1);

        gpio_config_special(PORTA, GPIO24, SOC_GPIO_PERIPH_G);
        gpio_config_special(PORTA, GPIO25, SOC_GPIO_PERIPH_G);

}

int main(void)
{
	usbd_device *usbd_dev;

	gclk_init(&clock);
	//rcc_periph_clock_enable(RCC_GPIOA);
	//gpio_config_input(BUTTON_PORT,BUTTON_PIN,GPIO_IN_FLAG_PULLUP);
	gpio_config_input(BUTTON_PORT,BUTTON_PIN,0);

	nvmctrl_init(0,0);

	usb_setup();

	if (PM->rcause != (1<<6))
	if (gpio_get(BUTTON_PORT, BUTTON_PIN)) {
	//if (gpio_get(PORTA, GPIO27)) {
		/* Boot the application if it's valid. */
		if ((*(volatile uint32_t *)APP_ADDRESS & 0x2FFE0000) == 0x20000000) {

			/* Set vector table base address. */
			//SCB_VTOR = APP_ADDRESS & 0xFFFF;
			SCB_VTOR = APP_ADDRESS;
			/* Initialise master stack pointer. */
			asm volatile("msr msp, %0"::"g"
				     (*(volatile uint32_t *)APP_ADDRESS));
			/* Jump to application. */
			(*(void (**)())(APP_ADDRESS + 4))();
		}
	}

	usbd_dev = usbd_init(&samd21_usb_driver, &dev, &config, usb_strings, 4, usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(usbd_dev, usbdfu_set_config);

	//nvic_enable_irq(NVIC_USB_IRQ);

        /* Connect USB cable */
        usbd_disconnect(usbd_dev, false);

	//gpio_clear(GPIOC, GPIO11);

	while (1)
		usbd_poll(usbd_dev);
}
