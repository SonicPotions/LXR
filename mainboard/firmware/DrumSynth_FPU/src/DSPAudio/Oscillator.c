/*
 * Oscillator.c
 *
 *  Created on: 02.04.2012
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


#include "Oscillator.h"
#include "random.h"
#include "Samples.h"
#include "MidiParser.h"
#include "MidiNoteNumbers.h"
#include "sequencer.h"

//#define PHASE_SHIFT ((uint32_t)1000000)
//#define F_SHIFT ((uint32_t)1)



//TODO die phaseInc berechnung kann man doch sicher per LUT machen!

//-----------------------------------------------------------
__inline uint8_t fast_log2 (const uint32_t val)
{
	return 31-__CLZ(val);
}
//-----------------------------------------------------------
//TODO hier wird *12 genommen und danach wieder durch 12 geteilt???
__inline int freqToMidiNote(const float f)
{
	return (12*fast_log2((uint32_t)(f/440.f)))+70;
}
//-----------------------------------------------------------
__inline uint8_t freqToTableIndex(const float f)
{
	return freqToMidiNote(f)/12;
}
//-----------------------------------------------------------
__inline uint32_t freq2PhaseIncr(const float f) //4096
{
	//return 4;
	return (((TABLESIZE*f)/REAL_FS))*1048576 ; // 1048576 <-> (<<20)
}
//-----------------------------------------------------------
__inline uint32_t freq2PhaseIncr1024(const float f)
{
	//return 4;
	return (((1024*f)/REAL_FS))*4194304 ; //(<<22)
}
//-----------------------------------------------------------
__inline uint32_t freq2PhaseIncr32767(const float f)
{
	//return 4;
	return (((1024*f)/REAL_FS))*131072 ; //(<<17)
}
//-----------------------------------------------------------
void calcSineBlock(OscInfo* osc, int16_t* buf, const uint8_t size ,const float gain)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		const uint32_t oscPhase = osc->phase;
		int16_t oscOut;

		const uint32_t index =  oscPhase;
		uint32_t  itg	= index>>20;

		#if INTERPOLATE_OSC
			oscOut = sine_table[itg++];
			const float frac = (index&0x7ffff)*0.0000019073486328125f;
			oscOut += frac*(sine_table[itg] - oscOut);
		#else
			oscOut = sine_table[itg];
		#endif

		osc->phase = oscPhase + osc->phaseInc;

		buf[i] = oscOut * gain;
	}
}
//-----------------------------------------------------------
int16_t calcSine(OscInfo* osc)
{
	const uint32_t oscPhase = osc->phase;
	int16_t oscOut;

	//const uint32_t inc = freq2PhaseIncr(osc->freq*osc->pitchMod*osc->modNodeValue);

	const uint32_t index =  oscPhase;
	uint32_t  itg	= index>>20;

#if INTERPOLATE_OSC
	oscOut = sine_table[itg++];
	float frac	= (index&0x7ffff)*0.0000019073486328125f;
	oscOut += frac*(sine_table[itg] - oscOut);
	//oscOut += frac*(oscOut - oscOut);
#else
	oscOut = sine_table[itg];
#endif

	osc->phase = oscPhase + osc->phaseInc;
	osc->output = oscOut;
	return oscOut;
}

//-----------------------------------------------------------
void calcFmSineBlock(OscInfo* osc, int16_t* modBuffer, int16_t* buf, uint8_t size,const float gain)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		const uint32_t oscPhase = osc->phase;
		int16_t oscOut ;

		//const uint32_t inc = freq2PhaseIncr(osc->freq*osc->pitchMod*osc->modNodeValue);
		const uint32_t index =  oscPhase + (((uint32_t)(modBuffer[i]*osc->fmMod))<<17);
		uint32_t  itg	= index>>20;

	#if INTERPOLATE_FM_OSC
		oscOut = sine_table[itg++];
		const float frac	= (index&0x7ffff)*0.0000019073486328125f;
		oscOut += frac*(sine_table[itg] - oscOut);
		//oscOut += frac*(oscOut - oscOut);
	#else
		oscOut = sine_table[itg];
	#endif

		osc->phase = oscPhase + osc->phaseInc;
		//osc->output = oscOut;
		buf[i] =  oscOut * gain;
	}
}
//-----------------------------------------------------------
int16_t calcFmSine(OscInfo* osc, OscInfo* modOsc)
{

	const uint32_t oscPhase 	= osc->phase;
	int16_t oscOut ;

	//const uint32_t inc = freq2PhaseIncr(osc->freq*osc->pitchMod);
	const uint32_t index =  oscPhase + (((uint32_t)(modOsc->output*osc->fmMod))<<17);
	uint32_t  itg	= index>>20;

#if INTERPOLATE_FM_OSC
	oscOut = sine_table[itg++];
	const float frac	= (index&0x7ffff)*0.0000019073486328125f;
	oscOut += frac*(sine_table[itg] - oscOut);
	//oscOut += frac*(oscOut - oscOut);
#else
	oscOut = sine_table[itg];
#endif

	osc->phase = oscPhase + osc->phaseInc;
	osc->output = oscOut;
	return oscOut;
};
//---------------------------------------------------------------
void calcFmBlock(OscInfo* osc, const int16_t table[][1024], int16_t* modBuffer, int16_t* buf, uint8_t size ,const float gain)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		const uint32_t oscPhase = osc->phase;
		int16_t oscOut ;
		const uint32_t index =  oscPhase + (((uint32_t)(modBuffer[i]*osc->fmMod))<<19);

		uint32_t  itg	= index>>22;

	#if INTERPOLATE_FM_OSC
		oscOut = table[osc->tableOffset][itg++];
		const float frac	= (index&0x003FFFFF)*2.38418579101562e-07f;
		oscOut += frac*(table[osc->tableOffset][itg] - oscOut);

	#else
		oscOut = table[overtoneIndex][itg];
	#endif

		osc->phase = oscPhase + osc->phaseInc;
		osc->output = oscOut;
		buf[i] = oscOut * gain;
	}
}
//-----------------------------------------------------------
int16_t calcFm(OscInfo* osc, OscInfo* modOsc, const int16_t table[][1024])
{

	const uint32_t oscPhase = osc->phase;
	int16_t oscOut ;

	//const float currentFreq = osc->freq*osc->pitchMod;
	//const uint32_t inc = freq2PhaseIncr1024(currentFreq);

	const uint32_t index =  oscPhase + (((uint32_t)(modOsc->output*osc->fmMod))<<19);

	uint32_t  itg	= index>>22;

	//uint8_t overtoneIndex = freqToTableIndex(currentFreq);
	//overtoneIndex = overtoneIndex>10?10:overtoneIndex;


#if INTERPOLATE_FM_OSC
	oscOut = table[osc->tableOffset][itg++];
	const float frac	= (index&0x003FFFFF)*2.38418579101562e-07f;
	oscOut += frac*(table[osc->tableOffset][itg] - oscOut);
//	oscOut += frac*(oscOut - oscOut);

#else
	oscOut = table[overtoneIndex][itg];
#endif

	osc->phase = oscPhase + osc->phaseInc;
	osc->output = oscOut;
	return oscOut;
}
//-----------------------------------------------------------
void calcNextOscSampleFmBlock(OscInfo* osc, int16_t* modBuffer, int16_t* buf, uint8_t size ,const float gain)
{
	switch(osc->waveform)
		{
		case SINE:
			calcFmSineBlock(osc,modBuffer,buf,size,gain);
			break;

		case SAW:
			calcFmBlock(osc,sawTable,modBuffer,buf,size,gain);
			break;

		case TRI:
			calcFmBlock(osc, triTable, modBuffer,buf,size,gain);
			break;

		case REC:
			calcFmBlock(osc,recTable,modBuffer,buf,size,gain);
			break;

		case NOISE:
			//TODO FM noise - needed since its tuned digi noise!
			calcNoiseBlock(osc,buf,size,gain);
			break;

		case CRASH:
			calcSampleOscFmBlock(osc,modBuffer,buf,size,gain);
			break;

		default:
			break;

		}

}
//-----------------------------------------------------------
int16_t calcNextOscSampleFm(OscInfo* osc,OscInfo* modOsc)
{
	switch(osc->waveform)
	{
	case SINE:
		return calcFmSine(osc,modOsc);
		break;

	case SAW:
		return calcFm(osc,modOsc,sawTable);
		break;

	case TRI:
		return calcFm(osc,modOsc, triTable);
		break;

	case REC:
		return calcFm(osc,modOsc,recTable);
		break;

	case NOISE:
		return calcNoise(osc);
		break;

	case CRASH:
		return calcSampleOscFm(osc,modOsc);
		break;

	default:

		return 0;
		break;

	}
	return 0;
}
//-----------------------------------------------------------
void calcNoiseBlock(OscInfo* osc, int16_t* buf, const uint8_t size ,const float gain)
{



#if 1 //use overflow flag

	int i;
	for(i=0;i<size;i++)
	{
		const uint32_t lastPhase = osc->phase;
		osc->phase += osc->phaseInc;

		//check overflow flag, ( if osc->phase == osc->phaseInc a reset occured ==> retrigger, too
		/*
		APSR_Type apsr;
		apsr.w = __get_APSR();
		if(apsr.b.V )//||  (osc->phase == osc->phaseInc))
		*/
		if(lastPhase > osc->phase)
		{
			//overflow happened -> phaseWrapped
			osc->output = GetRngValue(); //normal pitched white noise
/*
			uint16_t rnd = GetRngValue();
			if( rnd > 0x00ff)
			{
				if( rnd > 0x000f)
				{
					osc->output = 30000;//32767;

				}
				else
				{
					osc->output = 30000;//-32768;
				}

			}

*/


		}
		/*
		else
		{
				osc->output = 0;
		}
		*/
		buf[i] = osc->output * gain;
	}
#else
	int i;
	for(i=0;i<size;i++)
	{
		const uint32_t lastPhase = osc->phase;
		//TODO check C: Carry (or Unsigned Overflow) flag instead of compare with old value
		osc->phase = lastPhase + osc->phaseInc;

		if(lastPhase>osc->phase)
		{
			osc->output = (GetRngValue()&0xffff) * gain;

		}
		buf[i] = osc->output;
	}

#endif
}
//-----------------------------------------------------------
int16_t calcNoise(OscInfo* osc)
{


#if 1 //use overflow flag


	osc->phase += osc->phaseInc;

	//check overflow flag, ( if osc->phase == osc->phaseInc a reset occured ==> retrigger, too
	APSR_Type apsr;
	apsr.w = __get_APSR();
	if(apsr.b.V ||  (osc->phase == osc->phaseInc))
	{
		//overflow happened -> phaseWrapped
		//osc->output = GetRngValue(); //normal pitched white noise

		uint16_t rnd = GetRngValue();
		if( rnd > 0x00ff)
		{
			if( rnd > 0x000f)
			{
				osc->output = 32767;
			}
			else
			{
				osc->output = -32768;
			}

		}



	}
	else
	{
			osc->output = 0;
	}
	return osc->output;
#else
//	const uint32_t inc = freq2PhaseIncr(osc->freq*osc->pitchMod);
	const uint32_t lastPhase = osc->phase;
	//TODO check C: Carry (or Unsigned Overflow) flag instead of compare with old value



	osc->phase = lastPhase + osc->phaseInc;



	if(lastPhase>osc->phase)
	{
		osc->output = GetRngValue();

	}
	return osc->output;
#endif
}
//-----------------------------------------------------------
void calcNextOscSampleBlock(OscInfo* osc, int16_t* buf, const uint8_t size, const float gain)
{
	switch(osc->waveform)
		{
		case SINE:
			calcSineBlock(osc,buf,size, gain);
			break;

		case SAW:
			calcWavetableOscBlock(osc,sawTable,buf,size, gain);
			break;

		case TRI:
			calcWavetableOscBlock(osc,triTable,buf,size, gain);
			break;

		case REC:
			calcWavetableOscBlock(osc,recTable,buf,size, gain);
			break;

		case NOISE:
			calcNoiseBlock(osc,buf,size, gain);
			break;

		case CRASH:
			calcSampleOscBlock(osc,buf,size, gain);
			break;

		default:
			break;
		}
}
//-----------------------------------------------------------

// calculate an oscillator
int16_t calcNextOscSample(OscInfo* osc)
{
	switch(osc->waveform)
	{
	case SINE:
		return calcSine(osc);
		break;

	case SAW:
		return calcWavetableOsc(osc,sawTable);
		break;

	case TRI:
		return calcWavetableOsc(osc,triTable);
		break;

	case REC:
		return calcWavetableOsc(osc,recTable);
		break;

	case NOISE:
		return calcNoise(osc);
		break;

	case CRASH:
		return calcSampleOsc(osc);
		break;

	default:

		return 0;
		break;

	}
	return 0;
};
//---------------------------------------------------------------
void calcWavetableOscBlock(OscInfo* osc, const int16_t table[][1024], int16_t* buf, const uint8_t size ,const float gain)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		const uint32_t oscPhase = osc->phase;
			int16_t oscOut ;
			uint32_t  itg	= oscPhase>>22;
		#if INTERPOLATE_OSC
			//todo use ldm instead of ldr to laod multiple values from memory
			oscOut = table[osc->tableOffset][itg++];
			const float frac = (itg&0x003FFFFF)*2.38418579101562e-07f; //2.38... => 1.f/0x3fffff
			oscOut += frac*(table[osc->tableOffset][itg] - oscOut);
			//oscOut += frac*(oscOut - oscOut);
		#else
			oscOut = table[osc->tableOffset][itg];
		#endif

			osc->phase = oscPhase + osc->phaseInc;
			//osc->output = oscOut;
			buf[i] = oscOut * gain;
	}
}

//---------------------------------------------------------------
int16_t calcWavetableOsc(OscInfo* osc,  const int16_t table[][1024])
{
	const uint32_t oscPhase = osc->phase;
	int16_t oscOut ;

//	const float currentFreq = osc->freq*osc->pitchMod;
//	const uint32_t inc = freq2PhaseIncr1024(currentFreq);

	uint32_t  itg	= oscPhase>>22;

	//uint8_t overtoneIndex = freqToTableIndex(currentFreq);
	//overtoneIndex = overtoneIndex>10?10:overtoneIndex;

#if INTERPOLATE_OSC
	//todo use ldm instead of ldr to laod multiple values from memory
	oscOut = table[osc->tableOffset][itg++];
	const float frac = (itg&0x003FFFFF)*2.38418579101562e-07f; //2.38... => 1.f/0x3fffff
	oscOut += frac*(table[osc->tableOffset][itg] - oscOut);
	//oscOut += frac*(oscOut - oscOut);
#else
	oscOut = table[osc->tableOffset][itg];
#endif

	osc->phase = oscPhase + osc->phaseInc;
	osc->output = oscOut;
	return oscOut;

};
//------------------------------------------------------------------
void calcSampleOscFmBlock(OscInfo* osc,int16_t* modBuffer, int16_t* buf, uint8_t size ,const float gain)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		const uint32_t oscPhase 	= osc->phase;
		int16_t oscOut ;

	//	const uint32_t inc = freq2PhaseIncr32767(osc->freq*osc->pitchMod);
		const uint32_t index =  oscPhase + (((uint32_t)(modBuffer[i]*osc->fmMod))<<14);
		uint32_t  itg	= index>>17;

	#if INTERPOLATE_FM_OSC
		oscOut = crashSample[itg++];
		const float frac	= (index&20000)*0.00000762939453125f;//=> * 1/0x20000
		oscOut += frac*(crashSample[itg] - oscOut);
		//oscOut += frac*(oscOut - oscOut);
	#else
		oscOut = crashSample[itg];
	#endif

		oscOut = (oscOut - 127)*256;

		osc->phase = oscPhase + osc->phaseInc;
		osc->output = oscOut;
		buf[i] = oscOut * gain;
	}
}
//------------------------------------------------------------------
int16_t calcSampleOscFm(OscInfo* osc, OscInfo* modOsc)
{

	const uint32_t oscPhase 	= osc->phase;
	int16_t oscOut ;

//	const uint32_t inc = freq2PhaseIncr32767(osc->freq*osc->pitchMod);
	const uint32_t index =  oscPhase + (((uint32_t)(modOsc->output*osc->fmMod))<<14);
	uint32_t  itg	= index>>17;

#if INTERPOLATE_FM_OSC
	oscOut = crashSample[itg++];
	const float frac	= (index&20000)*0.00000762939453125f;//=> * 1/0x20000
	oscOut += frac*(crashSample[itg] - oscOut);
	//oscOut += frac*(oscOut - oscOut);
#else
	oscOut = crashSample[itg];
#endif

	oscOut = (oscOut - 127)*256;

	osc->phase = oscPhase + osc->phaseInc;
	osc->output = oscOut;
	return oscOut;
};
//---------------------------------------------------------------
void calcSampleOscBlock(OscInfo* osc, int16_t* buf, const uint8_t size ,const float gain)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		const uint32_t oscPhase = osc->phase;
		int16_t oscOut ;
		uint32_t  itg	= oscPhase>>17;


	#if INTERPOLATE_OSC
		//todo use ldm instead of ldr to laod multiple values from memory
		oscOut = crashSample[itg++];
		const float frac = (oscPhase&20000)*0.00000762939453125f;//=> * 1/0x20000
		oscOut += frac*(crashSample[itg] - oscOut);
		//oscOut += frac*(oscOut - oscOut);
	#else
		oscOut = crashSample[itg];
	#endif
		oscOut = (oscOut - 127)*256;

		osc->phase = oscPhase + osc->phaseInc;
		//osc->output = oscOut;
		buf[i] = oscOut * gain;
	}
}
//---------------------------------------------------------------
int16_t calcSampleOsc(OscInfo* osc)
{
	const uint32_t oscPhase = osc->phase;
	int16_t oscOut ;

	/*
	const float currentFreq = osc->freq*osc->pitchMod;
	const uint32_t inc = freq2PhaseIncr32767(currentFreq);
	*/

	uint32_t  itg	= oscPhase>>17;



#if INTERPOLATE_OSC
	//todo use ldm instead of ldr to laod multiple values from memory
	oscOut = crashSample[itg++];
	const float frac = (oscPhase&20000)*0.00000762939453125f;//=> * 1/0x20000
	oscOut += frac*(crashSample[itg] - oscOut);
	//oscOut += frac*(oscOut - oscOut);
#else
	oscOut = crashSample[itg];
#endif
	oscOut = (oscOut - 127)*256;

	osc->phase = oscPhase + osc->phaseInc;
	osc->output = oscOut;
	return oscOut;

};
 //-----------------------------------------------------------
void osc_calcSineFreq(OscInfo* osc)
{
	osc->phaseInc = freq2PhaseIncr(osc->freq*osc->pitchMod*osc->modNodeValue);
}
//-----------------------------------------------------------
void osc_calcNoiseFreq(OscInfo* osc)
{
	osc_calcSineFreq(osc);
}
//-----------------------------------------------------------
void osc_calcWavetableFreq(OscInfo* osc)
{
	const float currentFreq = osc->freq*osc->pitchMod*osc->modNodeValue;
	osc->phaseInc = freq2PhaseIncr1024(currentFreq);

	const uint8_t overtoneIndex = freqToTableIndex(currentFreq);
	osc->tableOffset = overtoneIndex>10?10:overtoneIndex;
}
//-----------------------------------------------------------
void osc_calcSampleFreq(OscInfo* osc)
{
	const float currentFreq = osc->freq*osc->pitchMod*osc->modNodeValue;
	osc->phaseInc = freq2PhaseIncr32767(currentFreq);
}
//-----------------------------------------------------------
 void osc_setFreq(OscInfo* osc)
 {
		switch(osc->waveform)
		{
		case SINE:
			osc_calcSineFreq(osc);
			break;

		case SAW:
			osc_calcWavetableFreq(osc);
			break;

		case TRI:
			osc_calcWavetableFreq(osc);
			break;

		case REC:
			osc_calcWavetableFreq(osc);
			break;

		case NOISE:
			osc_calcNoiseFreq(osc);
			break;

		case CRASH:
			osc_calcSampleFreq(osc);
			break;

		default:
			break;

		}
 }
 //-----------------------------------------------------------
 void osc_setBaseNote(OscInfo* osc, uint8_t baseNote)
 {

	 //get fine tune
	 const float cent = midiParser_calcDetune(osc->midiFreq&0xff);
	 //calc coarse tune
	// uint16_t note =  MidiNoteFrequencies[osc->midiFreq>>8] + baseNote;
	 int16_t note =  (osc->midiFreq>>8) + (baseNote-SEQ_DEFAULT_NOTE);
	 if(note>127)note=127;
	 if(note<0)note=0;

	 /*
	 if(note>=128)
		 osc->freq = MidiNoteFrequencies[127]*cent;
	 else
		 */
		 osc->freq = MidiNoteFrequencies[note]*cent;

	 osc->baseNote = baseNote;

	 //TODO midiParser muss noch angepasst werden und die base note wohl gespeichert. sonst tut modulieren per midi nicht
 };

 //-----------------------------------------------------------
 void osc_recalcFreq(OscInfo* osc)
 {
	 //get fine tune
	 const float cent = midiParser_calcDetune(osc->midiFreq&0xff);
	 //calc coarse tune
	 //uint16_t note =  MidiNoteFrequencies[osc->midiFreq>>8] + osc->baseNote;
	 int16_t note =  (osc->midiFreq>>8) + (osc->baseNote-SEQ_DEFAULT_NOTE);

	 if(note>127)note=127;
	 	 if(note<0)note=0;
/*
	 if(note>=128)
		 osc->freq = MidiNoteFrequencies[127]*cent;
	 else
		 */
		 osc->freq = MidiNoteFrequencies[note]*cent;

 }
 //-----------------------------------------------------------
