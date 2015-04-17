/*
 * Wireless.c
 *
 * Created: 4/15/2015 11:18:21 AM
 *  Author: Justin Jonas
 */ 
#include "config.h"
#include "wireless.h"
#include "asf.h"
#include "nwk.h"
#include "phy.h"

extern uint8_t* TEMP_QUEUE;
extern uint8_t* REGISTER_QUEUE;


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
			memcpy(ind->data,TEMP_QUEUE, ind->size);
			break;
		case REGISTER_ADDR:
			memcpy(ind->data,REGISTER_QUEUE, ind->size);
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