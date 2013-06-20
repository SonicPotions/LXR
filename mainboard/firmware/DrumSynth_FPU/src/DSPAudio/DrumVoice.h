/*
 * DrumVoice.h
 *
 *  Created on: 03.04.2012
 *      Author: Julian
 */

#ifndef DRUMVOICE_H_
#define DRUMVOICE_H_

#include "Oscillator.h"
#include "config.h"
#include "ResonantFilter.h"
#include "distortion.h"
#include "SlopeEg2.h"
#include "Decay.h"
#include "modulationNode.h"
#include "lfo.h"
#include "1PoleLp.h"
#include "transientGenerator.h"
#include "snapEg.h"
#include "dither.h"
//------------------------------------------------------------------------
enum
{
	IS_PLAYING = 0x1,
};
//------------------------------------------------------------------------
typedef struct VoiceStruct
{
	OscInfo 	osc;		// the main fm oscillator
	OscInfo 	modOsc;		// the mod osc

	float		fmModAmount;

	float	 	vol;		// volume of the voice
	float 		velo;
	//float		panL;		// [0:1]
	//float		panR;		// [0:1]
	uint8_t 	pan; //
	float 		panModifier;

	int16_t 		oscSample;

	DecayEg		oscPitchEg;
	float		egPitchModAmount;
	float 		offset;				//the start offset for the waveform - used to generate clicks

	TransientGenerator transGen;
	Lfo 		lfo;
	SlopeEg2	oscVolEg;
	float 		egValueOscVol;
	float 		volEgValueBlock[OUTPUT_DMA_SIZE];
	Distortion distortion;

#if ENABLE_DRUM_SVF
	ResonantFilter filter;
	uint8_t		filterType; // bit 0 = lp, bit 1 = hp, bit 3 = bp on/off
#endif

	OnePoleFilter ampFilter;
#if (AMP_EG_SYNC==0)
	float ampFilterInput;
	float lastGain;
#endif

#if ENABLE_MIX_OSC
	uint8_t mixOscs;
#endif

	float decimationCnt;
	float decimationRate;
	SnapEg snapEg;

	uint8_t volumeMod;	//modulate volume by midi velocity if 1

	Dither dither;
} DrumVoice;
//------------------------------------------------------------------------


//array holding all the voices
extern DrumVoice voiceArray[NUM_VOICES];

//initialize all the parameters to sane values
void initDrumVoice();

void Drum_trigger(const uint8_t voiceNr, const uint8_t vol, const uint8_t note);

/** block based calculation*/
void calcDrumVoiceSyncBlock(const uint8_t voiceNr, int16_t* buf, const uint8_t size);

/** calculate envelopes etc (all 16 samples */
void calcDrumVoiceAsync(const uint8_t voiceNr);

//-0.5 = left 0=both max 0.5=right*/
void setPan(const uint8_t voiceNr, const uint8_t pan);

/** set the oscillator start phase for all OSCs*/
void drum_setPhase(const uint8_t phase, const uint8_t voiceNr);

#endif /* DRUMVOICE_H_ */
