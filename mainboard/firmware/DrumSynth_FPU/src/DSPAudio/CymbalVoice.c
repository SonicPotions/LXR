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
	//Pan= -0.5 to 0.5
	/*
	cymbalVoice.panL = sqrtf(0.5f-pan);
	cymbalVoice.panR = sqrtf(0.5+pan);
	*/
	cymbalVoice.panL = squareRootLut[pan];
	cymbalVoice.panR = squareRootLut[127-pan];

}
//---------------------------------------------------

void Cymbal_init()
{
	//int i;
	//for(i=0;i<NUM_CYM;i++)
	{
		Cymbal_setPan(0.f);
		cymbalVoice.vol = 0.8f;

		cymbalVoice.panModifier = 1.f;

	//	toneControl_init(&cymbalVoice.toneControl);

		transient_init(&cymbalVoice.transGen);

		/*
		cymbalVoice.noiseOsc.freq = 440;
		cymbalVoice.noiseOsc.waveform = 1;
		cymbalVoice.noiseOsc.fmMod = 0;
		cymbalVoice.noiseOsc.midiFreq = 70<<8;
		cymbalVoice.noiseOsc.pitchMod = 1.0f;
		*/
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

		//DecayEg_init(&cymbalVoice.oscVolEg);
		slopeEg2_init(&cymbalVoice.oscVolEg);

		SVF_init(&cymbalVoice.filter);

	}
}
//---------------------------------------------------
//---------------------------------------------------
void Cymbal_trigger( const uint8_t vel, const uint8_t note)
{
	lfo_retrigger(4);
	//update velocity modulation
	modNode_updateValue(&velocityModulators[4],vel/127.f);

//	voiceArray[voiceNr].status |= IS_PLAYING;
	//cymbalVoice.osc.phase = 0;
	//snareVoice.noiseOsc.phase = 0;
	cymbalVoice.osc.phase = 0;
	cymbalVoice.modOsc.phase = 0;
	cymbalVoice.modOsc2.phase = 0;

	osc_setBaseNote(&cymbalVoice.osc,note);
	osc_setBaseNote(&cymbalVoice.modOsc,note);
	osc_setBaseNote(&cymbalVoice.modOsc2,note);

	//cymbalVoice.oscVolEg.d = isOpen?cymbalVoice.decayOpen:cymbalVoice.decayClosed;

	//DecayEg_trigger(&cymbalVoice.oscVolEg);
	slopeEg2_trigger(&cymbalVoice.oscVolEg);
	cymbalVoice.velo = vel/127.f;
//	ADEG_trigger(&snareVoice[voiceNr].noiseVolEg);

	transient_trigger(&cymbalVoice.transGen);


}
//---------------------------------------------------
void Cymbal_calcAsync()
{

	//calc and dispatch LFO
	//lfo_dispatchNextValue(&cymbalVoice.lfo);

	//calc the osc  vol eg
	//cymbalVoice.egValueOscVol = DecayEg_calc(&cymbalVoice.oscVolEg);
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
	//	int16_t* svfHp = SVF_getHpBlockInt(&cymbalVoice.filter);

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
		//bufferTool_addBuffersSaturating(buf,SVF_getHpBlockInt(&cymbalVoice.filter),size);

		calcDistBlock(&cymbalVoice.distortion,buf,size);

/*
		//MIDI velocity
		bufferTool_addGain(buf,cymbalVoice.velo,size);
		//channel volume
		bufferTool_addGain(buf,cymbalVoice.vol,size);
		//Amp EG
		bufferTool_addGain(buf,cymbalVoice.egValueOscVol,size);*/


}
//---------------------------------------------------
int16_t Cymbal_calcSync()
{
	//if(1)
	//if(cymbalVoice.oscVolEg.value>0.001f )
	//if(voiceArray[voiceNr].status & IS_PLAYING)
	{
		//calc next noise osc sample
	//	calcNoise(&cymbalVoice.noiseOsc);

		//calc next mod osc sample
		//calcSine(&cymbalVoice.modOsc);
		calcNextOscSample(&cymbalVoice.modOsc);
		//calcSine(&cymbalVoice.modOsc2);
		calcNextOscSample(&cymbalVoice.modOsc2);
		//calc next osc sample
		cymbalVoice.modOsc.output *= cymbalVoice.fmModAmount1;
		cymbalVoice.modOsc.output += cymbalVoice.modOsc2.output * cymbalVoice.fmModAmount2;

		//calc transient sample
		transient_calc(&cymbalVoice.transGen);

		const int16_t noise = calcNextOscSampleFm(&cymbalVoice.osc,&cymbalVoice.modOsc) + cymbalVoice.transGen.output;

	//	const int16_t noise = GetRngValue();
		SVF_calc(&cymbalVoice.filter,noise);//*0.9f);




		//cymbalVoice.noiseSample = SVF_getHP(&cymbalVoice.filter);



		//mix osc and noise source
#if CALC_TONE_CONTROL
		toneControl_calc(&cymbalVoice.toneControl, SVF_getHP(&cymbalVoice.filter)*cymbalVoice.egValueOscVol);

		return cymbalVoice.toneControl.output*cymbalVoice.vol*cymbalVoice.velo;
#else
		return SVF_getHP(&cymbalVoice.filter)*cymbalVoice.egValueOscVol*cymbalVoice.vol*cymbalVoice.velo;
#endif

	}
	//else return 0;

};
