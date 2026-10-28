#ifndef K_TASK_H
#define K_TASK_H

#include "common.h"

#define SHPR2 (*((volatile uint32_t*)0xE000ED1C))//SVC is bits 31-28
#define SHPR3 (*((volatile uint32_t*)0xE000ED20))//SysTick is bits 31-28, and PendSV is bits 23-20

task_t osGetTID(void);
void osYield(void);
void osPeriodYield();

#endif
