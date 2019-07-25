/*
 * display_status.h
 *
 *  Created on: Jul 7, 2019
 *      Author: khomin
 */

#ifndef STATUS_DISPLAY_STATUS_H_
#define STATUS_DISPLAY_STATUS_H_

#include <stdint.h>
#include <string.h>

typedef enum {
	E_Status_Display_init,
	E_Status_Display_init_GSM,
	E_Status_Display_init_PPP,
	E_Status_Display_wait_unitl_connect,
	E_Status_Display_ready_send,
	E_Status_Display_connected,
	E_Status_Display_connected_and_send,
	E_Status_Display_connect_lost,
	E_Status_Display_cert_empty,
}eStatusDisplayTypes;

void setDisplayStatus(eStatusDisplayTypes status);

#endif /* STATUS_DISPLAY_STATUS_H_ */
