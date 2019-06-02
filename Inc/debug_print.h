/* DEBUG macros */
#ifndef __DEBUG_PRINT_H_
#define __DEBUG_PRINT_H_

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef  DEBUG
#define DEBUG_LEVEL 3
#else
#define DEBUG_LEVEL 1
#endif

#define DEBUGSTR(x)         printf(x) 
#define DEBUGOUT(...)				printf(__VA_ARGS__)

#if (DEBUG_LEVEL > 0)
#define  DBGErr(...)        printf("ERROR: ") ;\
														printf(__VA_ARGS__);\
                            printf("\n");
#else
#define DBGErr(...)
#endif


#if (DEBUG_LEVEL > 1)

#define  DBGLog(...)   printf("DEBUG: ") ;\
                            printf(__VA_ARGS__);\
                            printf("\n");
#else
#define DBGLog(...)
#endif


#if (DEBUG_LEVEL > 2)
#define  DBGInfo(...)   printf("INFO: ") ;\
                            printf(__VA_ARGS__);\
                            printf("\n");
#else
#define DBGInfo(...)
#endif

#if (DEBUG_LEVEL > 2)
#define  DBGLwiIP(...)   printf("LwIP: ") ;\
                            printf(__VA_ARGS__);\
                            printf("\n");
#else
#define DBGLwiIP(...)
#endif

void ITM_Init(uint32_t SWOSpeed);
bool ITM_WaitReady(uint8_t timeout);
int8_t ITM_WriteByte(uint8_t byte);
void ITM_SendString(uint8_t * p_tbuf);
int sendChar(int c);

#endif

