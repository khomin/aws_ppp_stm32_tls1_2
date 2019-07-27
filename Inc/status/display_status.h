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

extern const char caption_display_init_system[];
extern const char caption_display_init_gsm[];
extern const char caption_display_init_ppp[];
extern const char caption_display_connecting_ppp[];
extern const char caption_display_connecting_mqtt[];
extern const char caption_display_mqtt_ready_publish[];
extern const char caption_display_fpga_rx_data[];
extern const char caption_display_fpga_rx_complete[];
extern const char caption_display_mqtt_send_data[];
extern const char caption_display_mqtt_send_total[];
extern const char caption_display_mqtt_send_error[];
extern const char caption_display_mqtt_certs_error[];
extern char caption_temp_buff[128];

void setDisplayStatus(char* pCaption);

#endif /* STATUS_DISPLAY_STATUS_H_ */
