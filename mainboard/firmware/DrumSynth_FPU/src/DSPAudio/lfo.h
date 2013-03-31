/*
 * lfo.h
 *
 *  Created on: 14.01.2013
 *      Author: Julian
 *
 *      Non bandlimited low freq. oscillator
 */

#ifndef LFO_H_
#define LFO_H_
//-------------------------------------------------------------
#include "stm32f4xx.h"
#include "Oscillator.h"
#include "wavetable.h"	//sine
#include "config.h"
#include "sequencer.h"
#include "modulationNode.h"
//-------------------------------------------------------------
#define LFO_SINE 		0x00
#define LFO_TRI			0x01
#define LFO_SAW_UP		0x02
#define LFO_SAW_DOWN	0x03
#define LFO_REC			0x04
#define LFO_NOISE		0x05
#define LFO_EXP_UP		0x06
#define LFO_EXP_DOWN	0x07


#define LFO_MAX_F 		200 //[Hz]
#define LFO_SR 			REAL_FS
//-------------------------------------------------------------
typedef struct LfoStruct
{
	uint32_t phase;		// the current phase of the LFO (upper bits cnt 0-255)
	uint32_t phaseInc;	// the phase increment controls the LFO frequency
	uint8_t waveform;	// selects the waveform
	uint8_t retrigger;	// defines the voice nr that retriggers the LFO (0=no retrigger)
	uint32_t phaseOffset;// the phase value to which the LFO is retriggered
	float rnd;
	uint8_t sync;
	float freq;
	ModulationNode modTarget;
} Lfo;
//-------------------------------------------------------------
void lfo_init(Lfo *lfo);
void lfo_dispatchNextValue(Lfo* lfo);
void lfo_setFreq(Lfo *lfo, float f);
void lfo_setSync(Lfo* lfo, uint8_t sync);
void lfo_recalcSync();
void lfo_retrigger(uint8_t voiceNr);

#endif /* LFO_H_ */
