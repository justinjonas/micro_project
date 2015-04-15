/*
 * Wireless.h
 *
 * Created: 4/15/2015 11:18:47 AM
 *  Author: Justin Jonas
 */ 


#ifndef WIRELESS_H_
#define WIRELESS_H_

typedef struct wireless_packet
{
	uint32_t data;
	uint8_t dst_addr;
	uint8_t sender_addr;
	};

void wireless_init(void);
//We will need to sent the struct of the payload and know where to send it to
void send_packet(struct wireless_packet packet);
bool receive_packet(NWK_DataInd_t *ind);





#endif /* WIRELESS_H_ */