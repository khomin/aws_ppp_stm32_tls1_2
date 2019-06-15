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
#include "aws_system_init.h"

extern xSemaphoreHandle sioWriteSemaphore;
extern sGsmUartParcer uartParcerStruct;
extern UART_HandleTypeDef huart3;
extern sGsmState gsmState;

ppp_pcb *ppp;
struct netif ppp_netif;

//-- destination address
extern sConnectSettings connectSettings;

//-- security structure
sConnectionPppStruct connectionPppStruct = {0};

//-- current state
static ePppState pppState = ppp_not_inited;

extern struct stats_ lwip_stats;
// static functions
static void tcpip_init_done(void * arg);
static void status_cb(ppp_pcb *pcb, int err_code, void *ctx);
static void ctx_cb_callback(void *p);
static u32_t output_cb(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx);

//static err_t tcp_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err);
static void dns_server_is_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg);
//static err_t server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
//static err_t server_err(void *arg, err_t err);
//static void server_close(struct tcp_pcb *pcb);
//void udp_dns_echo_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, u16_t port);

static void gsmPPP_rawInput(void *pvParamter);
static sGetDnsResult getIpByDns(const char *pDnsName, uint8_t len);

void gsmPPP_Tsk(void *pvParamter);

bool gsmPPP_Init(void) {
	xTaskCreate(gsmPPP_Tsk, "gsmPPP_Tsk", 1024, 0, tskIDLE_PRIORITY+3, NULL);
	return true;
}

void gsmPPP_Tsk(void *pvParamter) {
	uint8_t setup = 0;
	tcpip_init(tcpip_init_done, &setup);

	xTaskCreate(gsmPPP_rawInput, "pppRxData", 1024, 0, tskIDLE_PRIORITY+4, NULL);

	sioWriteSemaphore = xSemaphoreCreateBinary();
	connectionPppStruct.semphr = xSemaphoreCreateBinary();
	connectionPppStruct.rxData.rxSemh = xSemaphoreCreateBinary();

	ppp = pppos_create(&ppp_netif, output_cb, status_cb, ctx_cb_callback);

	ppp_set_default(ppp);

	/* Ask the peer for up to 2 DNS server addresses. */
	ppp_set_usepeerdns(ppp, 1);

	/* Auth configuration, this is pretty self-explanatory */
	ppp_set_auth(ppp, PPPAUTHTYPE_ANY, "gdata", "gdata");

	SYSTEM_Init();

	for(;;) {
		if(uartParcerStruct.ppp.pppModeEnable == true) {
			if((pppState != ppp_wait_for_connect) && (pppState != ppp_ready_work)) {
				pppState = ppp_wait_for_connect;

				if(ppp_connect(ppp, 0) == ERR_OK) {
					DBGInfo("PPP inited - OK");
				} else {
					DBGInfo("PPP: ppp_connect -ERROR");
					ppp_close(ppp, 0);
					pppState = ppp_not_inited;
					uartParcerStruct.ppp.pppModeEnable = false;
					gsmState.init = false;
					gsmState.notRespond = true;
				}
			}
		}

		if(pppState == ppp_ready_work) {
			DBGInfo("PPP: connect start...");
			gsmPPP_Connect(connectSettings.srvAddr, connectSettings.srvPort);
			DBGInfo("PPP: connect start -end");
			vTaskDelay(5000/portTICK_RATE_MS);
		}

		vTaskDelay(1000/portTICK_RATE_MS);
	}
}

//----------------------------
//--	private functions
//----------------------------

bool gsmPPP_Connect(char *pDestAddr, uint16_t port) {
	ip_addr_t resolved;
	uint8_t ipCut[4] = {0};

	if(pppState != ppp_ready_work) {
		return false;
	}
	// prepare ip address
	sscanf(pDestAddr, "%" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8,
			&ipCut[0], &ipCut[1], &ipCut[2], &ipCut[3]);

	if((ipCut[0] != 0) && (ipCut[1] != 0)
			&& (ipCut[2] != 0)&&(ipCut[3] != 0)) {
		IP4_ADDR(&connectionPppStruct.ipRemoteAddr, ipCut[0], ipCut[1], ipCut[2], ipCut[3]);
		DBGInfo("GSMPPP: connect without dns [%d.%d.%d.%d|%d]... ", ipCut[0], ipCut[1], ipCut[2], ipCut[3], port);
	} else{
		DBGInfo("GSMPPP: connect use dns %s... ", pDestAddr);
		sGetDnsResult dns_prop = getIpByDns(pDestAddr, strlen(pDestAddr));
		if(dns_prop.isValid) {
			resolved = dns_prop.resolved;
		}
	}

	//	if(connectionPppStruct.connected == false) {
	//		if(connectionPppStruct.tcpClient == NULL) {
	//			connectionPppStruct.tcpClient = tcp_new();
	//		}
	//		tcp_recv(connectionPppStruct.tcpClient, server_recv);
	//
	//		tcp_connect(connectionPppStruct.tcpClient[numConnect], &connectionPppStruct.ipRemoteAddr[numConnect], port, &tcp_connected_cb);
	//
	//		if(xSemaphoreTake(connectionPppStruct.semphr[numConnect], 10000/portTICK_PERIOD_MS) == pdTRUE) {
	//			connectionPppStruct.connected[numConnect] = true;
	//			DBGInfo("GSMPPP: connected %s", inet_ntoa(connectionPppStruct.ipRemoteAddr));
	//			return true;
	//		} else {
	//			DBGInfo("GSMPPP: connect timeout -error");
	//			return false;
	//		}
	//	} else {
	//		if(gsmLLR_ConnectServiceStatus(numConnect) == eOk) {
	//			DBGInfo("GSMPPP: connect -already connected %s", inet_ntoa(connectionPppStruct.ipRemoteAddr));
	//			return true;
	//		} else {
	//			DBGInfo("GSMPPP: connect -close");
	//			return false;
	//		}
	//	}
	return false;
}

//
//bool gsmPPP_Disconnect(uint8_t numConnect) {
//	if(pppState != ppp_ready_work) {
//		DBGInfo("GSMPPP: CONNECT ERROR - PPP closed");
//		return false;
//	}
//	if(connectionPppStruct.tcpClient[numConnect] == NULL) {
//		return false;
//	}
//	server_close(connectionPppStruct.tcpClient[numConnect]);
//	connectionPppStruct.connected[numConnect] = false;
//	return true;
//}
//
//bool gsmPPP_ConnectStatus(uint8_t numConnect) {
//	if(pppState != ppp_ready_work) {
//		DBGInfo("GSMPPP: CONNECT ERROR - PPP closed");
//		return false;
//	}
//	if(connectionPppStruct.tcpClient[numConnect]->state == ESTABLISHED) {
//		return true;
//	}
//	return false;
//}
//
//bool gsmPPP_SendData(uint8_t numConnect, uint8_t *pData, uint16_t len) {
//	if(pppState != ppp_ready_work) {
//		DBGInfo("GSMPPP: CONNECT ERROR - PPP closed");
//		return false;
//	}
//	//	if(tcp_write(connectionPppStruct.tcpClient[numConnect], pData, len, NULL) == ERR_OK) {
//	//		return true;
//	//	}else {
//	//		server_close(connectionPppStruct.tcpClient[numConnect]);
//	//		connectionPppStruct.connected[numConnect] = false;
//	//		connectionPppStruct.rxData[numConnect].rxBufferLen = 0;
//	//		memset(connectionPppStruct.rxData[numConnect].rxBuffer,0, sizeof(connectionPppStruct.rxData[numConnect].rxBuffer));
//	//	}
//	return false;
//}

sGetDnsResult getIpByDns(const char *pDnsName, uint8_t len) {
	sGetDnsResult result;
	result.isValid = false;

	if(pppState == ppp_ready_work) {
		if(connectionPppStruct.connected == false) {
			// if not created, when creat it
			if(connectionPppStruct.tcpClient == NULL) {
				connectionPppStruct.tcpClient = tcp_new();
			}
			// set listener event
//			tcp_recv(connectionPppStruct.tcpClient, server_recv);
			switch(dns_gethostbyname(pDnsName,
					&result.resolved,
					dns_server_is_found_cb,
					NULL)) {
			case ERR_OK: // numeric or cached, returned in resolved
				connectionPppStruct.ipRemoteAddr.addr = result.resolved.addr;
				result.isValid = true;
				break;
			case ERR_INPROGRESS: // need to ask, will return data via callback
				if(xSemaphoreTake(connectionPppStruct.semphr, 5000/portTICK_PERIOD_MS) != pdTRUE) {
//					server_close(connectionPppStruct.tcpClient);
					connectionPppStruct.connected = false;
					DBGInfo("GSMPPP: dns-ERROR");
				}
				break;
			}
		}
	}
	return result;
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
u32_t output_cb(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx) {
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
	struct netif *pppif = ppp_netif(pcb);
	LWIP_UNUSED_ARG(ctx);

	pppState = ppp_not_inited;

	switch(err_code) {
	case PPPERR_NONE: {
#if LWIP_DNS
		const ip_addr_t *ns;
#endif /* LWIP_DNS */
		DBGInfo("PPP: status_cb: Connected");
		pppState = ppp_ready_work;
#if PPP_IPV4_SUPPORT
		DBGInfo("PPP: our_ipaddr  = %s", ipaddr_ntoa(&pppif->ip_addr));
		DBGInfo("PPP: his_ipaddr  = %s", ipaddr_ntoa(&pppif->gw));
		DBGInfo("PPP: netmask     = %s", ipaddr_ntoa(&pppif->netmask));
#if LWIP_DNS
		ns = dns_getserver(0);
		DBGInfo("PPP: dns1        = %s", ipaddr_ntoa(ns));
		ns = dns_getserver(1);
		DBGInfo("PPP: dns2        = %s", ipaddr_ntoa(ns));
#endif /* LWIP_DNS */
#endif /* PPP_IPV4_SUPPORT */
#if PPP_IPV6_SUPPORT
		DBGInfo("PPP: our6_ipaddr = %s", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#endif /* PPP_IPV6_SUPPORT */
		break;
	}
	case PPPERR_PARAM: {
		DBGInfo("PPP: status_cb: Invalid parameter");
		break;
	}
	case PPPERR_OPEN: {
		DBGInfo("PPP: status_cb: Unable to open PPP session");
		pppState = ppp_ready_work;
		break;
	}
	case PPPERR_DEVICE: {
		DBGInfo("PPP: status_cb: Invalid I/O device for PPP");
		break;
	}
	case PPPERR_ALLOC: {
		DBGInfo("PPP: status_cb: Unable to allocate resources");
		break;
	}
	case PPPERR_USER: {
		DBGInfo("PPP: status_cb: User interrupt");
		break;
	}
	case PPPERR_CONNECT: {
		DBGInfo("PPP: status_cb: Connection lost");
		break;
	}
	case PPPERR_AUTHFAIL: {
		DBGInfo("PPP: status_cb: Failed authentication challenge");
		break;
	}
	case PPPERR_PROTOCOL: {
		DBGInfo("PPP: status_cb: Failed to meet protocol");
		break;
	}
	case PPPERR_PEERDEAD: {
		DBGInfo("PPP: status_cb: Connection timeout");
		break;
	}
	case PPPERR_IDLETIMEOUT: {
		DBGInfo("PPP: status_cb: Idle Timeout");
		break;
	}
	case PPPERR_CONNECTTIME: {
		DBGInfo("PPP: status_cb: Max connect time reached");
		break;
	}
	case PPPERR_LOOPBACK: {
		DBGInfo("PPP: status_cb: Loopback detected");
		break;
	}
	default: {
		DBGInfo("PPP: status_cb: Unknown error code %d", err_code);
		break;
	}
	}

	/*
	 * This should be in the switch case, this is put outside of the switch
	 * case for example readability.
	 */

	if (err_code == PPPERR_NONE) {
		return;
	}
	if (err_code == PPPERR_USER) {
		/* ppp_free(); -- can be called here */
		return;
	}

	/*
	 * Try to reconnect in 30 seconds, if you need a modem chatscript you have
	 * to do a much better signaling here ;-)
	 */
	ppp_connect(pcb, 30);
	/* OR ppp_listen(pcb); */
}

void ctx_cb_callback(void *p) {}

static void dns_server_is_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {

}

/* PPP over Serial: this is the input function to be called for received data. */
void gsmPPP_rawInput(void *pvParamter) {
	static u8_t tbuf[512] = {0};
	static u16_t tbuf_len = 0;
	vTaskDelay(1000/portTICK_RATE_MS);
	while(1) {
		if(uartParcerStruct.ppp.pppModeEnable) {
			while(xQueueReceive(uartParcerStruct.uart.rxQueue, &tbuf[tbuf_len], 5/portTICK_PERIOD_MS) == pdTRUE) {
				tbuf_len++;
				if((tbuf_len-1) >=  sizeof(tbuf)) {
					break;
				}
			}
			if(tbuf_len) {
				pppos_input(ppp, tbuf, tbuf_len);
				tbuf_len = 0;
			}
		} else {
			vTaskDelay(10/portTICK_RATE_MS);
		}
	}
}

//
//uint16_t gsmPPP_GetRxLenData(uint8_t numConnect) {
//	if(pppState != ppp_ready_work) {
//		DBGInfo("GSMPPP: CONNECT ERROR - PPP closed");
//		return false;
//	}
//	return connectionPppStruct.rxData[numConnect].rxBufferLen;
//}
//
//uint16_t gsmPPP_ReadRxData(uint8_t numConnect, uint8_t **ppData) {
//	if(pppState != ppp_ready_work) {
//		DBGInfo("GSMPPP: CONNECT ERROR - PPP closed");
//		return false;
//	}
//	if(connectionPppStruct.rxData[numConnect].rxBufferLen != 0) {
//		*ppData = (uint8_t *) connectionPppStruct.rxData[numConnect].rxBuffer;
//		uint16_t retLen = connectionPppStruct.rxData[numConnect].rxBufferLen;
//		connectionPppStruct.rxData[numConnect].rxBufferLen = 0;
//		return retLen;
//	}
//	return false;
//}
//
//err_t tcp_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err) {
//	for(uint8_t i=0; i<SERVERS_COUNT; i++) {
//		if(tpcb == connectionPppStruct.tcpClient[i]) {
//			DBGInfo("GSMPPP: connected (callback)%s", inet_ntoa(tpcb->local_ip.addr));
//			xSemaphoreGive(connectionPppStruct.semphr[i]);
//			break;
//		}
//	}
//	return ERR_OK;
//}
//
//err_t server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
//	LWIP_UNUSED_ARG(arg);
//
//	if(err == ERR_OK && p != NULL) {
//		tcp_recved(pcb, p->tot_len);
//		DBGInfo("GSMPPP:server_recv(): pbuf->len %d byte [%s]", p->len, inet_ntoa(pcb->remote_ip.addr));
//
//		for(uint8_t i=0; i<SERVERS_COUNT; i++) {
//			if(pcb->remote_ip.addr == connectionPppStruct.tcpClient[i]->remote_ip.addr) {
//				DBGInfo("GSMPPP: server_recv (callback) [%s]", inet_ntoa(pcb->remote_ip.addr));
//				if(p->len < sizeof(connectionPppStruct.rxData[i].rxBuffer)) {
//					memcpy(connectionPppStruct.rxData[i].rxBuffer, p->payload, p->len);
//					connectionPppStruct.rxData[i].rxBufferLen = p->len;
//					xSemaphoreGive(connectionPppStruct.rxData[i].rxSemh);
//					DBGInfo("GSMPPP: server_recv (callback) GIVE SEMPH[%s][%d]", inet_ntoa(pcb->remote_ip.addr), p->len);
//				} else {
//					DBGInfo("GSMPPP: server_recv p->len > sizeof(buf) -ERROR");
//				}
//			}
//		}
//		pbuf_free(p);
//	} else {
//		DBGInfo("\nserver_recv(): Errors-> ");
//		if (err != ERR_OK)
//			DBGInfo("1) Connection is not on ERR_OK state, but in %d state->\n", err);
//		if (p == NULL)
//			DBGInfo("2) Pbuf pointer p is a NULL pointer->\n ");
//		DBGInfo("server_recv(): Closing server-side connection...");
//		pbuf_free(p);
//		server_close(pcb);
//	}
//	return ERR_OK;
//}
//
//static void server_close(struct tcp_pcb *pcb) {
//	if(pcb->state != CLOSED) {
//		tcp_arg(pcb, NULL);
//		tcp_sent(pcb, NULL);
//		tcp_recv(pcb, NULL);
//		while(tcp_close(pcb) != ERR_OK) {
//			vTaskDelay(100/portTICK_PERIOD_MS);
//		}
//	}
//	for(uint8_t i=0; i<SERVERS_COUNT; i++) {
//		if(pcb == connectionPppStruct.tcpClient[i]) {
//			DBGInfo("GSMPPP: server_close (callback)%s", inet_ntoa(pcb->local_ip.addr));
//			connectionPppStruct.connected[i] = false;
//			connectionPppStruct.tcpClient[i] = NULL;
//		}
//	}
//}
//
//void udp_dns_echo_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, u16_t port) {
//	if (p != NULL) {
//		DBGInfo("GSMM: udp echo dns -OK!");
//		pbuf_free(p);
//	}
//}

/* Private functions ---------------------------------------------------------*/
static void tcpip_init_done(void * arg) {
	if (arg != NULL) {
		*((int *) arg) = 1;
	}
}
