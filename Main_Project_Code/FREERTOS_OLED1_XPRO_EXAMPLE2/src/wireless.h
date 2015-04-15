/*
 * Wireless.h
 *
 * Created: 4/15/2015 11:18:47 AM
 *  Author: Justin Jonas
 */ 


#ifndef WIRELESS_H_
#define WIRELESS_H_
#include "stdint.h"
#include "stdbool.h"
#include "config.h"
#include "asf.h"
#include "nwkRx.h"
#include "Wireless.h"
#include "asf.h"
#include "nwk.h"
#include "phy.h"

typedef struct wireless_packet
{
	uint8_t* data;
	uint8_t size;
	uint8_t dst_addr;
	uint8_t sender_addr;
	};

//Initialize the wireless settings
void wireless_init(void);

//Sends data based on the struct passed in with packet
void send_packet(struct wireless_packet packet);

//Callback function when a packet is received
bool receive_packet(NWK_DataInd_t *ind);

//Callback function for a confirmed sent packet
void send_packet_conf(NWK_DataReq_t *req);






#endif /* WIRELESS_H_ */