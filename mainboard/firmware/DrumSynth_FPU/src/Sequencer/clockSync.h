/*
 * clockSync.h
 *
 *  Created on: 02.02.2013
 *      Author: Julian
 *
 *      used to keep track of incoming midi clocks
 */

#ifndef CLOCKSYNC_H_
#define CLOCKSYNC_H_

#include "stm32f4xx.h"
#include "globals.h"
#include "config.h"
#include "sequencer.h"

#define WINDOW_SIZE 8
#define WINDOW_MASK 0x07


/*
 we have a midi clock resolution of 24 ppq
 our sequencer is running with 32 ppq
 24/32 => 12/16 => 6/8 => 3/4 ratio

 for every 3 midi clocks we have to advance 4 steps
 */

//called by midi clock
void sync_tick();
void sync_midiStartStop(uint8_t isStart);
uint8_t sync_getClockCnt();

#endif /* CLOCKSYNC_H_ */
