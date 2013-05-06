/*
 * TriggerOut.h
 *
 *  Created on: 06.05.2013
 *      Author: Julian
 */

#ifndef TRIGGEROUT_H_
#define TRIGGEROUT_H_

#include "globals.h"

#define PIN_TRACK_1 GPIO_Pin_8	//Port D
#define PIN_TRACK_2 GPIO_Pin_9	//Port D
#define PIN_TRACK_3 GPIO_Pin_10	//Port D
#define PIN_TRACK_4 GPIO_Pin_11	//Port D
#define PIN_TRACK_5 GPIO_Pin_12	//Port D
#define PIN_TRACK_6 GPIO_Pin_13	//Port D
#define PIN_TRACK_7 GPIO_Pin_14	//Port D

#define PIN_CLOCK_1 GPIO_Pin_15	//Port D
#define PIN_CLOCK_2 GPIO_Pin_9	//Port A
#define PIN_RESET GPIO_Pin_10	//Port A

#define PULSE_LENGTH 20 //[ms]

enum
{
	TRIGGER_1 = 0,
	TRIGGER_2,
	TRIGGER_3,
	TRIGGER_4,
	TRIGGER_5,
	TRIGGER_6,
	TRIGGER_7,

	CLOCK_1,
	CLOCK_2,
	TRIGGER_RESET,
	NUM_PINS,
};

extern uint8_t trigger_dividerClock1;
extern uint8_t trigger_dividerClock2;

void trigger_init();
void trigger_tick();
void trigger_triggerVoice(uint8_t voice);
void trigger_clockTick();
void trigger_reset(uint8_t value);


#endif /* TRIGGEROUT_H_ */
