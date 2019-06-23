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
#include "queue.h"
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
#include "../Common/AWS/Inc/net.h"
#include "cloud.h"
#include "aws_dev_mode_key_provisioning.h"
#include "aws_hello_world.h"
#include "./aws_system_init.h"
#include "net_internal.h"

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

static xQueueHandle gsmPppUartTranmitQueue;

//-- static functions
static void gsmPPP_rawInput(void *pvParamters);
static void gsmPPP_rawOutput(void *pvParamters);

void gsmPPP_Tsk(void *pvParamter);

static void tcpip_init_done(void * arg);
static void status_cb(ppp_pcb *pcb, int err_code, void *ctx);
static void ctx_cb_callback(void *p);
static u32_t output_cb(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx);

#define TRANSMIT_QUEUE_BUFF_LEN	1128

typedef struct {
	uint8_t data[TRANSMIT_QUEUE_BUFF_LEN];
	uint16_t len;
}sTransmitQueue;

bool gsmPPP_Init(void) {
	xTaskCreate(gsmPPP_Tsk, "gsmPPP_Tsk", 6128, 0, tskIDLE_PRIORITY+1, NULL);

	gsmPppUartTranmitQueue = xQueueCreate(12, sizeof(sTransmitQueue*));

	return true;
}

void gsmPPP_Tsk(void *pvParamter) {
	uint8_t setup = 0;
	tcpip_init(tcpip_init_done, &setup);

	xTaskCreate(gsmPPP_rawInput, "pppRxDataRx", 1024, 0, tskIDLE_PRIORITY, NULL);
	xTaskCreate(gsmPPP_rawOutput, "pppRxDataTx", 1024, 0, tskIDLE_PRIORITY, NULL);

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

static u8_t tbuf[TRANSMIT_QUEUE_BUFF_LEN] = {0};
static u16_t tbuf_len = 0;

//----------------------------
//--	private functions
//----------------------------
/* PPP over Serial: this is the input function to be called for received data. */
void gsmPPP_rawInput(void *pvParamter) {
	vTaskDelay(1000/portTICK_RATE_MS);
	while(1) {
		if(uartParcerStruct.ppp.pppModeEnable) {
			//--- receive
			while(xQueueReceive(uartParcerStruct.uart.rxQueue, &tbuf[tbuf_len], 2/portTICK_PERIOD_MS) == pdTRUE) {
				tbuf_len++;
				if(tbuf_len >=  sizeof(tbuf)-1) {
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

void gsmPPP_rawOutput(void *pvParamter) {
	sTransmitQueue * p = NULL;
	vTaskDelay(1000/portTICK_RATE_MS);
	while(1) {
		if(uartParcerStruct.ppp.pppModeEnable) {
			//--- transmit
			while(xQueueReceive(gsmPppUartTranmitQueue, &p, portMAX_DELAY) == pdTRUE) {
				if(p != NULL) {
					HAL_StatusTypeDef result;
					do {
						result = HAL_UART_Transmit_IT(&huart3, p->data, p->len);
					} while(result != HAL_OK);
					DBGInfo("PPP: out_raw %s", (char*)p->data);
					free(p);
				}
			}
		} else {
			vTaskDelay(10/portTICK_RATE_MS);
		}
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
	sTransmitQueue *p = malloc(sizeof(sTransmitQueue));
	memcpy(p->data, data, len);
	p->len = len;

	if(len >= TRANSMIT_QUEUE_BUFF_LEN) {
		DBGErr("output_cb -buffer overflow");
	}

	if(xQueueSend(gsmPppUartTranmitQueue, &p, portMAX_DELAY) == pdTRUE) {
		retLen = len;
	} else {
		DBGErr("output_cb -send data to queue -ERROR (queue full");
	}
	return retLen;
}

void ctx_cb_callback(void *p) {}

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

/* Private functions ---------------------------------------------------------*/
static void tcpip_init_done(void * arg) {
	if (arg != NULL) {
		*((int *) arg) = 1;
	}
}
