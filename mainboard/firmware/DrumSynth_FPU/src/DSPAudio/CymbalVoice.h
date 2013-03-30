/*
 * CymbalVoice.h
 *
 *  Created on: 16.06.2012
 *      Author: Julian
 */

#ifndef CYMBALVOICE_H_
#define CYMBALVOICE_H_

#include "stm32f4xx.h"
#include "Oscillator.h"
#include "config.h"
#include "ResonantFilter.h"
//#include "Decay.h"
#include "SlopeEG2.h"
#include "random.h"
#include "toneControl.h"
#include "transientGenerator.h"
#include "BufferTools.h"
#include "lfo.h"
#include "distortion.h"

typedef struct CymbalStruct
{

	OscInfo 	osc;		// the tonal oscillator
	OscInfo 	modOsc;		// the mod osc1
	OscInfo 	modOsc2;	// the mod osc2
	float		fmModAmount1;
	float		fmModAmount2;

//	uint8_t		filterType; // bit 0 = lp, bit 1 = hp, bit 3 = bp on/off
	float	 	vol;		// volume of the voice
	float		panL;		// [0:1]
	float		panR;		// [0:1]
	float 		panModifier;

	float 		velo;

	int32_t		noiseSample;

	ResonantFilter filter;
	uint8_t		filterType;

	//ToneControl toneControl;
	Lfo 		lfo;
	TransientGenerator transGen;
	Distortion distortion;

//	float		decayClosed;
//	float		decayOpen;

	//DecayEg		oscVolEg;
	SlopeEg2		oscVolEg;
	float 		egValueOscVol;

	uint8_t volumeMod;	//modulate volume by midi velocity if 1

} CymbalVoice;

extern CymbalVoice cymbalVoice;


//initialize all the parameters to sane values
void Cymbal_init();

void Cymbal_trigger( const uint8_t vel, const uint8_t note);

void Cymbal_calcSyncBlock(int16_t* buf, const uint8_t size);
/** claculate the oscillators and sample based stuff*/
int16_t Cymbal_calcSync();
/** calculate envelopes etc (all 16 samples */
void Cymbal_calcAsync();

//-0.5 = left 0=both max 0.5=right*/
void Cymbal_setPan(const uint8_t pan);



#endif /* CYMBALVOICE_H_ */
