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

//instance of the snare voice
SnareVoice snareVoice;

//---------------------------------------------------
void Snare_setPan(const uint8_t pan)
{
	/*
	//Pan= -0.5 to 0.5
	snareVoice[voiceNr].panL = sqrtf(0.5f-pan);
	snareVoice[voiceNr].panR = sqrtf(0.5+pan);
	*/

	snareVoice.panL = squareRootLut[127-pan];
	snareVoice.panR = squareRootLut[pan];
}
//---------------------------------------------------

//---------------------------------------------------
void Snare_init()
{

		SnapEg_init(&snareVoice.snapEg);
		Snare_setPan(0.f);
		snareVoice.vol = 0.8f;

		snareVoice.panModifier = 1.f;

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

//		toneControl_init(&snareVoice.toneControl);

		DecayEg_init(&snareVoice.oscPitchEg);
		snareVoice.egPitchModAmount = 0.5f;

		slopeEg2_init(&snareVoice.oscVolEg);
	//		ADEG_init(&voiceArray[i].noiseVolEg);

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


	//snareVoice.osc.phase = 0;

	//only reset phase if envelope is closed
	//if((snareVoice.volEgValueBlock[15]<=0.01f) || (snareVoice.transGen.waveform==1))
	{
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
	}


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
	//calc and dispatch LFO
	//lfo_dispatchNextValue(&snareVoice.lfo);

	//add modulation eg to osc freq (1 = no change. a+eg = original freq + modulation
	const float egPitchVal = DecayEg_calc(&snareVoice.oscPitchEg);
	const float pitchEgValue = egPitchVal*snareVoice.egPitchModAmount;
	snareVoice.osc.pitchMod = 1+pitchEgValue;

	//calc the osc  vol eg
	snareVoice.egValueOscVol = slopeEg2_calc(&snareVoice.oscVolEg);

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
//---------------------------------------------------
int16_t Snare_calcSync()
{
	//if(1)
//	if(snareVoice[voiceNr].oscVolEg.value>0.001f )
	//if(voiceArray[voiceNr].status & IS_PLAYING)
	{
		//calc next noise osc sample
		calcNoise(&snareVoice.noiseOsc);

		//SVF_calc(&snareVoice[voiceNr].filter,snareVoice[voiceNr].noiseOsc.output*0.7f);
		SVF_calc(&snareVoice.filter,snareVoice.noiseOsc.output*1.2f);

		snareVoice.noiseSample = 0;

		snareVoice.noiseSample += (snareVoice.filterType&0x1)*SVF_getLP(&snareVoice.filter);
		snareVoice.noiseSample += ((snareVoice.filterType&0x2)>>1)*SVF_getHP(&snareVoice.filter);
		snareVoice.noiseSample += ((snareVoice.filterType&0x4)>>2)*SVF_getBP(&snareVoice.filter);

		//calc transient sample
		transient_calc(&snareVoice.transGen);

		//snareVoice[voiceNr].noiseSample = SVF_getBP(&snareVoice[voiceNr].filter);
		snareVoice.noiseSample = __SSAT(snareVoice.noiseSample,16)+snareVoice.transGen.output;


		//calc next osc sample
		snareVoice.oscSample = calcNextOscSample(&snareVoice.osc);




		//mix osc and noise source
		//int32_t mixed = voiceArray[voiceNr].oscSample*egValOsc * (1.f-voiceArray[voiceNr].mix) + voiceArray[voiceNr].noiseSample*egValNoise*voiceArray[voiceNr].mix;
		const float mixed = snareVoice.noiseSample*(snareVoice.mix) + snareVoice.oscSample*(1.f-snareVoice.mix);

#if CALC_TONE_CONTROL
		toneControl_calc(&snareVoice.toneControl,mixed);
		//return calcDist(&snareVoice[voiceNr].distortion,mixed*snareVoice[voiceNr].egValueOscVol*snareVoice[voiceNr].vol);
		return snareVoice.toneControl.output*snareVoice.egValueOscVol*snareVoice.vol*snareVoice.velo;
#else
		return mixed*snareVoice.egValueOscVol*snareVoice.vol*snareVoice.velo;
#endif

	}
	//else return 0;

};
//------------------------------------------------------------------------
