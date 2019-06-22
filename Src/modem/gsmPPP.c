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
#include "task.h"
#include "stats.h"
#include "dns.h"
#include "lwip/tcpip.h"
#include "ip_addr.h"
#include "netif/ppp/pppos.h"
#include "netif/ppp/pppapi.h"
#include "netif/ppp/pppoe.h"
#include "netif/ppp/pppol2tp.h"
#include "aws_system_init.h"
#include "net.h"
#include "cloud.h"

extern xSemaphoreHandle sioWriteSemaphore;
extern sGsmUartParcer uartParcerStruct;
extern UART_HandleTypeDef huart3;
extern sGsmState gsmState;

ppp_pcb *ppp;
struct netif ppp_netif;
extern net_hnd_t hnet;

//-- destination address
extern sConnectSettings connectSettings;

//-- security structure
sConnectionPppStruct connectionPppStruct = {0};

//-- current state
ePppState pppState = ppp_not_inited;

extern struct stats_ lwip_stats;
// static functions
static void tcpip_init_done(void * arg);
static void status_cb(ppp_pcb *pcb, int err_code, void *ctx);
static void ctx_cb_callback(void *p);
static u32_t output_cb(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx);

static err_t tcp_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err);
static void dns_server_is_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg);
static err_t server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
static err_t server_err(void *arg, err_t err);
static void server_close(struct tcp_pcb *pcb);
//void udp_dns_echo_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, u16_t port);

static void gsmPPP_rawInput(void *pvParamter);

static void prvWifiConnect( void );

void gsmPPP_Tsk(void *pvParamter);

bool gsmPPP_Init(void) {
	xTaskCreate(gsmPPP_Tsk, "gsmPPP_Tsk", 4096, 0, tskIDLE_PRIORITY, NULL);
	return true;
}

#include "aws_dev_mode_key_provisioning.h"
#include "aws_hello_world.h"
#include "./aws_system_init.h"
#include "net_internal.h"

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
			DBGLog("AWS PPP: module initialized.\r\n");

			platform_init();
			subscribe_publish_sensor_values();
			platform_deinit();

			while(1) {
				vTaskDelay(500/portTICK_RATE_MS);
			}
		}

		vTaskDelay(1000/portTICK_RATE_MS);
	}
}

//----------------------------
//--	private functions
//----------------------------

bool gsmPPP_Connect(uint8_t* destIp, uint16_t port) {
	if(pppState != ppp_ready_work) {
		DBGInfo("GSMPPP: gsmPPP_Connect - ppp no ready");
		return false;
	}

	// TODO: test
	//	destIp[0] = 31;
	//	destIp[1] = 10;
	//	destIp[2] = 4;
	//	destIp[3] = 146;
	//	port = 45454;

	IP4_ADDR(&connectionPppStruct.ipRemoteAddr, destIp[0], destIp[1], destIp[2], destIp[3]);
	DBGInfo("GSMPPP: connect without dns [%d.%d.%d.%d|%d]... ", destIp[0], destIp[1], destIp[2], destIp[3], port);

	if(connectionPppStruct.connected == false) {
		if(connectionPppStruct.tcpClient == NULL) {
			connectionPppStruct.tcpClient = tcp_new();
		}
		tcp_recv(connectionPppStruct.tcpClient, server_recv);

		tcp_connect(connectionPppStruct.tcpClient, &connectionPppStruct.ipRemoteAddr, port, &tcp_connected_cb);

		if(xSemaphoreTake(connectionPppStruct.semphr, 10000/portTICK_PERIOD_MS) == pdTRUE) {
			connectionPppStruct.connected = true;
			DBGInfo("GSMPPP: connected %s", inet_ntoa(connectionPppStruct.ipRemoteAddr));
			return true;
		} else {
			DBGInfo("GSMPPP: connect timeout -error");
			return false;
		}
	} else {
		if(gsmLLR_ConnectServiceStatus(0) == eOk) {
			DBGInfo("GSMPPP: connect -already connected %s", inet_ntoa(connectionPppStruct.ipRemoteAddr));
			return true;
		} else {
			DBGInfo("GSMPPP: connect -close");
			return false;
		}
	}
	return false;
}

bool gsmPPP_Disconnect(uint8_t numConnect) {
	if(pppState != ppp_ready_work) {
		DBGInfo("GSMPPP: gsmPPP_Disconnect - no connected");
		return false;
	}
	if(connectionPppStruct.tcpClient == NULL) {
		return false;
	}
	server_close(connectionPppStruct.tcpClient);
	connectionPppStruct.connected = false;
	return true;
}


bool gsmPPP_SendData(uint8_t numConnect, uint8_t *pData, uint16_t len) {
	bool res = false;
	if(pppState != ppp_ready_work) {
		DBGInfo("GSMPPP: gsmPPP_SendData - no connected");
		return false;
	}
	if(tcp_write(connectionPppStruct.tcpClient, (void*)pData, len, NULL) == ERR_OK) {
		DBGInfo("GSMPPP: gsmPPP_SendData -ok [len=%d]", len);
		res = true;
	} else {
		DBGInfo("GSMPPP: gsmPPP_SendData -err [len=%d]", len);
		server_close(connectionPppStruct.tcpClient);
		connectionPppStruct.connected = false;
		connectionPppStruct.rxData.rxBufferLen = 0;
		memset(connectionPppStruct.rxData.rxBuffer,0, sizeof(connectionPppStruct.rxData.rxBuffer));
	}
	return res;
}

static bool dns_is_found = false;
static ip_addr_t dns_found_ipaddr;

sGetDnsResult getIpByDns(const char *pDnsName, uint8_t len) {
	sGetDnsResult result;
	result.isValid = false;

	// TODO: test
	//	ulIPAddres = 0x92040a1f;

	if(pppState == ppp_ready_work) {
		dns_is_found = false;

		switch(dns_gethostbyname(pDnsName, &result.resolved, dns_server_is_found_cb, NULL)) {
		case ERR_OK: // numeric or cached, returned in resolved
			DBGInfo("GSM-PPP: dns -ERR_OK");
			dns_is_found = true;
			break;
			// need to ask, will return data via callback
		case ERR_INPROGRESS: {
			DBGInfo("GSM-PPP: dns -ERR_INPROGRESS");
			uint8_t time_out = 0;
			while(dns_is_found == false) {
				time_out ++;
				vTaskDelay(500/portTICK_RATE_MS);
				if(time_out >= 10) {
					break;
				}
			}
			break;
		}}
	}

	if(dns_is_found) {
		result.resolved.addr = dns_found_ipaddr.addr;
		result.isValid = true;
	}

	return result;
}


static void dns_server_is_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
	if ((ipaddr) && (ipaddr->addr)) {
		dns_found_ipaddr.addr = ipaddr->addr;
		dns_is_found = true;
	}
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

		DBGInfo("PPP: status_cb: Unable to open PPP session");
		pppState = ppp_ready_work;
		net_ctxt_t *ctxt = (net_ctxt_t *)hnet;
		ctxt->lwip_netif = &ppp_netif;
		break;
	}
	case PPPERR_PARAM: {
		DBGInfo("PPP: status_cb: Invalid parameter");
		break;
	}
	case PPPERR_OPEN: {
		DBGInfo("PPP: status_cb: Unable to open PPP session");
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

/* PPP over Serial: this is the input function to be called for received data. */
void gsmPPP_rawInput(void *pvParamter) {
	static u8_t tbuf[512] = {0};
	static u16_t tbuf_len = 0;
	vTaskDelay(1000/portTICK_RATE_MS);
	while(1) {
		if(uartParcerStruct.ppp.pppModeEnable) {
			while(xQueueReceive(uartParcerStruct.uart.rxQueue, &tbuf[tbuf_len], 2/portTICK_PERIOD_MS) == pdTRUE) {
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


uint16_t gsmPPP_GetRxLenData() {
	if(pppState != ppp_ready_work) {
		DBGInfo("GSMPPP: gsmPPP_GetRxLenData - no connected");
		return false;
	}
	return connectionPppStruct.rxData.rxBufferLen;
}

uint16_t gsmPPP_ReadRxData(uint8_t *ppData, uint16_t maxLen, uint32_t timeout) {
	uint16_t ret = 0;
	if(pppState == ppp_ready_work) {
		vTaskDelay(timeout * 1000/portTICK_PERIOD_MS);
		DBGInfo("GSMPPP: gsmPPP_ReadRxData - timeout...");
		if(connectionPppStruct.rxData.rxBufferLen != 0) {
			if(connectionPppStruct.rxData.rxBufferLen < maxLen) {
				memcpy(ppData, connectionPppStruct.rxData.rxBuffer, connectionPppStruct.rxData.rxBufferLen);
				uint16_t retLen = connectionPppStruct.rxData.rxBufferLen;
				return retLen;
			} else {
				DBGInfo("GSMPPP: gsmPPP_ReadRxData - rxData > max");
			}
		}
	} else {
		DBGInfo("GSMPPP: gsmPPP_ReadRxData - no connected");
	}

	connectionPppStruct.rxData.rxBufferLen = 0;

	return ret;
}

err_t tcp_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err) {
	if(tpcb == connectionPppStruct.tcpClient) {
		DBGInfo("GSMPPP: connected (callback)%s", inet_ntoa(tpcb->local_ip.addr));
		xSemaphoreGive(connectionPppStruct.semphr);
	}
	return ERR_OK;
}

err_t server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
	LWIP_UNUSED_ARG(arg);

	if(err == ERR_OK && p != NULL) {
		tcp_recved(pcb, p->tot_len);
		DBGInfo("GSMPPP:server_recv(): pbuf->len %d byte [%s]", p->len, inet_ntoa(pcb->remote_ip.addr));

		if(pcb->remote_ip.addr == connectionPppStruct.tcpClient->remote_ip.addr) {
			DBGInfo("GSMPPP: server_recv (callback) [%s]", inet_ntoa(pcb->remote_ip.addr));
			if(p->len < sizeof(connectionPppStruct.rxData.rxBuffer)) {
				memcpy(connectionPppStruct.rxData.rxBuffer, p->payload, p->len);
				connectionPppStruct.rxData.rxBufferLen = p->len;
				xSemaphoreGive(connectionPppStruct.rxData.rxSemh);
				DBGInfo("GSMPPP: server_recv (callback) GIVE SEMPH[%s][%d]", inet_ntoa(pcb->remote_ip.addr), p->len);
			} else {
				DBGInfo("GSMPPP: server_recv p->len > sizeof(buf) -ERROR");
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
	if(pcb == connectionPppStruct.tcpClient) {
		DBGInfo("GSMPPP: server_close (callback)%s", inet_ntoa(pcb->remote_ip.addr));
		connectionPppStruct.connected = false;
		connectionPppStruct.tcpClient = NULL;
	}
}

/* Private functions ---------------------------------------------------------*/
static void tcpip_init_done(void * arg) {
	if (arg != NULL) {
		*((int *) arg) = 1;
	}
}
