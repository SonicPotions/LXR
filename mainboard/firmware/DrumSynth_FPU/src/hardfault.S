/*
 * hardfault.S
 *
 *  Created on: 24.08.2012
 *      Author: Julian
 * see http://blog.frankvh.com/2011/12/07/cortex-m3-m4-hard-fault-handler/
 */

/*
.syntax unified
.cpu cortex-m4
.thumb

.global HardFault_Handler
.extern hard_fault_handler_c

HardFault_Handler:
  TST LR, #4
  ITE EQ
  MRSEQ R0, MSP
  MRSNE R0, PSP
  B hard_fault_handler_c

*/
