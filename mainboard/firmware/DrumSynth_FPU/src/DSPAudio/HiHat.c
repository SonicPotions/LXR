/*
 * HiHat.c
 *
 *  Created on: 18.04.2012
 *      Author: Julian
 */


#include "HiHat.h"
#include "squareRootLut.h"
#include "modulationNode.h"

HiHatVoice hatVoice;


float cheapSixRec(float f)
{
	float out=0;
	return out;
}


//---------------------------------------------------
void HiHat_setPan(const uint8_t pan)
{
	hatVoice.panL = squareRootLut[127-pan];
	hatVoice.panR = squareRootLut[pan];
}
//---------------------------------------------------

void HiHat_init()
{
	HiHat_setPan(0.f);
	hatVoice.vol = 0.8f;

	hatVoice.panModifier = 1.f;

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
}
//---------------------------------------------------
//---------------------------------------------------
void HiHat_trigger( uint8_t vel, uint8_t isOpen, const uint8_t note)
{

	lfo_retrigger(5);

	//update velocity modulation
	modNode_updateValue(&velocityModulators[5],vel/127.f);

	osc_setBaseNote(&hatVoice.osc,note);
	osc_setBaseNote(&hatVoice.modOsc,note);
	osc_setBaseNote(&hatVoice.modOsc2,note);

	hatVoice.oscVolEg.decay = isOpen?hatVoice.decayOpen:hatVoice.decayClosed;

	slopeEg2_trigger(&hatVoice.oscVolEg);
	hatVoice.velo = vel/127.f;
	transient_trigger(&hatVoice.transGen);
}
//---------------------------------------------------
void HiHat_calcAsync( )
{
	//calc the osc  vol eg
	hatVoice.egValueOscVol = slopeEg2_calc(&hatVoice.oscVolEg);

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

