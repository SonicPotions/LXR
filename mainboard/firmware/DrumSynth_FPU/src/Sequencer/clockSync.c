/*
 * clockSync.c
 *
 *  Created on: 03.02.2013
 *      Author: Julian
 */


#include "clockSync.h"

static float sync_window[WINDOW_SIZE];
float sync_timeLast = 0;
uint8_t sync_writePos = 0;
uint8_t sync_armed = 0;

uint8_t sync_clockCnt = 0;

uint16_t sync_calcBpm(float timePerPulse)
{
	// 1 Minute has 60 * 1000 [ms]
	//we have 24 pulses per quarter
	//120 bpm = 120 quarters per minute

	float quarterDuration = 24 * (timePerPulse*4);
	float bpm = (60 * 1000) / quarterDuration;

	return bpm;
}

//called by midi clock
void sync_tick()
{
/*
	if(sync_armed)
	{
		sync_armed = 0;
		seq_setRunning(1);
	}
	*/
	float time = systick_ticks - sync_timeLast;
	sync_timeLast = systick_ticks;

	sync_window[sync_writePos++] = time;
	sync_writePos &= WINDOW_MASK;

	//calc average time in [ms] between midi clock pulses
	int i;
	float avg=0;
	for(i=0;i<WINDOW_SIZE;i++)
	{
		avg += sync_window[i];
	}
	avg /= WINDOW_SIZE;

	seq_setBpm(sync_calcBpm(avg));

	sync_clockCnt++;

	//now handle sequencer transport (4 steps every 3 clocks)
	if(sync_clockCnt == 1 || sync_clockCnt >= 4) {
		sync_clockCnt = 1;
		seq_resetDeltaAndTick();
	}


}

void sync_midiStartStop(uint8_t isStart)
{
	if(isStart) {
		// this message does NOT mean start immediately! it just 'arms' the seq to start at the next MIDI_CLOCK received
		sync_clockCnt = 0;
		seq_syncStepCnt = 3;
		//sync_armed = 1;
		seq_setRunning(1);
		sync_timeLast = systick_ticks;
	} else {
		seq_setRunning(0);
	}
}

uint8_t sync_getClockCnt()
{
	return sync_clockCnt;
}
