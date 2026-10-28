#include <stdio.h>
#include <stdint.h>
#include "main.h"
#include "k_task.h"
#include "k_mem.h"
#include "common.h"

extern void INIT_THREAD(void);
extern void SWITCH_CONTEXT(void);
extern void DO_NOTHING(void);

int thread_counter = 0;
const  int create_thread = 1;
unsigned int svc_number;
int MAIN_STACK_SIZE = 0x400;
int THREAD_STACK_SIZE = 0x400;
int MAX_STACK_SIZE = 0x4000;
uint32_t *stackptr;
int taskBitMask = 0;

uint32_t* MSP_INIT_VAL; // should cast to integer for arithmetic operations

TCB *active_task;
TCB task_arr[MAX_TASKS];

// 0: Kernel has not been initialized
// 1: Kernel has been initialized
// 2: Kernel has been started
int kernel_state = 0;

void null_task(void) {
	while (1) {
		osYield();
	}
}


void osKernelInit(){
	DO_NOTHING();
	__asm("SVC #1");
}

void loadTaskStack(int taskId) {
	uint32_t* stack_low = (uint32_t*)k_mem_alloc(task_arr[taskId].stack_size);
	
	if (!stack_low) {
		stackptr = NULL;
		return;
	}

	free_list_node* node = (char *)stack_low - sizeof(free_list_node);
	node->owner = taskId;

	stackptr = (uint32_t *)((uint32_t)stack_low + (uint32_t)task_arr[taskId].stack_size);
	task_arr[taskId].stack_low = (uint32_t)stack_low;

	*(--stackptr) = 1<<24; //This is xPSR, should be Thumb mode
	*(--stackptr) = (uint32_t)task_arr[taskId].ptask; //the function name

	for (int i = 0; i < 14; ++i) { // hardware saves other registers
		*(--stackptr) = 0xA;
	}

}
	

int osCreateTask(TCB* task){
	task_t newTaskId = -1;

	for(int i = 1; i < MAX_TASKS; ++i) {
		if((taskBitMask & (1 << i)) == 0) {
			newTaskId = i;
			break;
		}
	}

	if(newTaskId == -1) {
		return RTX_ERR;
	} else {

		if(task->stack_size < STACK_SIZE) {
			return RTX_ERR;
		}

		// initialize task
		task->tid= newTaskId;
		task->state = READY;

		task_arr[newTaskId].ptask = task->ptask;
		task_arr[newTaskId].stack_size = task->stack_size;
		task_arr[newTaskId].tid = task->tid;
		task_arr[newTaskId].state = task->state;
		task_arr[newTaskId].time_remaining = 5;
		task_arr[newTaskId].deadline = 5;

		loadTaskStack(newTaskId);

		if (!stackptr)
			return RTX_ERR;

		task_arr[newTaskId].stack_high = stackptr;

		taskBitMask |= 1 << newTaskId;

		return RTX_OK;
	}
}

int osKernelStart(){
	if (!kernel_state || kernel_state == 2) {
		return RTX_ERR;
	}

	__asm("SVC #3");
}

uint32_t osSched(U32 curr_stack_high) {
	if (active_task) {
		active_task->stack_high = curr_stack_high;
	}

	if (active_task && active_task->state == RUNNING)
		active_task->state = READY;

	int curr = active_task ? active_task->tid%15 + 1 : 1;
	int minTaskId = 0;

	for (int i = 1; i < MAX_TASKS; ++i) {
		if ((taskBitMask & (1 << curr)) && task_arr[curr].state == READY) {
			if (minTaskId == 0) {
				minTaskId = curr;
			} else if (task_arr[curr].time_remaining < task_arr[minTaskId].time_remaining) {
				minTaskId = curr;
			}
		}

		curr = curr%15 + 1;
	}

	active_task = &task_arr[minTaskId];
	active_task->state = RUNNING;

	return active_task->stack_high;
}


task_t osGetTID(void) {
	if (active_task) return active_task->tid;
	return KERNEL_ID;
}

void osYield(void){
	DO_NOTHING();
	__asm("SVC #2");
}

int osTaskInfo(task_t TID, TCB* task_copy){
	for (int i = 0; i < MAX_TASKS; ++i) {
		if ((taskBitMask & (1 << i)) && (task_arr[i].tid == TID)) {
			task_copy->state = task_arr[i].state;
			task_copy->tid = task_arr[i].tid;
			task_copy->ptask = task_arr[i].ptask;
			task_copy->stack_high = task_arr[i].stack_high;
			task_copy->stack_size = task_arr[i].stack_size;
			return RTX_OK;
		}
	}
	return RTX_ERR;
}

int osTaskExit(void){
	task_t task_id = osGetTID();

	if (task_id == KERNEL_ID) {
		return RTX_ERR;
	}

	task_arr[task_id].state = DORMANT;
	taskBitMask ^= (1 << task_id);

	k_mem_dealloc(active_task->stack_low);
	active_task = NULL;

	osYield();
}


void SVC_Handler_Main( unsigned int *svc_args )
{

  /*
  * Stack contains:
  * r0, r1, r2, r3, r12, r14, the return address and xPSR
  * First argument (r0) is svc_args[0]
  */
  svc_number = ( ( char * )svc_args[ 6 ] )[ -2 ] ;


  if (svc_number == 1) {
	  	HAL_SuspendTick();
		MSP_INIT_VAL = *(uint32_t**)0x0;
		active_task = NULL;
		kernel_state = 1;

		taskBitMask = 0;
		for (int i = 1; i < MAX_TASKS; ++i) {
			task_arr[i].state = DORMANT;
			task_arr[i].tid = i;
		}
		return;

  } else if (svc_number == 2) {
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		active_task->time_remaining = active_task->deadline;
  } else if (svc_number == 3) {

		kernel_state = 2;

		SHPR3 = (SHPR3 & ~(0xFFU << 24)) | (0xF0U << 24);//SysTick is lowest priority (highest number)
		SHPR3 = (SHPR3 & ~(0xFFU << 16)) | (0xE0U << 16);//PendSV is in the middle

		SHPR2 = (SHPR2 & ~(0xFFU << 24)) | (0xD0U << 24);//SVC is highest priority (lowest number)

		task_arr[0].ptask = (uint32_t)null_task;
		task_arr[0].stack_size = STACK_SIZE;
		task_arr[0].tid = TID_NULL;
		task_arr[0].state = READY;
		task_arr[0].deadline = 99999;
		task_arr[0].time_remaining = 99999;
		
		loadTaskStack(0);
		task_arr[0].stack_high = stackptr;

		int minTaskId = 1;
		for (unsigned int i = 1; i < MAX_TASKS; ++i) {
			if ((taskBitMask & (1 << i)) && task_arr[i].state == READY) {
				if (task_arr[i].deadline < task_arr[minTaskId].deadline) {
					minTaskId = i;
				}
			}
		}


		active_task = &task_arr[minTaskId];
		active_task->state = RUNNING;

		__set_PSP(active_task->stack_high);
		uwTick = 0;
		HAL_SetTickFreq(1);
		HAL_ResumeTick();
		INIT_THREAD();
  } else if (svc_number == 4) {
		active_task->state = SLEEPING;
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
  } else if (svc_number == 5) {

		active_task->state = SLEEPING;

		if (active_task->time_remaining >= active_task->deadline)
			active_task->time_remaining = active_task->deadline;

		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
  }
}

void SysTick_Handler(void) {
	HAL_IncTick();

	bool awake = false, active = false;
	for (int i = 1; i < MAX_TASKS; ++i) {
		if ((taskBitMask&(1 << i)) && task_arr[i].state != DORMANT) {
			--task_arr[i].time_remaining;

			if (!task_arr[i].time_remaining) {

				if (active_task == &task_arr[i])
					active = true;

				if (task_arr[i].state == SLEEPING) {
					task_arr[i].state = READY;
					awake = true;
				}

				task_arr[i].time_remaining = task_arr[i].deadline;
			}

		}
	}

	if (awake)
		for (int i = 0; i < 2500; ++i);

	if (active || awake)
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}


void osSleep(int timeInMs) {
	DO_NOTHING();
	active_task->time_remaining = timeInMs;
	__asm("SVC #4");
}

void osPeriodYield() {
	DO_NOTHING();
	__asm("SVC #5");
}

int osSetDeadline(int deadline, task_t TID) {
	HAL_SuspendTick();

	if (deadline > 0 && task_arr[TID].state == READY) {
		task_arr[TID].deadline = deadline;
		task_arr[TID].time_remaining = deadline;
		HAL_ResumeTick();
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		return RTX_OK;
	} else {
		HAL_ResumeTick();
		return RTX_ERR;
	}
}

int osCreateDeadlineTask(int deadline, TCB *task) {
	task_t newTaskId = -1;

	for(int i = 1; i < MAX_TASKS; ++i) {
		if((taskBitMask & (1 << i)) == 0) {
			newTaskId = i;
			break;
		}
	}

	if(newTaskId == -1) {
		return RTX_ERR;
	} else {

		if(task->stack_size < STACK_SIZE) {
			return RTX_ERR;
		}

		// initialize task
		task->tid= newTaskId;
		task->state = READY;

		task_arr[newTaskId].ptask = task->ptask;
		task_arr[newTaskId].stack_size = task->stack_size;
		task_arr[newTaskId].tid = task->tid;
		task_arr[newTaskId].state = task->state;
		task_arr[newTaskId].time_remaining = deadline;
		task_arr[newTaskId].deadline = deadline;

		loadTaskStack(newTaskId);

		if (!stackptr)
			return RTX_ERR;

		task_arr[newTaskId].stack_high = stackptr;

		taskBitMask |= 1 << newTaskId;

		return RTX_OK;
	}
}
