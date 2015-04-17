#include <asf.h>
#include "tasks.h"
#include "sys.h"
#include "conf_sio2host.h"
#include "config.h"
#include "nwk.h"
#include "phy.h"

typedef struct wireless_packet
{
	uint8_t* data;
	uint8_t size;
	uint8_t dst_addr;
	uint8_t sender_addr;
};

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

//Queues
xQueueHandle TEMP_QUEUE;
xQueueHandle REGISTER_QUEUE;

//tasks
static void lcd_task(void *params);
static void test_task(void *params);

//functions
static void configure_console(void);
void updateDisplay();
void wireless_init(void);
void send_packet(struct wireless_packet packet);	//Sends data based on the struct passed in with packet
bool receive_packet(NWK_DataInd_t *ind);			//Callback function when a packet is received
void send_packet_conf(NWK_DataReq_t *req);			//Callback function for a confirmed sent packet

int main (void)
{
	system_init();
	delay_init();
	//board_init();
	//wireless_sys_init();
	
	configure_console();
	TEMP_QUEUE = xQueueCreate( 15, sizeof(struct wireless_packet) );
	REGISTER_QUEUE = xQueueCreate( 15, sizeof(struct wireless_packet) );

	
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
	const uint16_t xDelay = 50;
	
	struct wireless_packet packet_received;

	while(1)
	{
		if(xQueueReceive(TEMP_QUEUE, &packet_received, 100))
			roomTemp = packet_received.data;
		
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

void wireless_init(void)
{
	NWK_SetAddr(APP_ADDR);
	NWK_SetPanId(APP_PANID);
	PHY_SetChannel(APP_CHANNEL);
	#ifdef PHY_AT86RF212
	PHY_SetBand(APP_BAND);
	PHY_SetModulation(APP_MODULATION);
	#endif
	PHY_SetRxState(true);
	PHY_SetTxPower(0x23);
	NWK_SetSecurityKey((uint8_t *)APP_SECURITY_KEY);
	NWK_OpenEndpoint(APP_ENDPOINT, receive_packet);
}

//We will need to sent the struct of the payload and know where to send it to
// TODO: Change the endpoint, destination addr, and the data payload to send
void send_packet(struct wireless_packet packet)
{
	NWK_DataReq_t appDataReq;

	appDataReq.dstAddr = packet.dst_addr;
	appDataReq.dstEndpoint = packet.dst_addr;
	appDataReq.srcEndpoint = APP_ENDPOINT;
	appDataReq.options = NWK_OPT_ENABLE_SECURITY;
	appDataReq.data = packet.data;
	appDataReq.size = packet.size;
	appDataReq.confirm = send_packet_conf;
	NWK_DataReq(&appDataReq);

}


//When a packet is received, parse the data into the correct queues
bool receive_packet(NWK_DataInd_t *ind)
{	
	switch(ind->srcAddr)
	{
		case TEMP_ADDR:
			//memcpy(ind->data,TEMP_QUEUE, ind->size);
			xQueueSendToBackFromISR(TEMP_QUEUE, ind->data, NULL);
			break;
		case REGISTER_ADDR:
			//memcpy(ind->data,REGISTER_QUEUE, ind->size);
			xQueueSendToBackFromISR(REGISTER_QUEUE, ind->data, NULL);
			break;
		default:
			return false;
			//break;
			//Call a function to add a new temp sensor and register to the network		
	}
	return true;
}


void send_packet_conf(NWK_DataReq_t *req)
{
	if (NWK_SUCCESS_STATUS == req->status){
		LED_Toggle(LED0);
		//release the semaphore ;
	}
}