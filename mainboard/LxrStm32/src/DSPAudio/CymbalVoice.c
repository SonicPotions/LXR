/*
 * CymbalVoice.c
 *
 *  Created on: 16.06.2012
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


#include "CymbalVoice.h"
#include "squareRootLut.h"
#include "modulationNode.h"
#include "TriggerOut.h"

CymbalVoice cymbalVoice;
//---------------------------------------------------
void Cymbal_setPan(const uint8_t pan)
{
	//cymbalVoice.panL = squareRootLut[127-pan];
	//cymbalVoice.panR = squareRootLut[pan];
	cymbalVoice.pan = pan;
}
//---------------------------------------------------

void Cymbal_init()
{

	SnapEg_init(&cymbalVoice.snapEg);
	Cymbal_setPan(0);
	cymbalVoice.vol = 0.8f;

	//cymbalVoice.panModifier = 1.f;

	transient_init(&cymbalVoice.transGen);

	cymbalVoice.fmModAmount1 = 0.5f;
	cymbalVoice.fmModAmount2 = 0.5f;

	setDistortionShape(&cymbalVoice.distortion, 2.f);

	cymbalVoice.modOsc.freq = 440;
	cymbalVoice.modOsc.waveform = SINE;
	cymbalVoice.modOsc.fmMod = 0;
	cymbalVoice.modOsc.midiFreq = 70<<8;
	cymbalVoice.modOsc.pitchMod = 1.0f;
	cymbalVoice.modOsc.modNodeValue = 1;

	cymbalVoice.modOsc2.freq = 440;
	cymbalVoice.modOsc2.waveform = NOISE;//SINE;
	cymbalVoice.modOsc2.fmMod = 0;
	cymbalVoice.modOsc2.midiFreq = 70<<8;
	cymbalVoice.modOsc2.pitchMod = 1.0f;
	cymbalVoice.modOsc2.modNodeValue = 1;

	cymbalVoice.osc.freq = 440;
	cymbalVoice.osc.waveform = 1;
	cymbalVoice.osc.fmMod = 1;
	cymbalVoice.osc.midiFreq = 70<<8;
	cymbalVoice.osc.pitchMod = 1.0f;
	cymbalVoice.osc.modNodeValue = 1;

	cymbalVoice.volumeMod = 1;

	slopeEg2_init(&cymbalVoice.oscVolEg);

	SVF_init(&cymbalVoice.filter);

	lfo_init(&cymbalVoice.lfo);

}
//---------------------------------------------------
void Cymbal_trigger( const uint8_t vel, const uint8_t note)
{
	lfo_retrigger(4);
	//update velocity modulation
	modNode_updateValue(&velocityModulators[4],vel/127.f);

	float offset = 1;
	if(cymbalVoice.transGen.waveform==1) //offset mode
	{
		offset -= cymbalVoice.transGen.volume;
	}
	if(cymbalVoice.osc.waveform == SINE)
		cymbalVoice.osc.phase = (0x3ff<<20)*offset;//voiceArray[voiceNr].osc.startPhase ;
	else if(cymbalVoice.osc.waveform > SINE && cymbalVoice.osc.waveform <= REC)
		cymbalVoice.osc.phase = (0xff<<20)*offset;
	else
		cymbalVoice.osc.phase = 0;

	cymbalVoice.modOsc.phase = 0;
	cymbalVoice.modOsc2.phase = 0;

	osc_setBaseNote(&cymbalVoice.osc,note);
	osc_setBaseNote(&cymbalVoice.modOsc,note);
	osc_setBaseNote(&cymbalVoice.modOsc2,note);

	slopeEg2_trigger(&cymbalVoice.oscVolEg);
	cymbalVoice.velo = vel/127.f;

	transient_trigger(&cymbalVoice.transGen);

	SnapEg_trigger(&cymbalVoice.snapEg);
}
//---------------------------------------------------
void Cymbal_calcAsync()
{
	//calc the osc  vol eg
	cymbalVoice.egValueOscVol = slopeEg2_calc(&cymbalVoice.oscVolEg);

	//turn off trigger signal if trigger gate mode is on and volume == 0
	if(trigger_isGateModeOn())
	{
		if(!cymbalVoice.egValueOscVol) {
			trigger_triggerVoice(TRIGGER_5, TRIGGER_OFF);
			voiceControl_noteOff(TRIGGER_5);
		}
	}

	//calc snap EG if transient sample 0 is activated
	if(cymbalVoice.transGen.waveform == 0)
	{
		const float snapVal = SnapEg_calc(&cymbalVoice.snapEg,cymbalVoice.transGen.pitch);
		cymbalVoice.osc.pitchMod = 1 + snapVal*cymbalVoice.transGen.volume;
	}

	//update osc phaseInc
	osc_setFreq(&cymbalVoice.osc);
	osc_setFreq(&cymbalVoice.modOsc);
	osc_setFreq(&cymbalVoice.modOsc2);
}
//---------------------------------------------------
void Cymbal_calcSyncBlock(int16_t* buf, const uint8_t size)
{
		int16_t mod[size];
		int16_t mod2[size];
		//calc next mod osc sample
		calcNextOscSampleBlock(&cymbalVoice.modOsc,mod,size,cymbalVoice.fmModAmount1);
		calcNextOscSampleBlock(&cymbalVoice.modOsc2,mod2,size,cymbalVoice.fmModAmount2);

		//combine both mod oscs to 1 modulation signal
		bufferTool_addBuffersSaturating(mod,mod2,size);

		calcNextOscSampleFmBlock(&cymbalVoice.osc,mod,buf,size,1.f) ;
		SVF_calcBlockZDF(&cymbalVoice.filter,cymbalVoice.filterType,buf,size);

		//calc transient sample
		transient_calcBlock(&cymbalVoice.transGen,mod,size);

		uint8_t j;
		if(cymbalVoice.volumeMod)
		{
			for(j=0;j<size;j++)
			{
				//add filter to buffer
				buf[j] = (__QADD16(buf[j],mod[j])) ;
				buf[j] *=  cymbalVoice.velo * cymbalVoice.vol * cymbalVoice.egValueOscVol;
			}
		}
		else
		{
			for(j=0;j<size;j++)
			{
				//add filter to buffer
				buf[j] = (__QADD16(buf[j],mod[j])) ;
				buf[j] *=  cymbalVoice.vol * cymbalVoice.egValueOscVol;
			}
		}
		calcDistBlock(&cymbalVoice.distortion,buf,size);
}
//---------------------------------------------------

