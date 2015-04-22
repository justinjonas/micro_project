#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "sys.h"
#if SAMD || SAMR21
#include "system.h"
#else
#include "led.h"
#include "sysclk.h"
#endif
#include "phy.h"
#include "nwk.h"
#include "sysTimer.h"
#include "sio2host.h"
#include "asf.h"
#include "conf_sio2host.h"

//////////////////////////Our stuff///////////////////////////
#define HEAT "heat"
#define COOL "cool"

typedef struct wireless_packet
{
	uint8_t* data;
	uint8_t size;
	uint8_t dst_addr;
	uint8_t sender_addr;
};

void send_packet(struct wireless_packet myPacket);

typedef struct room
{
	uint8_t tempSensorAddress;
	uint8_t registerAddress;
	int roomNumber;
	char ventStatus;
	uint8_t temp;
};

//LCD
char degree = 0xDF;
char* mode = COOL;
uint8_t roomSelection = 1;
uint8_t targetTemp = 70;
uint8_t roomTemp = 0;
char ventStatus = 'X';

//rooms
int numberOfRooms = 0;
struct room rooms[30];

static void rooms_init(void)
{
	numberOfRooms = 2;
	rooms[0].registerAddress = 3;
	rooms[0].ventStatus = 'X';
	rooms[0].roomNumber = 1;
	rooms[0].temp = 0;
	rooms[0].tempSensorAddress = 2;
	rooms[1].registerAddress = 5;
	rooms[1].ventStatus = 'X';
	rooms[1].roomNumber = 2;
	rooms[1].temp = 68;
	rooms[1].tempSensorAddress = 4;
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

void cycleRooms(void)
{
	int n = roomSelection - 1;
	
	roomTemp = rooms[n].temp;
	ventStatus = rooms[n].ventStatus;
	roomSelection = rooms[n].roomNumber;

	if( roomSelection < numberOfRooms)
		roomSelection++;
	else
		roomSelection = 1;
}

void update_register_task(void)
{	
	struct wireless_packet myPacket;
	myPacket.sender_addr = APP_ADDR;
	myPacket.dst_addr = 3;	//register address
	myPacket.size = sizeof(uint8_t);
	
	for(int i = 0; i < numberOfRooms-1; i++)
	{
		if(!strcmp(mode,COOL))
		{
			if(rooms[i].temp > targetTemp && rooms[i].ventStatus == 'X')
			{
				rooms[i].ventStatus = 'O';
				myPacket.data = 1;	//open vent
				send_packet(myPacket);
			}else if(rooms[i].temp < targetTemp && rooms[i].ventStatus == 'O')
			{
				rooms[i].ventStatus = 'X';
				myPacket.data = 0;	//close vent
				send_packet(myPacket);
			}
		}
		else if(!strcmp(mode,HEAT))
		{
			if(rooms[i].temp < targetTemp && rooms[i].ventStatus == 'X')
			{
				rooms[i].ventStatus = 'O';
				myPacket.data = 1;	//open vent
				send_packet(myPacket);
			}else if(rooms[i].temp > targetTemp && rooms[i].ventStatus == 'O')
			{
				rooms[i].ventStatus = 'X';
				myPacket.data = 0;	//close vent
				send_packet(myPacket);
			}					
		}
	}
}

static void extint_callback(void)	//button press
{
	if(targetTemp > 85){
		targetTemp = 60;
		if(!strcmp(mode,HEAT))
			mode = COOL;
		else
			mode = HEAT;
	}
	else{
		targetTemp++;
	}
}

/** Configures the External Interrupt Controller to detect changes in the board
 *  button state.
 */
static void configure_extint(void)	//button press
{
	struct extint_chan_conf eint_chan_conf;
	extint_chan_get_config_defaults(&eint_chan_conf);

	eint_chan_conf.gpio_pin           = BUTTON_0_EIC_PIN;
	eint_chan_conf.gpio_pin_mux       = BUTTON_0_EIC_MUX;
	eint_chan_conf.detection_criteria = EXTINT_DETECT_BOTH;
	eint_chan_conf.filter_input_signal = true;
	extint_chan_set_config(BUTTON_0_EIC_LINE, &eint_chan_conf);
}

/** Configures and registers the External Interrupt callback function with the
 *  driver.
 */
static void configure_eic_callback(void)	//button press
{
	extint_register_callback(extint_callback,
			BUTTON_0_EIC_LINE,
			EXTINT_CALLBACK_TYPE_DETECT);
	extint_chan_enable_callback(BUTTON_0_EIC_LINE,
			EXTINT_CALLBACK_TYPE_DETECT);
}
//////////////////////////END OF OUR STUFF/////////////////////////

/*- Definitions ------------------------------------------------------------*/
#ifdef NWK_ENABLE_SECURITY
  #define APP_BUFFER_SIZE     (NWK_MAX_PAYLOAD_SIZE - NWK_SECURITY_MIC_SIZE)
#else
  #define APP_BUFFER_SIZE     NWK_MAX_PAYLOAD_SIZE
#endif

static uint8_t rx_data[APP_RX_BUF_SIZE] = {-1};
static struct usart_module cdc_uart_module;

/*- Types ------------------------------------------------------------------*/
typedef enum AppState_t {
	APP_STATE_INITIAL,
	APP_STATE_IDLE,
} AppState_t;

/*- Prototypes -------------------------------------------------------------*/
static void appSendData(void);
static void appInit(void);
static void APP_TaskHandler(void);

/*- Variables --------------------------------------------------------------*/
static AppState_t appState = APP_STATE_INITIAL;
static SYS_Timer_t appTimer;
static NWK_DataReq_t appDataReq;
static bool appDataReqBusy = false;
static uint8_t appDataReqBuffer[APP_BUFFER_SIZE];
static uint8_t appUartBuffer[APP_BUFFER_SIZE];
static uint8_t appUartBufferPtr = 0;

/*- Implementations --------------------------------------------------------*/

/*************************************************************************//**
*****************************************************************************/
static void appDataConf(NWK_DataReq_t *req)
{
	if (NWK_SUCCESS_STATUS == req->status){
		appDataReqBusy = false;
		LED_Toggle(LED0);
	}
}		

/*************************************************************************//**
*****************************************************************************/
static void appSendData(void)
{
	if (appDataReqBusy || 0 == appUartBufferPtr) {
		return;
	}

	memcpy(appDataReqBuffer, appUartBuffer, appUartBufferPtr);

	appDataReq.dstAddr = 3;
	appDataReq.dstEndpoint = 3;
	appDataReq.srcEndpoint = 1;
	appDataReq.options = NWK_OPT_ENABLE_SECURITY;
	appDataReq.data = appDataReqBuffer;
	appDataReq.size = appUartBufferPtr;
	appDataReq.confirm = appDataConf;
	NWK_DataReq(&appDataReq);

	appUartBufferPtr = 0;
	appDataReqBusy = true;
	//LED_Toggle(LED0);
}

/*************************************************************************//**
*****************************************************************************/
static void appTimerHandler(SYS_Timer_t *timer)
{
	appSendData();
	(void)timer;
}

/*************************************************************************//**
*****************************************************************************/
static void APP_TaskHandler(void)
{	
	if(!appDataReqBusy)
	{
		
		//appSendData();
	}
}

/*************************************************************************//**
*****************************************************************************/
static bool appDataInd(NWK_DataInd_t *ind)
{
	for (uint8_t i = 0; i < ind->size; i++)
	{
		rx_data[i] = ind->data[i];
	}
	
	for( int n = 0; n < numberOfRooms; n++ )
	{
		if( ind->srcAddr == rooms[n].tempSensorAddress )
			rooms[n].temp = rx_data[0];
	}
		
	for( int n = 0; n < APP_RX_BUF_SIZE; n++ )
		rx_data[n] = -1;
			
	return true;
}

/*************************************************************************//**
*****************************************************************************/
static void appInit(void)
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
	NWK_OpenEndpoint(APP_ENDPOINT, appDataInd);

	appTimer.interval = APP_FLUSH_TIMER_INTERVAL;
	appTimer.mode = SYS_TIMER_INTERVAL_MODE;
	appTimer.handler = appTimerHandler;
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

void send_packet(struct wireless_packet myPacket)
{
	char tx_data[1];

	if( myPacket.data == 1 )
		tx_data[0] = 1;
	else
		tx_data[0] = 0;

	if(!appDataReqBusy){
		for (uint16_t i = 0; i < sizeof(uint8_t); i++) {
			if (appUartBufferPtr == sizeof(appUartBuffer)) {
				appSendData();
			}

			if (appUartBufferPtr < sizeof(appUartBuffer)) {
				appUartBuffer[appUartBufferPtr++] = tx_data[i];
			}
		}
		appSendData();
	}

	//NWK_DataReq_t appDataReq;
//
	//appDataReq.dstAddr = myPacket.dst_addr;
	//appDataReq.dstEndpoint = myPacket.dst_addr;
	//appDataReq.srcEndpoint = APP_ENDPOINT;
	//appDataReq.options = NWK_OPT_ENABLE_SECURITY;
	//appDataReq.data = myPacket.data;
	//appDataReq.size = myPacket.size;
	//appDataReq.confirm = appDataConf;
	//NWK_DataReq(&appDataReq);
}

int main(void)
{
	irq_initialize_vectors();
	#if SAMD || SAMR21
	system_init();
	delay_init();
	#else
	sysclk_init();
	board_init();
	#endif
	SYS_Init();
	//sio2host_init();
	configure_console();
	printf("we made it");
	cpu_irq_enable();
	LED_On(LED0);
	appInit();
	
	configure_extint();
	configure_eic_callback();
	system_interrupt_enable_global();
	rooms_init();
	uint16_t tick = 0;
	
	while (1)
	{
		SYS_TaskHandler();
		if( tick % 2000 == 0 )
			updateDisplay();
		if( tick == 0 )
			cycleRooms();
		if( tick % 20000 == 0 )
			update_register_task();
		
		tick++;
	}
}
