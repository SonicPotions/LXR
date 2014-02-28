/*
 * Snare.h
 *
 *  Created on: 16.04.2012
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

#define USE_PEAK 0
//------------------------------------------------------------------------
typedef struct SnareStruct
{
	OscInfo 	osc;		// the tonal oscillator
	OscInfo 	noiseOsc;	// the noise osc


	uint8_t		filterType; // bit 0 = lp, bit 1 = hp, bit 3 = bp on/off
	float	 	vol;		// volume of the voice
	//float		panL;		// [0:1]
	//float		panR;		// [0:1]
	uint8_t pan;
	//float 		panModifier;

	float   	mix;		// defines the mix between osc and noise [0 = 100% osc: 1 = 100% noise]
	float 		velo;

	int16_t		oscSample;
	int32_t		noiseSample;

#if USE_PEAK
	PeakFilter  filter;
#else
	ResonantFilter filter;
#endif

	DecayEg		oscPitchEg;
	float		egPitchModAmount;

	Lfo 		lfo;
	TransientGenerator transGen;

	SlopeEg2		oscVolEg;
	float 		egValueOscVol;

	Distortion distortion;
	SnapEg snapEg;

	uint8_t volumeMod;	//modulate volume by midi velocity if 1

} SnareVoice;

//array holding all the voices
extern SnareVoice snareVoice;

//initialize all the parameters to sane values
void Snare_init();

void Snare_trigger(const uint8_t vel, const uint8_t note);

/** claculate the oscillators and sample based stuff*/
void Snare_calcSyncBlock(int16_t* buf, const uint8_t size);

/** calculate envelopes etc (all 16 samples */
void Snare_calcAsync();

//-0.5 = left 0=both max 0.5=right*/
void Snare_setPan(const uint8_t pan);
#endif /* SNARE_H_ */
