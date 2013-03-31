/*
 * CymbalVoice.c
 *
 *  Created on: 16.06.2012
 *      Author: Julian
 */

#include "CymbalVoice.h"
#include "squareRootLut.h"
#include "modulationNode.h"

CymbalVoice cymbalVoice;
//---------------------------------------------------
void Cymbal_setPan(const uint8_t pan)
{
	cymbalVoice.panL = squareRootLut[pan];
	cymbalVoice.panR = squareRootLut[127-pan];
}
//---------------------------------------------------

void Cymbal_init()
{
	Cymbal_setPan(0.f);
	cymbalVoice.vol = 0.8f;

	cymbalVoice.panModifier = 1.f;

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

}
//---------------------------------------------------
//---------------------------------------------------
void Cymbal_trigger( const uint8_t vel, const uint8_t note)
{
	lfo_retrigger(4);
	//update velocity modulation
	modNode_updateValue(&velocityModulators[4],vel/127.f);

	cymbalVoice.osc.phase = 0;
	cymbalVoice.modOsc.phase = 0;
	cymbalVoice.modOsc2.phase = 0;

	osc_setBaseNote(&cymbalVoice.osc,note);
	osc_setBaseNote(&cymbalVoice.modOsc,note);
	osc_setBaseNote(&cymbalVoice.modOsc2,note);

	slopeEg2_trigger(&cymbalVoice.oscVolEg);
	cymbalVoice.velo = vel/127.f;

	transient_trigger(&cymbalVoice.transGen);
}
//---------------------------------------------------
void Cymbal_calcAsync()
{
	//calc the osc  vol eg
	cymbalVoice.egValueOscVol = slopeEg2_calc(&cymbalVoice.oscVolEg);

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

