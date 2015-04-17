#include <asf.h>
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
char degree = 0xDF;
char mode[5] = COOL;
int roomSelection = 1;
int targetTemp = 70;
int roomTemp = 70;
int ventStatus = "X";

static struct usart_module cdc_uart_module;

//semaphores
static xSemaphoreHandle wireless_mutex;
static xSemaphoreHandle temp_queue_mutex;
static xSemaphoreHandle new_sensor_queue_mutex;

//tasks
static void lcd_task(void *params);
static void test_task(void *params);

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

	xTaskCreate(test_task,
		(const char *)"test",
		1024,
		NULL,
		1,
		NULL);
	
	vTaskStartScheduler();
	
	while(1){}

}

//Tasks

static void lcd_task(void *params)
{
	uint16_t xLastWakeTime = xTaskGetTickCount();
	//period
	const uint16_t xWakePeriod = 1000;

	while(1)
	{
		updateDisplay();

		/* Block until xWakePeriod ticks since previous call */
        //vTaskDelayUntil(&xLastWakeTime, xWakePeriod);
		vTaskDelay(100);
	}
}

static void test_task(void *params)
{
	//period
	const uint16_t xDelay = 1000;
	
	roomTemp = 0;

	while(1)
	{
		roomTemp++;
		roomTemp%=99;

		/* Block for xDelay ms */
		vTaskDelay(xDelay);
	}
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
	//clear the display
	//printf("                                ");
	//set cursor to beginning
	putchar(254);
	putchar(128);
	//update display
	printf("Mode:%s  Rm:%2dTarget:%2d%c %2d%c %s", mode, roomSelection, targetTemp, degree, roomTemp, degree, ventStatus);
}