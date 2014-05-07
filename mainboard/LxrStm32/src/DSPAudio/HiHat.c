/*
 * HiHat.c
 *
 *  Created on: 18.04.2012
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



#include "HiHat.h"
#include "squareRootLut.h"
#include "modulationNode.h"
#include "TriggerOut.h"

HiHatVoice hatVoice;

//---------------------------------------------------
void HiHat_setPan(const uint8_t pan)
{
	//hatVoice.panL = squareRootLut[127-pan];
	//hatVoice.panR = squareRootLut[pan];
	hatVoice.pan = pan;
}
//---------------------------------------------------

void HiHat_init()
{
	SnapEg_init(&hatVoice.snapEg);
	HiHat_setPan(0);
	hatVoice.vol = 0.8f;

	//hatVoice.panModifier = 1.f;

	transient_init(&hatVoice.transGen);

	hatVoice.fmModAmount1 = 0.5f;
	hatVoice.fmModAmount2 = 0.5f;

	setDistortionShape(&hatVoice.distortion, 2.f);

	hatVoice.modOsc.freq = 440;
	hatVoice.modOsc.waveform = SINE;
	hatVoice.modOsc.fmMod = 0;
	hatVoice.modOsc.midiFreq = 70<<8;
	hatVoice.modOsc.pitchMod = 1.0f;
	hatVoice.modOsc.modNodeValue = 1;

	hatVoice.modOsc2.freq = 440;
	hatVoice.modOsc2.waveform = NOISE;//SINE;
	hatVoice.modOsc2.fmMod = 0;
	hatVoice.modOsc2.midiFreq = 70<<8;
	hatVoice.modOsc2.pitchMod = 1.0f;
	hatVoice.modOsc2.modNodeValue = 1;

	hatVoice.osc.freq = 440;
	hatVoice.osc.waveform = 1;
	hatVoice.osc.fmMod = 1;
	hatVoice.osc.midiFreq = 70<<8;
	hatVoice.osc.pitchMod = 1.0f;
	hatVoice.osc.modNodeValue = 1;

	hatVoice.volumeMod = 1;

	slopeEg2_init(&hatVoice.oscVolEg);

	SVF_init(&hatVoice.filter);

	lfo_init(&hatVoice.lfo);
}
//---------------------------------------------------
void HiHat_trigger( uint8_t vel, uint8_t isOpen, const uint8_t note)
{
	lfo_retrigger(5);

	//update velocity modulation
	modNode_updateValue(&velocityModulators[5],vel/127.f);

	float offset = 1;
	if(hatVoice.transGen.waveform==1) //offset mode
	{
		offset -= hatVoice.transGen.volume;
	}
	if(hatVoice.osc.waveform == SINE)
		hatVoice.osc.phase = (0x3ff<<20)*offset;//voiceArray[voiceNr].osc.startPhase ;
	else if(hatVoice.osc.waveform > SINE && hatVoice.osc.waveform <= REC)
		hatVoice.osc.phase = (0xff<<20)*offset;
	else
		hatVoice.osc.phase = 0;

	osc_setBaseNote(&hatVoice.osc,note);
	osc_setBaseNote(&hatVoice.modOsc,note);
	osc_setBaseNote(&hatVoice.modOsc2,note);

	hatVoice.isOpen = isOpen;
	hatVoice.oscVolEg.decay = isOpen?hatVoice.decayOpen:hatVoice.decayClosed;

	slopeEg2_trigger(&hatVoice.oscVolEg);
	hatVoice.velo = vel/127.f;
	transient_trigger(&hatVoice.transGen);

	SnapEg_trigger(&hatVoice.snapEg);
}
//---------------------------------------------------
void HiHat_calcAsync( )
{
	//calc the osc  vol eg
	hatVoice.egValueOscVol = slopeEg2_calc(&hatVoice.oscVolEg);

	//turn off trigger signal if trigger gate mode is on and volume == 0
	if(trigger_isGateModeOn())
	{
		if(!hatVoice.egValueOscVol)
		{
			if(hatVoice.isOpen)
			{
				trigger_triggerVoice(TRIGGER_7, TRIGGER_OFF);
				voiceControl_noteOff(TRIGGER_7);
			} else {
				trigger_triggerVoice(TRIGGER_6, TRIGGER_OFF);
				voiceControl_noteOff(TRIGGER_6);
			}
		}
	}

	//calc snap EG if transient sample 0 is activated
	if(hatVoice.transGen.waveform == 0)
	{
		const float snapVal = SnapEg_calc(&hatVoice.snapEg, hatVoice.transGen.pitch);
		hatVoice.osc.pitchMod = 1 + snapVal*hatVoice.transGen.volume;
	}

	osc_setFreq(&hatVoice.osc);
	osc_setFreq(&hatVoice.modOsc);
	osc_setFreq(&hatVoice.modOsc2);
}
//---------------------------------------------------
void HiHat_calcSyncBlock(int16_t* buf, const uint8_t size)
{
	//2 buffers for the mod oscs
	int16_t mod1[size],mod2[size];
	//calc next mod osc samples, scaled with mod amount
	calcNextOscSampleBlock(&hatVoice.modOsc,mod1,size, hatVoice.fmModAmount1);
	calcNextOscSampleBlock(&hatVoice.modOsc2,mod2,size,  hatVoice.fmModAmount2);

	//combine both mod oscs to 1 modulation signal
	bufferTool_addBuffersSaturating(mod1,mod2,size);

	calcNextOscSampleFmBlock(&hatVoice.osc,mod1,buf,size,0.5f) ;

	SVF_calcBlockZDF(&hatVoice.filter,hatVoice.filterType,buf,size);

	//calc transient sample
	transient_calcBlock(&hatVoice.transGen,mod1,size);

	uint8_t j;
	if(hatVoice.volumeMod)
	{
		for(j=0;j<size;j++)
		{
			//add filter to buffer
			buf[j] = __QADD16(buf[j],mod1[j]);
			buf[j] *= hatVoice.velo * hatVoice.vol * hatVoice.egValueOscVol;
		}
	}
	else
	{
		for(j=0;j<size;j++)
		{
			//add filter to buffer
			buf[j] = __QADD16(buf[j],mod1[j]);
			buf[j] *= hatVoice.vol * hatVoice.egValueOscVol;
		}
	}

	calcDistBlock(&hatVoice.distortion,buf,size);
}
//---------------------------------------------------

