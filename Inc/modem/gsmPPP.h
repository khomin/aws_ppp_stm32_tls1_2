#ifndef GSMPPP_H
#define GSMPPP_H

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "lwip/udp.h"
#include "lwip/tcpip.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/ip.h"
#include "dns.h"
#include "netif/ppp/ppp.h"
#include "lwip/sio.h"
#include "lwip/inet.h"
#include <stdbool.h>
#include "gsmLLR.h"

typedef struct {
	xSemaphoreHandle semphr[SERVERS_COUNT];
	ip_addr_t ipRemoteAddr[SERVERS_COUNT];
	struct tcp_pcb* tcpClient[SERVERS_COUNT];
	struct{
		uint8_t rxBuffer[1532];
		uint16_t rxBufferLen;
		xSemaphoreHandle rxSemh;
	}rxData[SERVERS_COUNT];
	bool connected[SERVERS_COUNT];
}sConnectionPppStruct;

bool gsmPPP_Init(void);
bool gsmPPP_Connect(uint8_t numConnect, char *pDestAddr, uint16_t port);
bool gsmPPP_Disconnect(uint8_t numConnect);
bool gsmPPP_SendData(uint8_t numConnect, uint8_t *pData, uint16_t len);
uint16_t gsmPPP_GetRxLenData(uint8_t numConnect);
uint16_t gsmPPP_ReadRxData(uint8_t numConnect, uint8_t **pData);
bool gsmPPP_ConnectStatus(uint8_t numConnect);
xSemaphoreHandle * gsmPPP_GetRxSemaphorePoint(uint8_t numService);

void gsmPPP_wtdControl();

#endif
