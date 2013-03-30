/*
 * Oscillator.h
 *
 *  Created on: 01.04.2012
 *      Author: Julian
 */

#ifndef OSCILLATOR_H_
#define OSCILLATOR_H_

#include "stm32f4xx.h"
#include "datatypes.h"
#include "wavetable.h"
#include "config.h"
//-----------------------------------------------------------

//the available osc waveforms
#define SINE 		0x00
#define TRI			0x01
#define SAW			0x02
#define REC			0x03
#define NOISE		0x04
#define CRASH		0x05


#define phaseShift 1000

//-----------------------------------------------------------

typedef struct OscStruct
{
	int16_t 	output;
	uint32_t	phaseInc;
	uint32_t	phase;		// the current phase of the osc 8bit indexing 24 interpolation
	float	 	freq;		// frequency in [Hz]
	uint8_t		waveform;	// the selected waveform of the osc

	uint8_t		tableOffset;	//overtone table


	float		pitchMod;	// modulation value for frequency
	float 		fmMod;

	float		modNodeValue;

	//for midi
	uint16_t	midiFreq;  //upper 8 bit coarse, lower 8 bit fine -> the sound edit freq offset
	uint8_t baseNote;		// the last played midi note
	uint32_t startPhase;		// the OSC is reset to this phase on retrigger
} OscInfo;
//-----------------------------------------------------------

//extern OscInfo osc1;
//extern OscInfo osc2;

void initOsc();
//-----------------------------------------------------------
__inline uint32_t freq2PhaseIncr(float f);
//-----------------------------------------------------------
void calcSineBlock(OscInfo* osc, int16_t* buf, const uint8_t size ,const float gain);
//-----------------------------------------------------------
int16_t calcSine(OscInfo* osc);
//-----------------------------------------------------------
void calcFmSineBlock(OscInfo* osc, int16_t* modBuffer, int16_t* buf, uint8_t size ,const float gain);
//-----------------------------------------------------------
int16_t calcFmSine(OscInfo* osc, OscInfo* modOsc);
//-----------------------------------------------------------
void calcFmBlock(OscInfo* osc, const int16_t table[][1024], int16_t* modBuffer, int16_t* buf, uint8_t size ,const float gain);
//-----------------------------------------------------------
int16_t calcFm(OscInfo* osc, OscInfo* modOsc,  const int16_t table[][1024]);
//-----------------------------------------------------------
void calcWavetableOscBlock(OscInfo* osc, const int16_t table[][1024], int16_t* buf, const uint8_t size ,const float gain);
//-----------------------------------------------------------
int16_t calcWavetableOsc(OscInfo* osc, const int16_t table[][1024]);
//-----------------------------------------------------------
void calcNoiseBlock(OscInfo* osc, int16_t* buf, const uint8_t size ,const float gain);
//-----------------------------------------------------------
int16_t calcNoise(OscInfo* osc);
//-----------------------------------------------------------
int16_t calcSampleOscFm(OscInfo* osc, OscInfo* modOsc);
//-----------------------------------------------------------
//used to play back the crash cymbal sample
void calcSampleOscFmBlock(OscInfo* osc,int16_t* modBuffer, int16_t* buf, uint8_t size ,const float gain);
//-----------------------------------------------------------
void calcNextOscSampleBlock(OscInfo* osc, int16_t* buf, const uint8_t size ,const float gain);
//-----------------------------------------------------------
// calculate an oscillator
int16_t calcNextOscSample(OscInfo* osc);
 //-----------------------------------------------------------
void osc_setFreq(OscInfo* osc);
//-----------------------------------------------------------
void calcNextOscSampleFmBlock(OscInfo* osc, int16_t* modBuffer, int16_t* buf, uint8_t size ,const float gain);
 //-----------------------------------------------------------
// calculate an fm oscillator
int16_t calcNextOscSampleFm(OscInfo* osc, OscInfo* modOsc);
//-----------------------------------------------------------
void calcSampleOscBlock(OscInfo* osc, int16_t* buf, const uint8_t size ,const float gain);
//-----------------------------------------------------------
int16_t calcSampleOsc(OscInfo* osc);
//-----------------------------------------------------------
/** set the played midi note.*/
void osc_setBaseNote(OscInfo* osc, uint8_t baseNote);
//-----------------------------------------------------------
/** recalculate the frequency if either the base note or the offset note value changed*/
void osc_recalcFreq(OscInfo* osc);

#endif /* OSCILLATOR_H_ */
