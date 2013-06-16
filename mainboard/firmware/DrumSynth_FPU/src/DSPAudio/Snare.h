/*
 * Snare.h
 *
 *  Created on: 16.04.2012
 *      Author: Julian
 */

#ifndef SNARE_H_
#define SNARE_H_

#include "stm32f4xx.h"
#include "Oscillator.h"
#include "config.h"
#include "ResonantFilter.h"
#include "distortion.h"
#include "Decay.h"
#include "SlopeEg2.h"
#include "transientGenerator.h"
#include "BufferTools.h"
#include "lfo.h"
#include "snapEg.h"
//#define NUM_SNARES 1//NUM_VOICES-1

#define USE_PEAK 0
//------------------------------------------------------------------------
typedef struct SnareStruct
{
	OscInfo 	osc;		// the tonal oscillator
	OscInfo 	noiseOsc;	// the noise osc


	uint8_t		filterType; // bit 0 = lp, bit 1 = hp, bit 3 = bp on/off
	float	 	vol;		// volume of the voice
	float		panL;		// [0:1]
	float		panR;		// [0:1]
	float 		panModifier;

	float   	mix;		// defines the mix between osc and noise [0 = 100% osc: 1 = 100% noise]
	float 		velo;

	int16_t		oscSample;
	int32_t		noiseSample;

#if USE_PEAK
	PeakFilter  filter;
#else
	ResonantFilter filter;
#endif
//	uint8_t		filterMode; // bit 1: LP on/off - bit 2: HP on/off - bit 3: BP on/off


	DecayEg		oscPitchEg;
	float		egPitchModAmount;

//	ToneControl toneControl;
	Lfo 		lfo;
	TransientGenerator transGen;


	//AdEg		oscVolEg;
	SlopeEg2		oscVolEg;
	float 		egValueOscVol;
//	AdEg		noiseVolEg;
	Distortion distortion;
	SnapEg snapEg;


	uint8_t volumeMod;	//modulate volume by midi velocity if 1


} SnareVoice;

//array holding all the voices
extern SnareVoice snareVoice;


//initialize all the parameters to sane values
void Snare_init();

void Snare_trigger(const uint8_t vel, const uint8_t note);

void Snare_calcSyncBlock(int16_t* buf, const uint8_t size);
/** claculate the oscillators and sample based stuff*/
int16_t Snare_calcSync();
/** calculate envelopes etc (all 16 samples */
void Snare_calcAsync();

//-0.5 = left 0=both max 0.5=right*/
void Snare_setPan(const uint8_t pan);
#endif /* SNARE_H_ */
