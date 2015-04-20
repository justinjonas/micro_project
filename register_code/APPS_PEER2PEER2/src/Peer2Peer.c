
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "sys.h"
#include "system.h"
#include "phy.h"
#include "nwk.h"
#include "sysTimer.h"
#include "sio2host.h"
#include "asf.h"

/*- Definitions ------------------------------------------------------------*/
#define OPEN	1
#define CLOSE	0

#define GPIO_1 EXT1_PIN_7//PA18
#define GPIO_2 EXT1_PIN_8//PA19

#ifdef NWK_ENABLE_SECURITY
  #define APP_BUFFER_SIZE     (NWK_MAX_PAYLOAD_SIZE - NWK_SECURITY_MIC_SIZE)
#else
  #define APP_BUFFER_SIZE     NWK_MAX_PAYLOAD_SIZE
#endif

static uint8_t rx_data[APP_RX_BUF_SIZE];

/*- Types ------------------------------------------------------------------*/
typedef enum AppState_t {
	APP_STATE_INITIAL,
	APP_STATE_IDLE,
} AppState_t;

/*- Prototypes -------------------------------------------------------------*/
static void appSendData(void);
static void appInit(void);
void pin_init(void);
void open_register(void);
void close_register(void);

/*- Variables --------------------------------------------------------------*/
static NWK_DataReq_t appDataReq;
static bool appDataReqBusy = false;
static uint8_t appDataReqBuffer[APP_BUFFER_SIZE];
static uint8_t appUartBuffer[APP_BUFFER_SIZE];
static uint8_t appUartBufferPtr = 0;
//Always start with register closed
bool register_status = 0;
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

	appDataReq.dstAddr = 2;
	appDataReq.dstEndpoint = APP_DST_ENDPOINT;
	appDataReq.srcEndpoint = APP_ENDPOINT;
	appDataReq.options = NWK_OPT_ENABLE_SECURITY;
	appDataReq.data = appDataReqBuffer;
	appDataReq.size = appUartBufferPtr;
	appDataReq.confirm = appDataConf;
	NWK_DataReq(&appDataReq);

	appUartBufferPtr = 0;
	appDataReqBusy = true;
}


/*************************************************************************//**
*****************************************************************************/
static bool appDataInd(NWK_DataInd_t *ind)
{
	LED_Toggle(LED0);
	for (uint8_t i = 0; i < ind->size; i++) {
		rx_data[i] = ind->data[i];
	}
	
	switch(rx_data[0])
	{
		case OPEN:
			if(register_status)
				break;
			else
				open_register();
			break;
		case CLOSE:
			if(!register_status)
				break;
			else
				close_register();
			break;			
	}
	rx_data[0]=0;
	return true;
}


void open_register(void)
{
	port_pin_set_output_level(GPIO_1, false);
	port_pin_set_output_level(GPIO_2, true);
	delay_ms(300);
	port_pin_set_output_level(GPIO_1, false);
	port_pin_set_output_level(GPIO_2, false);
	register_status = 1;
}

void close_register(void)
{
	port_pin_set_output_level(GPIO_1, true);
	port_pin_set_output_level(GPIO_2, false);
	delay_ms(300);
	port_pin_set_output_level(GPIO_1, false);
	port_pin_set_output_level(GPIO_2, false);
	register_status = 0;
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
}

void pin_init(void)
{
		struct port_config config_port_pin;

		port_get_config_defaults(&config_port_pin);

		config_port_pin.direction  = PORT_PIN_DIR_OUTPUT;
		config_port_pin.input_pull = PORT_PIN_PULL_DOWN;

		port_pin_set_config(GPIO_1, &config_port_pin);

		port_pin_set_config(GPIO_2, &config_port_pin);
		
		port_pin_set_output_level(GPIO_1, false);
		port_pin_set_output_level(GPIO_2, false);
}

int main(void)
{
	irq_initialize_vectors();
	system_init();
	delay_init();
	SYS_Init();
	
	cpu_irq_enable();
	
	appInit();
	pin_init();
	LED_On(LED0);
	
	while (1) {
		SYS_TaskHandler();
	}
}
