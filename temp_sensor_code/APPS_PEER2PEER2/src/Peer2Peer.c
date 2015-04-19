/*- Includes ---------------------------------------------------------------*/
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
#include <asf.h>
#include "conf_sio2host.h"
#include "adc.h"

/*- Definitions ------------------------------------------------------------*/
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
static void APP_TaskHandler(void);
void configure_adc(void);
static void configure_console(void);
void timer_init(void);
void timer_callback(void);

/*- Variables --------------------------------------------------------------*/
static SYS_Timer_t timer;
static NWK_DataReq_t appDataReq;
static bool appDataReqBusy = false;
static uint8_t appDataReqBuffer[APP_BUFFER_SIZE];
static uint8_t appUartBuffer[APP_BUFFER_SIZE];
static uint8_t appUartBufferPtr = 0;
static struct usart_module cdc_uart_module;

struct adc_module adc_instance;
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

	appDataReq.dstAddr = 0;
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
static void APP_TaskHandler(void)
{

	const char* tx_data = "You Rock";
	
	if(!appDataReqBusy){
		if(!strcmp((const char*)rx_data, "this rocks"))
			LED_Toggle(LED0);
		for (uint16_t i = 0; i < sizeof("You Rock"); i++) {
			if (appUartBufferPtr == sizeof(appUartBuffer)) {
				appSendData();
			}

			if (appUartBufferPtr < sizeof(appUartBuffer)) {
				appUartBuffer[appUartBufferPtr++] = tx_data[i];
			}
		}
		appSendData();
	}
}

/*************************************************************************//**
*****************************************************************************/
static bool appDataInd(NWK_DataInd_t *ind)
{
	for (uint8_t i = 0; i < ind->size; i++) {
		rx_data[i] = ind->data[i];
	}
	//APP_TaskHandler();
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
}


void configure_adc(void)
{
	struct adc_config config_adc;

	adc_get_config_defaults(&config_adc);

	config_adc.reference = ADC_REFERENCE_INT1V;		//ADC_REFERENCE_INTVCC0;	//reference voltage on pin 9: PA04
	config_adc.resolution = ADC_RESOLUTION_12BIT;			//12 bit resolution
	config_adc.divide_result = ADC_DIVIDE_RESULT_DISABLE;	//Don't divide result register after accumulation
	config_adc.positive_input = ADC_POSITIVE_INPUT_PIN10;	//voltage positive input on pin 10: PB02
	config_adc.negative_input = ADC_NEGATIVE_INPUT_PIN7;	//voltage negative input as internal ground : PA07

	adc_init(&adc_instance, ADC, &config_adc);

	adc_enable(&adc_instance);
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
	usart_conf.baudrate    = USART_HOST_BAUDRATE;

	stdio_serial_init(&cdc_uart_module, USART_HOST, &usart_conf);
	usart_enable(&cdc_uart_module);
}

//void timer_init(void)
//{
	//appTimer.interval = APP_FLUSH_TIMER_INTERVAL;
	//appTimer.mode = SYS_TIMER_INTERVAL_MODE;
	//appTimer.handler = timer_callback;
//}

void timer_callback(void)
{
	uint16_t result;
	adc_start_conversion(&adc_instance);
	delay_ms(200);
	do {
		/* Wait for conversion to be done and read out result */
	} while (adc_read(&adc_instance, &result) == STATUS_BUSY);
	adc_clear_status(&adc_instance,adc_get_status(&adc_instance));
	uint32_t far;
	far = 9.0/5.0*((float)result*.0002441406*6.0/.01)+32.0;
	
	memcpy(appUartBuffer,&far, sizeof(far));
	appUartBufferPtr = sizeof(far);
	
	appSendData();
	delay_ms(500);
	printf("The temp is %d     ",far);
	printf("The result is %d     ",result);
			
	
	
}
int main(void)
{
	//irq_initialize_vectors();
	system_init();
	delay_init();
	configure_adc();
	SYS_Init();
	configure_console();
	
	appInit();
	cpu_irq_enable();
	LED_On(LED0);
	
	//adc_start_conversion(&adc_instance);
	while (1) {
		SYS_TaskHandler();
		timer_callback();
	}
}
