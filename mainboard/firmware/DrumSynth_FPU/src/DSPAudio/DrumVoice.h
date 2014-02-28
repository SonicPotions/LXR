/*
 * DrumVoice.h
 *
 *  Created on: 03.04.2012
 * ------------------------------------------------------------------------------------------------------------------------
 *  Copyright 2013 Julian Schmidt
 *  Julian@sonic-potions.com
 * ------------------------------------------------------------------------------------------------------------------------
 *  This file is part of the Sonic Potions LXR drumsynth firmware.
 * ------------------------------------------------------------------------------------------------------------------------
 *  Redistribution and use of the LXR code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *       - The code may not be sold, nor may it be used in a commercial product or activity.
 *
 *       - Redistributions that are modified from the original source must include the complete
 *         source code, including the source code for all components used by a binary built
 *         from the modified sources. However, as a special exception, the source code distributed
 *         need not include anything that is normally distributed (in either source or binary form)
 *         with the major components (compiler, kernel, and so on) of the operating system on which
 *         the executable runs, unless that component itself accompanies the executable.
 *
 *       - Redistributions must reproduce the above copyright notice, this list of conditions and the
 *         following disclaimer in the documentation and/or other materials provided with the distribution.
 * ------------------------------------------------------------------------------------------------------------------------
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *   USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ------------------------------------------------------------------------------------------------------------------------
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
	uint8_t 	pan;
	//float 		panModifier;

	int16_t 	oscSample;

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

#ifdef USE_AMP_FILTER
	OnePoleFilter ampFilter;
#endif

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
