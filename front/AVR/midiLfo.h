/*
 * midiLfo.h
 *
 * This is a crude, non limited LFO to modulate some of the soundchip parameters over the UART
 *
 * Created: 28.04.2012 15:22:09
 *  Author: Julian
 */ 


#ifndef MIDILFO_H_
#define MIDILFO_H_


#include <avr/io.h>
#include "config.h"
#if USE_LFO
#define SINE		0
#define SAW_UP		1
#define SAW_DOWN	2
#define SQUARE		3
#define RND			4

typedef struct LfoStruct
{
	volatile uint32_t phase;		// the current phase of the LFO (upper bits cnt 0-255)
	uint32_t phaseInc;	// the phase increment controls the LFO frequency
	uint8_t waveform;	// selects the waveform
	uint8_t retrigger;	// defines the voice nr that retriggers the LFO (0=no retrigger)
	uint16_t phaseOffset;// the phase value to which the LFO is retriggered
	
	uint8_t rngValue;	// a buffer to hold the last rng value
	
}Lfo;

#define NUM_LFO 6
extern Lfo lfoArray[NUM_LFO];
//------------------------------------------------------------------------------------
void lfo_init();
//------------------------------------------------------------------------------------
void lfo_setFrequency(uint8_t lfoNumber, uint8_t freq);
//------------------------------------------------------------------------------------
void lfo_calc();
//------------------------------------------------------------------------------------
void lfo_setBPM(uint8_t bpm);
//------------------------------------------------------------------------------------
void lfo_resetCurrentDestination(uint8_t lfoNr);
//------------------------------------------------------------------------------------
void lfo_retrigger(uint8_t voiceNr);
//------------------------------------------------------------------------------------
void lfo_disable(uint8_t disable);
//------------------------------------------------------------------------------------
/** used for the tempo synced mode to keep lfos tight on the sequencer.
whenever a whole 4/4 bar is played the lfo's phases are reset*/
void lfo_resync(uint8_t voiceNr);
//------------------------------------------------------------------------------------
uint8_t lfo_pseudoRng();
//------------------------------------------------------------------------------------
#endif /* MIDILFO_H_ */
#endif