/*---------------------------------------------------------------------------------------------------------------------+
| Runtime stats configuration
+---------------------------------------------------------------------------------------------------------------------*/
#ifndef _RTS_H_
#define _RTS_H_
#ifdef __cplusplus
 extern "C" {
#endif 
#include "stm32f4xx_hal.h"
#include "stdint.h"

#define RTS_TIMx_CLK_ENABLE __TIM6_CLK_ENABLE
#define RTS_TIMx 						TIM6
#define RTS_TIMx_IRQn       TIM6_DAC_IRQn
#define RTS_TIMx_IRQHandler TIM6_DAC_IRQHandler
#define RTS_TIMx_IRQ_PRIORITY   15

void     ConfigureRuntimeStatsTimer(void);
uint32_t GetRunTimeStatsTimer(void);
void     resetRunTimeCounter(void);
	 
#ifdef __cplusplus
 }
#endif 
#endif
