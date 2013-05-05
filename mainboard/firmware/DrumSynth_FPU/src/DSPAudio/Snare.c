/*
 * Snare.c
 *
 *  Created on: 17.04.2012
 *      Author: Julian
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

//	voiceArray[voiceNr].status |= IS_PLAYING;
	snareVoice.osc.phase = 0;
	//snareVoice.noiseOsc.phase = 0;

	DecayEg_trigger(&snareVoice.oscPitchEg);
	slopeEg2_trigger(&snareVoice.oscVolEg);
	snareVoice.velo = vel/127.f;
//	ADEG_trigger(&snareVoice[voiceNr].noiseVolEg);

	osc_setBaseNote(&snareVoice.osc,note);
	//TODO noise muss mit transponiert werden

	transient_trigger(&snareVoice.transGen);

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


	osc_setFreq(&snareVoice.osc);
	osc_setFreq(&snareVoice.noiseOsc);


}
//---------------------------------------------------
void Snare_calcSyncBlock(int16_t* buf, const uint8_t size)
{

	int16_t transBuf[size];

	calcNoiseBlock(&snareVoice.noiseOsc,buf,size,0.5f);
	SVF_calcBlockZDF(&snareVoice.filter,snareVoice.filterType,buf,size);

	/*
	if((snareVoice.filterType&0x1))
	{
		//get low pass
		bufferTool_moveBuffer(buf,SVF_getLpBlockInt(&snareVoice.filter),size);
	}
	else
	{
		//clear buffer
		bufferTool_clearBuffer(buf,size);
	}
	if((snareVoice.filterType&0x2)) bufferTool_addBuffersSaturating(buf,SVF_getHpBlockInt(&snareVoice.filter),size);
	if((snareVoice.filterType&0x4)) bufferTool_addBuffersSaturating(buf,SVF_getBpBlockInt(&snareVoice.filter),size);
	*/


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
