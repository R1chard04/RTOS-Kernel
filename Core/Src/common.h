#ifndef COMMON_H
#define COMMON_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#define TID_NULL 0 			// predefined Task ID for the NULL task
#define MAX_TASKS 16 		// maximum number of tasks in the system
#define STACK_SIZE 0x200 	// min. size of each task’s stack
#define DORMANT 0 			// state of terminated task
#define READY 1 			// state of task that can be scheduled but is not running
#define RUNNING 2 			// state of running task
#define SLEEPING 3 			// state of sleeping task
#define RTX_OK true
#define RTX_ERR false
#define KERNEL_ID 100

typedef unsigned int U32;
typedef unsigned short U16;
typedef char U8;
typedef unsigned int task_t;


typedef struct free_list_node {
  uint32_t size;
  task_t owner;
  bool free;
  struct free_list_node* next;
} free_list_node;


typedef struct task_control_block{
	void (*ptask)(void* args); //entry address
	U32 stack_high; //starting address of stack (high address)
	U32 stack_low;
	task_t tid; //task ID
	U8 state; //task's state
	U16 stack_size; //stack size. Must be a multiple of 8
	U32 time_remaining;
	U32 deadline;
} TCB;

#endif
