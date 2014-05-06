/*
 * ParameterArray.c
 *
 *  Created on: 06.01.2013
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



#include "ParameterArray.h"
#include "DrumVoice.h"
#include "CymbalVoice.h"
#include "MidiParser.h"
#include "HiHat.h"
#include "Snare.h"
#include "mixer.h"


 Parameter parameterArray[END_OF_SOUND_PARAMETERS];
//the parameter numbers from the AVR/Frontpanel
//####################################################################
void paramArray_setParameter(uint16_t idx, ptrValue newValue)
{
	// --AS **PATROT we don't want to set any that have a null ptr
	if(idx>=END_OF_SOUND_PARAMETERS || parameterArray[idx].ptr==0) return;

	switch(parameterArray[idx].type)
	{
	case TYPE_UINT8:
		*((uint8_t*)parameterArray[idx].ptr) = newValue.itg;
		break;

	case TYPE_UINT32:
		*((uint32_t*)parameterArray[idx].ptr) = newValue.itg;
			break;

	case TYPE_FLT:
		*((float*)parameterArray[idx].ptr) 	= newValue.flt;
		break;

	case TYPE_SPECIAL_F: //freq
		*((float*)parameterArray[idx].ptr) 	= newValue.flt;
		break;
	case TYPE_SPECIAL_P: //Pan

		break;

	default:
		break;

	}
}
//---------------------------------------------------------------------
void parameterArray_init()
{

	parameterArray[PAR_OSC_WAVE_DRUM1].ptr 	= &voiceArray[0].osc.waveform;
	parameterArray[PAR_OSC_WAVE_DRUM1].type = TYPE_UINT8;

	parameterArray[PAR_OSC_WAVE_DRUM2].ptr 	= &voiceArray[1].osc.waveform;
	parameterArray[PAR_OSC_WAVE_DRUM2].type = TYPE_UINT8;

	parameterArray[PAR_OSC_WAVE_DRUM3].ptr 	= &voiceArray[2].osc.waveform;
	parameterArray[PAR_OSC_WAVE_DRUM3].type = TYPE_UINT8;

	parameterArray[PAR_OSC_WAVE_SNARE].ptr 	= &snareVoice.osc.waveform;;
	parameterArray[PAR_OSC_WAVE_SNARE].type = TYPE_UINT8;

	parameterArray[PAR_WAVE1_CYM].ptr 		= &cymbalVoice.osc.waveform;
	parameterArray[PAR_WAVE1_CYM].type 		= TYPE_UINT8;

	parameterArray[PAR_WAVE1_HH].ptr 		=  &hatVoice.osc.waveform;
	parameterArray[PAR_WAVE1_HH].type 		= TYPE_UINT8;


	parameterArray[PAR_COARSE1].ptr 		= &voiceArray[0].osc.modNodeValue;
	parameterArray[PAR_COARSE1].type 		= TYPE_SPECIAL_F;

	parameterArray[PAR_FINE1].ptr 			= &voiceArray[0].osc.modNodeValue;
	parameterArray[PAR_FINE1].type 			= TYPE_SPECIAL_F;

	parameterArray[PAR_COARSE2].ptr 		= &voiceArray[1].osc.modNodeValue;
	parameterArray[PAR_COARSE2].type 		= TYPE_SPECIAL_F;

	parameterArray[PAR_FINE2].ptr 			= &voiceArray[1].osc.modNodeValue;
	parameterArray[PAR_FINE2].type 			= TYPE_SPECIAL_F;

	parameterArray[PAR_COARSE3].ptr 		= &voiceArray[2].osc.modNodeValue;
	parameterArray[PAR_COARSE3].type 		= TYPE_SPECIAL_F;

	parameterArray[PAR_FINE3].ptr 			= &voiceArray[2].osc.modNodeValue;
	parameterArray[PAR_FINE3].type 			= TYPE_SPECIAL_F;

	parameterArray[PAR_COARSE4].ptr 		= &snareVoice.osc.modNodeValue;
	parameterArray[PAR_COARSE4].type 		= TYPE_SPECIAL_F;

	parameterArray[PAR_FINE4].ptr 			= &snareVoice.osc.modNodeValue;
	parameterArray[PAR_FINE4].type 			= TYPE_SPECIAL_F;

	parameterArray[PAR_COARSE5].ptr 		= &cymbalVoice.osc.modNodeValue;
	parameterArray[PAR_COARSE5].type 		= TYPE_SPECIAL_F;

	parameterArray[PAR_FINE5].ptr 			= &cymbalVoice.osc.modNodeValue;
	parameterArray[PAR_FINE5].type		 	= TYPE_SPECIAL_F;

	parameterArray[PAR_COARSE6].ptr 		= &hatVoice.osc.modNodeValue;
	parameterArray[PAR_COARSE6].type 		= TYPE_SPECIAL_F;

	parameterArray[PAR_FINE6].ptr 			= &hatVoice.osc.modNodeValue;
	parameterArray[PAR_FINE6].type 			= TYPE_SPECIAL_F;


	parameterArray[PAR_MOD_WAVE_DRUM1].ptr 	= &voiceArray[0].modOsc.waveform;
	parameterArray[PAR_MOD_WAVE_DRUM1].type = TYPE_UINT8;

	parameterArray[PAR_MOD_WAVE_DRUM2].ptr 	= &voiceArray[1].modOsc.waveform;
	parameterArray[PAR_MOD_WAVE_DRUM2].type = TYPE_UINT8;

	parameterArray[PAR_MOD_WAVE_DRUM3].ptr 	= &voiceArray[2].modOsc.waveform;
	parameterArray[PAR_MOD_WAVE_DRUM3].type = TYPE_UINT8;



	parameterArray[PAR_WAVE2_CYM].ptr 		= &cymbalVoice.modOsc.waveform;
	parameterArray[PAR_WAVE2_CYM].type 		= TYPE_UINT8;

	parameterArray[PAR_WAVE3_CYM].ptr 		= &cymbalVoice.modOsc2.waveform;;
	parameterArray[PAR_WAVE3_CYM].type 		= TYPE_UINT8;

	parameterArray[PAR_WAVE2_HH].ptr 		= &hatVoice.modOsc.waveform;
	parameterArray[PAR_WAVE2_HH].type 		= TYPE_UINT8;

	parameterArray[PAR_WAVE3_HH].ptr 		= &hatVoice.modOsc2.waveform;
	parameterArray[PAR_WAVE3_HH].type 		= TYPE_UINT8;

	parameterArray[PAR_NOISE_FREQ1].ptr 	= &snareVoice.noiseOsc.modNodeValue;
	parameterArray[PAR_NOISE_FREQ1].type 	= TYPE_SPECIAL_F;
	parameterArray[PAR_MIX1].ptr 			= &snareVoice.mix;
	parameterArray[PAR_MIX1].type 			= TYPE_FLT;

	parameterArray[PAR_MOD_OSC_F1_CYM].ptr 	= &cymbalVoice.modOsc.modNodeValue;
	parameterArray[PAR_MOD_OSC_F1_CYM].type = TYPE_SPECIAL_F;

	parameterArray[PAR_MOD_OSC_F2_CYM].ptr 	= &cymbalVoice.modOsc2.modNodeValue;
	parameterArray[PAR_MOD_OSC_F2_CYM].type = TYPE_SPECIAL_F;		//70

	parameterArray[PAR_MOD_OSC_GAIN1_CYM].ptr 		= &cymbalVoice.fmModAmount1;
	parameterArray[PAR_MOD_OSC_GAIN1_CYM].type 		= TYPE_FLT;

	parameterArray[PAR_MOD_OSC_GAIN2_CYM].ptr 		= &cymbalVoice.fmModAmount2;
	parameterArray[PAR_MOD_OSC_GAIN2_CYM].type		= TYPE_FLT;

	parameterArray[PAR_MOD_OSC_F1].ptr 		= &hatVoice.modOsc.modNodeValue;
	parameterArray[PAR_MOD_OSC_F1].type 	= TYPE_SPECIAL_F;

	parameterArray[PAR_MOD_OSC_F2].ptr 		= &hatVoice.modOsc2.modNodeValue;;
	parameterArray[PAR_MOD_OSC_F2].type 	= TYPE_SPECIAL_F;

	parameterArray[PAR_MOD_OSC_GAIN1].ptr 	= &hatVoice.fmModAmount1;
	parameterArray[PAR_MOD_OSC_GAIN1].type 	= TYPE_FLT;

	parameterArray[PAR_MOD_OSC_GAIN2].ptr 	= &hatVoice.fmModAmount2;
	parameterArray[PAR_MOD_OSC_GAIN2].type 	= TYPE_FLT;

	parameterArray[PAR_FILTER_FREQ_1].ptr 	= &voiceArray[0].filter.f;
	parameterArray[PAR_FILTER_FREQ_1].type 	= TYPE_FLT;//TYPE_SPECIAL_FILTER_F;

	parameterArray[PAR_FILTER_FREQ_2].ptr 	= &voiceArray[1].filter.f;
	parameterArray[PAR_FILTER_FREQ_2].type 	= TYPE_FLT;//TYPE_SPECIAL_FILTER_F;

	parameterArray[PAR_FILTER_FREQ_3].ptr 	= &voiceArray[2].filter.f;
	parameterArray[PAR_FILTER_FREQ_3].type 	= TYPE_FLT;//TYPE_SPECIAL_FILTER_F;

	parameterArray[PAR_FILTER_FREQ_4].ptr 	= &snareVoice.filter.f;
	parameterArray[PAR_FILTER_FREQ_4].type 	= TYPE_FLT;//TYPE_SPECIAL_FILTER_F;

	parameterArray[PAR_FILTER_FREQ_5].ptr 	= &cymbalVoice.filter.f;
	parameterArray[PAR_FILTER_FREQ_5].type 	= TYPE_FLT;//TYPE_SPECIAL_FILTER_F;

	parameterArray[PAR_FILTER_FREQ_6].ptr 	= &hatVoice.filter.f;
	parameterArray[PAR_FILTER_FREQ_6].type 	= TYPE_FLT;//TYPE_SPECIAL_FILTER_F;

	parameterArray[PAR_RESO_1].ptr 			= &voiceArray[0].filter.q;
	parameterArray[PAR_RESO_1].type 		= TYPE_FLT;

	parameterArray[PAR_RESO_2].ptr 			= &voiceArray[1].filter.q;
	parameterArray[PAR_RESO_2].type 		= TYPE_FLT;

	parameterArray[PAR_RESO_3].ptr 			= &voiceArray[2].filter.q;
	parameterArray[PAR_RESO_3].type 		= TYPE_FLT;

	parameterArray[PAR_RESO_4].ptr 			= &snareVoice.filter.q;
	parameterArray[PAR_RESO_4].type 		= TYPE_FLT;


	parameterArray[PAR_RESO_5].ptr 			= &cymbalVoice.filter.q;
	parameterArray[PAR_RESO_5].type 		= TYPE_FLT;

	parameterArray[PAR_RESO_6].ptr 			= &hatVoice.filter.q;
	parameterArray[PAR_RESO_6].type 		= TYPE_FLT;

	parameterArray[PAR_VELOA1].ptr 			= &voiceArray[0].oscVolEg.attack;
	parameterArray[PAR_VELOA1].type 		= TYPE_FLT;

	parameterArray[PAR_VELOD1].ptr 			= &voiceArray[0].oscVolEg.decay;
	parameterArray[PAR_VELOD1].type 		= TYPE_FLT;

	parameterArray[PAR_VELOA2].ptr 			= &voiceArray[1].oscVolEg.attack;
	parameterArray[PAR_VELOA2].type 		= TYPE_FLT;

	parameterArray[PAR_VELOD2].ptr 			= &voiceArray[1].oscVolEg.decay;
	parameterArray[PAR_VELOD2].type 		= TYPE_FLT;

	parameterArray[PAR_VELOA3].ptr 			= &voiceArray[2].oscVolEg.attack;
	parameterArray[PAR_VELOA3].type 		= TYPE_FLT;

	parameterArray[PAR_VELOD3].ptr 			= &voiceArray[2].oscVolEg.decay;
	parameterArray[PAR_VELOD3].type 		= TYPE_FLT;

	parameterArray[PAR_VELOA4].ptr 			= &snareVoice.oscVolEg.attack;
	parameterArray[PAR_VELOA4].type 		= TYPE_FLT;

	parameterArray[PAR_VELOD4].ptr 			= &snareVoice.oscVolEg.decay;
	parameterArray[PAR_VELOD4].type 		= TYPE_FLT;

	parameterArray[PAR_VELOA5].ptr 			= &cymbalVoice.oscVolEg.attack;
	parameterArray[PAR_VELOA5].type 		= TYPE_FLT;

	parameterArray[PAR_VELOD5].ptr 			= &cymbalVoice.oscVolEg.decay;
	parameterArray[PAR_VELOD5].type 		= TYPE_FLT;

	parameterArray[PAR_VELOA6].ptr 			= &hatVoice.oscVolEg.attack;
	parameterArray[PAR_VELOA6].type 		= TYPE_FLT;

	parameterArray[PAR_VELOD6_CLOSED].ptr 	= &hatVoice.decayClosed;
	parameterArray[PAR_VELOD6_CLOSED].type 	= TYPE_FLT;

	parameterArray[PAR_VELOD6_OPEN].ptr 	= &hatVoice.decayOpen;
	parameterArray[PAR_VELOD6_OPEN].type 	= TYPE_FLT;

	parameterArray[PAR_VOL_SLOPE1].ptr 		= &voiceArray[0].oscVolEg.slope;
	parameterArray[PAR_VOL_SLOPE1].type 	= TYPE_FLT;

	parameterArray[PAR_VOL_SLOPE2].ptr 		= &voiceArray[1].oscVolEg.slope;
	parameterArray[PAR_VOL_SLOPE2].type 	= TYPE_FLT;

	parameterArray[PAR_VOL_SLOPE3].ptr 		= &voiceArray[2].oscVolEg.slope;
	parameterArray[PAR_VOL_SLOPE3].type 	= TYPE_FLT;

	parameterArray[PAR_VOL_SLOPE4].ptr 		= &snareVoice.oscVolEg.slope;
	parameterArray[PAR_VOL_SLOPE4].type 	= TYPE_FLT;

	parameterArray[PAR_VOL_SLOPE5].ptr 		= &cymbalVoice.oscVolEg.slope;
	parameterArray[PAR_VOL_SLOPE5].type 	= TYPE_FLT;

	parameterArray[PAR_VOL_SLOPE6].ptr 		= &hatVoice.oscVolEg.slope;
	parameterArray[PAR_VOL_SLOPE6].type 	= TYPE_FLT;

	parameterArray[PAR_REPEAT4].ptr 		= &snareVoice.oscVolEg.repeat;
	parameterArray[PAR_REPEAT4].type 		= TYPE_UINT8;

	parameterArray[PAR_REPEAT5].ptr 		= &cymbalVoice.oscVolEg.repeat;
	parameterArray[PAR_REPEAT5].type 		= TYPE_UINT8;

	parameterArray[PAR_MOD_EG1].ptr 		= &voiceArray[0].oscPitchEg.decay;
	parameterArray[PAR_MOD_EG1].type 		= TYPE_FLT;

	parameterArray[PAR_MOD_EG2].ptr 		= &voiceArray[1].oscPitchEg.decay;
	parameterArray[PAR_MOD_EG2].type 		= TYPE_FLT;

	parameterArray[PAR_MOD_EG3].ptr 		= &voiceArray[2].oscPitchEg.decay;
	parameterArray[PAR_MOD_EG3].type 		= TYPE_FLT;

	parameterArray[PAR_MOD_EG4].ptr 		= &snareVoice.oscPitchEg.decay;
	parameterArray[PAR_MOD_EG4].type 		= TYPE_FLT;
	parameterArray[PAR_MODAMNT1].ptr 		= &voiceArray[0].egPitchModAmount;
	parameterArray[PAR_MODAMNT1].type 		= TYPE_FLT;
	parameterArray[PAR_MODAMNT2].ptr 		= &voiceArray[1].egPitchModAmount;
	parameterArray[PAR_MODAMNT2].type 		= TYPE_FLT;
	parameterArray[PAR_MODAMNT3].ptr 		= &voiceArray[2].egPitchModAmount;
	parameterArray[PAR_MODAMNT3].type 		= TYPE_FLT;

	parameterArray[PAR_MODAMNT4].ptr 		= &snareVoice.egPitchModAmount;
	parameterArray[PAR_MODAMNT4].type 		= TYPE_FLT;

	parameterArray[PAR_PITCH_SLOPE1].ptr 	= &voiceArray[0].oscPitchEg.slope;
	parameterArray[PAR_PITCH_SLOPE1].type 	= TYPE_FLT;

	parameterArray[PAR_PITCH_SLOPE2].ptr 	= &voiceArray[1].oscPitchEg.slope;
	parameterArray[PAR_PITCH_SLOPE2].type 	= TYPE_FLT;

	parameterArray[PAR_PITCH_SLOPE3].ptr 	= &voiceArray[2].oscPitchEg.slope;
	parameterArray[PAR_PITCH_SLOPE3].type 	= TYPE_FLT;

	parameterArray[PAR_PITCH_SLOPE4].ptr 	= &snareVoice.oscPitchEg.slope;
	parameterArray[PAR_PITCH_SLOPE4].type 	= TYPE_FLT;

	parameterArray[PAR_FMAMNT1].ptr 		= &voiceArray[0].fmModAmount;
	parameterArray[PAR_FMAMNT1].type 		= TYPE_FLT;

	parameterArray[PAR_FM_FREQ1].ptr 		= &voiceArray[0].modOsc.modNodeValue;
	parameterArray[PAR_FM_FREQ1].type 		= TYPE_SPECIAL_F;//TYPE_UINT32;

	parameterArray[PAR_FMAMNT2].ptr 		= &voiceArray[1].fmModAmount;
	parameterArray[PAR_FMAMNT2].type 		= TYPE_FLT;

	parameterArray[PAR_FM_FREQ2].ptr 		= &voiceArray[1].modOsc.modNodeValue;
	parameterArray[PAR_FM_FREQ2].type 		= TYPE_SPECIAL_F;//TYPE_UINT32;

	parameterArray[PAR_FMAMNT3].ptr 		= &voiceArray[2].fmModAmount;
	parameterArray[PAR_FMAMNT3].type 		= TYPE_FLT;

	parameterArray[PAR_FM_FREQ3].ptr 		= &voiceArray[2].modOsc.modNodeValue;
	parameterArray[PAR_FM_FREQ3].type 		= TYPE_SPECIAL_F;//TYPE_UINT32;


	parameterArray[PAR_VOL1].ptr 			= &voiceArray[0].vol;
	parameterArray[PAR_VOL1].type			= TYPE_FLT;
	parameterArray[PAR_VOL2].ptr 			= &voiceArray[1].vol;
	parameterArray[PAR_VOL2].type			= TYPE_FLT;
	parameterArray[PAR_VOL3].ptr 			= &voiceArray[2].vol;
	parameterArray[PAR_VOL3].type			= TYPE_FLT;
	parameterArray[PAR_VOL4].ptr 			= &snareVoice.vol;
	parameterArray[PAR_VOL4].type 			= TYPE_FLT;
	parameterArray[PAR_VOL5].ptr 			= &cymbalVoice.vol;
	parameterArray[PAR_VOL5].type 			= TYPE_FLT;
	parameterArray[PAR_VOL6].ptr 			= &hatVoice.vol;
	parameterArray[PAR_VOL6].type 			= TYPE_FLT;

	parameterArray[PAR_PAN1].ptr 			= &voiceArray[0].pan;
	parameterArray[PAR_PAN1].type 			= TYPE_UINT8;
	parameterArray[PAR_PAN2].ptr 			= &voiceArray[1].pan;
	parameterArray[PAR_PAN2].type 			= TYPE_UINT8;
	parameterArray[PAR_PAN3].ptr 			= &voiceArray[2].pan;
	parameterArray[PAR_PAN3].type 			= TYPE_UINT8;
	parameterArray[PAR_PAN4].ptr 			= &snareVoice.pan;
	parameterArray[PAR_PAN4].type 			= TYPE_UINT8;
	parameterArray[PAR_PAN5].ptr 			= &cymbalVoice.pan;
	parameterArray[PAR_PAN5].type 			= TYPE_UINT8;
	parameterArray[PAR_PAN6].ptr 			= &hatVoice.pan;
	parameterArray[PAR_PAN6].type 			= TYPE_UINT8;

	parameterArray[PAR_DRIVE1].ptr 			= &voiceArray[0].distortion.shape;
	parameterArray[PAR_DRIVE1].type 		= TYPE_FLT;

	parameterArray[PAR_DRIVE2].ptr 			= &voiceArray[1].distortion.shape;
	parameterArray[PAR_DRIVE2].type 		= TYPE_FLT;

	parameterArray[PAR_DRIVE3].ptr 			= &voiceArray[2].distortion.shape;
	parameterArray[PAR_DRIVE3].type 		= TYPE_FLT;

	parameterArray[PAR_SNARE_DISTORTION].ptr 	= &snareVoice.distortion.shape;
	parameterArray[PAR_SNARE_DISTORTION].type 	= TYPE_FLT;

	parameterArray[PAR_CYMBAL_DISTORTION].ptr 	= &cymbalVoice.distortion.shape;
	parameterArray[PAR_CYMBAL_DISTORTION].type 	= TYPE_FLT;

	parameterArray[PAR_HAT_DISTORTION].ptr 		= &hatVoice.distortion.shape;
	parameterArray[PAR_HAT_DISTORTION].type 	= TYPE_FLT;

	parameterArray[PAR_VOICE_DECIMATION1].ptr 	= &mixer_decimation_rate[0];
	parameterArray[PAR_VOICE_DECIMATION1].type 	= TYPE_FLT;
	parameterArray[PAR_VOICE_DECIMATION2].ptr 	= &mixer_decimation_rate[1];
	parameterArray[PAR_VOICE_DECIMATION2].type 	= TYPE_FLT;
	parameterArray[PAR_VOICE_DECIMATION3].ptr 	= &mixer_decimation_rate[2];
	parameterArray[PAR_VOICE_DECIMATION3].type 	= TYPE_FLT;
	parameterArray[PAR_VOICE_DECIMATION4].ptr 	= &mixer_decimation_rate[3];
	parameterArray[PAR_VOICE_DECIMATION4].type 	= TYPE_FLT;
	parameterArray[PAR_VOICE_DECIMATION5].ptr 	= &mixer_decimation_rate[4];
	parameterArray[PAR_VOICE_DECIMATION5].type 	= TYPE_FLT;
	parameterArray[PAR_VOICE_DECIMATION6].ptr 	= &mixer_decimation_rate[5];
	parameterArray[PAR_VOICE_DECIMATION6].type 	= TYPE_FLT;
	parameterArray[PAR_VOICE_DECIMATION_ALL].ptr= &mixer_decimation_rate[6];
	parameterArray[PAR_VOICE_DECIMATION_ALL].type 	= TYPE_FLT;

	parameterArray[PAR_FREQ_LFO1].ptr 		= &voiceArray[0].lfo.modNodeValue;
	parameterArray[PAR_FREQ_LFO1].type 		= TYPE_SPECIAL_F;//TYPE_FLT;
	parameterArray[PAR_FREQ_LFO2].ptr 		= &voiceArray[1].lfo.modNodeValue;
	parameterArray[PAR_FREQ_LFO2].type 		= TYPE_SPECIAL_F;//TYPE_FLT;
	parameterArray[PAR_FREQ_LFO3].ptr 		= &voiceArray[2].lfo.modNodeValue;
	parameterArray[PAR_FREQ_LFO3].type 		= TYPE_SPECIAL_F;//TYPE_FLT;
	parameterArray[PAR_FREQ_LFO4].ptr 		= &snareVoice.lfo.modNodeValue;
	parameterArray[PAR_FREQ_LFO4].type 		= TYPE_SPECIAL_F;//TYPE_FLT;
	parameterArray[PAR_FREQ_LFO5].ptr 		= &cymbalVoice.lfo.modNodeValue;
	parameterArray[PAR_FREQ_LFO5].type 		= TYPE_SPECIAL_F;//TYPE_FLT;
	parameterArray[PAR_FREQ_LFO6].ptr 		= &hatVoice.lfo.modNodeValue;
	parameterArray[PAR_FREQ_LFO6].type 		= TYPE_SPECIAL_F;//TYPE_FLT;

	parameterArray[PAR_AMOUNT_LFO1].ptr 	= &voiceArray[0].lfo.modTarget.amount;
	parameterArray[PAR_AMOUNT_LFO1].type 	= TYPE_FLT;
	parameterArray[PAR_AMOUNT_LFO2].ptr 	= &voiceArray[1].lfo.modTarget.amount;
	parameterArray[PAR_AMOUNT_LFO2].type 	= TYPE_FLT;
	parameterArray[PAR_AMOUNT_LFO3].ptr 	= &voiceArray[2].lfo.modTarget.amount;
	parameterArray[PAR_AMOUNT_LFO3].type 	= TYPE_FLT;
	parameterArray[PAR_AMOUNT_LFO4].ptr 	= &snareVoice.lfo.modTarget.amount;
	parameterArray[PAR_AMOUNT_LFO4].type 	= TYPE_FLT;
	parameterArray[PAR_AMOUNT_LFO5].ptr 	= &cymbalVoice.lfo.modTarget.amount;
	parameterArray[PAR_AMOUNT_LFO5].type 	= TYPE_FLT;
	parameterArray[PAR_AMOUNT_LFO6].ptr 	= &hatVoice.lfo.modTarget.amount;
	parameterArray[PAR_AMOUNT_LFO6].type 	= TYPE_FLT;
	//######################################
	//######## END OF MIDI DATASIZE ########
	//######## PARAM NR 127 REACHED ########
	//######################################

	parameterArray[PAR_FILTER_DRIVE_1].ptr 	= &voiceArray[0].filter.drive;
	parameterArray[PAR_FILTER_DRIVE_1].type = TYPE_FLT;

	parameterArray[PAR_FILTER_DRIVE_2].ptr 	= &voiceArray[1].filter.drive;
	parameterArray[PAR_FILTER_DRIVE_2].type = TYPE_FLT;

	parameterArray[PAR_FILTER_DRIVE_3].ptr 	= &voiceArray[2].filter.drive;
	parameterArray[PAR_FILTER_DRIVE_3].type = TYPE_FLT;

	parameterArray[PAR_FILTER_DRIVE_4].ptr 	= &snareVoice.filter.drive;
	parameterArray[PAR_FILTER_DRIVE_4].type = TYPE_FLT;

	parameterArray[PAR_FILTER_DRIVE_5].ptr 	= &cymbalVoice.filter.drive;
	parameterArray[PAR_FILTER_DRIVE_5].type = TYPE_FLT;

	parameterArray[PAR_FILTER_DRIVE_6].ptr 	= &hatVoice.filter.drive;
	parameterArray[PAR_FILTER_DRIVE_6].type = TYPE_FLT;

	parameterArray[PAR_MIX_MOD_1].ptr 		= &voiceArray[0].mixOscs;
	parameterArray[PAR_MIX_MOD_1].type 		= TYPE_UINT8;

	parameterArray[PAR_MIX_MOD_2].ptr 		= &voiceArray[1].mixOscs;
	parameterArray[PAR_MIX_MOD_2].type 		= TYPE_UINT8;

	parameterArray[PAR_MIX_MOD_3].ptr 		= &voiceArray[2].mixOscs;
	parameterArray[PAR_MIX_MOD_3].type 		= TYPE_UINT8;



	parameterArray[PAR_VOLUME_MOD_ON_OFF1].ptr 			= &voiceArray[0].volumeMod;
	parameterArray[PAR_VOLUME_MOD_ON_OFF1].type 		= TYPE_UINT8;

	parameterArray[PAR_VOLUME_MOD_ON_OFF2].ptr 			= &voiceArray[1].volumeMod;
	parameterArray[PAR_VOLUME_MOD_ON_OFF2].type		 	= TYPE_UINT8;

	parameterArray[PAR_VOLUME_MOD_ON_OFF3].ptr 			= &voiceArray[2].volumeMod;
	parameterArray[PAR_VOLUME_MOD_ON_OFF3].type 		= TYPE_UINT8;

	parameterArray[PAR_VOLUME_MOD_ON_OFF4].ptr 			= &snareVoice.volumeMod;;
	parameterArray[PAR_VOLUME_MOD_ON_OFF4].type 		= TYPE_UINT8;

	parameterArray[PAR_VOLUME_MOD_ON_OFF5].ptr 			= &cymbalVoice.volumeMod;
	parameterArray[PAR_VOLUME_MOD_ON_OFF5].type 		= TYPE_UINT8;

	parameterArray[PAR_VOLUME_MOD_ON_OFF6].ptr 			= &hatVoice.volumeMod;
	parameterArray[PAR_VOLUME_MOD_ON_OFF6].type 		= TYPE_UINT8;

	parameterArray[PAR_VELO_MOD_AMT_1].ptr 	= &velocityModulators[0].amount;
	parameterArray[PAR_VELO_MOD_AMT_1].type = TYPE_FLT;

	parameterArray[PAR_VELO_MOD_AMT_2].ptr 	= &velocityModulators[1].amount;
	parameterArray[PAR_VELO_MOD_AMT_2].type = TYPE_FLT;

	parameterArray[PAR_VELO_MOD_AMT_3].ptr 	= &velocityModulators[2].amount;
	parameterArray[PAR_VELO_MOD_AMT_3].type = TYPE_FLT;

	parameterArray[PAR_VELO_MOD_AMT_4].ptr 	= &velocityModulators[3].amount;
	parameterArray[PAR_VELO_MOD_AMT_4].type = TYPE_FLT;

	parameterArray[PAR_VELO_MOD_AMT_5].ptr 	= &velocityModulators[4].amount;
	parameterArray[PAR_VELO_MOD_AMT_5].type = TYPE_FLT;

	parameterArray[PAR_VELO_MOD_AMT_6].ptr 	= &velocityModulators[5].amount;
	parameterArray[PAR_VELO_MOD_AMT_6].type = TYPE_FLT;

	parameterArray[PAR_VEL_DEST_1].ptr 		= &velocityModulators[0].destination;
	parameterArray[PAR_VEL_DEST_1].type 	= TYPE_UINT8;

	parameterArray[PAR_VEL_DEST_2].ptr 		= &velocityModulators[1].destination;
	parameterArray[PAR_VEL_DEST_2].type 	= TYPE_UINT8;

	parameterArray[PAR_VEL_DEST_3].ptr 		= &velocityModulators[2].destination;
	parameterArray[PAR_VEL_DEST_3].type 	= TYPE_UINT8;

	parameterArray[PAR_VEL_DEST_4].ptr 		= &velocityModulators[3].destination;
	parameterArray[PAR_VEL_DEST_4].type 	= TYPE_UINT8;

	parameterArray[PAR_VEL_DEST_5].ptr 		= &velocityModulators[4].destination;
	parameterArray[PAR_VEL_DEST_5].type 	= TYPE_UINT8;

	parameterArray[PAR_VEL_DEST_6].ptr 		= &velocityModulators[5].destination;
	parameterArray[PAR_VEL_DEST_6].type 	= TYPE_UINT8;

	parameterArray[PAR_WAVE_LFO1].ptr 		= &voiceArray[0].lfo.waveform;
	parameterArray[PAR_WAVE_LFO1].type = TYPE_UINT8;
	parameterArray[PAR_WAVE_LFO2].ptr 		= &voiceArray[1].lfo.waveform;
	parameterArray[PAR_WAVE_LFO2].type = TYPE_UINT8;
	parameterArray[PAR_WAVE_LFO3].ptr 		= &voiceArray[2].lfo.waveform;
	parameterArray[PAR_WAVE_LFO3].type = TYPE_UINT8;
	parameterArray[PAR_WAVE_LFO4].ptr 		= &snareVoice.lfo.waveform;
	parameterArray[PAR_WAVE_LFO4].type = TYPE_UINT8;
	parameterArray[PAR_WAVE_LFO5].ptr 		= &cymbalVoice.lfo.waveform;
	parameterArray[PAR_WAVE_LFO5].type = TYPE_UINT8;
	parameterArray[PAR_WAVE_LFO6].ptr 		= &hatVoice.lfo.waveform;
	parameterArray[PAR_WAVE_LFO6].type = TYPE_UINT8;

	/*
	PAR_VOICE_LFO1,
	PAR_VOICE_LFO2,
	PAR_VOICE_LFO3,
	PAR_VOICE_LFO4,
	PAR_VOICE_LFO5,
	PAR_VOICE_LFO6,

	PAR_TARGET_LFO1,
	PAR_TARGET_LFO2,
	PAR_TARGET_LFO3,
	PAR_TARGET_LFO4,
	PAR_TARGET_LFO5,
	PAR_TARGET_LFO6,
	*/

	parameterArray[PAR_RETRIGGER_LFO1].ptr 	= &voiceArray[0].lfo.retrigger;
	parameterArray[PAR_RETRIGGER_LFO1].type = TYPE_UINT8;
	parameterArray[PAR_RETRIGGER_LFO2].ptr 	= &voiceArray[1].lfo.retrigger;
	parameterArray[PAR_RETRIGGER_LFO2].type = TYPE_UINT8;
	parameterArray[PAR_RETRIGGER_LFO3].ptr 	= &voiceArray[2].lfo.retrigger;
	parameterArray[PAR_RETRIGGER_LFO3].type = TYPE_UINT8;
	parameterArray[PAR_RETRIGGER_LFO4].ptr 	= &snareVoice.lfo.retrigger;
	parameterArray[PAR_RETRIGGER_LFO4].type = TYPE_UINT8;
	parameterArray[PAR_RETRIGGER_LFO5].ptr 	= &cymbalVoice.lfo.retrigger;
	parameterArray[PAR_RETRIGGER_LFO5].type = TYPE_UINT8;
	parameterArray[PAR_RETRIGGER_LFO6].ptr 	= &hatVoice.lfo.retrigger;
	parameterArray[PAR_RETRIGGER_LFO6].type = TYPE_UINT8;

	parameterArray[PAR_SYNC_LFO1].ptr 		= &voiceArray[0].lfo.sync;
	parameterArray[PAR_SYNC_LFO1].type		= TYPE_UINT8;
	parameterArray[PAR_SYNC_LFO2].ptr 		= &voiceArray[1].lfo.sync;
	parameterArray[PAR_SYNC_LFO2].type 		= TYPE_UINT8;
	parameterArray[PAR_SYNC_LFO3].ptr 		= &voiceArray[2].lfo.sync;
	parameterArray[PAR_SYNC_LFO3].type 		= TYPE_UINT8;
	parameterArray[PAR_SYNC_LFO4].ptr 		= &snareVoice.lfo.sync;
	parameterArray[PAR_SYNC_LFO4].type 		= TYPE_UINT8;
	parameterArray[PAR_SYNC_LFO5].ptr 		= &cymbalVoice.lfo.sync;
	parameterArray[PAR_SYNC_LFO5].type 		= TYPE_UINT8;
	parameterArray[PAR_SYNC_LFO6].ptr 		= &hatVoice.lfo.sync;
	parameterArray[PAR_SYNC_LFO6].type 		= TYPE_UINT8;

	parameterArray[PAR_OFFSET_LFO1].ptr 	= &voiceArray[0].lfo.phaseOffset;
	parameterArray[PAR_OFFSET_LFO1].type 	= TYPE_UINT32;
	parameterArray[PAR_OFFSET_LFO2].ptr 	= &voiceArray[1].lfo.phaseOffset;
	parameterArray[PAR_OFFSET_LFO2].type 	= TYPE_UINT32;
	parameterArray[PAR_OFFSET_LFO3].ptr 	= &voiceArray[2].lfo.phaseOffset;
	parameterArray[PAR_OFFSET_LFO3].type 	= TYPE_UINT32;
	parameterArray[PAR_OFFSET_LFO4].ptr 	= &snareVoice.lfo.phaseOffset;
	parameterArray[PAR_OFFSET_LFO4].type 	= TYPE_UINT32;
	parameterArray[PAR_OFFSET_LFO5].ptr 	= &cymbalVoice.lfo.phaseOffset;
	parameterArray[PAR_OFFSET_LFO5].type 	= TYPE_UINT32;
	parameterArray[PAR_OFFSET_LFO6].ptr 	= &hatVoice.lfo.phaseOffset;
	parameterArray[PAR_OFFSET_LFO6].type 	= TYPE_UINT32;

	parameterArray[PAR_FILTER_TYPE_1].ptr 	= &voiceArray[0].filterType;
	parameterArray[PAR_FILTER_TYPE_1].type 	= TYPE_UINT8;
	parameterArray[PAR_FILTER_TYPE_2].ptr 	= &voiceArray[1].filterType;
	parameterArray[PAR_FILTER_TYPE_2].type 	= TYPE_UINT8;
	parameterArray[PAR_FILTER_TYPE_3].ptr 	= &voiceArray[2].filterType;
	parameterArray[PAR_FILTER_TYPE_3].type 	= TYPE_UINT8;
	parameterArray[PAR_FILTER_TYPE_4].ptr 	= &snareVoice.filterType;
	parameterArray[PAR_FILTER_TYPE_4].type 	= TYPE_UINT8;
	parameterArray[PAR_FILTER_TYPE_5].ptr 	= &cymbalVoice.filterType;;
	parameterArray[PAR_FILTER_TYPE_5].type 	= TYPE_UINT8;
	parameterArray[PAR_FILTER_TYPE_6].ptr 	= &hatVoice.filterType;;
	parameterArray[PAR_FILTER_TYPE_6].type 	= TYPE_UINT8;

	parameterArray[PAR_TRANS1_VOL].ptr 		= &voiceArray[0].transGen.volume;
	parameterArray[PAR_TRANS1_VOL].type 	= TYPE_FLT;
	parameterArray[PAR_TRANS2_VOL].ptr 		= &voiceArray[1].transGen.volume;;
	parameterArray[PAR_TRANS2_VOL].type 	= TYPE_FLT;
	parameterArray[PAR_TRANS3_VOL].ptr 		= &voiceArray[2].transGen.volume;;
	parameterArray[PAR_TRANS3_VOL].type 	= TYPE_FLT;
	parameterArray[PAR_TRANS4_VOL].ptr 		= &snareVoice.transGen.volume;
	parameterArray[PAR_TRANS4_VOL].type 	= TYPE_FLT;
	parameterArray[PAR_TRANS5_VOL].ptr 		= &cymbalVoice.transGen.volume;
	parameterArray[PAR_TRANS5_VOL].type 	= TYPE_FLT;
	parameterArray[PAR_TRANS6_VOL].ptr 		= &hatVoice.transGen.volume;
	parameterArray[PAR_TRANS6_VOL].type 	= TYPE_FLT;



	parameterArray[PAR_TRANS1_WAVE].ptr 	= &voiceArray[0].transGen.waveform;
	parameterArray[PAR_TRANS1_WAVE].type 	= TYPE_UINT8;
	parameterArray[PAR_TRANS2_WAVE].ptr 	= &voiceArray[1].transGen.waveform;;
	parameterArray[PAR_TRANS2_WAVE].type 	= TYPE_UINT8;
	parameterArray[PAR_TRANS3_WAVE].ptr 	= &voiceArray[2].transGen.waveform;;
	parameterArray[PAR_TRANS3_WAVE].type 	= TYPE_UINT8;
	parameterArray[PAR_TRANS4_WAVE].ptr 	= &snareVoice.transGen.waveform;
	parameterArray[PAR_TRANS4_WAVE].type 	= TYPE_UINT8;
	parameterArray[PAR_TRANS5_WAVE].ptr 	= &cymbalVoice.transGen.waveform;
	parameterArray[PAR_TRANS5_WAVE].type 	= TYPE_UINT8;
	parameterArray[PAR_TRANS6_WAVE].ptr 	= &hatVoice.transGen.waveform;
	parameterArray[PAR_TRANS6_WAVE].type 	= TYPE_UINT8;


	parameterArray[PAR_TRANS1_FREQ].ptr 	= &voiceArray[0].transGen.pitch;
	parameterArray[PAR_TRANS1_FREQ].type 	= TYPE_FLT;
	parameterArray[PAR_TRANS2_FREQ].ptr 	= &voiceArray[1].transGen.pitch;
	parameterArray[PAR_TRANS2_FREQ].type 	= TYPE_FLT;
	parameterArray[PAR_TRANS3_FREQ].ptr 	= &voiceArray[2].transGen.pitch;;
	parameterArray[PAR_TRANS3_FREQ].type 	= TYPE_FLT;
	parameterArray[PAR_TRANS4_FREQ].ptr 	= &snareVoice.transGen.pitch;
	parameterArray[PAR_TRANS4_FREQ].type 	= TYPE_FLT;
	parameterArray[PAR_TRANS5_FREQ].ptr 	= &cymbalVoice.transGen.pitch;
	parameterArray[PAR_TRANS5_FREQ].type 	= TYPE_FLT;
	parameterArray[PAR_TRANS6_FREQ].ptr 	= &hatVoice.transGen.pitch;
	parameterArray[PAR_TRANS6_FREQ].type 	= TYPE_FLT;

	parameterArray[PAR_AUDIO_OUT1].ptr 		= &mixer_audioRouting[0];
	parameterArray[PAR_AUDIO_OUT1].type 	= TYPE_UINT8;
	parameterArray[PAR_AUDIO_OUT2].ptr 		= &mixer_audioRouting[1];
	parameterArray[PAR_AUDIO_OUT2].type		= TYPE_UINT8;
	parameterArray[PAR_AUDIO_OUT3].ptr 		= &mixer_audioRouting[2];
	parameterArray[PAR_AUDIO_OUT3].type 	= TYPE_UINT8;
	parameterArray[PAR_AUDIO_OUT4].ptr 		= &mixer_audioRouting[3];
	parameterArray[PAR_AUDIO_OUT4].type 	= TYPE_UINT8;
	parameterArray[PAR_AUDIO_OUT5].ptr 		= &mixer_audioRouting[4];
	parameterArray[PAR_AUDIO_OUT5].type 	= TYPE_UINT8;
	parameterArray[PAR_AUDIO_OUT6].ptr 		= &mixer_audioRouting[5];
	parameterArray[PAR_AUDIO_OUT6].type 	= TYPE_UINT8;

	parameterArray[PAR_MIDI_NOTE1].ptr 		= &midi_NoteOverride[0];
	parameterArray[PAR_MIDI_NOTE1].type 	= TYPE_UINT8;
	parameterArray[PAR_MIDI_NOTE2].ptr 		= &midi_NoteOverride[1];
	parameterArray[PAR_MIDI_NOTE2].type		= TYPE_UINT8;
	parameterArray[PAR_MIDI_NOTE3].ptr 		= &midi_NoteOverride[2];
	parameterArray[PAR_MIDI_NOTE3].type 	= TYPE_UINT8;
	parameterArray[PAR_MIDI_NOTE4].ptr 		= &midi_NoteOverride[3];
	parameterArray[PAR_MIDI_NOTE4].type 	= TYPE_UINT8;
	parameterArray[PAR_MIDI_NOTE5].ptr 		= &midi_NoteOverride[4];
	parameterArray[PAR_MIDI_NOTE5].type 	= TYPE_UINT8;
	parameterArray[PAR_MIDI_NOTE6].ptr 		= &midi_NoteOverride[5];
	parameterArray[PAR_MIDI_NOTE6].type 	= TYPE_UINT8;
	parameterArray[PAR_MIDI_NOTE7].ptr 		= &midi_NoteOverride[6];
	parameterArray[PAR_MIDI_NOTE7].type 	= TYPE_UINT8;
	//#########################################
	//######## End of sound Parameters ########
	//#########################################
}
