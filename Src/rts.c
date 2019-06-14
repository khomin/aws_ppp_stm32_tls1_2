#include "rts.h"
/*---------------------------------------------------------------------------------------------------------------------+
| global variables
+---------------------------------------------------------------------------------------------------------------------*/
 
volatile uint16_t timxOverflowCount;
TIM_HandleTypeDef   RTS_TIMx_Handle;
/*---------------------------------------------------------------------------------------------------------------------+
| global functions
+---------------------------------------------------------------------------------------------------------------------*/

void resetRunTimeCounter(void)
{
	timxOverflowCount = 0;
	RTS_TIMx_Handle.Instance->CNT = 0;
}
 
uint32_t GetRunTimeStatsTimer(void)
{
	return ((timxOverflowCount << 16) | RTS_TIMx_Handle.Instance->CNT);
}
 
/**
 * \brief Configures TIMx for runtime stats.
 *
 * Configures TIMx for runtime stats.
 */
 
void ConfigureRuntimeStatsTimer(void)//TimerForRuntimeStats(void)
{
	RTS_TIMx_CLK_ENABLE();
	RTS_TIMx_Handle.Instance = RTS_TIMx;
	RTS_TIMx_Handle.Init.Period = 0xFFFF; //
	RTS_TIMx_Handle.Init.Prescaler = ((SystemCoreClock)/10000) - 1; // for System clock at 120MHz, TIM6 runs at 1MHz
	RTS_TIMx_Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	RTS_TIMx_Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
	HAL_TIM_Base_Init(&RTS_TIMx_Handle);

	HAL_NVIC_SetPriority(RTS_TIMx_IRQn, RTS_TIMx_IRQ_PRIORITY, 0);
	HAL_NVIC_EnableIRQ(RTS_TIMx_IRQn);

	if (HAL_TIM_Base_Start_IT(&RTS_TIMx_Handle) != HAL_OK) {
			/* Starting Error */
	}
}
 
/*---------------------------------------------------------------------------------------------------------------------+
| ISRs
+---------------------------------------------------------------------------------------------------------------------*/
 
/**
 * \brief TIMx_IRQHandler
 *
 * TIMx_IRQHandler
 */
 
void RTS_TIMx_IRQHandler(void)
{
	timxOverflowCount++;
	RTS_TIMx_Handle.Instance->SR = 0;
}
