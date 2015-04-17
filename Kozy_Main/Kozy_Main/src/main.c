/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * This is a bare minimum user application template.
 *
 * For documentation of the board, go \ref group_common_boards "here" for a link
 * to the board-specific documentation.
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to system_init()
 * -# Basic usage of on-board LED and button
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */
#include <asf.h>
#include "lcd.h"
#include "wireless.h"
#include "tasks.h"
#include "sys.h"
#include "conf_sio2host.h"

uint8_t* TEMP_QUEUE;
uint8_t* REGISTER_QUEUE;

//static struct usart_module cdc_uart_module;

static void lcd_task(void *params);

static void lcd_task(void *params)
{
	while(1){
		putchar(128);
		putchar(256);
		printf("lcd_task");
		vTaskDelay(20);
	}
}


//static void configure_console(void)
//{
	//struct usart_config usart_conf;
//
	//usart_get_config_defaults(&usart_conf);
	//usart_conf.mux_setting = HOST_SERCOM_MUX_SETTING;
	//usart_conf.pinmux_pad0 = HOST_SERCOM_PINMUX_PAD0;
	//usart_conf.pinmux_pad1 = HOST_SERCOM_PINMUX_PAD1;
	//usart_conf.pinmux_pad2 = HOST_SERCOM_PINMUX_PAD2;
	//usart_conf.pinmux_pad3 = HOST_SERCOM_PINMUX_PAD3;
	//usart_conf.baudrate    = 9600;
//
	//stdio_serial_init(&cdc_uart_module, USART_HOST, &usart_conf);
	//usart_enable(&cdc_uart_module);
//}

int main (void)
{
	//struct usart_module cdc_uart_module;
	system_init();
	delay_init();
	//board_init();
	//wireless_sys_init();
	
	configure_console();
	puts("It works to here!");
	
	xTaskCreate(lcd_task,
		(const char *)"LCD",
		1024,
		NULL,
		2,
		NULL);
	
	vTaskStartScheduler();
	
	while(1){}
	
	/* This skeleton code simply sets the LED to the state of the button. */
	//while (1) {
		///* Is button pressed? */
		//if (port_pin_get_input_level(BUTTON_0_PIN) == BUTTON_0_ACTIVE) {
			///* Yes, so turn LED on. */
			//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
		//} else {
			///* No, so turn LED off. */
			//port_pin_set_output_level(LED_0_PIN, !LED_0_ACTIVE);
		//}
	//}
}
