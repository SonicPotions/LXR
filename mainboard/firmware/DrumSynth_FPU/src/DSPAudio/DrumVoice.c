/*
 * DrumVoice.c
 *
 *  Created on: 03.04.2012
 *      Author: Julian
 */

#include "DrumVoice.h"
#include "Oscillator.h"
#include "random.h"
#include "math.h"
#include "squareRootLut.h"
#include "BufferTools.h"
#include "ParameterArray.h"
#include "modulationNode.h"

//---------------------------------------------------
DrumVoice voiceArray[NUM_VOICES];
//---------------------------------------------------
void setPan(const uint8_t voiceNr, const uint8_t pan)
{
	//Pan= -0.5 to 0.5
	/*
	voiceArray[voiceNr].panL = sqrtf(0.5f-pan);
	voiceArray[voiceNr].panR = sqrtf(0.5+pan);
	*/
	voiceArray[voiceNr].panL = squareRootLut[pan];
	voiceArray[voiceNr].panR = squareRootLut[127-pan];

}
//---------------------------------------------------
void drum_setPhase(const uint8_t phase, const uint8_t voiceNr)
{
	const uint32_t startPhase = (phase/127.f)*0xffffffff;
	voiceArray[voiceNr].osc.startPhase = startPhase;
	voiceArray[voiceNr].modOsc.startPhase = startPhase;
}
//---------------------------------------------------
void initDrumVoice()
{
	//initOsc();


	int i;
	for(i=0;i<NUM_VOICES;i++)
	{
//		voiceArray[i].mix = 0.5f;
		setPan(i,0.f);
		voiceArray[i].vol = 0.8f;

		voiceArray[i].panModifier = 1.f;
//		voiceArray[i].filterMode = LP;

		voiceArray[i].fmModAmount = 0.5f;

		transient_init(&voiceArray[i].transGen);

#if ENABLE_DRUM_SVF
		SVF_init(&voiceArray[i].filter);
		voiceArray[i].filterType = 0x01;
#endif


		//toneControl_init(&voiceArray[i].toneControl);

		lfo_init(&voiceArray[i].lfo);


		voiceArray[i].modOsc.freq = 440;
		voiceArray[i].modOsc.waveform = 1;
		voiceArray[i].modOsc.fmMod = 0;
		voiceArray[i].modOsc.midiFreq = 70<<8;
		voiceArray[i].modOsc.pitchMod = 1.0f;
		voiceArray[i].modOsc.modNodeValue = 1;

		voiceArray[i].volumeMod = 1;

		voiceArray[i].osc.freq = 440;
		voiceArray[i].osc.modNodeValue = 1;
		voiceArray[i].osc.waveform = TRI+i; //for testing init to tri,saw,rec
		voiceArray[i].osc.fmMod = 0;
		voiceArray[i].osc.midiFreq = 70<<8;

		DecayEg_init(&voiceArray[i].oscPitchEg);
		voiceArray[i].egPitchModAmount = 0.5f;

		slopeEg2_init(&voiceArray[i].oscVolEg);
//		ADEG_init(&voiceArray[i].noiseVolEg);

		setDistortionShape(&voiceArray[i].distortion, 2.f);

		initOnePole(&voiceArray[i].ampFilter);
		setOnePoleCoef(&voiceArray[i].ampFilter,0.05f);


#if ENABLE_MIX_OSC
		voiceArray[i].mixOscs = true;
#endif

		voiceArray[i].decimationCnt = 0;
		voiceArray[i].decimationRate = 1;


	}


	//custom sounds

	slopeEg2_setDecay(&voiceArray[0].oscVolEg,50,AMP_EG_SYNC);
	slopeEg2_setAttack(&voiceArray[0].oscVolEg,0,AMP_EG_SYNC);

	voiceArray[0].vol = 0.9f;
	voiceArray[0].osc.freq = 110;


}
//---------------------------------------------------
void Drum_trigger(const uint8_t voiceNr, const uint8_t vol, const uint8_t note)
{

	lfo_retrigger(voiceNr);

	//update velocity modulation
	modNode_updateValue(&velocityModulators[voiceNr],vol/127.f);

//	voiceArray[voiceNr].status |= IS_PLAYING;
	//only reset phase if envelope is closed
	if(voiceArray[voiceNr].volEgValueBlock[15]<=0.01f)
	{
		if(voiceArray[voiceNr].osc.waveform == SINE)
			voiceArray[voiceNr].osc.phase = 0x3ff<<20;//voiceArray[voiceNr].osc.startPhase ;
		else if(voiceArray[voiceNr].osc.waveform > SINE && voiceArray[voiceNr].osc.waveform <= REC)
			voiceArray[voiceNr].osc.phase = 0xff<<20;
		else
			voiceArray[voiceNr].osc.phase = 0;

		if(voiceArray[voiceNr].modOsc.waveform == SINE)
			voiceArray[voiceNr].modOsc.phase = 0x3ff<<20;//voiceArray[voiceNr].osc.startPhase ;
		else if(voiceArray[voiceNr].modOsc.waveform > SINE && voiceArray[voiceNr].modOsc.waveform <= REC)
			voiceArray[voiceNr].modOsc.phase = 0xff<<20;
		else
			voiceArray[voiceNr].modOsc.phase = 0;
	}

	osc_setBaseNote(&voiceArray[voiceNr].osc,note);
	osc_setBaseNote(&voiceArray[voiceNr].modOsc,note);


	DecayEg_trigger(&voiceArray[voiceNr].oscPitchEg);
	slopeEg2_trigger(&voiceArray[voiceNr].oscVolEg);
	voiceArray[voiceNr].velo = vol/127.f;
//	ADEG_trigger(&voiceArray[voiceNr].noiseVolEg);

	transient_trigger(&voiceArray[voiceNr].transGen);

	//reset amp eg value
//	voiceArray[voiceNr].egValueOscVol = 0;
}
//---------------------------------------------------
void stopVoice(const uint8_t voiceNr)
{

}
//---------------------------------------------------
void calcDrumVoiceAsync(const uint8_t voiceNr)
{


	//add modulation eg to osc freq (1 = no change. a+eg = original freq + modulation
	const float egPitchVal = DecayEg_calc(&voiceArray[voiceNr].oscPitchEg);
	const float pitchEgValue = egPitchVal*voiceArray[voiceNr].egPitchModAmount;
	voiceArray[voiceNr].osc.pitchMod = 1+pitchEgValue;

	// fm amount with pitch eg
	voiceArray[voiceNr].osc.fmMod = voiceArray[voiceNr].fmModAmount * egPitchVal;

	//calc the osc + noise vol eg
#if (AMP_EG_SYNC==0)
	voiceArray[voiceNr].ampFilterInput = slopeEg2_calc(&voiceArray[voiceNr].oscVolEg);
#endif

	//update osc phaseInc
	osc_setFreq(&voiceArray[voiceNr].osc);
	osc_setFreq(&voiceArray[voiceNr].modOsc);

//	VeloMod_originalValueChanged(PAR_COARSE1);
//	VeloMod_originalValueChanged(PAR_FM_TUNE1);

}

//---------------------------------------------------
void calcDrumVoiceSyncBlock(const uint8_t voiceNr, int16_t* buf, const uint8_t size)
{
	int16_t modBuf[size];

	//calc vol EG
	calcOnePoleBlockFixedInput(&voiceArray[voiceNr].ampFilter, voiceArray[voiceNr].ampFilterInput,voiceArray[voiceNr].volEgValueBlock, size);

	//calc next mod osc sampleBlock
	calcNextOscSampleBlock(&voiceArray[voiceNr].modOsc,modBuf,size,voiceArray[voiceNr].fmModAmount);

	if(voiceArray[voiceNr].mixOscs)
	{
		//calc main osc buffer
		calcNextOscSampleBlock(&voiceArray[voiceNr].osc,buf,size, (1.f-voiceArray[voiceNr].fmModAmount));
		//add mod buffer to main osc buffer
		bufferTool_addBuffersSaturating(buf,modBuf,size);

	}
	else
	{
		calcNextOscSampleFmBlock(&voiceArray[voiceNr].osc,modBuf,buf,size,1.0f);
	}

	//calc transient sample
	transient_calcBlock(&voiceArray[voiceNr].transGen,modBuf,size);

	//Mix with transient buffer
	bufferTool_addBuffersSaturating(buf,modBuf,size);


	//calc filter block
	/*
	if(voiceNr==2)
	{
		SVF_calcBlockZDF(&voiceArray[voiceNr].filter,buf,size);
	}
	else
		*/


	SVF_calcBlockZDF(&voiceArray[voiceNr].filter,voiceArray[voiceNr].filterType,buf,size);

	/*
	//if((voiceArray[voiceNr].filterType&0x1))
	if((voiceArray[voiceNr].filterType==1))
	{
		//get low pass
		bufferTool_moveBuffer(buf,SVF_getLpBlockInt(&voiceArray[voiceNr].filter),size);
	}

	//if((voiceArray[voiceNr].filterType&0x2))
	else if((voiceArray[voiceNr].filterType==2))
		bufferTool_moveBuffer(buf,SVF_getHpBlockInt(&voiceArray[voiceNr].filter),size);

	//if((voiceArray[voiceNr].filterType&0x4))
	else if((voiceArray[voiceNr].filterType==3))
		bufferTool_moveBuffer(buf,SVF_getBpBlockInt(&voiceArray[voiceNr].filter),size);

	else if((voiceArray[voiceNr].filterType==4))
	{
		//peak = hp-lp
		bufferTool_moveBuffer(buf,SVF_getHpBlockInt(&voiceArray[voiceNr].filter),size);
		bufferTool_subBuffersSaturating(buf,SVF_getLpBlockInt(&voiceArray[voiceNr].filter),size);
	}
	else if((voiceArray[voiceNr].filterType==4))
	{
		//notch = high + low
		bufferTool_moveBuffer(buf,SVF_getLpBlockInt(&voiceArray[voiceNr].filter),size);
		bufferTool_addBuffersSaturating(buf,SVF_getHpBlockInt(&voiceArray[voiceNr].filter),size);
	}
	*/



	//attentuate main OSCs by amp EG
	bufferTool_multiplyWithFloatBuffer(buf,voiceArray[voiceNr].volEgValueBlock,size);


	//MIDI velocity
	if(voiceArray[voiceNr].volumeMod)
	{
		bufferTool_addGain(buf,voiceArray[voiceNr].velo,size);
	}
	//distortion
#if (USE_FILTER_DRIVE == 0)
	calcDistBlock(&voiceArray[voiceNr].distortion,buf,size);
#endif
	//channel volume
	bufferTool_addGain(buf,voiceArray[voiceNr].vol,size);



}
//---------------------------------------------------
int16_t calcDrumVoiceSync(const uint8_t voiceNr)
{
		//calc next mod osc sample
		calcNextOscSample(&voiceArray[voiceNr].modOsc);

		//calc next osc sample
		//voiceArray[voiceNr].oscSample = calcFmSine(&voiceArray[voiceNr].osc,&voiceArray[voiceNr].modOsc);

#if ENABLE_MIX_OSC
		if(voiceArray[voiceNr].mixOscs)
			voiceArray[voiceNr].oscSample = calcNextOscSample(&voiceArray[voiceNr].osc)*(1.f-voiceArray[voiceNr].fmModAmount)  + voiceArray[voiceNr].modOsc.output * voiceArray[voiceNr].fmModAmount;
		else
			voiceArray[voiceNr].oscSample = calcNextOscSampleFm(&voiceArray[voiceNr].osc,&voiceArray[voiceNr].modOsc);
#else
		voiceArray[voiceNr].oscSample = calcNextOscSampleFm(&voiceArray[voiceNr].osc,&voiceArray[voiceNr].modOsc);

#endif


		//calc transient sample
		transient_calc(&voiceArray[voiceNr].transGen);

		//mix osc and noise source
		const int32_t mixed = (voiceArray[voiceNr].transGen.output+voiceArray[voiceNr].oscSample*voiceArray[voiceNr].egValueOscVol);

#if CALC_TONE_CONTROL
		const int16_t satMix = __SSAT(mixed*0.5,16);

		toneControl_calc(&voiceArray[voiceNr].toneControl,satMix);
#endif

#if ENABLE_DRUM_SVF
		SVF_calc(&voiceArray[voiceNr].filter,mixed);
		int32_t filtered = (voiceArray[voiceNr].filterType&0x1)*SVF_getLP(&voiceArray[voiceNr].filter);
		filtered += ((voiceArray[voiceNr].filterType&0x2)>>1)*SVF_getHP(&voiceArray[voiceNr].filter);
		filtered += ((voiceArray[voiceNr].filterType&0x4)>>2)*SVF_getBP(&voiceArray[voiceNr].filter);
#endif

		//calc the osc + noise vol eg
#if AMP_EG_SYNC
		voiceArray[voiceNr].egValueOscVol = calcOnePole(&voiceArray[voiceNr].ampFilter,slopeEg2_calc(&voiceArray[voiceNr].oscVolEg));
		//voiceArray[voiceNr].egValueOscVol = slopeEg2_calc(&voiceArray[voiceNr].oscVolEg);

#else
		voiceArray[voiceNr].egValueOscVol = calcOnePole(&voiceArray[voiceNr].ampFilter, voiceArray[voiceNr].ampFilterInput);
#endif

#if CALC_TONE_CONTROL
		return calcDist(&voiceArray[voiceNr].distortion,voiceArray[voiceNr].toneControl.output*voiceArray[voiceNr].velo)*voiceArray[voiceNr].vol;
#else
		return calcDist(&voiceArray[voiceNr].distortion,filtered*voiceArray[voiceNr].velo)*voiceArray[voiceNr].vol;
#endif



};
//------------------------------------------------------------------------
#if 0
int16_t calcDrumVoiceSyncBlock(const uint8_t voiceNr, int16_t* audioBuffer)
{
	/* vars set in async routine
	  	voiceArray[voiceNr].osc.pitchMod
		voiceArray[voiceNr].osc.fmMod
		voiceArray[voiceNr].egValueOscVol
	 */

	//calc next mod osc sample
	calcNextOscSampleBlock(&voiceArray[voiceNr].modOsc);

	//calc next osc sample
	voiceArray[voiceNr].oscSample = calcNextOscSampleFm(&voiceArray[voiceNr].osc,&voiceArray[voiceNr].modOsc);

	//calc transient sample
	transient_calc(&voiceArray[voiceNr].transGen);

	//mix osc and noise source

	const int32_t mixed = voiceArray[voiceNr].transGen.output+voiceArray[voiceNr].oscSample*voiceArray[voiceNr].egValueOscVol;
	const int16_t satMix = __SSAT(mixed,16);
	toneControl_calc(&voiceArray[voiceNr].toneControl,satMix);
	return calcDist(&voiceArray[voiceNr].distortion,voiceArray[voiceNr].toneControl.output*voiceArray[voiceNr].velo)*voiceArray[voiceNr].vol;




};
#endif

