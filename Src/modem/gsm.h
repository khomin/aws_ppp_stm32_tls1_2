#ifndef __GSM_H
#define __GSM_H

#include <stdint.h>
#include "gsmLLR.h"

#define MAX_SRV_ADDR_LEN						64
#define MAX_SRV_PASS_LEN						16
#define MAX_GPRS_APN_LEN						64
#define MAX_GPRS_USER_LEN						16
#define MAX_GPRS_PASS_LEN						16

void gsmTaskInit(void);

#endif /* __GSM_DRIVER_H */
