/*
 * Oscillator.c
 *
 *  Created on: 02.04.2012
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


#include "Oscillator.h"
#include "random.h"
#include "Samples.h"
#include "MidiParser.h"
#include "MidiNoteNumbers.h"
#include "sequencer.h"
#include <string.h>
#include "modulationNode.h"


//TODO die phaseInc berechnung kann man doch sicher per LUT machen!
//-----------------------------------------------------------
__inline uint8_t fast_log2 (const uint32_t val)
{
	return 31-__CLZ(val);
}
//-----------------------------------------------------------
__inline int freqToMidiNote(const float f)
{
	return (12*fast_log2((uint32_t)(f/440.f)))+70;
}
//-----------------------------------------------------------
__inline uint8_t freqToTableIndex(const float f)
{
	return freqToMidiNote(f)/12;
}
//-----------------------------------------------------------
uint32_t freq2PhaseIncr(const float f) //4096
{
	return (((TABLESIZE*f)/REAL_FS))*1048576 ; // 1048576 <-> (<<20)
}
//-----------------------------------------------------------
__inline uint32_t freq2PhaseIncr1024(const float f)
{
	return (((1024*f)/REAL_FS))*4194304 ; //(<<22)
}
//-----------------------------------------------------------
void osc_resetSamplePlayback(OscInfo* osc)
{
   /* Called by each voice trigger path after phase reset. Without this, a
      one-shot imported sample that reached its end would remain silent on the
      next trigger, and long samples would resume from their previous offset. */
   osc->samplePosition = 0u;
   osc->sampleFraction = 0u;
   osc->sampleActive = 1u;
}
//-----------------------------------------------------------
__inline uint32_t freq2PhaseIncr32767(const float f)
{
	return (((1024*f)/REAL_FS))*131072 ; //(<<17)
}
//-----------------------------------------------------------

/* Scratch buffers for waveform-blend rendering.
   Block-sized static globals: two waveforms are rendered into these before the
   per-sample blend loop runs. Static so they are not allocated on the audio stack.
   Size = OUTPUT_DMA_SIZE (16 frames). */
static int16_t osc_interp_a[OUTPUT_DMA_SIZE];
static int16_t osc_interp_b[OUTPUT_DMA_SIZE];

/* -----------------------------------------------------------------------
   osc_clearWaveInterp
   Resets blend fields to inactive. Call on the temporary OscInfo copies used
   inside the blend helpers so recursive calcNextOscSampleBlock() calls do not
   re-enter the blend path.
   Input/output: osc -- waveInterpFrac=0.f, waveInterpNext=osc->waveform,
   waveInterpGeneration=0.
   ----------------------------------------------------------------------- */
void osc_clearWaveInterp(OscInfo* osc)
{
   osc->waveInterpFrac = 0.f;
   osc->waveInterpNext = osc->waveform;
   osc->waveInterpGeneration = 0u;
}

/* -----------------------------------------------------------------------
   osc_setWaveInterp
   Writes waveform-blend target state. Called by modNode_updateValue() inside
   the TYPE_UINT8 branch for waveform parameters.
   Inputs:
     osc          - the oscillator to update.
     fracValue    - blend fraction in (0.0, 1.0).
     nextWaveform - the integer waveform ID above the current integer floor.
     generation   - current block generation from modNode_getWaveInterpGeneration().
   Output: osc->waveInterpFrac, waveInterpNext, waveInterpGeneration written.
   ----------------------------------------------------------------------- */
void osc_setWaveInterp(OscInfo* osc, float fracValue, uint8_t nextWaveform, uint32_t generation)
{
   osc->waveInterpFrac = fracValue;
   osc->waveInterpNext = nextWaveform;
   osc->waveInterpGeneration = generation;
}

/* -----------------------------------------------------------------------
   osc_waveInterpActive  (static, called only inside Oscillator.c)
   Guard function called at the top of each calcNextOscSample[Fm][Block]
   dispatcher. Returns 1 only when a valid blend is active for this oscillator.
   Returning 0 causes the normal waveform-switch path to run unchanged.

   Conditions that return 0 (in order of cheapest check first):
     1. modNode_getWaveInterpEnabled() is 0  (global flag off)
     2. osc->waveInterpGeneration != modNode_getWaveInterpGeneration()
           (blend state from a prior DSP block -- auto-expired)
     3. osc->waveInterpFrac <= 0.f or >= 1.f  (no fractional blend needed)
     4. osc->waveInterpNext <= osc->waveform   (illegal or reverse direction)
     5. osc->waveform or waveInterpNext exceeds the highest valid waveform ID

   Input: osc (read-only). Output: 0 or 1.
   ----------------------------------------------------------------------- */
static uint8_t osc_waveInterpActive(const OscInfo* osc)
{
   if (!modNode_getWaveInterpEnabled()) {
      return 0u;
   }
   if (osc->waveInterpGeneration != modNode_getWaveInterpGeneration()) {
      return 0u;
   }
   if (osc->waveInterpFrac <= 0.f || osc->waveInterpFrac >= 1.f) {
      return 0u;
   }
   if (osc->waveInterpNext <= osc->waveform) {
      return 0u;
   }
   const uint8_t numSamples = sampleMemory_getNumSamples();
   const uint8_t maxWave = (numSamples > 0u)
                         ? (uint8_t)(OSC_SAMPLE_START + numSamples - 1u)
                         : (uint8_t)(OSC_SAMPLE_START - 1u);
   if (osc->waveform > maxWave || osc->waveInterpNext > maxWave) {
      return 0u;
   }
   return 1u;
}

/* -----------------------------------------------------------------------
   calcPeriodicInterpBlock  (static)
   Block-mode waveform blend for non-FM oscillators.
   Renders waveform and waveInterpNext into osc_interp_a and osc_interp_b
   by calling calcNextOscSampleBlock() on temporary copies with interp cleared
   (to prevent re-entry). Then blends per sample by waveInterpFrac.
   Inputs: osc, buf (OUTPUT_DMA_SIZE int16 output), size, gain.
   Output: buf filled. osc->phase advanced by phaseInc * size.
   ----------------------------------------------------------------------- */
static void calcPeriodicInterpBlock(OscInfo* osc, int16_t* buf, const uint8_t size, const float gain)
{
   OscInfo oscA = *osc;
   OscInfo oscB = *osc;
   const float frac = osc->waveInterpFrac;
   uint8_t i;

   oscA.waveform = osc->waveform;
   osc_clearWaveInterp(&oscA);
   oscB.waveform = osc->waveInterpNext;
   osc_setFreq(&oscB);
   osc_clearWaveInterp(&oscB);

   calcNextOscSampleBlock(&oscA, osc_interp_a, size, 1.f);
   calcNextOscSampleBlock(&oscB, osc_interp_b, size, 1.f);

   for (i = 0u; i < size; i++) {
      const int16_t a = osc_interp_a[i];
      const int16_t b = osc_interp_b[i];
      buf[i] = (int16_t)((a + frac * (float)(b - a)) * gain);
   }

   /* State writeback: restore all standard oscillator state from oscA 
      (phase, NOISE PRNG iterations, etc) so it advances naturally. */
   *osc = oscA;
   /* If either neighbor was a sample, ensure sample playback state advances. 
      oscB might have been a sample while oscA was not. */
   if (oscB.samplePosition > oscA.samplePosition) {
       osc->samplePosition = oscB.samplePosition;
       osc->sampleFraction = oscB.sampleFraction;
       osc->sampleActive   = oscB.sampleActive;
   }
}

/* -----------------------------------------------------------------------
   calcPeriodicInterp  (static)
   Scalar waveform blend for non-FM oscillators. Same logic as the block
   version but produces one sample. Used by calcNextOscSample().
   Output: blended int16. osc->phase advanced by one phaseInc. osc->output set.
   ----------------------------------------------------------------------- */
static int16_t calcPeriodicInterp(OscInfo* osc)
{
   const float frac = osc->waveInterpFrac;
   OscInfo oscA = *osc;
   OscInfo oscB = *osc;
   int16_t a, b, out;

   oscA.waveform = osc->waveform;
   osc_clearWaveInterp(&oscA);
   oscB.waveform = osc->waveInterpNext;
   osc_setFreq(&oscB);
   osc_clearWaveInterp(&oscB);

   /* Render one sample by routing through the block path with size=1.
      This reuses calcNextOscSampleBlock()'s waveform dispatch without
      duplicating the switch logic. */
   calcNextOscSampleBlock(&oscA, &a, 1u, 1.f);
   calcNextOscSampleBlock(&oscB, &b, 1u, 1.f);

   out = (int16_t)(a + frac * (float)(b - a));

   *osc = oscA;
   if (oscB.samplePosition > oscA.samplePosition) {
       osc->samplePosition = oscB.samplePosition;
       osc->sampleFraction = oscB.sampleFraction;
       osc->sampleActive   = oscB.sampleActive;
   }
   osc->output = out;
   return out;
}

/* -----------------------------------------------------------------------
   calcPeriodicInterpFmBlock  (static)
   Block-mode waveform blend for FM oscillators.
   Identical to calcPeriodicInterpBlock but routes through
   calcNextOscSampleFmBlock(). Needed because the FM dispatch path applies
   the FM modulator index before the wavetable lookup.
   Inputs: osc, modBuffer (FM modulator samples), buf, size, gain.
   Output: buf filled. osc->phase advanced. osc->output = last blended sample.
   ----------------------------------------------------------------------- */
static void calcPeriodicInterpFmBlock(OscInfo* osc, int16_t* modBuffer, int16_t* buf, const uint8_t size, const float gain)
{
   OscInfo oscA = *osc;
   OscInfo oscB = *osc;
   const float frac = osc->waveInterpFrac;
   int16_t lastOut = 0;
   uint8_t i;

   oscA.waveform = osc->waveform;
   osc_clearWaveInterp(&oscA);
   oscB.waveform = osc->waveInterpNext;
   osc_setFreq(&oscB);
   osc_clearWaveInterp(&oscB);

   calcNextOscSampleFmBlock(&oscA, modBuffer, osc_interp_a, size, 1.f);
   calcNextOscSampleFmBlock(&oscB, modBuffer, osc_interp_b, size, 1.f);

   for (i = 0u; i < size; i++) {
      const int16_t a = osc_interp_a[i];
      const int16_t b = osc_interp_b[i];
      lastOut = (int16_t)(a + frac * (float)(b - a));
      buf[i] = (int16_t)(lastOut * gain);
   }

   *osc = oscA;
   if (oscB.samplePosition > oscA.samplePosition) {
       osc->samplePosition = oscB.samplePosition;
       osc->sampleFraction = oscB.sampleFraction;
       osc->sampleActive   = oscB.sampleActive;
   }
   osc->output = lastOut;
}

/* -----------------------------------------------------------------------
   calcPeriodicInterpFm  (static)
   Scalar waveform blend for FM oscillators.
   Inputs: osc, modOsc (provides modOsc->output as modulator sample).
   Output: blended int16. osc->phase advanced. osc->output set.
   ----------------------------------------------------------------------- */
static int16_t calcPeriodicInterpFm(OscInfo* osc, OscInfo* modOsc)
{
   const float frac = osc->waveInterpFrac;
   OscInfo oscA = *osc;
   OscInfo oscB = *osc;
   int16_t a, b, out;
   int16_t modBuf[1] = { modOsc->output };

   oscA.waveform = osc->waveform;
   osc_clearWaveInterp(&oscA);
   oscB.waveform = osc->waveInterpNext;
   osc_setFreq(&oscB);
   osc_clearWaveInterp(&oscB);

   calcNextOscSampleFmBlock(&oscA, modBuf, &a, 1u, 1.f);
   calcNextOscSampleFmBlock(&oscB, modBuf, &b, 1u, 1.f);

   out = (int16_t)(a + frac * (float)(b - a));
   
   *osc = oscA;
   if (oscB.samplePosition > oscA.samplePosition) {
       osc->samplePosition = oscB.samplePosition;
       osc->sampleFraction = oscB.sampleFraction;
       osc->sampleActive   = oscB.sampleActive;
   }
   osc->output = out;
   return out;
}

//-----------------------------------------------------------
void calcSineBlock(OscInfo* osc, int16_t* buf, const uint8_t size ,const float gain)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		const uint32_t oscPhase = osc->phase;
		int16_t oscOut;

		const uint32_t index =  oscPhase;
		uint32_t  itg	= index>>20;

		#if INTERPOLATE_OSC
			oscOut = sine_table[itg++];
			const float frac = (index&0x7ffff)*0.0000019073486328125f;
			oscOut += frac*(sine_table[itg] - oscOut);
		#else
			oscOut = sine_table[itg];
		#endif

		osc->phase = oscPhase + osc->phaseInc;

		buf[i] = oscOut * gain;
	}
}
//-----------------------------------------------------------
int16_t calcSine(OscInfo* osc)
{
	const uint32_t oscPhase = osc->phase;
	int16_t oscOut;

	const uint32_t index =  oscPhase;
	uint32_t  itg	= index>>20;

#if INTERPOLATE_OSC
	oscOut = sine_table[itg++];
	float frac	= (index&0x7ffff)*0.0000019073486328125f;
	oscOut += frac*(sine_table[itg] - oscOut);
#else
	oscOut = sine_table[itg];
#endif

	osc->phase = oscPhase + osc->phaseInc;
	osc->output = oscOut;
	return oscOut;
}

//-----------------------------------------------------------
void calcFmSineBlock(OscInfo* osc, int16_t* modBuffer, int16_t* buf, uint8_t size,const float gain)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		const uint32_t oscPhase = osc->phase;
		int16_t oscOut ;

		const uint32_t index =  oscPhase + (((uint32_t)(modBuffer[i]*osc->fmMod))<<17);
		uint32_t  itg	= index>>20;

	#if INTERPOLATE_FM_OSC
		oscOut = sine_table[itg++];
		const float frac	= (index&0x7ffff)*0.0000019073486328125f;
		oscOut += frac*(sine_table[itg] - oscOut);
	#else
		oscOut = sine_table[itg];
	#endif

		osc->phase = oscPhase + osc->phaseInc;
		buf[i] =  oscOut * gain;
	}
}
//-----------------------------------------------------------
int16_t calcFmSine(OscInfo* osc, OscInfo* modOsc)
{
	const uint32_t oscPhase 	= osc->phase;
	int16_t oscOut ;

	const uint32_t index =  oscPhase + (((uint32_t)(modOsc->output*osc->fmMod))<<17);
	uint32_t  itg	= index>>20;

#if INTERPOLATE_FM_OSC
	oscOut = sine_table[itg++];
	const float frac	= (index&0x7ffff)*0.0000019073486328125f;
	oscOut += frac*(sine_table[itg] - oscOut);
#else
	oscOut = sine_table[itg];
#endif

	osc->phase = oscPhase + osc->phaseInc;
	osc->output = oscOut;
	return oscOut;
};
//---------------------------------------------------------------
void calcFmBlock(OscInfo* osc, const int16_t table[][1024], int16_t* modBuffer, int16_t* buf, uint8_t size ,const float gain)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		const uint32_t oscPhase = osc->phase;
		int16_t oscOut ;
		const uint32_t index =  oscPhase + (((uint32_t)(modBuffer[i]*osc->fmMod))<<19);

		uint32_t  itg	= index>>22;

	#if INTERPOLATE_FM_OSC
		oscOut = table[osc->tableOffset][itg++];
		const float frac	= (index&0x003FFFFF)*2.38418579101562e-07f;
		oscOut += frac*(table[osc->tableOffset][itg] - oscOut);

	#else
		oscOut = table[overtoneIndex][itg];
	#endif

		osc->phase = oscPhase + osc->phaseInc;
		osc->output = oscOut;
		buf[i] = oscOut * gain;
	}
}
//-----------------------------------------------------------
int16_t calcFm(OscInfo* osc, OscInfo* modOsc, const int16_t table[][1024])
{

	const uint32_t oscPhase = osc->phase;
	int16_t oscOut ;

	const uint32_t index =  oscPhase + (((uint32_t)(modOsc->output*osc->fmMod))<<19);

	uint32_t  itg	= index>>22;

#if INTERPOLATE_FM_OSC
	oscOut = table[osc->tableOffset][itg++];
	const float frac	= (index&0x003FFFFF)*2.38418579101562e-07f;
	oscOut += frac*(table[osc->tableOffset][itg] - oscOut);
#else
	oscOut = table[overtoneIndex][itg];
#endif

	osc->phase = oscPhase + osc->phaseInc;
	osc->output = oscOut;
	return oscOut;
}
//-----------------------------------------------------------
void calcNextOscSampleFmBlock(OscInfo* osc, int16_t* modBuffer, int16_t* buf, uint8_t size ,const float gain)
{
	if (osc_waveInterpActive(osc)) {
		calcPeriodicInterpFmBlock(osc, modBuffer, buf, size, gain);
		return;
	}

	switch(osc->waveform)
		{
		case SINE:
			calcFmSineBlock(osc,modBuffer,buf,size,gain);
			break;

		case SAW:
			calcFmBlock(osc,sawTable,modBuffer,buf,size,gain);
			break;

		case TRI:
			calcFmBlock(osc, triTable, modBuffer,buf,size,gain);
			break;

		case REC:
			calcFmBlock(osc,recTable,modBuffer,buf,size,gain);
			break;

		case NOISE:
			//TODO FM noise - needed since its tuned digi noise!
			calcNoiseBlock(osc,buf,size,gain);
			break;

		case CRASH:
			calcSampleOscFmBlock(osc,modBuffer,buf,size,gain);
			break;

		default:
			calcUserSampleOscFmBlock(osc,modBuffer,buf,size,gain);
			break;

		}
}
//-----------------------------------------------------------
int16_t calcNextOscSampleFm(OscInfo* osc,OscInfo* modOsc)
{
	if (osc_waveInterpActive(osc)) {
		return calcPeriodicInterpFm(osc, modOsc);
	}

	switch(osc->waveform)
	{
	case SINE:
		return calcFmSine(osc,modOsc);
		break;

	case SAW:
		return calcFm(osc,modOsc,sawTable);
		break;

	case TRI:
		return calcFm(osc,modOsc, triTable);
		break;

	case REC:
		return calcFm(osc,modOsc,recTable);
		break;

	case NOISE:
		return calcNoise(osc);
		break;

	case CRASH:
		return calcSampleOscFm(osc,modOsc);
		break;

	default:

		return 0;
		break;

	}
	return 0;
}
//-----------------------------------------------------------
void calcNoiseBlock(OscInfo* osc, int16_t* buf, const uint8_t size ,const float gain)
{
	int i;
	for(i=0;i<size;i++)
	{
		const uint32_t lastPhase = osc->phase;
		osc->phase += osc->phaseInc;

		if(lastPhase > osc->phase)
		{
			//overflow happened -> phaseWrapped
			osc->output = GetRngValue(); //normal pitched white noise
		}

		buf[i] = osc->output * gain;
	}
}
//-----------------------------------------------------------
int16_t calcNoise(OscInfo* osc)
{
	osc->phase += osc->phaseInc;

	//check overflow flag, ( if osc->phase == osc->phaseInc a reset occured ==> retrigger, too
	APSR_Type apsr;
	apsr.w = __get_APSR();
	if(apsr.b.V ||  (osc->phase == osc->phaseInc))
	{
		//overflow happened -> phaseWrapped

		uint16_t rnd = GetRngValue();
		if( rnd > 0x00ff)
		{
			if( rnd > 0x000f)
			{
				osc->output = 32767;
			}
			else
			{
				osc->output = -32768;
			}

		}
	}
	else
	{
			osc->output = 0;
	}
	return osc->output;
}
//-----------------------------------------------------------
void calcNextOscSampleBlock(OscInfo* osc, int16_t* buf, const uint8_t size, const float gain)
{
	/* Waveform blend guard: if modulation has set a fractional waveform target
	   for this oscillator this DSP block, render both neighbors and blend. */
	if (osc_waveInterpActive(osc)) {
		calcPeriodicInterpBlock(osc, buf, size, gain);
		return;
	}

	switch(osc->waveform)
		{
		case SINE:
			calcSineBlock(osc,buf,size, gain);
			break;

		case SAW:
			calcWavetableOscBlock(osc,sawTable,buf,size, gain);
			break;

		case TRI:
			calcWavetableOscBlock(osc,triTable,buf,size, gain);
			break;

		case REC:
			calcWavetableOscBlock(osc,recTable,buf,size, gain);
			break;

		case NOISE:
			calcNoiseBlock(osc,buf,size, gain);
			break;

		case CRASH:
			calcSampleOscBlock(osc,buf,size, gain);
			break;

		default://sample playback
			if( osc->waveform - OSC_SAMPLE_START > sampleMemory_getNumSamples() ) return; // return if out of range

			calcUserSampleOscBlock(osc,buf,size, gain);
			break;
		}
}
//-----------------------------------------------------------
int16_t calcNextOscSample(OscInfo* osc)
{
	if (osc_waveInterpActive(osc)) {
		return calcPeriodicInterp(osc);
	}

	switch(osc->waveform)
	{
	case SINE:
		return calcSine(osc);
		break;

	case SAW:
		return calcWavetableOsc(osc,sawTable);
		break;

	case TRI:
		return calcWavetableOsc(osc,triTable);
		break;

	case REC:
		return calcWavetableOsc(osc,recTable);
		break;

	case NOISE:
		return calcNoise(osc);
		break;

	case CRASH:
		return calcSampleOsc(osc);
		break;

	default:

		return 0;
		break;

	}
	return 0;
};
//---------------------------------------------------------------
void calcWavetableOscBlock(OscInfo* osc, const int16_t table[][1024], int16_t* buf, const uint8_t size ,const float gain)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		const uint32_t oscPhase = osc->phase;
			int16_t oscOut ;
			uint32_t  itg	= oscPhase>>22;
		#if INTERPOLATE_OSC
			//todo use ldm instead of ldr to laod multiple values from memory
			oscOut = table[osc->tableOffset][itg++];
			const float frac = (itg&0x003FFFFF)*2.38418579101562e-07f; //2.38... => 1.f/0x3fffff
			oscOut += frac*(table[osc->tableOffset][itg] - oscOut);
		#else
			oscOut = table[osc->tableOffset][itg];
		#endif

			osc->phase = oscPhase + osc->phaseInc;
			buf[i] = oscOut * gain;
	}
}

//---------------------------------------------------------------
int16_t calcWavetableOsc(OscInfo* osc,  const int16_t table[][1024])
{
	const uint32_t oscPhase = osc->phase;
	int16_t oscOut ;

	uint32_t  itg	= oscPhase>>22;

#if INTERPOLATE_OSC
	//todo use ldm instead of ldr to laod multiple values from memory
	oscOut = table[osc->tableOffset][itg++];
	const float frac = (itg&0x003FFFFF)*2.38418579101562e-07f; //2.38... => 1.f/0x3fffff
	oscOut += frac*(table[osc->tableOffset][itg] - oscOut);
#else
	oscOut = table[osc->tableOffset][itg];
#endif

	osc->phase = oscPhase + osc->phaseInc;
	osc->output = oscOut;
	return oscOut;

};

//------------------------------------------------------------------
void calcSampleOscFmBlock(OscInfo* osc,int16_t* modBuffer, int16_t* buf, uint8_t size ,const float gain)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		const uint32_t oscPhase 	= osc->phase;
		int16_t oscOut ;

		const uint32_t index =  oscPhase + (((uint32_t)(modBuffer[i]*osc->fmMod))<<14);
		uint32_t  itg	= index>>17;

	#if INTERPOLATE_FM_OSC
		oscOut = crashSample[itg++];
		const float frac	= (index&20000)*0.00000762939453125f;//=> * 1/0x20000
		oscOut += frac*(crashSample[itg] - oscOut);
	#else
		oscOut = crashSample[itg];
	#endif

		oscOut = (oscOut - 127)*256;

		osc->phase = oscPhase + osc->phaseInc;
		osc->output = oscOut;
		buf[i] = oscOut * gain;
	}
}
//------------------------------------------------------------------
int16_t calcSampleOscFm(OscInfo* osc, OscInfo* modOsc)
{

	const uint32_t oscPhase 	= osc->phase;
	int16_t oscOut ;

	const uint32_t index =  oscPhase + (((uint32_t)(modOsc->output*osc->fmMod))<<14);
	uint32_t  itg	= index>>17;

#if INTERPOLATE_FM_OSC
	oscOut = crashSample[itg++];
	const float frac	= (index&20000)*0.00000762939453125f;//=> * 1/0x20000
	oscOut += frac*(crashSample[itg] - oscOut);
#else
	oscOut = crashSample[itg];
#endif

	oscOut = (oscOut - 127)*256;

	osc->phase = oscPhase + osc->phaseInc;
	osc->output = oscOut;
	return oscOut;
};
//------------------------------------------------------------------
/* USER-SAMPLE FM SCALING CONSTANTS  (Session 039)
   WHAT: fixed-point scale and clamp for the imported-sample phase-modulation
         offset computed in calcUserSampleOscFmBlock().
   WHY:  osc_calcUserSampleFreq() uses freq2PhaseIncr32767(), so an imported
         sample advances 1024 frames per oscillator cycle with a 17-bit
         fractional read position. The wavetable FM path (calcFm /
         calcFmBlock) offsets its 32-bit phase by (mod*fmMod)<<19, i.e.
         mod*fmMod/8192 cycles = mod*fmMod/8 frames = (mod*fmMod)<<14 in the
         17-bit domain. Scaling by 2^14 therefore gives imported samples the
         same modulation index as SINE/SAW/TRI/REC; the built-in CRASH path
         (calcSampleOscFmBlock) already uses the same <<14.
         The product clamp keeps product*SCALE inside int32_t
         (131071 x 16384 = 2^31 - 16384) even if a modulation route ever
         pushes fmMod above 1.0.
   INPUT/OUTPUT: compile-time constants only.
   ACCESSORS: calcUserSampleOscFmBlock() (this file) only.
   AFFILIATES: freq2PhaseIncr32767(), osc_calcUserSampleFreq(),
               calcSampleOscFmBlock() (this file). */
#define OSC_USER_SAMPLE_FM_SCALE        16384     /* 2^14: (mod*fmMod) -> 17-bit fixed frames */
#define OSC_USER_SAMPLE_FM_PRODUCT_MAX  131071    /* |mod*fmMod| clamp so product*SCALE fits int32_t */

/* IMPORTED-SAMPLE FM BLOCK RENDERER  (Session 039; replaces the Phase 2 stub)
   WHAT: renders `size` frames of the imported sample selected by
         osc->waveform - OSC_SAMPLE_START, phase-modulated by modBuffer.
         Each output frame is read at (unmodulated play head + FM offset);
         the play head itself advances exactly as calcUserSampleOscBlock()
         advances it.
   WHY:  the sample-import Phase 2 stub discarded modBuffer and delegated to
         the non-FM reader, so selecting an imported sample on any FM carrier
         (Drum 1-3 in FM mode, Cymbal, Hihat) silently removed FM. All LXR
         "FM" is phase modulation of the read index, and imported samples now
         follow that same model. Keeping the play-head advance unmodulated
         means FM cannot change a one-shot's length, end detection, or
         sampleActive timing, and with fmMod == 0 the output is bit-identical
         to calcUserSampleOscBlock().
         Bounds: one-shots clamp the modulated read to [0, size-1]; loops wrap
         it with one 32-bit modulo (hardware divide on Cortex-M4) only when
         the read leaves the sample, so a tiny loop with a large offset still
         costs O(1). Interpolation uses the same "nextPos < sampleSize" rule as
         the non-FM path (no wrap) so fmMod == 0 stays bit-identical.
   INPUT:  osc       - carrier oscillator: waveform, samplePosition,
                       sampleFraction (17-bit), sampleActive, phaseInc, fmMod.
           modBuffer - `size` modulator samples, already scaled by the
                       voice's modulator gain (e.g. fmModAmount).
           buf       - output buffer of `size` int16 frames.
           size      - frames to render (OUTPUT_DMA_SIZE in practice).
           gain      - output gain applied per frame.
   OUTPUT: buf filled with PCM, or zeros for an invalid slot, zero-length or
           unmapped sample, or a finished one-shot. osc->samplePosition,
           sampleFraction and sampleActive advance as in the non-FM path;
           osc->output holds the last rendered frame.
   ACCESSORS: calcNextOscSampleFmBlock() default branch (this file), reached
              from DrumVoice.c calcDrumVoiceSyncBlock() (FM mode),
              CymbalVoice.c and HiHat.c HiHat_calcSyncBlock().
   AFFILIATES: calcUserSampleOscBlock() (non-FM twin; keep the two loops in
               step), osc_resetSamplePlayback() (trigger re-arm),
               sampleMemory_getSampleInfo()/getSampleSizeFrames()/
               isSampleLooped() (SampleRom/SampleMemory.c). */
void calcUserSampleOscFmBlock(OscInfo* osc,int16_t* modBuffer, int16_t* buf, uint8_t size ,const float gain)
{
   const uint8_t sampleIndex = (uint8_t)(osc->waveform - OSC_SAMPLE_START);
   const uint8_t sampleCount = sampleMemory_getNumSamples();

   /* Guard first so a stale AVR waveform value such as s1 after only s0 loaded
      produces silence rather than reading past the SampleInfo table. */
   if(sampleIndex >= sampleCount)
   {
      memset(buf, 0, size * sizeof(int16_t));
      return;
   }

   SampleInfo info = sampleMemory_getSampleInfo(sampleIndex);
   const uint32_t sampleSize = sampleMemory_getSampleSizeFrames(info);
   const uint8_t looped = sampleMemory_isSampleLooped(info);
   int16_t* sampleData = (int16_t*)((int8_t*)(info.offset));

   /* Same metadata/one-shot guard as calcUserSampleOscBlock(). */
   if(sampleSize == 0u || info.offset < SAMPLE_ROM_START_ADDRESS || !osc->sampleActive)
   {
      memset(buf, 0, size * sizeof(int16_t));
      return;
   }

   uint8_t i;
   for(i = 0; i < size; i++)
   {
      uint32_t pos = osc->samplePosition;
      int16_t oscOut;

      if(pos >= sampleSize)
      {
         if(looped)
         {
            pos %= sampleSize;
         }
         else
         {
            /* One-shot finished: identical gate behaviour to the non-FM path. */
            osc->sampleActive = 0u;
            osc->output = 0;
            buf[i] = 0;
            continue;
         }
      }

      /* Signed phase-modulation offset in 17-bit fixed frames. Multiply, never
         left-shift, a negative value; the CRASH path's (uint32_t)(negative
         float) cast is undefined behaviour and must not be copied. */
      int32_t fmProduct = (int32_t)(modBuffer[i] * osc->fmMod);
      if(fmProduct > OSC_USER_SAMPLE_FM_PRODUCT_MAX)
         fmProduct = OSC_USER_SAMPLE_FM_PRODUCT_MAX;
      else if(fmProduct < -OSC_USER_SAMPLE_FM_PRODUCT_MAX)
         fmProduct = -OSC_USER_SAMPLE_FM_PRODUCT_MAX;
      const int32_t fmOffset = fmProduct * OSC_USER_SAMPLE_FM_SCALE;

      /* Split into floor(frames) + positive 17-bit fraction, then add to the
         unmodulated head. GCC/ARM >> on int32_t is arithmetic (floor). */
      uint32_t readFrac = (osc->sampleFraction & 0x1ffffu) + ((uint32_t)fmOffset & 0x1ffffu);
      int32_t readPos = (int32_t)pos + (fmOffset >> 17) + (int32_t)(readFrac >> 17);
      readFrac &= 0x1ffffu;

      if(looped)
      {
         const int32_t size32 = (int32_t)sampleSize;
         if(readPos >= size32 || readPos < 0)
         {
            readPos %= size32;
            if(readPos < 0)
               readPos += size32;
         }
      }
      else if(readPos < 0)
      {
         readPos = 0;
         readFrac = 0u;
      }
      else if(readPos >= (int32_t)sampleSize)
      {
         readPos = (int32_t)sampleSize - 1;
         readFrac = 0u;
      }

#if INTERPOLATE_OSC
      oscOut = sampleData[readPos];
      {
         const uint32_t nextPos = (uint32_t)readPos + 1u;
         if(nextPos < sampleSize)
         {
            const float frac = readFrac * 0.00000762939453125f; /* 1/0x20000 */
            oscOut += frac * (sampleData[nextPos] - oscOut);
         }
      }
#else
      oscOut = sampleData[readPos];
#endif

      /* Unmodulated play-head advance: must match calcUserSampleOscBlock(). */
      osc->sampleFraction += osc->phaseInc;
      osc->samplePosition += (osc->sampleFraction >> 17);
      osc->sampleFraction &= 0x1ffffu;

      if(looped && osc->samplePosition >= sampleSize)
      {
         osc->samplePosition %= sampleSize;
      }

      osc->output = oscOut;
      buf[i] = oscOut * gain;
   }
}
//---------------------------------------------------------------
void calcUserSampleOscBlock(OscInfo* osc, int16_t* buf, const uint8_t size ,const float gain)
{
   const uint8_t sampleIndex = (uint8_t)(osc->waveform - OSC_SAMPLE_START);
   const uint8_t sampleCount = sampleMemory_getNumSamples();

   /* Phase 2 fixed the off-by-one: valid slots are 0..sampleCount-1. Any
      waveform outside that range is zero-filled so an empty menu slot cannot
      dereference uncommitted flash metadata. */
   if(sampleIndex >= sampleCount)
   {
      memset(buf, 0, size * sizeof(int16_t));
      return;
   }

   SampleInfo info = sampleMemory_getSampleInfo(sampleIndex);
   const uint32_t sampleSize = sampleMemory_getSampleSizeFrames(info);
   const uint8_t looped = sampleMemory_isSampleLooped(info);
   int16_t* sampleData = (int16_t*)((int8_t*)(info.offset));

   /* Metadata guard: zero-length samples, impossible offsets, and completed
      one-shots all render silence. Looped samples never clear sampleActive. */
   if(sampleSize == 0u || info.offset < SAMPLE_ROM_START_ADDRESS || !osc->sampleActive)
   {
      memset(buf, 0, size * sizeof(int16_t));
      return;
   }

   uint8_t i;
   for(i = 0; i < size; i++)
   {
      uint32_t pos = osc->samplePosition;
      uint32_t nextPos = pos + 1u;
      int16_t oscOut = 0;

      if(pos >= sampleSize)
      {
         if(looped)
         {
            /* /loops samples wrap at the imported frame count. */
            pos %= sampleSize;
         }
         else
         {
            /* /samples are one-shots: clear the gate once the read head passes
               the final frame, then output zeros until the next trigger reset. */
            osc->sampleActive = 0u;
            buf[i] = 0;
            continue;
         }
      }

#if INTERPOLATE_OSC
      oscOut = sampleData[pos];
      if(nextPos < sampleSize)
      {
         const float frac = (osc->sampleFraction & 0x1ffffu) * 0.00000762939453125f;
         oscOut += frac * (sampleData[nextPos] - oscOut);
      }
#else
      oscOut = sampleData[pos];
#endif

      osc->sampleFraction += osc->phaseInc;
      osc->samplePosition += (osc->sampleFraction >> 17);
      osc->sampleFraction &= 0x1ffffu;

      if(looped && sampleSize > 0u && osc->samplePosition >= sampleSize)
      {
         osc->samplePosition %= sampleSize;
      }

      buf[i] = oscOut * gain;
   }
}
//---------------------------------------------------------------
void calcSampleOscBlock(OscInfo* osc, int16_t* buf, const uint8_t size ,const float gain)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		const uint32_t oscPhase = osc->phase;
		int16_t oscOut ;
		uint32_t  itg	= oscPhase>>17;


	#if INTERPOLATE_OSC
		//todo use ldm instead of ldr to laod multiple values from memory
		oscOut = crashSample[itg++];
		const float frac = (oscPhase&20000)*0.00000762939453125f;//=> * 1/0x20000
		oscOut += frac*(crashSample[itg] - oscOut);
	#else
		oscOut = crashSample[itg];
	#endif
		oscOut = (oscOut - 127)*256;

		osc->phase = oscPhase + osc->phaseInc;
		buf[i] = oscOut * gain;
	}
}
//---------------------------------------------------------------
int16_t calcSampleOsc(OscInfo* osc)
{
	const uint32_t oscPhase = osc->phase;
	int16_t oscOut ;

	uint32_t  itg	= oscPhase>>17;

#if INTERPOLATE_OSC
	//todo use ldm instead of ldr to laod multiple values from memory
	oscOut = crashSample[itg++];
	const float frac = (oscPhase&20000)*0.00000762939453125f;//=> * 1/0x20000
	oscOut += frac*(crashSample[itg] - oscOut);
#else
	oscOut = crashSample[itg];
#endif
	oscOut = (oscOut - 127)*256;

	osc->phase = oscPhase + osc->phaseInc;
	osc->output = oscOut;
	return oscOut;

};
 //-----------------------------------------------------------
void osc_calcSineFreq(OscInfo* osc)
{
	osc->phaseInc = freq2PhaseIncr(osc->freq*osc->pitchMod*osc->modNodeValue);
}
//-----------------------------------------------------------
void osc_calcNoiseFreq(OscInfo* osc)
{
	osc_calcSineFreq(osc);
}
//-----------------------------------------------------------
void osc_calcWavetableFreq(OscInfo* osc)
{
	const float currentFreq = osc->freq*osc->pitchMod*osc->modNodeValue;
	osc->phaseInc = freq2PhaseIncr1024(currentFreq);

	const uint8_t overtoneIndex = freqToTableIndex(currentFreq);
	osc->tableOffset = overtoneIndex>10?10:overtoneIndex;
}
//-----------------------------------------------------------
void osc_calcSampleFreq(OscInfo* osc)
{
	const float currentFreq = osc->freq*osc->pitchMod*osc->modNodeValue;
	osc->phaseInc = freq2PhaseIncr32767(currentFreq);
}
//-----------------------------------------------------------
void osc_calcUserSampleFreq(OscInfo* osc)
{
	const float currentFreq = osc->freq*osc->pitchMod*osc->modNodeValue;
	osc->phaseInc = freq2PhaseIncr32767(currentFreq);
}
//-----------------------------------------------------------
 void osc_setFreq(OscInfo* osc)
 {
		switch(osc->waveform)
		{
		case SINE:
			osc_calcSineFreq(osc);
			break;

		case SAW:
			osc_calcWavetableFreq(osc);
			break;

		case TRI:
			osc_calcWavetableFreq(osc);
			break;

		case REC:
			osc_calcWavetableFreq(osc);
			break;

		case NOISE:
			osc_calcNoiseFreq(osc);
			break;


		case CRASH:
			osc_calcSampleFreq(osc);
			break;

		//samples
		default:
			osc_calcUserSampleFreq(osc);
			break;

		}
 }
 //-----------------------------------------------------------
 void osc_setBaseNote(OscInfo* osc, uint8_t baseNote)
 {

	 //get fine tune
	 const float cent = midiParser_calcDetune(osc->midiFreq&0xff);
	 //calc coarse tune
	 int16_t note =  (osc->midiFreq>>8) + (baseNote-SEQ_DEFAULT_NOTE);
	 if(note>127)note=127;
	 if(note<0)note=0;

	 osc->freq = MidiNoteFrequencies[note]*cent;
	 osc->baseNote = baseNote;
 };

 //-----------------------------------------------------------
 void osc_recalcFreq(OscInfo* osc)
 {
	 //get fine tune
	 const float cent = midiParser_calcDetune(osc->midiFreq&0xff);
	 //calc coarse tune
	 int16_t note =  (osc->midiFreq>>8) + (osc->baseNote-SEQ_DEFAULT_NOTE);

	 if(note>127)note=127;
 	 if(note<0)note=0;

	 osc->freq = MidiNoteFrequencies[note]*cent;
 }
 //-----------------------------------------------------------
