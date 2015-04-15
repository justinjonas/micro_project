/*
 * lcd.c
 *
 * Created: 4/15/2015 2:11:41 PM
 *  Author: itcl
 */ 
#include "lcd.h"
#include "sio2host.h"
#include "stdio_serial.h"
#include "conf_sio2host.h"

void configure_console(void)
{
	static struct usart_module cdc_uart_module;
	struct usart_config usart_conf;

	usart_get_config_defaults(&usart_conf);
	usart_conf.mux_setting = HOST_SERCOM_MUX_SETTING;
	usart_conf.pinmux_pad0 = HOST_SERCOM_PINMUX_PAD0;
	usart_conf.pinmux_pad1 = HOST_SERCOM_PINMUX_PAD1;
	usart_conf.pinmux_pad2 = HOST_SERCOM_PINMUX_PAD2;
	usart_conf.pinmux_pad3 = HOST_SERCOM_PINMUX_PAD3;
	usart_conf.baudrate    = 9600;

	stdio_serial_init(&cdc_uart_module, USART_HOST, &usart_conf);
	usart_enable(&cdc_uart_module);
}

void update_display(char* mode_name, char* mode_type)
{
	//set cursor to start
	//putchar(254);
	//putchar(128);
	printf("Mode:");
	printf(mode_type);
}
