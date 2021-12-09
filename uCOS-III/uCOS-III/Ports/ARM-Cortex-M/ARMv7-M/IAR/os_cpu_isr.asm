;********************************************************************************************************
;                                              uC/OS-III
;                                        The Real-Time Kernel
;
;                    Copyright 2009-2020 Silicon Laboratories Inc. www.silabs.com
;
;                                 SPDX-License-Identifier: APACHE-2.0
;
;               This software is subject to an open source license and is distributed by
;                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
;                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
;
;********************************************************************************************************

;********************************************************************************************************
;                                          designed by eric2013
;********************************************************************************************************
    RSEG CODE:CODE:NOROOT(2)
    THUMB
		
		
	EXTERN  OS_CPU_PendSVHandler
	EXTERN  OS_CPU_SysTickHandler
		
		
	PUBLIC PendSV_Handler
PendSV_Handler
    B OS_CPU_PendSVHandler
	B .
	
	PUBLIC SysTick_Handler
SysTick_Handler
    B OS_CPU_SysTickHandler
	B .
		
	END
		