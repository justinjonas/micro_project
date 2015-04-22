#include <asf.h>
#include "conf_sio2host.h"
#include <string.h>
#include "nwk.h"
#include "phy.h"
#include "sys.h"
#include "config.h"

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
	char registerStatus;
	int temp;
};

#define HEAT "heat"
#define COOL "cool"
#define GPIO_1 EXT1_PIN_7//PA18
#define GPIO_2 EXT1_PIN_8//PA19

//globals
int tick = 0;
//LCD
char degree = 0xDF;
char* mode = COOL;
int roomSelection = 1;
int targetTemp = 70;
int roomTemp = 70;
char ventStatus = 'X';
char report[160];
//temp data
int numberOfRooms = 0;
struct room rooms[30];

static struct usart_module cdc_uart_module;
//static NWK_DataReq_t appDataReq;
struct adc_module adc_instance;

//functions
static void new_sensor_task(void);
static void read_temp_task(void);
static void update_register_task(void);
static void cycle_room_task(void);
static void configure_console(void);
static void updateDisplay(void);
static void rooms_init(void);

static void configure_eic_callback(void);			//button press stuff
static void extint_callback(void);					//button press stuff
static void configure_extint(void);					//button press stuff

void configure_adc(void);
void pin_init(void);

// open_register(void);
//void close_register(void);

void wireless_init(void);
void send_packet(struct wireless_packet packet);	//Sends data based on the struct passed in with packet
static bool appDataInd(NWK_DataInd_t *ind);			//Callback function when a packet is received
void send_packet_conf(NWK_DataReq_t *req);			//Callback function for a confirmed sent packet

int main (void)
{
	system_init();
	delay_init();
	board_init();
	SYS_Init();
	configure_console();
	wireless_init();
	irq_initialize_vectors();
	configure_extint();
	configure_eic_callback();
	system_interrupt_enable_global();
	
	cpu_irq_enable();
	rooms_init();
	
	while(1)
	{
		SYS_TaskHandler();
		//update_register_task();
		//read_temp_task();
		//cycle_room_task();
		//updateDisplay();
		tick++;
		delay_ms(5);
	}
}

//Functions

static void cycle_room_task(void)
{	
	if( tick % 100 == 0 )
	{		
		roomTemp=rooms[roomSelection-1].temp;
		ventStatus=rooms[roomSelection-1].registerStatus;
		roomSelection=rooms[roomSelection-1].roomNumber;

		if( roomSelection-1 < numberOfRooms)
			roomSelection++;
		else
			roomSelection = 0;
	}
}

static void new_sensor_task(void)
{
	//dosome stuff
}

static void update_register_task(void)
{	
	for(int i = 0; i < numberOfRooms-1; i++)
	{
		if(!strcmp(mode,COOL))
		{
			if(rooms[i].temp > targetTemp && rooms[i].registerStatus == 'X')
			{
				rooms[i].registerStatus = 'O';
				//send open to register with address i
			}
			else if(rooms[i].temp < targetTemp && rooms[i].registerStatus == 'O')
			{
				rooms[i].registerStatus = 'X';
				//close_register();
			}
		}
		else if(!strcmp(mode,HEAT))
		{
			if(rooms[i].temp < targetTemp && rooms[i].registerStatus == 'X')
			{
				rooms[i].registerStatus = 'O';
				//open_register();
			}
			else if(rooms[i].temp > targetTemp && rooms[i].registerStatus == 'O')
			{
				rooms[i].registerStatus = 'X';
				//close_register();
			}					
		}
	}
}

static void rooms_init(void)
{
	numberOfRooms = 2;
	rooms[0].registerAddress = 3;
	rooms[0].registerStatus = 'X';
	rooms[0].roomNumber = 1;
	rooms[0].temp = 70;
	rooms[0].tempSensorAddress = 2;
	rooms[1].registerAddress = 5;
	rooms[1].registerStatus = 'X';
	rooms[1].roomNumber = 2;
	rooms[1].temp = 68;
	rooms[1].tempSensorAddress = 4;
}

/** Callback function for the EXTINT driver, called when an external interrupt
 *  detection occurs.
 */
static void extint_callback(void)
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

void updateDisplay(void)
{
	if( tick % 80000 == 0 )
	{
		//clear the display
		//set cursor to beginning
		putchar(254);
		putchar(128);
		//update display
		printf("Mode:%s  Rm:%2dTarget:%2d%c %2d%c %c", mode, roomSelection, targetTemp, degree, roomTemp, degree, ventStatus);
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

void wireless_init(void)
{
	NWK_SetAddr(APP_ADDR);
	NWK_SetPanId(APP_PANID);
	PHY_SetChannel(APP_CHANNEL);
	PHY_SetRxState(true);
	PHY_SetTxPower(0x23);
	NWK_SetSecurityKey((uint8_t *)APP_SECURITY_KEY);
	NWK_OpenEndpoint(APP_ENDPOINT, appDataInd);
}

//We will need to sent the struct of the payload and know where to send it to
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
static bool appDataInd(NWK_DataInd_t *ind)
{
	printf("received!");
	switch(ind->srcAddr)
	{
	//case TEMP_ADDR:
		////stuff
		//break;
	//case REGISTER_ADDR:
		////stuff
		//break;
	//default:
		//return false;
	}
	return true;
}

void send_packet_conf(NWK_DataReq_t *req)
{
	if (NWK_SUCCESS_STATUS == req->status)
		LED_Toggle(LED0);
}
