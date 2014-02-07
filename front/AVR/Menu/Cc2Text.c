/*
 * Cc2Text.c
 *
 * Created: 16.02.2013 14:34:59
 *  Author: Julian
 */ 
#include "CcNr2Text.h"
#include "menu.h"
#include <avr/pgmspace.h>

//-----------------------------------------------------------------
//**AUTOM **LFO **VELO modTargets definition
// elements are nameIdx and param
// see declaration in menu.h for more info about this
// this is arranged in voice order and lists valid mod targets.
// The index into this array is only important in that there is another
// array (below) that stores the beginnings and endings index for each voice. So
// be sure to update it when items in this array are added or removed
const ModTarg modTargets[] PROGMEM ={
// nothing
{TEXT_EMPTY,PAR_NONE},
// not applicable to any one voice (will only work for autom target)
{TEXT_SAMPLE_RATE,PAR_VOICE_DECIMATION_ALL},        //1
//////////////////////////////////////////////////
//Voice 1 - Drum 1
//  1 Oscillator
{TEXT_COARSE, PAR_COARSE1},                         //2
{TEXT_FINE, PAR_FINE1},
{TEXT_WAVEFORM, PAR_OSC_WAVE_DRUM1},
//  2 Amp EG
{TEXT_ATTACK, PAR_VELOA1},
{TEXT_DECAY, PAR_VELOD1},
{TEXT_EG_SLOPE, PAR_VOL_SLOPE1},
//  3 Modulation
{TEXT_PITCH_DECAY,PAR_MOD_EG1},
{TEXT_EG_SLOPE,PAR_PITCH_SLOPE1},
{TEXT_MOD_AMOUNT,PAR_MODAMNT1},						//10
{TEXT_VEL_DEST,PAR_VEL_DEST_1},
{TEXT_VEL_AMT,PAR_VELO_MOD_AMT_1},
{TEXT_VEL_MOD_VOL,PAR_VOLUME_MOD_ON_OFF1},
//  4 FM
{TEXT_FM_AMOUNT,PAR_FMAMNT1},
{TEXT_FM_FREQ,PAR_FM_FREQ1},
{TEXT_WAVEFORM,PAR_MOD_WAVE_DRUM1},
{TEXT_MODE,PAR_MIX_MOD_1},
//  5 transient sample
{TEXT_TRANSIENT_WAVE,PAR_TRANS1_WAVE},
{TEXT_TRANSIENT_VOLUME,PAR_TRANS1_VOL},
{TEXT_TRANSIENT_FREQ,PAR_TRANS1_FREQ},				//20
//  6 filter
{TEXT_FILTER_F,PAR_FILTER_FREQ_1},
{TEXT_FILTER_RESO,PAR_RESO_1},
{TEXT_FILTER_TYPE,PAR_FILTER_TYPE_1},
{TEXT_FILTER_DRIVE,PAR_FILTER_DRIVE_1},
//  7 LFO
{TEXT_FREQ_LFO,PAR_FREQ_LFO1},
{TEXT_SYNC_LFO,PAR_SYNC_LFO1},
{TEXT_MOD_LFO,PAR_AMOUNT_LFO1},
{TEXT_WAVE_LFO,PAR_WAVE_LFO1},
{TEXT_RETRIGGER_LFO,PAR_RETRIGGER_LFO1},
{TEXT_OFFSET_LFO,PAR_OFFSET_LFO1},					//30
{TEXT_TARGET_VOICE_LFO,PAR_VOICE_LFO1},
{TEXT_TARGET_LFO,PAR_TARGET_LFO1},
//  8 MIX
{TEXT_VOLUME,PAR_VOL1},
{TEXT_PAN,PAR_PAN1},
{TEXT_SAMPLE_RATE,PAR_VOICE_DECIMATION1},
{TEXT_DRIVE,PAR_DRIVE1},
{TEXT_NOTE,PAR_MIDI_NOTE1},

//////////////////////////////////////////////////
//Voice 2 - drum 2
//  1 Oscillator
{TEXT_COARSE,PAR_COARSE2},
{TEXT_FINE,PAR_FINE2},
{TEXT_WAVEFORM,PAR_OSC_WAVE_DRUM2},					//40
//  2 Amp EG
{TEXT_ATTACK,PAR_VELOA2},
{TEXT_DECAY,PAR_VELOD2},
{TEXT_EG_SLOPE,PAR_VOL_SLOPE2},
//  3 Modulation
{TEXT_PITCH_DECAY,PAR_MOD_EG2},
{TEXT_EG_SLOPE,PAR_PITCH_SLOPE2},
{TEXT_MOD_AMOUNT,PAR_MODAMNT2},
{TEXT_VEL_DEST,PAR_VEL_DEST_2},
{TEXT_VEL_AMT,PAR_VELO_MOD_AMT_2},
{TEXT_VEL_MOD_VOL,PAR_VOLUME_MOD_ON_OFF2},
//  4 FM
{TEXT_FM_AMOUNT,PAR_FMAMNT2},						//50
{TEXT_FM_FREQ,PAR_FM_FREQ2},
{TEXT_WAVEFORM,PAR_MOD_WAVE_DRUM2},
{TEXT_MODE,PAR_MIX_MOD_2},
//  5 transient sample
{TEXT_TRANSIENT_WAVE,PAR_TRANS2_WAVE},
{TEXT_TRANSIENT_VOLUME,PAR_TRANS2_VOL},
{TEXT_TRANSIENT_FREQ,PAR_TRANS2_FREQ},
//  6 filter
{TEXT_FILTER_F,PAR_FILTER_FREQ_2},
{TEXT_FILTER_RESO,PAR_RESO_2},
{TEXT_FILTER_TYPE,PAR_FILTER_TYPE_2},
{TEXT_FILTER_DRIVE,PAR_FILTER_DRIVE_2},				//60
//  7 LFO
{TEXT_FREQ_LFO,PAR_FREQ_LFO2},
{TEXT_SYNC_LFO,PAR_SYNC_LFO2},
{TEXT_MOD_LFO,PAR_AMOUNT_LFO2},
{TEXT_WAVE_LFO,PAR_WAVE_LFO2},
{TEXT_RETRIGGER_LFO,PAR_RETRIGGER_LFO2},
{TEXT_OFFSET_LFO,PAR_OFFSET_LFO2},
{TEXT_TARGET_VOICE_LFO,PAR_VOICE_LFO2},
{TEXT_TARGET_LFO,PAR_TARGET_LFO2},
//  8 MIX
{TEXT_VOLUME,PAR_VOL2},
{TEXT_PAN,PAR_PAN2},								//70
{TEXT_SAMPLE_RATE,PAR_VOICE_DECIMATION2},
{TEXT_DRIVE,PAR_DRIVE2},
{TEXT_NOTE,PAR_MIDI_NOTE2},

//////////////////////////////////////////////////
//Voice 3 - drum 3
//  1 Oscillator
{TEXT_COARSE,PAR_COARSE3},
{TEXT_FINE,PAR_FINE3},
{TEXT_WAVEFORM,PAR_OSC_WAVE_DRUM3},
//  2 Amp EG
{TEXT_ATTACK,PAR_VELOA3},
{TEXT_DECAY,PAR_VELOD3},
{TEXT_EG_SLOPE,PAR_VOL_SLOPE3},
//  3 Modulation
{TEXT_PITCH_DECAY,PAR_MOD_EG3},						//80
{TEXT_EG_SLOPE,PAR_PITCH_SLOPE3},
{TEXT_MOD_AMOUNT,PAR_MODAMNT3},
{TEXT_VEL_DEST,PAR_VEL_DEST_3},
{TEXT_VEL_AMT,PAR_VELO_MOD_AMT_3},
{TEXT_VEL_MOD_VOL,PAR_VOLUME_MOD_ON_OFF3},
//  4 FM
{TEXT_FM_AMOUNT,PAR_FMAMNT3},
{TEXT_FM_FREQ,PAR_FM_FREQ3},
{TEXT_WAVEFORM,PAR_MOD_WAVE_DRUM3},
{TEXT_MODE,PAR_MIX_MOD_3},
//  5 transient sample
{TEXT_TRANSIENT_WAVE,PAR_TRANS3_WAVE},				//90
{TEXT_TRANSIENT_VOLUME,PAR_TRANS3_VOL},
{TEXT_TRANSIENT_FREQ,PAR_TRANS3_FREQ},
//  6 filter
{TEXT_FILTER_F,PAR_FILTER_FREQ_3},
{TEXT_FILTER_RESO,PAR_RESO_3},
{TEXT_FILTER_TYPE,PAR_FILTER_TYPE_3},
{TEXT_FILTER_DRIVE,PAR_FILTER_DRIVE_3},
//  7 LFO
{TEXT_FREQ_LFO,PAR_FREQ_LFO3},
{TEXT_SYNC_LFO,PAR_SYNC_LFO3},
{TEXT_MOD_LFO,PAR_AMOUNT_LFO3},
{TEXT_WAVE_LFO,PAR_WAVE_LFO3},						//100
{TEXT_RETRIGGER_LFO,PAR_RETRIGGER_LFO3},
{TEXT_OFFSET_LFO,PAR_OFFSET_LFO3},
{TEXT_TARGET_VOICE_LFO,PAR_VOICE_LFO3},
{TEXT_TARGET_LFO,PAR_TARGET_LFO3},
//  8 MIX
{TEXT_VOLUME,PAR_VOL3},
{TEXT_PAN,PAR_PAN3},
{TEXT_SAMPLE_RATE,PAR_VOICE_DECIMATION3},
{TEXT_DRIVE,PAR_DRIVE3},
{TEXT_NOTE,PAR_MIDI_NOTE3},

//////////////////////////////////////////////////
//Voice 4 - snare/clap
//  1 Oscillator
{TEXT_COARSE,PAR_COARSE4},							//110
{TEXT_FINE,PAR_FINE4},
{TEXT_NOISE,PAR_NOISE_FREQ1},
{TEXT_MIX,PAR_MIX1},
{TEXT_WAVEFORM,PAR_OSC_WAVE_SNARE},
//  2 Amp EG
{TEXT_ATTACK,PAR_VELOA4},
{TEXT_DECAY,PAR_VELOD4},
{TEXT_REPEAT,PAR_REPEAT4},
{TEXT_EG_SLOPE,PAR_VOL_SLOPE4},
//  3 Modulation
{TEXT_PITCH_DECAY,PAR_MOD_EG4},
{TEXT_EG_SLOPE,PAR_PITCH_SLOPE4},					//120
{TEXT_MOD_AMOUNT,PAR_MODAMNT4},
{TEXT_VEL_DEST,PAR_VEL_DEST_4},
{TEXT_VEL_AMT,PAR_VELO_MOD_AMT_4},
{TEXT_VEL_MOD_VOL,PAR_VOLUME_MOD_ON_OFF4},
//  4 FM (no entries)
//  5 transient sample
{TEXT_TRANSIENT_WAVE,PAR_TRANS4_WAVE},
{TEXT_TRANSIENT_VOLUME,PAR_TRANS4_VOL},
{TEXT_TRANSIENT_FREQ,PAR_TRANS4_FREQ},
//  6 filter
{TEXT_FILTER_F,PAR_FILTER_FREQ_4},
{TEXT_FILTER_RESO,PAR_RESO_4},
{TEXT_FILTER_TYPE,PAR_FILTER_TYPE_4},				//130
{TEXT_FILTER_DRIVE,PAR_FILTER_DRIVE_4},
//  7 LFO
{TEXT_FREQ_LFO,PAR_FREQ_LFO4},
{TEXT_SYNC_LFO,PAR_SYNC_LFO4},
{TEXT_MOD_LFO,PAR_AMOUNT_LFO4},
{TEXT_WAVE_LFO,PAR_WAVE_LFO4},
{TEXT_RETRIGGER_LFO,PAR_RETRIGGER_LFO4},
{TEXT_OFFSET_LFO,PAR_OFFSET_LFO4},
{TEXT_TARGET_VOICE_LFO,PAR_VOICE_LFO4},
{TEXT_TARGET_LFO,PAR_TARGET_LFO4},
//  8 MIX
{TEXT_VOLUME,PAR_VOL4},								//140
{TEXT_PAN,PAR_PAN4},
{TEXT_SAMPLE_RATE,PAR_VOICE_DECIMATION4},
{TEXT_DRIVE,PAR_SNARE_DISTORTION},
{TEXT_NOTE,PAR_MIDI_NOTE4},

//////////////////////////////////////////////////
//Voice 5 - cymbal
//  1 Oscillator
{TEXT_COARSE,PAR_COARSE5},
{TEXT_FINE,PAR_FINE5},
{TEXT_WAVEFORM,PAR_WAVE1_CYM},
//  2 Amp EG
{TEXT_ATTACK,PAR_VELOA5},
{TEXT_DECAY,PAR_VELOD5},
{TEXT_REPEAT,PAR_REPEAT5},							//150
{TEXT_EG_SLOPE,PAR_VOL_SLOPE5},
//  3 Modulation
{TEXT_VEL_DEST,PAR_VEL_DEST_5},
{TEXT_VEL_AMT,PAR_VELO_MOD_AMT_5},
{TEXT_VEL_MOD_VOL,PAR_VOLUME_MOD_ON_OFF5},
//  4 FM
{TEXT_MOD_OSC1_FREQ,PAR_MOD_OSC_F1_CYM},
{TEXT_MOD_OSC2_FREQ,PAR_MOD_OSC_F2_CYM},
{TEXT_MOD_OSC1_GAIN,PAR_MOD_OSC_GAIN1_CYM},
{TEXT_MOD_OSC2_GAIN,PAR_MOD_OSC_GAIN2_CYM},
{TEXT_WAVEFORM,PAR_WAVE2_CYM},
{TEXT_WAVEFORM,PAR_WAVE3_CYM},						//160
//  5 transient sample
{TEXT_TRANSIENT_WAVE,PAR_TRANS5_WAVE},
{TEXT_TRANSIENT_VOLUME,PAR_TRANS5_VOL},
{TEXT_TRANSIENT_FREQ,PAR_TRANS5_FREQ},
//  6 filter
{TEXT_FILTER_F,PAR_FILTER_FREQ_5},
{TEXT_FILTER_RESO,PAR_RESO_5},
{TEXT_FILTER_TYPE,PAR_FILTER_TYPE_5},
{TEXT_FILTER_DRIVE,PAR_FILTER_DRIVE_5},
//  7 LFO
{TEXT_FREQ_LFO,PAR_FREQ_LFO5},
{TEXT_SYNC_LFO,PAR_SYNC_LFO5},
{TEXT_MOD_LFO,PAR_AMOUNT_LFO5},						//170
{TEXT_WAVE_LFO,PAR_WAVE_LFO5},
{TEXT_RETRIGGER_LFO,PAR_RETRIGGER_LFO5},
{TEXT_OFFSET_LFO,PAR_OFFSET_LFO5},
{TEXT_TARGET_VOICE_LFO,PAR_VOICE_LFO5},
{TEXT_TARGET_LFO,PAR_TARGET_LFO5},
//  8 MIX
{TEXT_VOLUME,PAR_VOL5},
{TEXT_PAN,PAR_PAN5},
{TEXT_SAMPLE_RATE,PAR_VOICE_DECIMATION5},
{TEXT_DRIVE,PAR_CYMBAL_DISTORTION},
{TEXT_NOTE,PAR_MIDI_NOTE5},							//180

//////////////////////////////////////////////////
//Voice 6 - HH open and closed
//  1 Oscillator
{TEXT_COARSE,PAR_COARSE6},
{TEXT_FINE,PAR_FINE6},
{TEXT_WAVEFORM,PAR_WAVE1_HH},
//  2 Amp EG
{TEXT_ATTACK,PAR_VELOA6},
{TEXT_DECAY_CLOSED,PAR_VELOD6_CLOSED},
{TEXT_DECAY_OPEN,PAR_VELOD6_OPEN},
{TEXT_EG_SLOPE, PAR_VOL_SLOPE6},
//  3 Modulation
{TEXT_VEL_DEST,PAR_VEL_DEST_6},
{TEXT_VEL_AMT,PAR_VELO_MOD_AMT_6},
{TEXT_VEL_MOD_VOL,PAR_VOLUME_MOD_ON_OFF6},			//190
//  4 FM
{TEXT_MOD_OSC1_FREQ,PAR_MOD_OSC_F1},
{TEXT_MOD_OSC2_FREQ,PAR_MOD_OSC_F2},
{TEXT_MOD_OSC1_GAIN,PAR_MOD_OSC_GAIN1},
{TEXT_MOD_OSC2_GAIN,PAR_MOD_OSC_GAIN2},
{TEXT_WAVEFORM,PAR_WAVE2_HH},
{TEXT_WAVEFORM,PAR_WAVE3_HH},
//  5 transient sample
{TEXT_TRANSIENT_WAVE,PAR_TRANS6_WAVE},
{TEXT_TRANSIENT_VOLUME,PAR_TRANS6_VOL},
{TEXT_TRANSIENT_FREQ,PAR_TRANS6_FREQ},
//  6 filter
{TEXT_FILTER_F,PAR_FILTER_FREQ_6},					//200
{TEXT_FILTER_RESO,PAR_RESO_6},
{TEXT_FILTER_TYPE,PAR_FILTER_TYPE_6},
{TEXT_FILTER_DRIVE,PAR_FILTER_DRIVE_6},
//  7 LFO
{TEXT_FREQ_LFO,PAR_FREQ_LFO6},
{TEXT_SYNC_LFO,PAR_SYNC_LFO6},
{TEXT_MOD_LFO,PAR_AMOUNT_LFO6},
{TEXT_WAVE_LFO,PAR_WAVE_LFO6},
{TEXT_RETRIGGER_LFO,PAR_RETRIGGER_LFO6},
{TEXT_OFFSET_LFO,PAR_OFFSET_LFO6},
{TEXT_TARGET_VOICE_LFO,PAR_VOICE_LFO6},				//210
{TEXT_TARGET_LFO,PAR_TARGET_LFO6},
//  8 MIX
{TEXT_VOLUME,PAR_VOL6},
{TEXT_PAN,PAR_PAN6},
{TEXT_SAMPLE_RATE,PAR_VOICE_DECIMATION6},
{TEXT_DRIVE,PAR_HAT_DISTORTION},
{TEXT_NOTE,PAR_MIDI_NOTE6},							//216

};

//**AUTOM modTargetVoiceOffsets definition
// this is the starting and ending offset for voice. index into above array.
// see declaration in menu.h for more info
const ModTargetVoiceOffset modTargetVoiceOffsets[6] PROGMEM = {
	/*voice 1*/ {   2,  37 }, //35
	/*voice 2*/ {  38,  73 }, //35
	/*voice 3*/ {  74, 109 }, //35
	/*voice 4*/ { 110, 144 }, //34
	/*voice 5*/ { 145, 180 }, //35
	/*voice 6*/ { 181, 216 }, //35
};

// These values are generated in the excel spreadsheet mod_targ_lineup.xls
// Will need to be regenerated if modTargets array changes above
const uint8_t modTargetGapMap[] PROGMEM = {
		0x3F,		0x3F,		0x4,		0x4,		0x3F,		0x3F,		0x3F,		0x1,		0x6,		0x3F,
		0x3C,		0x3C,		0x3C,		0x3F,		0x3F,		0x3F,		0x3,		0x3,		0x3,		0x3,
		0x38,		0x38,		0x3B,		0x3,		0x38,		0x3F,		0x3F,		0x3F,		0x3F,		0x3F,
		0x3F,		0x3F,		0x3F,		0x3F,		0x3F,		0x3F,		0x3F,		0x3F,		0x3F,		0x3F,
		0x3F,		0x3F,		0x3F,		0x3F,		0x3F,
};

#define NUM_TOT_TARGETS sizeof(modTargetGapMap)

//**AUTOM paramToModTarget definition
// see menu.h for more info
uint8_t paramToModTarget[END_OF_SOUND_PARAMETERS] ={};

// given a voice (0 to 5) and an index into modTargets (offset by modTargetVoiceOffsets)
// will return an index into modTargetGapMap
uint8_t getModTargetGapIndex(uint8_t voice, uint8_t modTargetIdx)
{
	// there are 6 voices. Each bit in the value of the modTargetGapMap element represents whether there is a gap at that
	// location (0) or a value (1). See the excel spreadsheet.
	uint8_t i, val, ctr=0;
	voice=5-voice; // invert
	for(i=0;i<NUM_TOT_TARGETS;i++) {
		val=pgm_read_byte(modTargetGapMap+i);
		if((val >> voice) & 0x01) {
			ctr++;
			if(ctr==modTargetIdx+1) {
				return i;
			}
		}
	}

	return 0xff; // problems
}

//**AUTOM getNumModTargets() definition
uint8_t getNumModTargets()
{
	return sizeof(modTargets)/sizeof(ModTarg);
}


//**AUTOM paramToModTargetInit()
void paramToModTargetInit()
{
	// populate this array with indices into modTargets so we can do reverse lookup
	uint8_t i;
	for(i=0;i<(sizeof(modTargets)/sizeof(ModTarg));i++){
		paramToModTarget[pgm_read_word(&modTargets[i].param)]=i;
	}
} 


// val is index into modTargets
// returns 1 based index or 0 for invalid
uint8_t voiceFromModTargValue(uint8_t val)
{
	if(pgm_read_byte(&modTargetVoiceOffsets[3].start) <= val) {
		// lower half
		if(pgm_read_byte(&modTargetVoiceOffsets[4].start) <= val) {
			if(pgm_read_byte(&modTargetVoiceOffsets[5].start) <= val) {
				if(pgm_read_byte(&modTargetVoiceOffsets[5].end) >= val) {
					return 6;
				} else {
					return 0; // invalid
				}
			} else {
				return 5;
			}
		} else {
			return 4;
		}
	} else {
		// upper half
		if(pgm_read_byte(&modTargetVoiceOffsets[0].start) <= val) {
			if(pgm_read_byte(&modTargetVoiceOffsets[1].start) <= val) {
				if(pgm_read_byte(&modTargetVoiceOffsets[2].start) <= val) {
					return 3;
				} else {
					return 2;
				}
			} else {
				return 1;
			}
		} else {
			return 0; // invalid
		}
	}
}

