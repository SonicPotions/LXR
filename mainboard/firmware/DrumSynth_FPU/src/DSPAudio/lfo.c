/*
 * lfo.c
 *
 *  Created on: 14.01.2013
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

//-------------------------------------------------------------
#include "lfo.h"
#include "random.h"
#include <math.h>

#include "DrumVoice.h"
#include "CymbalVoice.h"
#include "HiHat.h"
#include "Snare.h"
#include "valueShaper.h"
//-------------------------------------------------------------
void lfo_init(Lfo *lfo)
{
	lfo->phase 			= 0;
	lfo->phaseInc 		= 1;
	lfo->phaseOffset	= 0;
	lfo->retrigger		= 0;
	lfo->waveform		= SINE;
	lfo->sync			= 0;
	lfo->freq			= 1;
	lfo->modNodeValue	= 1;

	modNode_init(&lfo->modTarget);
}
//-------------------------------------------------------------
float lfo_calc(Lfo *lfo)
{
	uint32_t oldPhase 		= lfo->phase;
	float inc = lfo->phaseInc*lfo->modNodeValue;
	uint32_t incInt = inc;
	lfo->phase 			   += incInt;//lfo->phaseInc*lfo->modNodeValue;
	const uint8_t overflow 	= oldPhase>lfo->phase;

	switch(lfo->waveform)
	{
	//---
		case LFO_SINE:
			return ((sine_table[(lfo->phase>>20)]/32767.f) + 1)/2.f;
		break;
	//---
		case LFO_TRI:
			return (1.f-fabsf( (lfo->phase/(float)0xffffffff)*2-1 ) );
			break;
	//---
		case LFO_SAW_UP:
		return lfo->phase/(float)0xffffffff ;
		break;
	//---
		case LFO_REC:
		if(lfo->phase > 0x7fffffff)
		{
			return 1.f;
		}
		else
		{
			return 0.f;
		}
		break;
	//---
		case LFO_NOISE:
		if(overflow)
		{
			lfo->rnd = GetRngValue();
			lfo->rnd = lfo->rnd  / (float)0xffffffff ;
		}

			return lfo->rnd;
		break;
	//---
		case LFO_SAW_DOWN:
		return (0xffffffff-(lfo->phase)) / (float)0xffffffff;
		break;
	//---
		case LFO_EXP_UP:
			{
				float x = lfo->phase/(float)0xffffffff;
				return x*x*x;
			}
			break;
	//---
		case LFO_EXP_DOWN:
			{
				float x = (0xffffffff-(lfo->phase)) / (float)0xffffffff;
				return x*x*x;//valueShaperF2F(x,-0.9f);
			}
			break;
	//---
		default:
		return 0;
		break;
	}
	return 0;
}
//-------------------------------------------------------------
void lfo_dispatchNextValue(Lfo* lfo)
{
	float val = lfo_calc(lfo);
	modNode_updateValue(&lfo->modTarget,val);
}
//-------------------------------------------------------------
uint32_t lfo_calcPhaseInc(float freq, uint8_t sync)
{
	if(sync==0)//no sync
	{
		return freq/(LFO_SR) * 0xffffffff;
	}

	//sync is on

	float bpm =  seq_getBpm();

	float tempoAsFrequency = bpm > 0.0 ? bpm / 60.f /4: 0.0f; //bpm/60 = beat duration /4 = bar duration

	float scaler;

	switch(sync)
	{
		default: //no sync

		return freq/(LFO_SR) * 0xffffffff;
		break;

		case 1: // 4/1
		scaler = 0.25f;//NoteTypeScalers[Quadruple] * NoteDivisionScalers[Whole];
		break;

		case 2: // 2/1
		scaler = 0.5f;//NoteTypeScalers[Double] * NoteDivisionScalers[Whole];
		break;

		case 3: // 1/1
		scaler = 1;//NoteTypeScalers[Regular] * NoteDivisionScalers[Whole];
		break;

		case 4: // 1/2
		scaler = 2;// NoteTypeScalers[Regular] * NoteDivisionScalers[Half];
		break;

		case 5: //1/3
		scaler = 3;// NoteTypeScalers[Regular] * NoteDivisionScalers[Third];
		break;

		case 6: // 1/4
		scaler = 4;// NoteTypeScalers[Regular] * NoteDivisionScalers[Quarter];
		break;

		case 7: // 1/6
		scaler = 6;// NoteTypeScalers[Regular] * NoteDivisionScalers[Sixth];
		break;

		case 8: // 1/8
		scaler = 8;// NoteTypeScalers[Regular] * NoteDivisionScalers[Eighth];
		break;

		case 9: // 1/12
		scaler = 12;//NoteTypeScalers[Regular] * NoteDivisionScalers[Twelfth];
		break;

		case 10: // 1/16
		scaler = 16;//NoteTypeScalers[Regular] * NoteDivisionScalers[Sixteenth];
		break;

		case 11: // 1/32
		scaler = 32;//NoteTypeScalers[Regular] * NoteDivisionScalers[Sixteenth];
		break;
	}

	float lfoSyncfreq = tempoAsFrequency * scaler;
	return (lfoSyncfreq / (LFO_SR)) * 0xffffffff;
}
//-------------------------------------------------------------
void lfo_setFreq(Lfo *lfo, float f)
{
	f += 1;
	f = f/128.f;
	f = f*f*f;
	lfo->freq = f*LFO_MAX_F;
	lfo->phaseInc = lfo_calcPhaseInc(lfo->freq,lfo->sync);
}
//-------------------------------------------------------------
void lfo_setSync(Lfo* lfo, uint8_t sync)
{
	lfo->sync = sync;
	lfo->phaseInc = lfo_calcPhaseInc(lfo->freq,lfo->sync);
}
//-------------------------------------------------------------
void lfo_recalcSync()
{
	Lfo* lfo = &voiceArray[0].lfo;
	lfo->phaseInc = lfo_calcPhaseInc(lfo->freq,lfo->sync);

	lfo = &voiceArray[1].lfo;
	lfo->phaseInc = lfo_calcPhaseInc(lfo->freq,lfo->sync);

	lfo = &voiceArray[2].lfo;
	lfo->phaseInc = lfo_calcPhaseInc(lfo->freq,lfo->sync);

	lfo = &snareVoice.lfo;
	lfo->phaseInc = lfo_calcPhaseInc(lfo->freq,lfo->sync);

	lfo = &cymbalVoice.lfo;
	lfo->phaseInc = lfo_calcPhaseInc(lfo->freq,lfo->sync);

	lfo = &hatVoice.lfo;
	lfo->phaseInc = lfo_calcPhaseInc(lfo->freq,lfo->sync);
}
//-------------------------------------------------------------
void lfo_retrigger(uint8_t voice)
{
	if(voiceArray[0].lfo.retrigger == voice+1)
	{
		voiceArray[0].lfo.phase = voiceArray[0].lfo.phaseOffset;
	}
	if(voiceArray[1].lfo.retrigger == voice+1)
	{
		voiceArray[1].lfo.phase = voiceArray[1].lfo.phaseOffset;
	}
	if(voiceArray[2].lfo.retrigger == voice+1)
	{
		voiceArray[2].lfo.phase = voiceArray[2].lfo.phaseOffset;
	}
	if(snareVoice.lfo.retrigger == voice+1)
	{
		snareVoice.lfo.phase = snareVoice.lfo.phaseOffset;
	}
	if(cymbalVoice.lfo.retrigger == voice+1)
	{
		cymbalVoice.lfo.phase = cymbalVoice.lfo.phaseOffset;
	}
	if(hatVoice.lfo.retrigger == voice+1)
	{
		hatVoice.lfo.phase = hatVoice.lfo.phaseOffset;
	}
}
//-------------------------------------------------------------
