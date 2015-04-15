/*
 * Wireless.c
 *
 * Created: 4/15/2015 11:18:21 AM
 *  Author: Justin Jonas
 */ 


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
	if (appDataReqBusy || 0 == appUartBufferPtr) {
		return;
	}
	NWK_DataReq_t appDataReq;
	
	memcpy(appDataReqBuffer, appUartBuffer, appUartBufferPtr);

	appDataReq.dstAddr = packet.dst_addr;
	appDataReq.dstEndpoint = packet.dst_addr;
	appDataReq.srcEndpoint = APP_ENDPOINT;
	appDataReq.options = NWK_OPT_ENABLE_SECURITY;
	appDataReq.data = &packet.data;
	appDataReq.size = sizeof(packet.data);
	appDataReq.confirm = appDataConf;
	NWK_DataReq(&appDataReq);

	appUartBufferPtr = 0;
	appDataReqBusy = true;
}


//When a packet is received, parse the data into the correct queues
bool receive_packet(NWK_DataInd_t *ind)
{
	for (uint8_t i = 0; i < ind->size; i++) {
		rx_data[i] = ind->data[i];
	}
	printf("%s",rx_data);
	return true;
}