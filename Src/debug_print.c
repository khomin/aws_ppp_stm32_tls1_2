/*
 * debug_print.c
 *
 *  Created on: May 25, 2019
 *      Author: khomin
 */

#include "debug_print.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include "stdbool.h"

/* Private function prototypes -----------------------------------------------*/
#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE {
  /* Place your implementation of fputc here */
	sendChar(ch);
	return ch;
}

#ifdef 	ITM
#define ITM_Port8(n)    (*((volatile unsigned char *)(0xE0000000+4*n)))
#define ITM_Port16(n)   (*((volatile unsigned short*)(0xE0000000+4*n)))
#define ITM_Port32(n)   (*((volatile unsigned long *)(0xE0000000+4*n)))

#define DEMCR_          (*((volatile unsigned long *)(0xE000EDFC)))
#define TRCENA          0x01000000
#endif

void ITM_Init(uint32_t SWOSpeed) {
	//uint32_t SWOSpeed = 1000000; //1000kbps
	uint32_t SWOPrescaler =  (SystemCoreClock / (SWOSpeed * 1000)); // SWOSpeed in kHz
	CoreDebug->DEMCR = (1 << CoreDebug_DEMCR_TRCENA_Pos);
	DBGMCU->CR = 0x00000027; //Enabling TRACE_IOEN, DBG_STANDBY, DBG_STOP, DBG_SLEEP
	*((volatile unsigned *) 0xE00400F0) = 0x00000002; // "Selected PIN Protocol Register": Select which protocol to use for trace output (2: SWO)
	*((volatile unsigned *) 0xE0040010) = SWOPrescaler; // "Async Clock Prescaler Register". Scale the baud rate of the asynchronous output
	*((volatile unsigned *) 0xE0000FB0) = 0xC5ACCE55; // ITM Lock Access Register, C5ACCE55 enables more write access to Control Register 0xE00 :: 0xFFC
	*((volatile unsigned *) 0xE0000E80) = 0x0001000D; // ITM Trace Control Register
	*((volatile unsigned *) 0xE0000E40) = 0x0000000F; // ITM Trace Privilege Register
	*((volatile unsigned *) 0xE0000E00) = 0x00000001; // ITM Trace Enable Register. Enabled tracing on stimulus ports. One bit per stimulus port.
	*((volatile unsigned *) 0xE0001000) = 0x400003FE; // DWT_CTRL
	*((volatile unsigned *) 0xE0040304) = 0x00000100; // Formatter and Flush Control Register
}

bool ITM_WaitReady(uint8_t timeout)
{
	uint32_t timingdelay;
	timingdelay = HAL_GetTick() + timeout ;
	while( (ITM->PORT[0].u8 & 1) == 0)
	{
		if (HAL_GetTick() >= timingdelay)
		{
			return false;
		}
	}
	return true;
}

int8_t ITM_WriteByte(uint8_t byte) {
	uint8_t timeout=1;
	if (CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) // Trace enabled
	{
		if (ITM->TCR & ITM_TCR_ITMENA_Msk) // ITM enabled
		{
			if (ITM->TER & (1ul << 0)) // ITM Port #0 enabled
			{
				if (ITM_WaitReady(timeout)) {
					ITM->PORT[0].u8 = byte;
				}
			}
		}
	}
	return byte;
}

void ITM_SendString(uint8_t * p_tbuf) {
	volatile uint32_t time_out = 0;
	// Trace enabled
	if (CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) {
		// ITM enabled
		if (ITM->TCR & ITM_TCR_ITMENA_Msk) {
			// ITM Port #0 enabled
			if (ITM->TER & (1ul << 0)) {
				do {
					time_out = 0;
					while (ITM->PORT[0].u32 == 0) {
						time_out++;
						if (time_out > 2000) {
							return;
						}
					}
					ITM->PORT[0].u8 = *p_tbuf;
					p_tbuf++;
				} while (*p_tbuf != '\0');
			}
		}
	}
}

/**
 * @brief  Print a character on the HyperTerminal
 * @param  c: The character to be printed
 * @retval None
 */
int sendChar(int c) {
	ITM_WriteByte(c);
	return (c);
}
