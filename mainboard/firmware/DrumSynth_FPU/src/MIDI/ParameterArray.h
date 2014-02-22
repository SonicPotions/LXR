/*
 * ParameterArray.h
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


#ifndef PARAMETERARRAY_H_
#define PARAMETERARRAY_H_

#define TYPE_UINT8 				0	// byte
#define TYPE_FLT 				1	// float
#define TYPE_SPECIAL_F			2	// float value targeting modNodeValue (as opposed to actual parameter)
#define TYPE_UINT32				3	// 32 bit int
#define TYPE_SPECIAL_P			4	// pan
#define TYPE_SPECIAL_FILTER_F	5	// not used apparently

// --AS **PATROT this list needs to correspond exactly with the sound parameters in the AVR side because
// modulation targets are sent across and they have to match in this list
enum ParamEnums
{

	PAR_NONE,	//TODO this is modwheel	- stupid offset
	PAR_MOD_WHEEL ,

	//waveform parameters need to be grouped
	//makes the special case to show their names instead of 0-127 values easier

	PAR_OSC_WAVE_DRUM1 = PAR_MOD_WHEEL,
	PAR_OSC_WAVE_DRUM2,
	PAR_OSC_WAVE_DRUM3,
	PAR_OSC_WAVE_SNARE,
		PAR_NRPN_DATA_ENTRY_COARSE,
	PAR_WAVE1_CYM,
	PAR_WAVE1_HH,

	PAR_COARSE1,
	PAR_FINE1,
	PAR_COARSE2,
	PAR_FINE2,
	PAR_COARSE3,
	PAR_FINE3,
	PAR_COARSE4,
	PAR_FINE4,
	PAR_COARSE5,
	PAR_FINE5,
	PAR_COARSE6,
	PAR_FINE6,

	PAR_MOD_WAVE_DRUM1,
	PAR_MOD_WAVE_DRUM2,
	PAR_MOD_WAVE_DRUM3,
	PAR_WAVE2_CYM,
	PAR_WAVE3_CYM,
	PAR_WAVE2_HH,
	PAR_WAVE3_HH,

	PAR_NOISE_FREQ1,
	PAR_MIX1,

	PAR_MOD_OSC_F1_CYM,
	PAR_MOD_OSC_F2_CYM,		//70
	PAR_MOD_OSC_GAIN1_CYM,
	PAR_MOD_OSC_GAIN2_CYM,
	PAR_MOD_OSC_F1,
	PAR_MOD_OSC_F2,
	PAR_MOD_OSC_GAIN1,
	PAR_MOD_OSC_GAIN2,

	PAR_FILTER_FREQ_1,
	PAR_FILTER_FREQ_2,
	PAR_FILTER_FREQ_3,
	PAR_FILTER_FREQ_4,
	PAR_FILTER_FREQ_5,
	PAR_FILTER_FREQ_6,

	PAR_RESO_1,
	PAR_RESO_2,
	PAR_RESO_3,
	PAR_RESO_4,
	PAR_RESO_5,
	PAR_RESO_6,

	PAR_VELOA1,
	PAR_VELOD1,
	PAR_VELOA2,
	PAR_VELOD2,
	PAR_VELOA3,
	PAR_VELOD3,
	PAR_VELOA4,
	PAR_VELOD4,
	PAR_VELOA5,
	PAR_VELOD5,
	PAR_VELOA6,
	PAR_VELOD6_CLOSED,
	PAR_VELOD6_OPEN,

	PAR_VOL_SLOPE1,
	PAR_VOL_SLOPE2,
	PAR_VOL_SLOPE3,
	PAR_VOL_SLOPE4,
	PAR_VOL_SLOPE5,
	PAR_VOL_SLOPE6,

	PAR_REPEAT4,
	PAR_REPEAT5,

	PAR_MOD_EG1,
	PAR_MOD_EG2,
	PAR_MOD_EG3,
	PAR_MOD_EG4,

	PAR_MODAMNT1,
	PAR_MODAMNT2,
	PAR_MODAMNT3,
	PAR_MODAMNT4,

	PAR_PITCH_SLOPE1,
	PAR_PITCH_SLOPE2,
	PAR_PITCH_SLOPE3,
	PAR_PITCH_SLOPE4,

	PAR_FMAMNT1,
	PAR_FM_FREQ1,
	PAR_FMAMNT2,
	PAR_FM_FREQ2,
	PAR_FMAMNT3,
	PAR_FM_FREQ3,

	PAR_VOL1,
	PAR_VOL2,
	PAR_VOL3,
	PAR_VOL4,
	PAR_VOL5,
	PAR_VOL6,

	PAR_PAN1,
	PAR_PAN2,
	PAR_PAN3,
		PAR_NRPN_FINE,
		PAR_NRPN_COARSE,
	PAR_PAN4,
	PAR_PAN5,
	PAR_PAN6,

	PAR_DRIVE1,
	PAR_DRIVE2,
	PAR_DRIVE3,
	PAR_SNARE_DISTORTION,
	PAR_CYMBAL_DISTORTION,
	PAR_HAT_DISTORTION,

	PAR_VOICE_DECIMATION1,
	PAR_VOICE_DECIMATION2,
	PAR_VOICE_DECIMATION3,
	PAR_VOICE_DECIMATION4,
	PAR_VOICE_DECIMATION5,
	PAR_VOICE_DECIMATION6,
	PAR_VOICE_DECIMATION_ALL,

	PAR_FREQ_LFO1 ,
	PAR_FREQ_LFO2,
	PAR_FREQ_LFO3,
	PAR_FREQ_LFO4,
	PAR_FREQ_LFO5,
	PAR_FREQ_LFO6,

	PAR_AMOUNT_LFO1,
	PAR_AMOUNT_LFO2,
	PAR_AMOUNT_LFO3,
	PAR_AMOUNT_LFO4,
	PAR_AMOUNT_LFO5,
	PAR_AMOUNT_LFO6,

			PAR_RESERVED4,
	//######################################
	//######## END OF MIDI DATASIZE ########
	//######## PARAM NR 127 REACHED ########
	//######################################

	PAR_FILTER_DRIVE_1,
	PAR_FILTER_DRIVE_2,
	PAR_FILTER_DRIVE_3,
	PAR_FILTER_DRIVE_4,
	PAR_FILTER_DRIVE_5,
	PAR_FILTER_DRIVE_6,

	PAR_MIX_MOD_1,
	PAR_MIX_MOD_2,
	PAR_MIX_MOD_3,

	PAR_VOLUME_MOD_ON_OFF1,
	PAR_VOLUME_MOD_ON_OFF2,
	PAR_VOLUME_MOD_ON_OFF3,
	PAR_VOLUME_MOD_ON_OFF4,
	PAR_VOLUME_MOD_ON_OFF5,
	PAR_VOLUME_MOD_ON_OFF6,

	PAR_VELO_MOD_AMT_1,
	PAR_VELO_MOD_AMT_2,
	PAR_VELO_MOD_AMT_3,
	PAR_VELO_MOD_AMT_4,
	PAR_VELO_MOD_AMT_5,
	PAR_VELO_MOD_AMT_6,

	PAR_VEL_DEST_1,
	PAR_VEL_DEST_2,
	PAR_VEL_DEST_3,
	PAR_VEL_DEST_4,
	PAR_VEL_DEST_5,
	PAR_VEL_DEST_6,

	PAR_WAVE_LFO1,
	PAR_WAVE_LFO2,
	PAR_WAVE_LFO3,
	PAR_WAVE_LFO4,
	PAR_WAVE_LFO5,
	PAR_WAVE_LFO6,

	//the target and voice parameters must be after one another!
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

	PAR_RETRIGGER_LFO1,
	PAR_RETRIGGER_LFO2,
	PAR_RETRIGGER_LFO3,
	PAR_RETRIGGER_LFO4,
	PAR_RETRIGGER_LFO5,
	PAR_RETRIGGER_LFO6,

	PAR_SYNC_LFO1,
	PAR_SYNC_LFO2,
	PAR_SYNC_LFO3,
	PAR_SYNC_LFO4,
	PAR_SYNC_LFO5,
	PAR_SYNC_LFO6,

	PAR_OFFSET_LFO1,
	PAR_OFFSET_LFO2,
	PAR_OFFSET_LFO3,
	PAR_OFFSET_LFO4,
	PAR_OFFSET_LFO5,
	PAR_OFFSET_LFO6,

	PAR_FILTER_TYPE_1,
	PAR_FILTER_TYPE_2,
	PAR_FILTER_TYPE_3,
	PAR_FILTER_TYPE_4,
	PAR_FILTER_TYPE_5,
	PAR_FILTER_TYPE_6,

	PAR_TRANS1_VOL,
	PAR_TRANS2_VOL,
	PAR_TRANS3_VOL,
	PAR_TRANS4_VOL,
	PAR_TRANS5_VOL,
	PAR_TRANS6_VOL,

	PAR_TRANS1_WAVE,
	PAR_TRANS2_WAVE,
	PAR_TRANS3_WAVE,
	PAR_TRANS4_WAVE,
	PAR_TRANS5_WAVE,
	PAR_TRANS6_WAVE,

	PAR_TRANS1_FREQ,
	PAR_TRANS2_FREQ,
	PAR_TRANS3_FREQ,
	PAR_TRANS4_FREQ,
	PAR_TRANS5_FREQ,
	PAR_TRANS6_FREQ,

	PAR_AUDIO_OUT1,
	PAR_AUDIO_OUT2,
	PAR_AUDIO_OUT3,
	PAR_AUDIO_OUT4,
	PAR_AUDIO_OUT5,
	PAR_AUDIO_OUT6,

	//--AS
	PAR_MIDI_NOTE1,
	PAR_MIDI_NOTE2,
	PAR_MIDI_NOTE3,
	PAR_MIDI_NOTE4,
	PAR_MIDI_NOTE5,
	PAR_MIDI_NOTE6,
	PAR_MIDI_NOTE7, //110 - beware going over 127, can't fit into midi data, will need another msg
	END_OF_SOUND_PARAMETERS,
	//#########################################
	//######## End of sound Parameters ########
	//#########################################
/*
	//all parameters in this section are only there to be referenced from the menu
	//they are not saved anywhere

	PAR_ROLL= END_OF_SOUND_PARAMETERS,
	PAR_MORPH,

	PAR_ACTIVE_STEP,
	PAR_STEP_VOLUME,
	PAR_STEP_PROB,
	PAR_STEP_NOTE,

	PAR_EUKLID_LENGTH,
	PAR_EUKLID_STEPS,

	PAR_AUTOM_TRACK,

	PAR_P1_DEST,
	PAR_P2_DEST,

	PAR_P1_VAL,
	PAR_P2_VAL,

	PAR_SHUFFLE,

	PAR_PATTERN_BEAT,
	PAR_PATTERN_NEXT,
	PAR_TRACK_LENGTH,


	//#########################################
	//######## Global Parameters ##############
	//#########################################
	PAR_BEGINNING_OF_GLOBALS, //a placeholder to mark the beginning of the global var space not present in morph and not needed in the seq
	//global params
	PAR_BPM = PAR_BEGINNING_OF_GLOBALS,

	PAR_MIDI_CHAN_1,
	PAR_MIDI_CHAN_2,
	PAR_MIDI_CHAN_3,
	PAR_MIDI_CHAN_4,
	PAR_MIDI_CHAN_5,
	PAR_MIDI_CHAN_6,

	PAR_FETCH,
	PAR_FOLLOW,

	PAR_QUANTISATION,

	NUM_PARAMS
*/
};

#include "stm32f4xx.h"

typedef union
{
	float 	 flt;
	uint32_t itg;
} ptrValue;

typedef struct ParameterStruct
{
	void* 	ptr;
	uint8_t type;

} Parameter;

extern Parameter parameterArray[END_OF_SOUND_PARAMETERS];
void paramArray_setParameter(uint16_t idx, ptrValue newValue);
void parameterArray_init();

#endif /* PARAMETERARRAY_H_ */
