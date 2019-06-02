/*
 * gsmPPP.c
 *
 *  Created on: May 31, 2019
 *      Author: khomin
 */
#include "gsmPPP.h"
#include "gsm.h"
#include "gsmLLR2.h"
#include "string.h"
#include "cmsis_os2.h"
#include "time.h"
#include <time.h>
#include "debug_print.h"
#include "FreeRTOS.h"
#include "stats.h"
#include "dns.h"
#include "lwip/tcpip.h"
#include "ip_addr.h"

#include "netif/ppp/pppos.h"
#include "netif/ppp/pppapi.h"
#include "netif/ppp/pppoe.h"
#include "netif/ppp/pppol2tp.h"

extern xSemaphoreHandle sioWriteSemaphore;
extern sGsmUartParcer uartParcerStruct;
extern UART_HandleTypeDef huart3;
extern sGsmState gsmState;
static unsigned char pppStop = 0;

/* The PPP control block */
ppp_pcb *ppp;
/* The PPP IP interface */
struct netif ppp_netif;

bool pppIsOpen = false;
int pppNumport = 0;
sConnectionPppStruct connectionPppStruct = {0};
extern struct stats_ lwip_stats;
static void lwipStack_Init(void);
static void tcpip_init_done(void * arg);
static err_t tcpConnectedCallBack(void *arg, struct tcp_pcb *tpcb, err_t err);
static void linkStatusCB(void * ctx, int errCode, void * arg);

static err_t dns_server_event_is_found(const char *hostname,
		ip_addr_t *addr,
		dns_found_callback found, void *callback_arg,
		u8_t dns_addrtype
);

static err_t server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
static err_t server_err(void *arg, err_t err);
static void server_close(struct tcp_pcb *pcb);
void udp_dns_echo_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, u16_t port);

static void gsmPPP_RawInput(void *pvParamter);

void gsmPPP_Tsk(void *pvParamter);

bool gsmPPP_Init(void) {
	xTaskCreate(gsmPPP_Tsk, "GsmPppTsk", configMINIMAL_STACK_SIZE*3, NULL, tskIDLE_PRIORITY+2, NULL);
	return true;
}

/*
 * PPPoS serial output callback
 *
 * ppp_pcb, PPP control block
 * data, buffer to write to serial port
 * len, length of the data buffer
 * ctx, optional user-provided callback context pointer
 *
 * Return value: len if write succeed
 */
static u32_t output_cb(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx) {
	u32_t retLen = 0;
	if(HAL_UART_Transmit_IT(&huart3, data, len) == HAL_OK) {
		xSemaphoreTake(sioWriteSemaphore, portMAX_DELAY);
		retLen = len;
	} else {
		DBGInfo("PPP: [sio_write -ERRROR]");
	}
	return retLen;
}

void status_callback(ppp_pcb *pcb, int err_code, void *ctx) {

}

void ctx_cb_callback(void *p) {

}

extern sConnectSettings connectSettings;

void gsmPPP_Tsk(void *pvParamter) {
	int timeout = 0;
	uint8_t i;
	bool stateInit = false;

	lwipStack_Init();

	const osThreadAttr_t attr = {
			.name = "pppRxData",
			.priority = (osPriority_t) osPriorityBelowNormal5,
			.stack_size = 512
	};
	osThreadNew(gsmPPP_RawInput, NULL, &attr);

	sioWriteSemaphore = xSemaphoreCreateBinary();
	for(i=0; i<SERVERS_COUNT; i++) {
		connectionPppStruct.semphr[i] = xSemaphoreCreateBinary();
		connectionPppStruct.rxData[i].rxSemh = xSemaphoreCreateBinary();
	}

	ppp = pppos_create(&ppp_netif, output_cb, status_callback, ctx_cb_callback);

	ppp_set_default(ppp);

	/* Ask the peer for up to 2 DNS server addresses. */
	ppp_set_usepeerdns(ppp, 1);

	/* Auth configuration, this is pretty self-explanatory */
	ppp_set_auth(ppp, PPPAUTHTYPE_ANY, "gdata", "gdata");

	for(;;) {

		if(uartParcerStruct.ppp.pppModeEnable == true) {
			if(!pppIsOpen) {
				/*
				 * Initiate PPP negotiation, without waiting (holdoff=0), can only be called
				 * if PPP session is in the dead state (i.e. disconnected).
				 */
				if(ppp_connect(ppp, 5000) == ERR_OK) {
					DBGInfo("PPP init - OK");
					lwip_stats.link.drop = 0;
					lwip_stats.link.chkerr = 0;
					lwip_stats.link.err = 0;
					stateInit = true;

					vTaskDelay(10000/portTICK_RATE_MS);
					gsmPPP_Connect(0, connectSettings[0].srvAddr, connectSettings[0].srvPort);

				} else {
					DBGInfo("PPP: ppp_connect -ERROR");
//					pppClose(pppNumport);
					pppIsOpen = false;
					uartParcerStruct.ppp.pppModeEnable = false;
					gsmState.init = false;
					gsmState.notRespond = true;
				}

				//				pppNumport = pppOverSerialOpen(0, linkStatusCB, &pppIsOpen);
				//				pppStop = 0;
				//				timeout = 0;
				//				stateInit = false;
				//				while(timeout < 300) {
				//					if(pppIsOpen) {
				//						DBGInfo("PPP init - OK");
				//						lwip_stats.link.drop = 0;
				//						lwip_stats.link.chkerr = 0;
				//						lwip_stats.link.err = 0;
				//						stateInit = true;
				//						break;
				//					}else{
				//						timeout ++;
				//						vTaskDelay(100/portTICK_RATE_MS);
				//					}
				//				}
				//				if(stateInit == false) {
				//					DBGInfo("PPP init - TIMEOUT-ERROR");
				//					pppClose(pppNumport);
				//					pppIsOpen = false;
				//					uartParcerStruct.ppp.pppModeEnable = false;
				//					gsmState.init = false;
				//					gsmState.notRespond = true;
				//				}
			}
		}

		vTaskDelay(10000/portTICK_RATE_MS);
	}
}

/* PPP over Serial: this is the input function to be called for received data. */
//	void pppos_input(ppp_pcb *ppp, u8_t* data, int len);

void gsmPPP_RawInput(void *pvParamter) {
	static u8_t tbyte = 0;
	vTaskDelay(1000/portTICK_RATE_MS);
	while(1) {
		if(uartParcerStruct.ppp.pppModeEnable) {
			while(xQueueReceive(uartParcerStruct.uart.rxQueue, &tbyte, 100/portTICK_RATE_MS) == pdTRUE) {
				pppos_input(ppp, tbyte, 1);
			}
		} else {
			vTaskDelay(200/portTICK_RATE_MS);
		}
	}
}

void gsmPPP_wtdControl() {
	//	if((lwip_stats.link.drop !=0)
	//			|| (lwip_stats.link.chkerr !=0)) {
	//		lwip_stats.link.drop = 0;
	//		lwip_stats.link.chkerr = 0;
	//		DBGInfo("GSMM: DROPING FAIL!!! RESTART PPP");
	//		for(uint8_t i=0; i<SERVERS_COUNT; i++) {
	//			gsmPPP_Disconnect(i);
	//		}
	//		pppClose(pppNumport);
	//		pppIsOpen = false;
	//		uartParcerStruct.ppp.pppModeEnable = false;
	//		gsmState.init = false;
	//		gsmState.notRespond = true;
	//		vTaskDelay(500/portTICK_PERIOD_MS);
	//	}
	//	if( (lwip_stats.memp[2]->max > lwip_stats.memp[2]->avail)
	//			|| ((lwip_stats.memp[10]->err != 0)) )  { // PBUF_POOL
	//		DBGInfo("GSMM: ERROR Lwip -RESET");
	////		systemReset(_Reset_ErrorLwIpPcb);
	//	}
}

bool gsmPPP_Connect(uint8_t numConnect, char *pDestAddr, uint16_t port) {
	ip_addr_t resolved;
	bool useDns = false;
	uint8_t ipCut[4] = {0};

	if(!pppIsOpen) {
		DBGInfo("GSMPPP: CONNECT ERROR - PPP closed");
		return false;
	}
	sscanf(pDestAddr, "%hhu.%hhu.%hhu.%hhu", &ipCut[0], &ipCut[1], &ipCut[2], &ipCut[3]);
	if((ipCut[0]!=0)&&(ipCut[1]!=0)&&(ipCut[2]!=0)&&(ipCut[3]!=0)) {
		IP4_ADDR(&connectionPppStruct.ipRemoteAddr[numConnect], ipCut[0],ipCut[1],ipCut[2],ipCut[3]);
		useDns = false;
		DBGInfo("GSMPPP: connect... without dns [%d.%d.%d.%d|%d]", ipCut[0], ipCut[1], ipCut[2], ipCut[3], port);
	} else{
		useDns = true;
		DBGInfo("GSMPPP: connect... use dns %s", pDestAddr);
	}

	if(connectionPppStruct.connected[numConnect] == false) {
		if(connectionPppStruct.tcpClient[numConnect] == NULL) {
			connectionPppStruct.tcpClient[numConnect] = tcp_new();
		}
		tcp_recv(connectionPppStruct.tcpClient[numConnect], server_recv);

		if(useDns == true) {
			switch(dns_gethostbyname(pDestAddr, &resolved, dns_server_event_is_found, &numConnect)) {
			case ERR_OK: // numeric or cached, returned in resolved
				connectionPppStruct.ipRemoteAddr[numConnect].addr = resolved.addr;
				break;
			case ERR_INPROGRESS: // need to ask, will return data via callback
				if(xSemaphoreTake(connectionPppStruct.semphr[numConnect], 10000/portTICK_PERIOD_MS) != pdTRUE) {
					server_close(connectionPppStruct.tcpClient[numConnect]);
					connectionPppStruct.connected[numConnect] = false;
					DBGInfo("GSMPPP: dns-ERROR");
					return false;
				}else{ }
				break;
			}
		}

		tcp_connect(connectionPppStruct.tcpClient[numConnect], &connectionPppStruct.ipRemoteAddr[numConnect], port, &tcpConnectedCallBack);

		if(xSemaphoreTake(connectionPppStruct.semphr[numConnect], 10000/portTICK_PERIOD_MS) == pdTRUE) {
			connectionPppStruct.connected[numConnect] = true;
			DBGInfo("GSMPPP: connected %s", inet_ntoa(connectionPppStruct.ipRemoteAddr));
			return true;
		}else{
			DBGInfo("GSMPPP: connectTimeout-ERROR");
			return false;
		}
	}else{
		if(gsmLLR_ConnectServiceStatus(numConnect) == eOk) {
			DBGInfo("GSMPPP: CONNECT-already connected %s", inet_ntoa(connectionPppStruct.ipRemoteAddr));
			return true;
		}else{
			DBGInfo("GSMPPP: CONNECT CLOSE!!!");
			return false;
		}
	}
	return false;
}

bool gsmPPP_Disconnect(uint8_t numConnect) {
	if(!pppIsOpen) {
		DBGInfo("GSMPPP: CONNECT ERROR - PPP closed");
		return false;
	}
	if(connectionPppStruct.tcpClient[numConnect] == NULL) {
		return false;
	}
	server_close(connectionPppStruct.tcpClient[numConnect]);
	connectionPppStruct.connected[numConnect] = false;
	return true;
}

bool gsmPPP_ConnectStatus(uint8_t numConnect) {
	if(!pppIsOpen) {
		DBGInfo("GSMPPP: CONNECT ERROR - PPP closed");
		return false;
	}
	if(connectionPppStruct.tcpClient[numConnect]->state == ESTABLISHED) {
		return true;
	}
	return false;
}

bool gsmPPP_SendData(uint8_t numConnect, uint8_t *pData, uint16_t len) {
	if(!pppIsOpen) {
		DBGInfo("GSMPPP: CONNECT ERROR - PPP closed");
		return false;
	}
	//	if(tcp_write(connectionPppStruct.tcpClient[numConnect], pData, len, NULL) == ERR_OK) {
	//		return true;
	//	}else {
	//		server_close(connectionPppStruct.tcpClient[numConnect]);
	//		connectionPppStruct.connected[numConnect] = false;
	//		connectionPppStruct.rxData[numConnect].rxBufferLen = 0;
	//		memset(connectionPppStruct.rxData[numConnect].rxBuffer,0, sizeof(connectionPppStruct.rxData[numConnect].rxBuffer));
	//	}
	return false;
}

uint16_t gsmPPP_GetRxLenData(uint8_t numConnect) {
	if(!pppIsOpen) {
		DBGInfo("GSMPPP: CONNECT ERROR - PPP closed");
		return false;
	}
	return connectionPppStruct.rxData[numConnect].rxBufferLen;
}

uint16_t gsmPPP_ReadRxData(uint8_t numConnect, uint8_t **ppData) {
	if(!pppIsOpen) {
		DBGInfo("GSMPPP: CONNECT ERROR - PPP closed");
		return false;
	}
	if(connectionPppStruct.rxData[numConnect].rxBufferLen != 0) {
		*ppData = (uint8_t *) connectionPppStruct.rxData[numConnect].rxBuffer;
		uint16_t retLen = connectionPppStruct.rxData[numConnect].rxBufferLen;
		connectionPppStruct.rxData[numConnect].rxBufferLen = 0;
		return retLen;
	}
	return false;
}

err_t dns_server_event_is_found(const char *hostname,
		ip_addr_t *addr,
		dns_found_callback found,
		void *callback_arg,
		u8_t dns_addrtype) {
	err_t res = ERR_VAL;
	if(dns_addrtype < SERVERS_COUNT) {
		DBGInfo("GSMPPP: DEST FOUND %s", inet_ntoa(addr));
		connectionPppStruct.ipRemoteAddr[dns_addrtype].addr = addr->addr;
		xSemaphoreGive(connectionPppStruct.semphr[dns_addrtype]);
		res = ERR_OK;
	} else {
		DBGInfo("GSMPPP: DNS != SERVER%s", inet_ntoa(addr));
	}
	return res;
}

err_t tcpConnectedCallBack(void *arg, struct tcp_pcb *tpcb, err_t err) {
	for(uint8_t i=0; i<SERVERS_COUNT; i++) {
		if(tpcb == connectionPppStruct.tcpClient[i]) {
			DBGInfo("GSMPPP: connected (callback)%s", inet_ntoa(tpcb->local_ip.addr));
			xSemaphoreGive(connectionPppStruct.semphr[i]);
			break;
		}
	}
	return ERR_OK;
}

err_t server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
	LWIP_UNUSED_ARG(arg);

	if(err == ERR_OK && p != NULL) {
		tcp_recved(pcb, p->tot_len);
		DBGInfo("GSMPPP:server_recv(): pbuf->len %d byte\n [%s]", p->len, inet_ntoa(pcb->remote_ip.addr));

		for(uint8_t i=0; i<SERVERS_COUNT; i++) {
			if(pcb->remote_ip.addr == connectionPppStruct.tcpClient[i]->remote_ip.addr) {
				DBGInfo("GSMPPP: server_recv (callback) [%s]", inet_ntoa(pcb->remote_ip.addr));
				if(p->len < sizeof(connectionPppStruct.rxData[i].rxBuffer)) {
					memcpy(connectionPppStruct.rxData[i].rxBuffer, p->payload, p->len);
					connectionPppStruct.rxData[i].rxBufferLen = p->len;
					xSemaphoreGive(connectionPppStruct.rxData[i].rxSemh);
					DBGInfo("GSMPPP: server_recv (callback) GIVE SEMPH[%s][%d]", inet_ntoa(pcb->remote_ip.addr), p->len);
				}else{
					DBGInfo("GSMPPP: server_recv p->len > sizeof(buf) -ERROR");
				}
			}
		}
		pbuf_free(p);
	} else {
		DBGInfo("\nserver_recv(): Errors-> ");
		if (err != ERR_OK)
			DBGInfo("1) Connection is not on ERR_OK state, but in %d state->\n", err);
		if (p == NULL)
			DBGInfo("2) Pbuf pointer p is a NULL pointer->\n ");
		DBGInfo("server_recv(): Closing server-side connection...");
		pbuf_free(p);
		server_close(pcb);
	}
	return ERR_OK;
}

xSemaphoreHandle* gsmPPP_GetRxSemaphorePoint(uint8_t numService) {
	return (connectionPppStruct.rxData[numService].rxSemh);
}

static err_t server_poll(void *arg, struct tcp_pcb *pcb)
{
	static int counter = 1;
	LWIP_UNUSED_ARG(arg);
	LWIP_UNUSED_ARG(pcb);
	DBGInfo("\nserver_poll(): Call number %d\n", counter++);
	return ERR_OK;
}

static err_t server_err(void *arg, err_t err)
{
	LWIP_UNUSED_ARG(arg);
	LWIP_UNUSED_ARG(err);
	DBGInfo("\nserver_err(): Fatal error, exiting...\n");
	return ERR_OK;
}

static void server_close(struct tcp_pcb *pcb)
{
	if(pcb->state != CLOSED) {
		tcp_arg(pcb, NULL);
		tcp_sent(pcb, NULL);
		tcp_recv(pcb, NULL);
		while(tcp_close(pcb) != ERR_OK) {
			vTaskDelay(100/portTICK_PERIOD_MS);
		}
	}
	for(uint8_t i=0; i<SERVERS_COUNT; i++) {
		if(pcb == connectionPppStruct.tcpClient[i]) {
			DBGInfo("GSMPPP: server_close (callback)%s", inet_ntoa(pcb->local_ip.addr));
			connectionPppStruct.connected[i] = false;
			connectionPppStruct.tcpClient[i] = NULL;
		}
	}
}

void udp_dns_echo_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, u16_t port) {
	if (p != NULL) {
		DBGInfo("GSMM: udp echo dns -OK!");
		pbuf_free(p);
	}
}

/* Private functions ---------------------------------------------------------*/
/**
 * @brief  Initializes the lwIP stack
 * @param  None
 * @retval None
 */
void lwipStack_Init(void) {
	uint8_t setup = 0;
	tcpip_init(tcpip_init_done, &setup);
	//	while (!setup) {vTaskDelay(50/portTICK_RATE_MS);}
}

static void tcpip_init_done(void * arg) {
	if (arg != NULL) {
		*((int *) arg) = 1;
	}
}

/**
 * @brief  Returns the current time in milliseconds
 *         when LWIP_TIMERS == 1 and NO_SYS == 1
 * @param  None
 * @retval Time
 */
u32_t sys_now(void) {
	return HAL_GetTick();
}

//u32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len) {
//	unsigned long i = 0;
//	if(uartParcerStruct.ppp.pppModeEnable) {
//		while(xQueueReceive(uartParcerStruct.uart.rxQueue,&data[i], 0) == pdTRUE) {
//#ifdef PPP_SIO_DEBUG
//			if(i==0) {
//				DBGInfo("Reading PPP packet from UART");
//			}
//			DBGInfo("%0.2x ", data[i]);
//#endif
//			i++;
//			if (pppStop||(i==len)) {
//				pppStop = false;
//				return i;
//			}
//		}
//#ifdef PPP_SIO_DEBUG
//		if (i>0) {
//			DBGInfo("\n");
//		}
//#endif
//	}
//	return i;
//}

//u32_t sio_write(sio_fd_t fd, u8_t *data, u32_t len) {
//	u32_t retLen = 0;
//	if(uartParcerStruct.ppp.pppModeEnable) {
//		if(HAL_UART_Transmit_IT(&huart3, data, len) == HAL_OK) {
//			xSemaphoreTake(sioWriteSemaphore, portMAX_DELAY);
//			retLen = len;
//		} else {
//			DBGInfo("HAL ERRROR WRITE [sio_write]");
//		}
//	} else {
//		DBGInfo("sio_write not in PPP mode!");
//	}
//	return retLen;
//}

//void sio_read_abort(sio_fd_t fd) {
//	pppStop = true;
//	xQueueReset(uartParcerStruct.uart.rxQueue);
//}

//static void linkStatusCB(void * ctx, int errCode, void * arg) {
//	DBGInfo("GSMPP: linkStatusCB"); /* just wait */
//	//	bool *connected = (bool*)ctx;
//	//	struct ppp_addrs * addrs = arg;
//	//	switch (errCode) {
//	//	case PPPERR_NONE: { /* We are connected */
//	//		DBGInfo("ip_addr = %s", inet_ntoa(addrs->our_ipaddr));
//	//		DBGInfo("netmask = %s", inet_ntoa(addrs->netmask));
//	//		DBGInfo("dns1 = %s", inet_ntoa(addrs->dns1));
//	//		DBGInfo("dns2 = %s", inet_ntoa(addrs->dns2));
//	//		*connected = 1;
//	//		break;
//	//	}
//	//	case PPPERR_CONNECT: {
//	//		DBGInfo("lost connection"); /* just wait */
//	//		*connected = 0;
//	//		break;
//	//	}
//	//	default: { /* We have lost connection */
//	//		DBGInfo("connection error"); /* just wait */
//	//		*connected = 0;
//	//		break;
//	//	}
//	//	}
//}

u32_t sys_jiffies(void) {
	return xTaskGetTickCount();
}
