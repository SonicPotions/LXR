/*
 * timebase.h
 *
 * Created: 15.06.2012 16:17:25
 *  Author: Julian
 */ 


#ifndef TIMEBASE_H_
#define TIMEBASE_H_

#include <avr/interrupt.h>
#include <avr/io.h>
#include "../Menu/screensaver.h"

extern volatile uint16_t time_sysTick; /**< system time in 16,384 ms steps*/

/** this timer is the main timebase for the program.
it measures ticks since program start*/
void time_initTimer();





#endif /* TIMEBASE_H_ */
