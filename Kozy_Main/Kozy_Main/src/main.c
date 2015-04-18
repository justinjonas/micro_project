#include <asf.h>
#include "tasks.h"
#include "sys.h"
#include "conf_sio2host.h"
#include "config.h"
#include "nwk.h"
#include "phy.h"

//custom data structures
typedef struct wireless_packet
{
	uint8_t* data;
	uint8_t size;
	uint8_t dst_addr;
	uint8_t sender_addr;
};

typedef struct room
{
	uint8_t tempSensorAddress;
	uint8_t registerAddress;
	int roomNumber;
	int registerStatus;
	int temp;
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
char ventStatus = 'X';

//temp data
int numberOfRooms = 0;
//struct room rooms[30];

static struct usart_module cdc_uart_module;
static NWK_DataReq_t appDataReq;

//semaphores
static xSemaphoreHandle wireless_mutex;

//Queues
xQueueHandle TEMP_QUEUE;
xQueueHandle REGISTER_QUEUE;

//tasks
static void lcd_task(void *params);
static void new_sensor_task(void *params);
static void wireless_refresh(void *params);
static void analyze_temp_data(void *params);

//! Handle for about screen task
static xTaskHandle wireless_task_handle;


//functions
static void configure_console(void);
void updateDisplay(void);
void wireless_init(void);
void send_packet(struct wireless_packet packet);	//Sends data based on the struct passed in with packet
static bool appDataInd(NWK_DataInd_t *ind);			//Callback function when a packet is received
void send_packet_conf(NWK_DataReq_t *req);			//Callback function for a confirmed sent packet
static void configure_eic_callback(void);			//button press stuff
static void extint_callback(void);					//button press stuff
static void configure_extint(void);					//button press stuff

int main (void)
{
	irq_initialize_vectors();
	system_init();
	delay_init();
	//board_init();
	//wireless_sys_init();
	SYS_Init();
	wireless_init();
	configure_console();
	configure_extint();
	configure_eic_callback();
	system_interrupt_enable_global();
	TEMP_QUEUE = xQueueCreate( 15, sizeof(struct wireless_packet) );
	REGISTER_QUEUE = xQueueCreate( 15, sizeof(struct wireless_packet) );
	taskENABLE_INTERRUPTS();
	cpu_irq_enable();

	
	xTaskCreate(lcd_task,
		(const char *)"LCD",
		512,
		NULL,
		2,
		NULL);

	xTaskCreate(analyze_temp_data,
		(const char *)"Analyze Temp Data",
		1024,
		NULL,
		1,
		NULL);

	 xTaskCreate(new_sensor_task,
	 	(const char *)"new sensor",
	 	512,
	 	NULL,
	 	1,
	 	NULL);

	xTaskCreate(wireless_refresh,
		(const char *)"Wireless Refresh",
		512,
		NULL,
		1,
		NULL);
	
	//vTaskStartScheduler();
	
	struct wireless_packet dataToSend;
	
	dataToSend.data = 1;
	dataToSend.dst_addr = REGISTER_ADDR;
	dataToSend.sender_addr = APP_ADDR;
	dataToSend.size = sizeof(char);
	send_packet(dataToSend);
	
	while(1){
		SYS_TaskHandler();
			
	}

}

//Tasks

static void lcd_task(void *params)
{
	const uint16_t xDelay = 250;
	while(1)
	{
		updateDisplay();
		
		vTaskDelay(xDelay);
	}
}

static void analyze_temp_data(void *params)
{
	//period
	const uint16_t xDelay = 500;
	
	struct wireless_packet packet_received;

	while(1)
	{
		if(xQueueReceive(TEMP_QUEUE, &packet_received, 100))
			roomTemp = packet_received.data;
			
		//debugging
		roomSelection++;
		roomSelection%=99;
		
		/* Block for xDelay ms */
		vTaskDelay(xDelay);
	}
}

static void new_sensor_task(void *params)
{
	//this task will periodically run and add any new temperature sensors that get connected
	const uint16_t xDelay = 1000;

	while(1)
	{
		vTaskDelay(xDelay);
	}
}

static void wireless_refresh(void *params)
{
	const uint16_t xDelay = 10;

	while(1)
	{
		SYS_TaskHandler();	//needs to run as often as possible for wireless stuffs
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

void updateDisplay(void)
{
	//clear the display
	//set cursor to beginning
	putchar(254);
	putchar(128);
	//update display
	printf("Mode:%s  Rm:%2dTarget:%2d%c %2d%c %c", mode, roomSelection, targetTemp, degree, roomTemp, degree, ventStatus);
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
	//NWK_OpenEndpoint(APP_ENDPOINT, appDataInd);
}

//We will need to sent the struct of the payload and know where to send it to
// TODO: Change the endpoint, destination addr, and the data payload to send
void send_packet(struct wireless_packet packet)
{
	//NWK_DataReq_t appDataReq;

	appDataReq.dstAddr = packet.dst_addr;
	appDataReq.dstEndpoint = packet.dst_addr;
	appDataReq.srcEndpoint = APP_ENDPOINT;
	appDataReq.options = NWK_OPT_ENABLE_SECURITY;
	appDataReq.data = packet.data;
	appDataReq.size = packet.size;
	appDataReq.confirm = send_packet_conf;
	NWK_DataReq(&appDataReq);

}


////When a packet is received, parse the data into the correct queues
//static bool appDataInd(NWK_DataInd_t *ind)
//{	
	//printf("received!");
	//switch(ind->srcAddr)
	//{
		//case TEMP_ADDR:
			////memcpy(ind->data,TEMP_QUEUE, ind->size);
			////xQueueSendToBackFromISR(TEMP_QUEUE, ind->data, NULL);
			//break;
		//case REGISTER_ADDR:
			////memcpy(ind->data,REGISTER_QUEUE, ind->size);
			////xQueueSendToBackFromISR(REGISTER_QUEUE, ind->data, NULL);
			//break;
		//default:
			//return false;
			////break;
			////Call a function to add a new temp sensor and register to the network		
	//}
	//return true;
//}


void send_packet_conf(NWK_DataReq_t *req)
{
	if (NWK_SUCCESS_STATUS == req->status){
		LED_Toggle(LED0);
		//release the semaphore ;
	}
}

/** Configures the External Interrupt Controller to detect changes in the board
 *  button state.
 */
static void configure_extint(void)
{
	struct extint_chan_conf eint_chan_conf;
	extint_chan_get_config_defaults(&eint_chan_conf);

	eint_chan_conf.gpio_pin           = BUTTON_0_EIC_PIN;
	eint_chan_conf.gpio_pin_mux       = BUTTON_0_EIC_MUX;
	eint_chan_conf.detection_criteria = EXTINT_DETECT_BOTH;
	eint_chan_conf.filter_input_signal = true;
	extint_chan_set_config(BUTTON_0_EIC_LINE, &eint_chan_conf);
}

/** Callback function for the EXTINT driver, called when an external interrupt
 *  detection occurs.
 */
static void extint_callback(void)
{
	roomTemp++;
	//struct wireless_packet dataToSend;
	//
	//dataToSend.data = 1;
	//dataToSend.dst_addr = REGISTER_ADDR;
	//dataToSend.sender_addr = APP_ADDR;
	//dataToSend.size = sizeof(char);
	//send_packet(dataToSend);
	printf("button");
}

/** Configures and registers the External Interrupt callback function with the
 *  driver.
 */
static void configure_eic_callback(void)
{
	extint_register_callback(extint_callback,
			BUTTON_0_EIC_LINE,
			EXTINT_CALLBACK_TYPE_DETECT);
	extint_chan_enable_callback(BUTTON_0_EIC_LINE,
			EXTINT_CALLBACK_TYPE_DETECT);
}