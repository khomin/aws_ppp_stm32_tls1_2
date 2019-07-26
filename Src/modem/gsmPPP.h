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

#define TRANSMIT_QUEUE_BUFF_LEN					1128
#define GSM_PPP_RX_TASK_STACK_SIZE				8192
#define GSM_PPP_UART_QUEUE_LENGTH				12
#define GSM_PPP_RAW_INPUT_TASK_STACK_SIZE		1024

typedef struct {
	xSemaphoreHandle semphr;
	ip_addr_t ipRemoteAddr;
	struct tcp_pcb* tcpClient;
	struct{
		uint8_t rxBuffer[1532];
		uint16_t rxBufferLen;
		xSemaphoreHandle rxSemh;
	}rxData;
	bool connected;
}sConnectionPppStruct;

typedef struct {
	uint8_t data[TRANSMIT_QUEUE_BUFF_LEN];
	uint16_t len;
}sTransmitQueue;

typedef enum {
	ppp_not_inited,
	ppp_wait_for_connect,
	ppp_ready_work,
	ppp_disconnected
}ePppState;

typedef struct {
	bool isValid;
	ip_addr_t resolved;
}sGetDnsResult;

bool gsmPPP_Init(void);

#endif
