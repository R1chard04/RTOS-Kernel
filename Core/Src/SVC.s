.syntax unified
.cpu cortex-m4
.thumb

.global SVC_Handler_Main
.global SVC_Handler
.global INIT_THREAD
.global PendSV_Handler
.global osSched
.global DO_NOTHING

.thumb_func
DO_NOTHING:
	BX LR

.thumb_func
SVC_Handler:
  TST lr, #4
  ITE EQ
  MRSEQ r0, MSP
  MRSNE r0, PSP
  B SVC_Handler_Main


.thumb_func
 INIT_THREAD:
 	MRS 	r0, PSP
 	LDMIA 	r0!, {r4-r11}
 	MSR		PSP, r0

 	MOV LR, #0xFFFFFFFD

    BX      LR


.thumb_func
PendSV_Handler:

	# do XPSR
	# do PC
	# do LR, don't forget to decrement r0 by 4 to point to next memory address

	# Hardware saves 8 registers

	MRS r0, PSP
	STMDB r0!, {r4 - r11}
	MSR	PSP, r0

	# Value returned from osSched will be in r0, passing in stack high of current task through r0
	BL osSched

 	LDMIA 	r0!, {r4-r11}
 	MSR		PSP, r0

 	MOV LR, #0xFFFFFFFD

 	BX LR



