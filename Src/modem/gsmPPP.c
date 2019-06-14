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

ppp_pcb *ppp;
struct netif ppp_netif;

typedef enum {
	ppp_not_inited,
	ppp_wait_for_connect,
	ppp_ready_work,
	ppp_disconnected
}ePppState;

//-- destination address
extern sConnectSettings connectSettings;

//-- security structure
sConnectionPppStruct connectionPppStruct = {0};

//-- current state
static ePppState pppState = ppp_not_inited;

extern struct stats_ lwip_stats;
static void lwipStack_Init(void);
static void tcpip_init_done(void * arg);
static void status_cb(ppp_pcb *pcb, int err_code, void *ctx);

static err_t tcp_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err);
static err_t dns_server_event_is_found(const char *hostname,
		ip_addr_t *addr,
		dns_found_callback found, void *callback_arg,
		u8_t dns_addrtype
);
static err_t server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
static err_t server_err(void *arg, err_t err);
static void server_close(struct tcp_pcb *pcb);
void udp_dns_echo_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, u16_t port);

static void gsmPPP_rawInput(void *pvParamter);

void gsmPPP_Tsk(void *pvParamter);

bool gsmPPP_Init(void) {
	osThreadAttr_t attributes = {
			.name = "gsmPPP_Tsk",
			.priority = (osPriority_t) osPriorityNormal,
			.stack_size = 2048
	};
	osThreadNew(gsmPPP_Tsk, NULL, &attributes);
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
		retLen = len;
		DBGInfo("PPP: out_raw %s", data);
	} else {
		DBGInfo("PPP: [sio_write -ERRROR]");
	}
	return retLen;
}

/*
 * PPP status callback
 * ===================
 *
 * PPP status callback is called on PPP status change (up, down, …) from lwIP
 * core thread
 */

/* PPP status callback example */
static void status_cb(ppp_pcb *pcb, int err_code, void *ctx) {
//  struct netif *pppif = ppp_netif(pcb);
//  LWIP_UNUSED_ARG(ctx);
//
//  switch(err_code) {
//    case PPPERR_NONE: {
//#if LWIP_DNS
//      const ip_addr_t *ns;
//#endif /* LWIP_DNS */
//      DBGInfo("PPP: status_cb: Connected");
//#if PPP_IPV4_SUPPORT
//      DBGInfo("PPP: our_ipaddr  = %s", ipaddr_ntoa(&pppif->ip_addr));
//      DBGInfo("PPP: his_ipaddr  = %s", ipaddr_ntoa(&pppif->gw));
//      DBGInfo("PPP: netmask     = %s", ipaddr_ntoa(&pppif->netmask));
//#if LWIP_DNS
//      ns = dns_getserver(0);
//      DBGInfo("PPP: dns1        = %s", ipaddr_ntoa(ns));
//      ns = dns_getserver(1);
//      DBGInfo("PPP: dns2        = %s", ipaddr_ntoa(ns));
//#endif /* LWIP_DNS */
//#endif /* PPP_IPV4_SUPPORT */
//#if PPP_IPV6_SUPPORT
//      DBGInfo("PPP: our6_ipaddr = %s", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
//#endif /* PPP_IPV6_SUPPORT */
//      break;
//    }
//    case PPPERR_PARAM: {
//    	DBGInfo("PPP: status_cb: Invalid parameter");
//      break;
//    }
//    case PPPERR_OPEN: {
//    	DBGInfo("PPP: status_cb: Unable to open PPP session");
//		pppState = ppp_ready_work;
//      break;
//    }
//    case PPPERR_DEVICE: {
//    	DBGInfo("PPP: status_cb: Invalid I/O device for PPP");
//      break;
//    }
//    case PPPERR_ALLOC: {
//    	DBGInfo("PPP: status_cb: Unable to allocate resources");
//      break;
//    }
//    case PPPERR_USER: {
//      DBGInfo("PPP: status_cb: User interrupt");
//      break;
//    }
//    case PPPERR_CONNECT: {
//      DBGInfo("PPP: status_cb: Connection lost");
//      break;
//    }
//    case PPPERR_AUTHFAIL: {
//      DBGInfo("PPP: status_cb: Failed authentication challenge");
//      break;
//    }
//    case PPPERR_PROTOCOL: {
//      DBGInfo("PPP: status_cb: Failed to meet protocol");
//      break;
//    }
//    case PPPERR_PEERDEAD: {
//      DBGInfo("PPP: status_cb: Connection timeout");
//      break;
//    }
//    case PPPERR_IDLETIMEOUT: {
//      DBGInfo("PPP: status_cb: Idle Timeout");
//      break;
//    }
//    case PPPERR_CONNECTTIME: {
//      DBGInfo("PPP: status_cb: Max connect time reached");
//      break;
//    }
//    case PPPERR_LOOPBACK: {
//      DBGInfo("PPP: status_cb: Loopback detected");
//      break;
//    }
//    default: {
//      DBGInfo("PPP: status_cb: Unknown error code %d", err_code);
//      break;
//    }
//  }
//
///*
// * This should be in the switch case, this is put outside of the switch
// * case for example readability.
// */
//
//  if (err_code == PPPERR_NONE) {
//    return;
//  }
//
//  /* ppp_close() was previously called, don't reconnect */
//  if (err_code == PPPERR_USER) {
//    /* ppp_free(); -- can be called here */
//    return;
//  }
//
//  /*
//   * Try to reconnect in 30 seconds, if you need a modem chatscript you have
//   * to do a much better signaling here ;-)
//   */
//  ppp_connect(pcb, 30);
//  /* OR ppp_listen(pcb); */
}

static  void ctx_cb_callback(void *p) {}

void gsmPPP_Tsk(void *pvParamter) {
	lwipStack_Init();

	const osThreadAttr_t attr = {
			.name = "pppRxData",
			.priority = (osPriority_t) osPriorityBelowNormal2,
			.stack_size = 1024
	};
	osThreadNew(gsmPPP_rawInput, NULL, &attr);

	sioWriteSemaphore = xSemaphoreCreateBinary();
	for(uint8_t i=0; i<SERVERS_COUNT; i++) {
		connectionPppStruct.semphr[i] = xSemaphoreCreateBinary();
		connectionPppStruct.rxData[i].rxSemh = xSemaphoreCreateBinary();
	}

	ppp = pppos_create(&ppp_netif, output_cb, status_cb, ctx_cb_callback);

	ppp_set_default(ppp);

	/* Ask the peer for up to 2 DNS server addresses. */
	ppp_set_usepeerdns(ppp, 1);

	/* Auth configuration, this is pretty self-explanatory */
	ppp_set_auth(ppp, PPPAUTHTYPE_ANY, "gdata", "gdata");

	for(;;) {
		if(uartParcerStruct.ppp.pppModeEnable == true) {
			if((pppState != ppp_wait_for_connect) && (pppState != ppp_ready_work)) {
				if(ppp_connect(ppp, 0) == ERR_OK) {
					DBGInfo("PPP inited - OK");
//					lwip_stats.link.drop = 0;
//					lwip_stats.link.chkerr = 0;
//					lwip_stats.link.err = 0;
					pppState = ppp_wait_for_connect;
				} else {
//					DBGInfo("PPP: ppp_connect -ERROR");
//					ppp_close(ppp, 0);
//					pppState = ppp_not_inited;
//					uartParcerStruct.ppp.pppModeEnable = false;
//					gsmState.init = false;
//					gsmState.notRespond = true;
				}
			}
		}

//		if(pppState == ppp_ready_work) {
//			DBGInfo("PPP: connect start...");
//			gsmPPP_Connect(0, connectSettings[0].srvAddr, connectSettings[0].srvPort);
//			DBGInfo("PPP: connect start -end");
//			vTaskDelay(5000/portTICK_RATE_MS);
//		}

		vTaskDelay(1000/portTICK_RATE_MS);
	}
}

/* PPP over Serial: this is the input function to be called for received data. */
void gsmPPP_rawInput(void *pvParamter) {
	static u8_t tbuf[512] = {0};
	static u16_t tbuf_len = 0;
	vTaskDelay(1000/portTICK_RATE_MS);
	while(1) {
		if(uartParcerStruct.ppp.pppModeEnable) {
			while(xQueueReceive(uartParcerStruct.uart.rxQueue, &tbuf[tbuf_len], 2/portTICK_RATE_MS) == pdTRUE) {
				tbuf_len++;
				if(tbuf_len >= sizeof(tbuf)-1){
					break;
				}
			}
			if(tbuf_len) {
				DBGInfo("PPP: input_raw %s", tbuf);
				pppos_input(ppp, tbuf, tbuf_len);
				tbuf_len = 0;
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

	if(pppState == ppp_ready_work) {
		DBGInfo("GSMPPP: CONNECT ERROR - PPP closed");
		return false;
	}
	sscanf(pDestAddr, "%u.%u.%u.%u", &ipCut[0], &ipCut[1], &ipCut[2], &ipCut[3]);

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
				} else { }
				break;
			}
		}

		tcp_connect(connectionPppStruct.tcpClient[numConnect], &connectionPppStruct.ipRemoteAddr[numConnect], port, &tcp_connected_cb);

		if(xSemaphoreTake(connectionPppStruct.semphr[numConnect], 10000/portTICK_PERIOD_MS) == pdTRUE) {
			connectionPppStruct.connected[numConnect] = true;
			DBGInfo("GSMPPP: connected %s", inet_ntoa(connectionPppStruct.ipRemoteAddr));
			return true;
		} else {
			DBGInfo("GSMPPP: connectTimeout-ERROR");
			return false;
		}
	} else {
		if(gsmLLR_ConnectServiceStatus(numConnect) == eOk) {
			DBGInfo("GSMPPP: CONNECT-already connected %s", inet_ntoa(connectionPppStruct.ipRemoteAddr));
			return true;
		} else {
			DBGInfo("GSMPPP: CONNECT CLOSE!!!");
			return false;
		}
	}
	return false;
}

bool gsmPPP_Disconnect(uint8_t numConnect) {
	if(pppState == ppp_ready_work) {
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
	if(pppState == ppp_ready_work) {
		DBGInfo("GSMPPP: CONNECT ERROR - PPP closed");
		return false;
	}
	if(connectionPppStruct.tcpClient[numConnect]->state == ESTABLISHED) {
		return true;
	}
	return false;
}

bool gsmPPP_SendData(uint8_t numConnect, uint8_t *pData, uint16_t len) {
	if(pppState == ppp_ready_work) {
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
	if(pppState == ppp_ready_work) {
		DBGInfo("GSMPPP: CONNECT ERROR - PPP closed");
		return false;
	}
	return connectionPppStruct.rxData[numConnect].rxBufferLen;
}

uint16_t gsmPPP_ReadRxData(uint8_t numConnect, uint8_t **ppData) {
	if(pppState == ppp_ready_work) {
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

err_t tcp_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err) {
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
		DBGInfo("GSMPPP:server_recv(): pbuf->len %d byte [%s]", p->len, inet_ntoa(pcb->remote_ip.addr));

		for(uint8_t i=0; i<SERVERS_COUNT; i++) {
			if(pcb->remote_ip.addr == connectionPppStruct.tcpClient[i]->remote_ip.addr) {
				DBGInfo("GSMPPP: server_recv (callback) [%s]", inet_ntoa(pcb->remote_ip.addr));
				if(p->len < sizeof(connectionPppStruct.rxData[i].rxBuffer)) {
					memcpy(connectionPppStruct.rxData[i].rxBuffer, p->payload, p->len);
					connectionPppStruct.rxData[i].rxBufferLen = p->len;
					xSemaphoreGive(connectionPppStruct.rxData[i].rxSemh);
					DBGInfo("GSMPPP: server_recv (callback) GIVE SEMPH[%s][%d]", inet_ntoa(pcb->remote_ip.addr), p->len);
				} else {
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

static void server_close(struct tcp_pcb *pcb) {
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

u32_t sys_jiffies(void) {
	return xTaskGetTickCount();
}
