/*
 * Snare.c
 *
 *  Created on: 17.04.2012
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



#include "Snare.h"
#include "squareRootLut.h"
#include "modulationNode.h"
#include "TriggerOut.h"


//instance of the snare voice
SnareVoice snareVoice;

//---------------------------------------------------
void Snare_setPan(const uint8_t pan)
{
	//snareVoice.panL = squareRootLut[127-pan];
	//snareVoice.panR = squareRootLut[pan];
	snareVoice.pan = pan;
}
//---------------------------------------------------
void Snare_init()
{
	SnapEg_init(&snareVoice.snapEg);
	Snare_setPan(0);
	snareVoice.vol = 0.8f;

	//snareVoice.panModifier = 1.f;

	snareVoice.noiseOsc.freq = 440;
	snareVoice.noiseOsc.waveform = 1;
	snareVoice.noiseOsc.fmMod = 0;
	snareVoice.noiseOsc.midiFreq = 70<<8;
	snareVoice.noiseOsc.pitchMod = 1.0f;
	snareVoice.noiseOsc.modNodeValue = 1;

	snareVoice.osc.freq = 440;
	snareVoice.osc.waveform = 1;
	snareVoice.osc.fmMod = 0;
	snareVoice.osc.midiFreq = 70<<8;
	snareVoice.osc.modNodeValue = 1;

	setDistortionShape(&snareVoice.distortion, 2.f);

	snareVoice.volumeMod = 1;

	transient_init(&snareVoice.transGen);

	DecayEg_init(&snareVoice.oscPitchEg);
	snareVoice.egPitchModAmount = 0.5f;

	slopeEg2_init(&snareVoice.oscVolEg);

	setDistortionShape(&snareVoice.distortion, 2.f);

	SVF_init(&snareVoice.filter);

	lfo_init(&snareVoice.lfo);
}
//---------------------------------------------------
void Snare_trigger(const uint8_t vel, const uint8_t note)
{
	lfo_retrigger(3);
	//update velocity modulation
	modNode_updateValue(&velocityModulators[3],vel/127.f);

	float offset = 1;
	if(snareVoice.transGen.waveform==1) //offset mode
	{
		offset -= snareVoice.transGen.volume;
	}
	if(snareVoice.osc.waveform == SINE)
		snareVoice.osc.phase = (0x3ff<<20)*offset;//voiceArray[voiceNr].osc.startPhase ;
	else if(snareVoice.osc.waveform > SINE && snareVoice.osc.waveform <= REC)
		snareVoice.osc.phase = (0xff<<20)*offset;
	else
		snareVoice.osc.phase = 0;

	DecayEg_trigger(&snareVoice.oscPitchEg);
	slopeEg2_trigger(&snareVoice.oscVolEg);
	snareVoice.velo = vel/127.f;

	osc_setBaseNote(&snareVoice.osc,note);
	//TODO noise muss mit transponiert werden

	transient_trigger(&snareVoice.transGen);

	SnapEg_trigger(&snareVoice.snapEg);
}
//---------------------------------------------------
void Snare_calcAsync()
{
	//add modulation eg to osc freq (1 = no change. a+eg = original freq + modulation
	const float egPitchVal = DecayEg_calc(&snareVoice.oscPitchEg);
	const float pitchEgValue = egPitchVal*snareVoice.egPitchModAmount;
	snareVoice.osc.pitchMod = 1+pitchEgValue;

	//calc the osc  vol eg
	snareVoice.egValueOscVol = slopeEg2_calc(&snareVoice.oscVolEg);

	//turn off trigger signal if trigger gate mode is on and volume == 0
	if(trigger_isGateModeOn())
	{
		if(!snareVoice.egValueOscVol) {
			trigger_triggerVoice(TRIGGER_4, TRIGGER_OFF);
			voiceControl_noteOff(TRIGGER_4);
		}
	}

	//calc snap EG if transient sample 0 is activated
	if(snareVoice.transGen.waveform == 0)
	{
		const float snapVal = SnapEg_calc(&snareVoice.snapEg, snareVoice.transGen.pitch);
		snareVoice.osc.pitchMod += snapVal*snareVoice.transGen.volume;;
	}

	osc_setFreq(&snareVoice.osc);
	osc_setFreq(&snareVoice.noiseOsc);
}
//---------------------------------------------------
void Snare_calcSyncBlock(int16_t* buf, const uint8_t size)
{
	int16_t transBuf[size];

	calcNoiseBlock(&snareVoice.noiseOsc,buf,size,0.9f);
	SVF_calcBlockZDF(&snareVoice.filter,snareVoice.filterType,buf,size);

	//calc transient sample
	transient_calcBlock(&snareVoice.transGen,transBuf,size);
	bufferTool_addBuffersSaturating(buf,transBuf,size);

	//calc next osc sample
	calcNextOscSampleBlock(&snareVoice.osc,transBuf,size,(1.f-snareVoice.mix));
	//--AS apply filter to synthesized sound as well here if desired, or combine code for more efficiency
	//SVF_calcBlockZDF(&snareVoice.filter,snareVoice.filterType,transBuf,size);

	uint8_t j;
	if(snareVoice.volumeMod)
	{
		for(j=0;j<size;j++)
		{
			//add filter to buffer
			buf[j] *= snareVoice.mix;
			buf[j] = (__QADD16(buf[j],transBuf[j]));
			buf[j] *=  snareVoice.velo * snareVoice.vol * snareVoice.egValueOscVol;
		}
	}
	else
	{
		for(j=0;j<size;j++)
		{
			//add filter to buffer
			buf[j] *= snareVoice.mix;
			buf[j] = (__QADD16(buf[j],transBuf[j]));
			buf[j] *=  snareVoice.vol * snareVoice.egValueOscVol;
		}
	}

	calcDistBlock(&snareVoice.distortion,buf,size);
}
//------------------------------------------------------------------------
