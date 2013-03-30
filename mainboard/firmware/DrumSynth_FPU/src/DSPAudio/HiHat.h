/*
 * HiHat.h
 *
 *  Created on: 18.04.2012
 *      Author: Julian
 */

#ifndef HIHAT_H_
#define HIHAT_H_

#include "stm32f4xx.h"
#include "Oscillator.h"
#include "config.h"
#include "ResonantFilter.h"
//#include "Decay.h"
//#include "AD_EG.h"
#include "SlopeEg2.h"
#include "random.h"
#include "toneControl.h"
#include "transientGenerator.h"
#include "BufferTools.h"
#include "lfo.h"
#include "distortion.h"
#endif /* HIHAT_H_ */

//#define NUM_HAT 1

typedef struct HiHatStruct
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

//	ToneControl toneControl;

	ResonantFilter filter;
	uint8_t		filterType;

	TransientGenerator transGen;

	float		decayClosed;
	float		decayOpen;

	Distortion distortion;

	//DecayEg		oscVolEg;
	SlopeEg2		oscVolEg;
	float 		egValueOscVol;
	Lfo 		lfo;
	uint8_t volumeMod;	//modulate volume by midi velocity if 1

} HiHatVoice;

extern HiHatVoice hatVoice;

//initialize all the parameters to sane values
void HiHat_init();

void HiHat_trigger(uint8_t vel, uint8_t isOpen, const uint8_t note);

void HiHat_calcSyncBlock(int16_t* buf, const uint8_t size);
/** claculate the oscillators and sample based stuff*/
int16_t HiHat_calcSync();
/** calculate envelopes etc (all 16 samples */
void HiHat_calcAsync();

//-0.5 = left 0=both max 0.5=right*/
void HiHat_setPan(const uint8_t pan);

