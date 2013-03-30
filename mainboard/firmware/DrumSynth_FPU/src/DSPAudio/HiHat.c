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
	//Pan= -0.5 to 0.5
	/*
	hatVoice.panL = sqrtf(0.5f-pan);
	hatVoice.panR = sqrtf(0.5+pan);
	*/
	hatVoice.panL = squareRootLut[pan];
	hatVoice.panR = squareRootLut[127-pan];
}
//---------------------------------------------------

void HiHat_init()
{
	//int i;
	//for(i=0;i<NUM_HAT;i++)
	{
		HiHat_setPan(0.f);
		hatVoice.vol = 0.8f;

		hatVoice.panModifier = 1.f;

		transient_init(&hatVoice.transGen);

		//toneControl_init(&hatVoice.toneControl);

		/*
		hatVoice.noiseOsc.freq = 440;
		hatVoice.noiseOsc.waveform = 1;
		hatVoice.noiseOsc.fmMod = 0;
		hatVoice.noiseOsc.midiFreq = 70<<8;
		hatVoice.noiseOsc.pitchMod = 1.0f;
		*/
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

		//DecayEg_init(&hatVoice.oscVolEg);
		slopeEg2_init(&hatVoice.oscVolEg);

		SVF_init(&hatVoice.filter);

	}
}
//---------------------------------------------------
//---------------------------------------------------
void HiHat_trigger( uint8_t vel, uint8_t isOpen, const uint8_t note)
{

	lfo_retrigger(5);

	//update velocity modulation
	modNode_updateValue(&velocityModulators[5],vel/127.f);

//	voiceArray[voiceNr].status |= IS_PLAYING;
	//hatVoice.osc.phase = 0;
	//snareVoice.noiseOsc.phase = 0;
	/*
	hatVoice.osc.phase = 0;
	hatVoice.modOsc.phase = 0;
	hatVoice.modOsc2.phase = 0;
	*/

	osc_setBaseNote(&hatVoice.osc,note);
	osc_setBaseNote(&hatVoice.modOsc,note);
	osc_setBaseNote(&hatVoice.modOsc2,note);

	hatVoice.oscVolEg.decay = isOpen?hatVoice.decayOpen:hatVoice.decayClosed;
	//hatVoice.oscVolEg.d = isOpen?hatVoice.decayOpen:hatVoice.decayClosed;

	//DecayEg_trigger(&hatVoice.oscVolEg);
	slopeEg2_trigger(&hatVoice.oscVolEg);
	hatVoice.velo = vel/127.f;
//	ADEG_trigger(&snareVoice[voiceNr].noiseVolEg);

	transient_trigger(&hatVoice.transGen);


}
//---------------------------------------------------
void HiHat_calcAsync( )
{
	//calc and dispatch LFO
//	lfo_dispatchNextValue(&hatVoice.lfo);

	//calc the osc  vol eg
	//hatVoice.egValueOscVol = DecayEg_calc(&hatVoice.oscVolEg);
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

//	int16_t* svf = SVF_getHpBlockInt(&hatVoice.filter);

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
	/*
	bufferTool_addBuffersSaturating(buf,SVF_getHpBlockInt(&hatVoice.filter),size);

	//TODO All in 1 loop!!!!1elf!

	//MIDI velocity
	bufferTool_addGain(buf,hatVoice.velo,size);
	//channel volume
	bufferTool_addGain(buf,hatVoice.vol,size);
	//Amp EG
	bufferTool_addGain(buf,hatVoice.egValueOscVol,size);
*/


}
//---------------------------------------------------

int16_t HiHat_calcSync( )
{
	//if(1)
	//if(hatVoice.oscVolEg.value>0.001f )
	//if(voiceArray[voiceNr].status & IS_PLAYING)
	{
		//calc next noise osc sample
	//	calcNoise(&hatVoice.noiseOsc);

		//calc next mod osc sample
		calcNextOscSample(&hatVoice.modOsc);
		//calcSine(&hatVoice.modOsc2);
		calcNextOscSample(&hatVoice.modOsc2);
		//calc next osc sample
		hatVoice.modOsc.output *= hatVoice.fmModAmount1;
		hatVoice.modOsc.output += hatVoice.modOsc2.output * hatVoice.fmModAmount2;

		//calc transient sample
		transient_calc(&hatVoice.transGen);

		const int16_t noise = calcNextOscSampleFm(&hatVoice.osc,&hatVoice.modOsc) ;

	//	const int16_t noise = GetRngValue();
		SVF_calc(&hatVoice.filter,noise);





	//	hatVoice.noiseSample += (hatVoice.filterType&0x1)*SVF_getLP(&hatVoice.filter);
	//	hatVoice.noiseSample += ((hatVoice.filterType&0x2)>>1)*SVF_getHP(&hatVoice.filter);
		//hatVoice.noiseSample = SVF_getBP(&hatVoice.filter);

		hatVoice.noiseSample = SVF_getHP(&hatVoice.filter)+ hatVoice.transGen.output;
//		hatVoice.noiseSample = __SSAT(hatVoice.noiseSample,16);
		//int16_t n = (GetRngValue());
		//hatVoice.noiseSample += __SSAT(n<<shiftFactor2,16);




		//mix osc and noise source
		//int32_t mixed = voiceArray[voiceNr].oscSample*egValOsc * (1.f-voiceArray[voiceNr].mix) + voiceArray[voiceNr].noiseSample*egValNoise*voiceArray[voiceNr].mix;
	//	const int32_t mixed = hatVoice.noiseSample;

#if CALC_TONE_CONTROL
		toneControl_calc(&hatVoice.toneControl,hatVoice.noiseSample*hatVoice.egValueOscVol);

		//return calcDist(&snareVoice[voiceNr].distortion,mixed*snareVoice[voiceNr].egValueOscVol*snareVoice[voiceNr].vol);
		return hatVoice.toneControl.output*hatVoice.vol*hatVoice.velo;
#else

		//toneControl_calc(&hatVoice.toneControl,hatVoice.noiseSample*hatVoice.egValueOscVol);

		//return calcDist(&snareVoice[voiceNr].distortion,mixed*snareVoice[voiceNr].egValueOscVol*snareVoice[voiceNr].vol);
		return hatVoice.noiseSample*hatVoice.egValueOscVol*hatVoice.vol*hatVoice.velo;
#endif


	}
	//else return 0;

};
