/*
 * globals.h
 *
 *  Created on: 11.07.2012
 *      Author: Julian
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "stm32f4xx.h"

extern volatile uint32_t systick_ticks;

//messages
#define TIMEOUT 			-1
#define START_BOOTLOADER	1
#define START_PROGRAM		0

//state machine
#define STATE_FINISHED 		0
#define STATE_WAIT_CMD		3
#define STATE_CMD_RECEIVED	2
//Config
#define INIT_TIMEOUT 3000 //time to wait in [ms] for INIT_BOOTLOADER message after reset


#endif /* GLOBALS_H_ */
