#include <asf.h>
//#include "lcd.h"
#include "wireless.h"
#include "tasks.h"
#include "sys.h"
#include "conf_sio2host.h"

uint8_t* TEMP_QUEUE;
uint8_t* REGISTER_QUEUE;

#define HEAT "heat"
#define COOL "cool"

//globals
 //LCD
 char mode[5] = COOL;
 int roomSelection = 1;
 int targetTemp = 70;
 int roomTemp = 70;
 int ventStatus = "X";

static struct usart_module cdc_uart_module;

//tasks
static void lcd_task(void *params);

//functions
static void configure_console(void);
void updateDisplay();


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

//Tasks

static void lcd_task(void *params)
{
	updateDisplay();
	vTaskDelay(20);
}

//functions

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

void updateDisplay()
{
	char degree = 0xDF;
	//clear the display
	printf("                                ");
	//set cursor to beginning
	putchar(254);
	putchar(128);
	//update info
	printf("Mode:%s  Rm:%2dTarget:%2d%c %2d%c %s", mode, roomSelection, targetTemp, degree, roomTemp, degree, ventStatus);
}