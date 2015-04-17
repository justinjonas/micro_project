#include <asf.h>
//#include "lcd.h"
#include "wireless.h"
#include "tasks.h"
#include "sys.h"
#include "conf_sio2host.h"

uint8_t* TEMP_QUEUE;
uint8_t* REGISTER_QUEUE;

static struct usart_module cdc_uart_module;

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


static void configure_console(void)
{
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

int main (void)
{
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

}
