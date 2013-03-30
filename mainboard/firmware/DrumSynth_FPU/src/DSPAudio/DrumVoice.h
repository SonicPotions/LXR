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
//#include "AD_EG.h"
#include "SlopeEg2.h"
#include "Decay.h"
#include "modulationNode.h"
#include "lfo.h"


//#if (AMP_EG_SYNC==0)
#include "1PoleLp.h"
//#endif

#include "toneControl.h"
#include "transientGenerator.h"

/*
#define LP 0x1
#define HP 0x2
#define BP 0x4
*/
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
	float		panL;		// [0:1]
	float		panR;		// [0:1]
	float 		panModifier;
//	float   	mix;		// defines the mix between osc and noise [0 = 100% osc: 1 = 100% noise]
//	uint8_t 	status;		// the voice status e.g. running, stopped...


	int16_t 		oscSample;
//	int16_t			modOscSample;
//	int16_t		noiseSample;

//	ResonantFilter noiseFilter;
//	uint8_t		filterMode; // bit 1: LP on/off - bit 2: HP on/off - bit 3: BP on/off




	DecayEg		oscPitchEg;
	float		egPitchModAmount;
	float 		offset;				//the start offset for the waveform - used to generate clicks

	TransientGenerator transGen;

	//ToneControl toneControl;
	Lfo 		lfo;

	SlopeEg2	oscVolEg;
	float 		egValueOscVol;

	float 		volEgValueBlock[OUTPUT_DMA_SIZE];


//	AdEg		noiseVolEg;

	Distortion distortion;

#if ENABLE_DRUM_SVF
	ResonantFilter filter;
	uint8_t		filterType; // bit 0 = lp, bit 1 = hp, bit 3 = bp on/off
#endif

	OnePoleFilter ampFilter;
#if (AMP_EG_SYNC==0)
	float ampFilterInput;
#endif

#if ENABLE_MIX_OSC
	uint8_t mixOscs;
#endif

	float decimationCnt;
	float decimationRate;

	uint8_t volumeMod;	//modulate volume by midi velocity if 1




} DrumVoice;
//------------------------------------------------------------------------


//array holding all the voices
extern DrumVoice voiceArray[NUM_VOICES];

//initialize all the parameters to sane values
void initDrumVoice();

void Drum_trigger(const uint8_t voiceNr, const uint8_t vol, const uint8_t note);
void stopVoice(const uint8_t voiceNr);

/** claculate the oscillators and sample based stuff*/
int16_t calcDrumVoiceSync(const uint8_t voiceNr);

void calcDrumVoiceSyncBlock(const uint8_t voiceNr, int16_t* buf, const uint8_t size);

/** block based calculation*/
//int16_t calcDrumVoiceSyncBlock(const uint8_t voiceNr, int16_t* audioBuffer);
/** calculate envelopes etc (all 16 samples */
void calcDrumVoiceAsync(const uint8_t voiceNr);

//-0.5 = left 0=both max 0.5=right*/
void setPan(const uint8_t voiceNr, const uint8_t pan);

/** set the oscillator start phase for all OSCs*/
void drum_setPhase(const uint8_t phase, const uint8_t voiceNr);

#endif /* DRUMVOICE_H_ */
