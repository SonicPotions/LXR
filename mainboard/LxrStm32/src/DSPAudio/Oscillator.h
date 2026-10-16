/*
 * Oscillator.h
 *
 *  Created on: 01.04.2012
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


#ifndef OSCILLATOR_H_
#define OSCILLATOR_H_

#include "stm32f4xx.h"
#include "datatypes.h"
#include "wavetable.h"
#include "config.h"
#include "../SampleRom/SampleMemory.h"
//-----------------------------------------------------------

//the available osc waveforms
#define SINE 				0x00
#define TRI					0x01
#define SAW					0x02
#define REC					0x03
#define NOISE				0x04
#define CRASH				0x05
#define OSC_SAMPLE_START 	0x06

#define phaseShift 1000

//-----------------------------------------------------------

typedef struct OscStruct
{
	int16_t 	output;
	uint32_t	phaseInc;
	uint32_t	phase;		// the current phase of the osc 8bit indexing 24 interpolation
	uint32_t samplePosition;  /* User sample integer frame index; 32-bit so long imported WAVs can play past 64K frames. */
	uint32_t sampleFraction;  /* User sample 17-bit fractional accumulator paired with freq2PhaseIncr32767(). */
	uint8_t sampleActive;     /* One-shot playback gate; reset on trigger, cleared at end unless SampleInfo loop bit is set. */
	float	 	freq;		// frequency in [Hz]
	uint8_t		waveform;	// the selected waveform of the osc

	/* Waveform-blend state. Written by modNode_updateValue() when the interp flag
	   is on and a waveform parameter is being modulated to a fractional position.
	   waveInterpGeneration is compared against modNode_getWaveInterpGeneration()
	   each DSP block; a mismatch means the state is stale and interp is skipped.
	   waveInterpFrac == 0 or >= 1 means no blend is active. */
	float    waveInterpFrac;       // blend fraction: 0.0=waveform, 1.0=waveInterpNext
	uint8_t  waveInterpNext;       // target waveform ID to blend toward
	uint8_t  _wavePad;             // explicit alignment pad (float follows uint8_t)
	uint32_t waveInterpGeneration; // DSP block generation token from modNode

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
uint32_t freq2PhaseIncr(float f);
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
/* IMPORTED-SAMPLE FM BLOCK RENDERER  (Session 039)
   WHAT: renders `size` frames of the imported sample at
         osc->waveform - OSC_SAMPLE_START, phase-modulated by modBuffer
         (read offset = modBuffer[i] * osc->fmMod, scaled by 2^14 into the
         17-bit fixed-point frame domain, the same modulation index as the
         wavetable and CRASH FM paths).
   WHY:  FM must work whatever the carrier's waveform source. The previous
         stub ignored modBuffer.
   INPUT:  osc (play head, fmMod, sampleActive), modBuffer[size], buf[size],
           size, gain.
   OUTPUT: buf filled (zeros when the slot is invalid or a one-shot has
           finished); play head and one-shot gate advance exactly as
           calcUserSampleOscBlock(); osc->output = last frame.
   ACCESSORS: calcNextOscSampleFmBlock() default branch.
   AFFILIATES: calcUserSampleOscBlock(), osc_resetSamplePlayback(),
               calcSampleOscFmBlock() (CRASH precedent). */
void calcUserSampleOscFmBlock(OscInfo* osc,int16_t* modBuffer, int16_t* buf, uint8_t size ,const float gain);
//-----------------------------------------------------------
/* Render an imported user sample selected by osc->waveform - OSC_SAMPLE_START.
   Inputs: osc playback state, output buffer, frame count, gain. Output: buffer
   is filled with PCM or zeros when the slot is invalid/inactive. */
void calcUserSampleOscBlock(OscInfo* osc, int16_t* buf, const uint8_t size ,const float gain);
//-----------------------------------------------------------
int16_t calcSampleOsc(OscInfo* osc);
//-----------------------------------------------------------
/** set the played midi note.*/
void osc_setBaseNote(OscInfo* osc, uint8_t baseNote);
//-----------------------------------------------------------
/** recalculate the frequency if either the base note or the offset note value changed*/
void osc_recalcFreq(OscInfo* osc);
/* Reset imported-sample playback state on trigger. Input/output is the
   oscillator struct; samplePosition and sampleFraction are cleared and
   sampleActive is armed for one-shot playback. */
void osc_resetSamplePlayback(OscInfo* osc);

/* Resets all three waveform-blend fields to inactive state. Called on recursive
   calcNextOscSampleBlock() calls inside blend helpers to prevent re-entry.
   Input/output: osc -- waveInterpFrac=0, waveInterpNext=osc->waveform, generation=0. */
void osc_clearWaveInterp(OscInfo* osc);

/* Writes waveform-blend target state to an oscillator. Called from
   modNode_updateValue() when a TYPE_UINT8 waveform parameter is modulated
   to a fractional position and the interp budget permits.
   Inputs: osc, fracValue (0.0..1.0), nextWaveform, generation (from modNode). */
void osc_setWaveInterp(OscInfo* osc, float fracValue, uint8_t nextWaveform, uint32_t generation);

#endif /* OSCILLATOR_H_ */
