#include "main.h"
#include <stdio.h>
#include "common.h"
#include "k_task.h"
#include "k_mem.h"

int x = 0, y = 0;


void TaskA(void *) {
   for (int j = 0; j < 10; ++j){
	   for (int i = 0; i < 5; ++i);
	   osPeriodYield();
   }
}

void TaskB(void *) {
   for (int j = 0; j < 3; ++j){
	   for (int i = 0; i < 10; ++i);
	   osPeriodYield();
   }
}


int main(void)
{
	  HAL_Init();
	  /* Configure the system clock */
	  SystemClock_Config();

	  /* Initialize all configured peripherals */
	  MX_GPIO_Init();
	  MX_USART2_UART_Init();
	  /* MCU Configuration is now complete. Start writing your code below this line */

	  osKernelInit();
	  k_mem_init();

	  printf("\r\n-------------\r\n");

	  TCB st_mytask;
	    st_mytask.stack_size = STACK_SIZE;
	    st_mytask.ptask = &TaskA;
	    osCreateDeadlineTask(&st_mytask, 4);


	    st_mytask.ptask = &TaskB;
	    st_mytask.deadline = 11;
	    osCreateDeadlineTask(&st_mytask, 11);


	  osKernelStart();
	  while(1){}
}

