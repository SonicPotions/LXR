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


float ampSmoothValue = 0.1f;
//---------------------------------------------------
DrumVoice voiceArray[NUM_VOICES];
//---------------------------------------------------
void setPan(const uint8_t voiceNr, const uint8_t pan)
{
	//voiceArray[voiceNr].panL = squareRootLut[127-pan];
	//voiceArray[voiceNr].panR = squareRootLut[pan];
	voiceArray[voiceNr].pan = pan;
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
	int i;
	for(i=0;i<NUM_VOICES;i++)
	{

		SnapEg_init(&voiceArray[i].snapEg);
		setPan(i,0.f);
		voiceArray[i].vol = 0.8f;
		voiceArray[i].panModifier = 1.f;
		voiceArray[i].fmModAmount = 0.5f;
		transient_init(&voiceArray[i].transGen);
#if ENABLE_DRUM_SVF
		SVF_init(&voiceArray[i].filter);
		voiceArray[i].filterType = 0x01;
#endif
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
		setDistortionShape(&voiceArray[i].distortion, 2.f);

#ifdef USE_AMP_FILTER
		initOnePole(&voiceArray[i].ampFilter);
		setOnePoleCoef(&voiceArray[i].ampFilter,ampSmoothValue);
#endif

#if ENABLE_MIX_OSC
		voiceArray[i].mixOscs = true;
#endif
		voiceArray[i].decimationCnt = 0;
		voiceArray[i].decimationRate = 1;
	}
}
//---------------------------------------------------

void Drum_trigger(const uint8_t voiceNr, const uint8_t vol, const uint8_t note)
{
	lfo_retrigger(voiceNr);

	//update velocity modulation
	modNode_updateValue(&velocityModulators[voiceNr],vol/127.f);

	//only reset phase if envelope is closed
	if((voiceArray[voiceNr].volEgValueBlock[15]<=0.01f) || (voiceArray[voiceNr].transGen.waveform==1))
	{
		float offset = 1;
		if(voiceArray[voiceNr].transGen.waveform==1) //offset mode
		{
			offset -= voiceArray[voiceNr].transGen.volume;
#ifdef USE_AMP_FILTER
			setOnePoleCoef(&voiceArray[voiceNr].ampFilter,1.0f); //turn off amp filter for super snappy attack

		} else {
			setOnePoleCoef(&voiceArray[voiceNr].ampFilter,ampSmoothValue);
#endif
		}
		if(voiceArray[voiceNr].osc.waveform == SINE)
			voiceArray[voiceNr].osc.phase = (0x3ff<<20)*offset;//voiceArray[voiceNr].osc.startPhase ;
		else if(voiceArray[voiceNr].osc.waveform > SINE && voiceArray[voiceNr].osc.waveform <= REC)
			voiceArray[voiceNr].osc.phase = (0xff<<20)*offset;
		else
			voiceArray[voiceNr].osc.phase = 0;

		if(voiceArray[voiceNr].modOsc.waveform == SINE)
			voiceArray[voiceNr].modOsc.phase = (0x3ff<<20)*offset;//voiceArray[voiceNr].osc.startPhase ;
		else if(voiceArray[voiceNr].modOsc.waveform > SINE && voiceArray[voiceNr].modOsc.waveform <= REC)
			voiceArray[voiceNr].modOsc.phase = (0xff<<20)*offset;
		else
			voiceArray[voiceNr].modOsc.phase = 0;
	}

	osc_setBaseNote(&voiceArray[voiceNr].osc,note);
	osc_setBaseNote(&voiceArray[voiceNr].modOsc,note);


	DecayEg_trigger(&voiceArray[voiceNr].oscPitchEg);
	slopeEg2_trigger(&voiceArray[voiceNr].oscVolEg);
	voiceArray[voiceNr].velo = vol/127.f;

	transient_trigger(&voiceArray[voiceNr].transGen);

	SnapEg_trigger(&voiceArray[voiceNr].snapEg);
}
//---------------------------------------------------
void calcDrumVoiceAsync(const uint8_t voiceNr)
{
	//add modulation eg to osc freq (1 = no change. a+eg = original freq + modulation
	const float egPitchVal = DecayEg_calc(&voiceArray[voiceNr].oscPitchEg);
	const float pitchEgValue = egPitchVal*voiceArray[voiceNr].egPitchModAmount;
	voiceArray[voiceNr].osc.pitchMod = 1+pitchEgValue;

	//calc snap EG if transient sample 0 is activated
	if(voiceArray[voiceNr].transGen.waveform == 0)
	{
		const float snapVal = SnapEg_calc(&voiceArray[voiceNr].snapEg, voiceArray[voiceNr].transGen.pitch);
		voiceArray[voiceNr].osc.pitchMod += snapVal*voiceArray[voiceNr].transGen.volume;
	}

	// fm amount with pitch eg
	voiceArray[voiceNr].osc.fmMod = voiceArray[voiceNr].fmModAmount * egPitchVal;

	//calc the osc + noise vol eg
#if (AMP_EG_SYNC==0)
	voiceArray[voiceNr].lastGain = voiceArray[voiceNr].ampFilterInput;
	voiceArray[voiceNr].ampFilterInput = slopeEg2_calc(&voiceArray[voiceNr].oscVolEg);
#endif

	//update osc phaseInc
	osc_setFreq(&voiceArray[voiceNr].osc);
	osc_setFreq(&voiceArray[voiceNr].modOsc);
}

//---------------------------------------------------
void calcDrumVoiceSyncBlock(const uint8_t voiceNr, int16_t* buf, const uint8_t size)
{
	int16_t modBuf[size];

	//calc vol EG
#ifdef USE_AMP_FILTER
	calcOnePoleBlockFixedInput(&voiceArray[voiceNr].ampFilter, voiceArray[voiceNr].ampFilterInput,voiceArray[voiceNr].volEgValueBlock, size);
#endif

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
	SVF_calcBlockZDF(&voiceArray[voiceNr].filter,voiceArray[voiceNr].filterType,buf,size);

	//attentuate main OSCs by amp EG
	//bufferTool_multiplyWithFloatBuffer(buf,voiceArray[voiceNr].volEgValueBlock,size);

#ifdef USE_AMP_FILTER
	bufferTool_multiplyWithFloatBufferDithered(&voiceArray[voiceNr].dither, buf,voiceArray[voiceNr].volEgValueBlock,size);
#else
	//bufferTool_addGainDithered(&voiceArray[voiceNr].dither, buf,voiceArray[voiceNr].ampFilterInput,size);
	bufferTool_addGainInterpolated(buf,voiceArray[voiceNr].ampFilterInput, voiceArray[voiceNr].lastGain, size);
#endif

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


