#include "menu.h"
#include "../Hardware/lcd.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "../ledHandler.h"
#include <avr/pgmspace.h>
#include "../IO/uart.h"


#include "../Preset/PresetManager.h"
#include "../avrComms/avrCommsSendingProtocol.h"
//#include <util/delay.h>
#include <util/delay.h>

#include "menuPages.h"
#include "MenuText.h"
#include "../Hardware/timebase.h"
#include "../Hardware/SD/SPI_routines.h"
#include "screensaver.h"
#include "CcNr2Text.h"
#include "copyClearTools.h"
#include "../buttonHandler.h"
#include "../encoder.h"
#include <ctype.h>
#include "../front.h"

#define MACRO_VMORPH_DRUM1 36
#define MACRO_VMORPH_DRUM2 71
#define MACRO_VMORPH_DRUM3 106
#define MACRO_VMORPH_SNARE 140
#define MACRO_VMORPH_CYM   175
#define MACRO_VMORPH_HIHAT 210

#define MENU_SAMPLE_FLASH_ANIM_TICKS ((uint16_t)12u)
/* Timer2 ticks at roughly 13.1 ms. 153 ticks is about two seconds: long enough
   that normal progress packets keep the regular sample/loop screen alive, short
   enough that the silent final flash/count commit no longer looks stuck. */
#define MENU_SAMPLE_FLASH_IDLE_TICKS ((uint16_t)153u)

// uppercase 3 letters in buf
static void upr_three(char *buf);
// given a menuid and a param value fill buf with the short menu item value. will not exceed 3 chars
static void getMenuItemNameForValue(const uint8_t menuId, const uint8_t curParmVal, char *buf);
static void menu_waitSampleUploadResult(uint8_t *statusFlags);
static void menu_drawSampleUploadFlashDots(uint8_t phase);
static void menu_showSampleUploadFlashProgress(uint8_t phase);
static void menu_tickSampleUploadFlashProgress(void);
static void menu_maybeShowSampleUploadFlashIdle(void);
// given a menu id returns the number of entries
static uint8_t getMaxEntriesForMenu(uint8_t menuId);
// given a number, convert it to a note name
static void setNoteName(uint8_t num, char *buf);
/* AVR-owned final-flash display state. Normal sample/loop progress resets it;
   the wait loop starts it locally after a short idle period while still waiting
   for SAMPLE_UPLOAD_RESULT. This is display-only: STM remains responsible for
   SD reads, internal flash writes, and the final result packet. */
static uint8_t menu_sampleFlashProgressScreenActive = 0u;
static uint8_t menu_sampleFlashProgressAnimate = 0u;
static uint8_t menu_sampleFlashProgressPhase = 0u;
static uint16_t menu_sampleFlashProgressLastTick = 0u;
static uint8_t menu_sampleUploadProgressSeen = 0u;
static uint16_t menu_sampleUploadLastProgressTick = 0u;

static void menu_moveToMenuItem(int8_t inc);
static void menu_encoderChangeParameter(int8_t inc);
static void menu_encoderChangeShiftParameter(int8_t inc);
static int8_t menu_applyEncoderAcceleration(int8_t inc);
static void menu_applyEncoderDeltaToByte(uint8_t *paramValue, int8_t inc);
static uint8_t menu_getDtypeMenuEntryCount(uint16_t paramNr);
static void menu_normalizeDtypeMenuEncoderValue(uint16_t paramNr, uint8_t *paramValue);

#define ARROW_SIGN '>'

enum saveStateEnum
{
	SAVE_STATE_EDIT_TYPE,			// select one of the types of data to load/save (kit, pattern etc)
	SAVE_STATE_EDIT_PRESET_NR,		// editing the preset number slot
	SAVE_STATE_EDIT_NAME1,
	SAVE_STATE_EDIT_NAME2,
	SAVE_STATE_EDIT_NAME3,
	SAVE_STATE_EDIT_NAME4,
	SAVE_STATE_EDIT_NAME5,
	SAVE_STATE_EDIT_NAME6,
	SAVE_STATE_EDIT_NAME7,
	SAVE_STATE_EDIT_NAME8,
	SAVE_STATE_OK					// confirmation is active
};

/** a struct for some save page parameters.*/
struct {
	unsigned what:9;		/**< 0= save kit, 1 = save pattern, 2 = morph sound, etc */
	unsigned state:4;		/**< 0=edit kit/pat, 1=edit preset nr, 2-9 = edit name, 10=ok*/

} menu_saveOptions;
//static volatile char menu_currentPresetName[9]; //8 chars + '\0'

//Macro to check the param lock bit in the parameterFetch var
#define PARAMETER_LOCK_ACTIVE (1<<4)


//-----------------------------------------------------------------
/** array holding all the names for the parameters
 *  short name, category, long name
 * These correspond with NamesEnum in menu.h
 * */
const Name valueNames[NUM_NAMES] PROGMEM =
{
		{SHORT_EMPTY,CAT_EMPTY,LONG_EMPTY},					//empty						//0
		{SHORT_COARSE,CAT_OSC,LONG_COARSE},					//coarse tune
		{SHORT_FINE,CAT_OSC,LONG_FINE},						//fine tune
		{SHORT_ATTACK,CAT_VELO_EG,LONG_ATTACK},				//attack velocity
		{SHORT_DECAY,CAT_VELO_EG,LONG_DECAY},				//decay velocity
		{SHORT_DECAY,CAT_PITCH_EG,LONG_DECAY},				//pitch decay
		{SHORT_MOD,CAT_PITCH_MOD,LONG_AMOUNT},				//mod amount
		{SHORT_FM_AMNT,CAT_FM,LONG_AMOUNT},					//fm amount
		{SHORT_FREQ,CAT_FM,LONG_FREQ},						//fm frequency
		{SHORT_DRIVE, CAT_VOICE,LONG_DRIVE},				//drive
		{SHORT_VOL,CAT_VOICE,LONG_VOL},						//volume					//10
		{SHORT_PAN,CAT_VOICE,LONG_PAN},						//pan
		{SHORT_NOISE,CAT_NOISE,LONG_FREQ},					//noise frequency
		{SHORT_MIX,CAT_NOISE_OSC,LONG_MIX},					//mix
		{SHORT_REPEAT,CAT_VELO_EG,LONG_REPEAT_CNT},			//repeat
		{SHORT_FREQ,CAT_FILTER,LONG_FREQ},					//filter freq
		{SHORT_FIL_RESO,CAT_FILTER,LONG_RESONANCE},			//filter reso
		{SHORT_FIL_TYPE,CAT_FILTER,LONG_TYPE},				//filter type
		{SHORT_MOD_OSC1_FREQ,CAT_MOD_OSC,LONG_FREQ1},		//mod osc 1 freq
		{SHORT_MOD_OSC2_FREQ,CAT_MOD_OSC,LONG_FREQ2},		//mod osc 2 freq
		{SHORT_MOD_OSC1_GAIN,CAT_MOD_OSC,LONG_GAIN1},		//mod osc 1 gain			//20
		{SHORT_MOD_OSC2_GAIN,CAT_MOD_OSC,LONG_GAIN2},		//mod osc 2 gain

		{SHORT_FREQ,CAT_LFO,LONG_FREQ},						//freq lfo
		{SHORT_MOD,CAT_LFO,LONG_AMOUNT},					//mod lfo
		{SHORT_WAVE,CAT_LFO,LONG_WAVE},						//wave lfo
		{SHORT_DEST,CAT_LFO,LONG_DEST_PARAM},				//destination lfo (destination)

		{SHORT_SYNC,CAT_LFO,LONG_CLOCKSYNC},				//TEXT_SYNC_LFO,
		{SHORT_RETRIGGER,CAT_LFO,LONG_RETRIGGER},			//TEXT_RETRIGGER_LFO,
		{SHORT_OFFSET,CAT_LFO,LONG_OFFSET},					//TEXT_OFFSET_LFO,
		{SHORT_VOICE,CAT_LFO,LONG_DEST_VOICE},				//TEXT_TARGET_VOICE_LFO,

		{SHORT_SLOPE,CAT_VELO_EG,LONG_SLOPE},				//TEXT_EG_SLOPE,			//30

		{SHORT_DECAY1,CAT_VELO_EG,LONG_DECAY_CLOSED},		//TEXT_DECAY_CLOSED,
		{SHORT_DECAY2,CAT_VELO_EG,LONG_DECAY_OPEN},			//TEXT_DECAY_OPEN,

		{SHORT_WAVE,CAT_OSC,LONG_WAVE},						//TEXT_WAVEFORM

		{SHORT_WAVE,CAT_TRANS,LONG_WAVE},					//TEXT_TRANSIENT_WAVE
		{SHORT_VOL,CAT_TRANS,LONG_VOL},						//TEXT_TRANSIENT_VOLUME
		{SHORT_FREQ,CAT_TRANS,LONG_FREQ},					//TEXT_TRANSIENT_FREQ


		{SHORT_EQ_GAIN,CAT_EQ,LONG_GAIN},					//TEXT_EQ_GAIN
		{SHORT_EQ_FREQ,CAT_EQ,LONG_FREQ},					//TEXT_EQ_FREQ

#if USE_CODEC_EQ

		{SHORT_EQ,CAT_EQ,LONG_ENABLE},						//TEXT_CODEC_EQ_ENABLE
		{SHORT_MOD_OSC1_FREQ,CAT_EQ,LONG_FREQ},				//TEXT_CODEC_EQ_FB
		{SHORT_MOD_OSC2_FREQ,CAT_EQ,LONG_FREQ},				//TEXT_CODEC_EQ_FT
		{SHORT_MOD_OSC1_GAIN,CAT_EQ,LONG_GAIN},				//TEXT_CODEC_EQ_GB
		{SHORT_MOD_OSC2_GAIN,CAT_EQ,LONG_GAIN},				//TEXT_CODEC_EQ_GT

		{SHORT_LIMITER,CAT_LIMIT,LONG_ENABLE},				//TEXT_CODEC_LIMIT_ENABLE
		{SHORT_ATTACK,CAT_LIMIT,LONG_ATTACK},				//TEXT_CODEC_LIMIT_ATTACK
		{SHORT_RELEASE,CAT_LIMIT,LONG_RELEASE},				//TEXT_CODEC_LIMIT_RELEASE
		{SHORT_MAX,CAT_LIMIT,LONG_THRESH_MAX},				//TEXT_CODEC_LIMIT_MAX
		{SHORT_MIN,CAT_LIMIT,LONG_THRESH_MIN},				//TEXT_CODEC_LIMIT_MIN

#endif

		{SHORT_ROLL,CAT_PATTERN,LONG_ROLLRATE},				//TEXT_ROLL_SPEED
		{SHORT_MORPH,CAT_SOUND,LONG_MORPH},					//TEXT_X_FADE					//40


		{SHORT_VELOCITY,CAT_STEP,LONG_VELOCITY},			//TEXT_STEP_VELOCITY
		{SHORT_NOTE,CAT_STEP,LONG_NOTE},					//TEXT_NOTE
		{SHORT_PROBABILITY,CAT_STEP,LONG_PROBABILITY},		//TEXT_PROBABILITY,
		{SHORT_STEP,CAT_STEP,LONG_NUMBER},					//TEXT_ACTIVE_STEP

		{SHORT_LENGTH,CAT_PATTERN,LONG_LENGTH},				//TEXT_PAT_LENGTH,
      {SHORT_SCALE,CAT_PATTERN,LONG_SCALE},				//TEXT_PAT_SCALE,
		{SHORT_STEP,CAT_EUKLID,LONG_STEPS},					//TEXT_NUM_STEPS,
		{SHORT_ROTATION, CAT_EUKLID, LONG_ROTATION}, 		//TEXT_ROTATION
      {SHORT_SUBSTEP_ROTATION, CAT_EUKLID, LONG_SUBSTEP_ROTATION}, 		//TEXT_ROTATION
      
		{SHORT_BPM,CAT_GLOBAL,LONG_TEMPO},					//TEXT_BPM

		{SHORT_CHANNEL,CAT_VOICE,LONG_MIDI_CHANNEL},		//TEXT_MIDI_CHANNEL
		{SHORT_OUT,CAT_VOICE,LONG_AUDIO_OUT},				//TEXT_AUDIO_OUT				//50

		{SHORT_SR,CAT_VOICE,LONG_SAMPLE_RATE},				//TEXT_SAMPLE_RATE

		{SHORT_REPEAT,CAT_PATTERN,LONG_REPEAT_CNT},				//TEXT_PATTERN_REPEAT
		{SHORT_NXT,CAT_PATTERN,LONG_NEXT_PAT},				//TEXT_PATTERN_NEXT

		//	{SHORT_PHASE,CAT_OSC,LONG_PHASE},				//TEXT_OSC_PHASE
		{SHORT_MODE,CAT_OSC,LONG_MODE},				//TEXT_MODE mix or mod for fm osc
		{SHORT_VOL,CAT_OSC,LONG_VOL},				//TEXT_OSC_VOLUME fm oscillator volume in mix mode
		{SHORT_DRIVE, CAT_FILTER,LONG_DRIVE},			//TEXT_FILTER_DRIVE

		{SHORT_DEST,	CAT_VELOCITY,	LONG_DEST_PARAM},		//TEXT_VEL_DEST,
		{SHORT_FM_AMNT,	CAT_VELOCITY,	LONG_AMOUNT},			//TEXT_VEL_AMT,
		{SHORT_VOL,CAT_VELOCITY,	LONG_VOLUME_MOD},		//TEXT_VEL_MOD_VOL,

		{SHORT_FETCH,	CAT_PARAMETER,	LONG_FETCH},		//TEXT_FETCH					//60
		{SHORT_FOLLOW,	CAT_SEQUENCER,	LONG_FOLLOW},		//TEXT_FOLLOW
		{SHORT_QUANT,	CAT_SEQUENCER,	LONG_QUANTISATION},			//TEXT_QUANTISATION

		{SHORT_TRACK,	CAT_SEQUENCER,	LONG_AUTOMATION_TRACK},			//TEXT_AUTOMATION_TRACK

		{SHORT_DEST,	CAT_SEQUENCER,	LONG_AUTOMATION_DEST},	//TEXT_PARAM_DEST
		{SHORT_VALUE,	CAT_SEQUENCER,	LONG_AUTOMATION_VAL},	//TEXT_PARAM_VAL

		{SHORT_SHUFFLE,	CAT_SEQUENCER,	LONG_SHUFFLE},	//TEXT_SHUFFLE

		{SHORT_SCREEN_SAVER, CAT_GLOBAL,LONG_SCREENSAVER},	//TEXT_SCREENSAVER_ON_OFF

		{SHORT_EMPTY,CAT_EMPTY,LONG_EMPTY},					//SKIP

		{SHORT_X,CAT_GENERATOR,LONG_X},	//TEXT_POS_X,
		{SHORT_Y,CAT_GENERATOR,LONG_Y},	//TEXT_POS_Y,										//70
		{SHORT_FLUX,CAT_GENERATOR,LONG_FLUX},	//TEXT_FLUX,
		{SHORT_FREQ,CAT_GENERATOR,LONG_FREQ},	//TEXT_SOM_FREQ,
		{SHORT_MIDI,CAT_MIDI,LONG_MODE},	//TEXT_MIDI_MODE
		{SHORT_MIDI_ROUTING, CAT_MIDI, LONG_MIDI_ROUTING}, // TEXT_MIDI_ROUTING
		{SHORT_MIDI_FILT_TX, CAT_MIDI, LONG_MIDI_FILT_TX}, // TEXT_MIDI_FILT_TX
		{SHORT_MIDI_FILT_RX, CAT_MIDI, LONG_MIDI_FILT_RX}, // TEXT_MIDI_FILT_RX
		{SHORT_TRIGGER_IN, CAT_TRIGGER, LONG_TRIGGER_IN}, // TEXT_TRIGGER_IN_PPQ
		{SHORT_TRIGGER_OUT1, CAT_TRIGGER, LONG_TRIGGER_OUT1}, // TEXT_TRIGGER_OUT1_PPQ
		{SHORT_TRIGGER_OUT2, CAT_TRIGGER, LONG_TRIGGER_OUT2}, // TEXT_TRIGGER_OUT2_PPQ
		{SHORT_MODE, CAT_TRIGGER, LONG_TRIGGER_GATE_MODE}, //TEXT_TRIGGER_GATE_MODE
		{SHORT_BAR_RESET_MODE, CAT_SEQUENCER, LONG_BAR_RESET_MODE}, // TEXT_BAR_RESET_MODE
		{SHORT_CHANNEL, CAT_MIDI, LONG_MIDI_CHANNEL}, // TEXT_MIDI_CHAN_GLOBAL
      {SHORT_SEQ_PC_TIME, CAT_SEQUENCER, LONG_SEQ_PC_TIME}, // TEXT_SEQ_PC_TIME
      {SHORT_BUT_SHIFT_MODE, CAT_GLOBAL, LONG_BUT_SHIFT_MODE}, // TEXT_BUT_SHIFT_MODE
      {SHORT_LOAD_PERF_ON_BANK, CAT_GLOBAL, LONG_LOAD_PERF_ON_BANK}, // TEXT_LOAD_PERF_ON_BANK
      {SHORT_UNUSED5, CAT_UNUSED5, LONG_UNUSED5}, // previously SKIP_FIRST_ROLL      
      {SHORT_MORPH_VOICE, CAT_MORPH_VOICE, LONG_MORPH_VOICE}, // text for voice morph automation
      
      {SHORT_MORPH_DRUM1, CAT_MORPH_VOICE, LONG_MORPH_DRUM1}, 
      {SHORT_MORPH_DRUM2, CAT_MORPH_VOICE, LONG_MORPH_DRUM2}, 
      {SHORT_MORPH_DRUM3, CAT_MORPH_VOICE, LONG_MORPH_DRUM3}, 
      {SHORT_MORPH_SNARE, CAT_MORPH_VOICE, LONG_MORPH_SNARE}, 
      {SHORT_MORPH_CYM, CAT_MORPH_VOICE, LONG_MORPH_CYM}, 
      {SHORT_MORPH_HIHAT, CAT_MORPH_VOICE, LONG_MORPH_HIHAT}, 
            
      {SHORT_UNUSED1, CAT_UNUSED1, LONG_UNUSED1}, // text for macro 2 destination 1 assign
      {SHORT_UNUSED2, CAT_UNUSED2, LONG_UNUSED2}, // text for macro 2 destination 1 mod amount
      {SHORT_UNUSED3, CAT_UNUSED3, LONG_UNUSED3}, // text for macro 2 destination 2 assign
      {SHORT_UNUSED4, CAT_UNUSED4, LONG_UNUSED4},  // text for macro 2 destination 2 mod amount
      
      {SHORT_ROLL_NOTE, CAT_PATTERN, LONG_ROLL_NOTE},  
      {SHORT_ROLL_VELOCITY, CAT_PATTERN, LONG_ROLL_VELOCITY},  
      {SHORT_ROLL_MODE, CAT_SEQUENCER, LONG_ROLL_MODE}, 
      /* Global MIDI roll-note offset label. Uses the shared "rol" short text
         but places this saved note-routing setting in the MIDI category. */
      {SHORT_ROLL, CAT_MIDI, LONG_ROLL_NOTE_OFFSET},
      
      {SHORT_TRANSPOSE, CAT_TRANSPOSE, LONG_TRANSPOSE},
      {SHORT_TRANSPOSE_ON_OFF, CAT_TRANSPOSE, LONG_TRANSPOSE_ON_OFF},
      
      {SHORT_FILE_LOAD_BACKGROUND, CAT_FILE, LONG_FILE_LOAD_BACKGROUND},
      
      {SHORT_ENVELOPE_POSITION, CAT_VELO_EG, LONG_ENVELOPE_POSITION},
	  {SHORT_OSC_WAVE_INTERPOLATION, CAT_OSC, LONG_OSC_WAVE_INTERPOLATION},
};

//---------------------------------------------------------------
// Parameter types. These correspond with ParamEnums and entries in parameter_values
const enum Datatypes PROGMEM parameter_dtypes[NUM_PARAMS] = {
	    /*PAR_NONE*/ 			DTYPE_0B127,							// 0
	    /*PAR_OSC_WAVE_DRUM1*/ 	DTYPE_MENU | (MENU_WAVEFORM<<4),
	    /*PAR_OSC_WAVE_DRUM2*/ 	DTYPE_MENU | (MENU_WAVEFORM<<4),
	    /*PAR_OSC_WAVE_DRUM3*/ 	DTYPE_MENU | (MENU_WAVEFORM<<4),
	    /*PAR_OSC_WAVE_SNARE*/  DTYPE_MENU | (MENU_WAVEFORM<<4),
	    /*NRPN_DATA_ENTRY_COARSE*/ DTYPE_0B127,
	    /*PAR_WAVE1_CYM*/  		DTYPE_MENU | (MENU_WAVEFORM<<4),
	    /*PAR_WAVE1_HH*/  		DTYPE_MENU | (MENU_WAVEFORM<<4),
	    /*PAR_COARSE1*/ 		DTYPE_0B127,
	    /*PAR_FINE1*/ 			DTYPE_PM63,
	    /*PAR_COARSE2*/			DTYPE_0B127,							// 10
	    /*PAR_FINE2*/ 			DTYPE_PM63,
	    /*PAR_COARSE3*/ 		DTYPE_0B127,
	    /*PAR_FINE3*/ 			DTYPE_PM63,
	    /*PAR_COARSE4*/ 		DTYPE_0B127,
	    /*PAR_FINE4*/ 			DTYPE_PM63,
	    /*PAR_COARSE5*/			DTYPE_0B127,
	    /*PAR_FINE5*/ 			DTYPE_PM63,
	    /*PAR_COARSE6*/ 		DTYPE_0B127,
	    /*PAR_FINE6*/ 			DTYPE_PM63,
	    /*PAR_MOD_WAVE_DRUM1*/  DTYPE_MENU | (MENU_WAVEFORM<<4),		//20
	    /*PAR_MOD_WAVE_DRUM2*/  DTYPE_MENU | (MENU_WAVEFORM<<4),
	    /*PAR_MOD_WAVE_DRUM3*/  DTYPE_MENU | (MENU_WAVEFORM<<4),
	    /*PAR_WAVE2_CYM*/ 		DTYPE_MENU | (MENU_WAVEFORM<<4),
	    /*PAR_WAVE3_CYM*/ 		DTYPE_MENU | (MENU_WAVEFORM<<4),
	    /*PAR_WAVE2_HH*/ 		DTYPE_MENU | (MENU_WAVEFORM<<4),
	    /*PAR_WAVE3_HH*/ 		DTYPE_MENU | (MENU_WAVEFORM<<4),
	    /*PAR_NOISE_FREQ1*/ 	DTYPE_0B127,
	    /*PAR_MIX1*/ 			DTYPE_0B127,
	    /*PAR_MOD_OSC_F1_CYM*/ 	DTYPE_0B127,
	    /*PAR_MOD_OSC_F2_CYM*/ 	DTYPE_0B127,							//30
	    /*PAR_MOD_OSC_GAIN1_CYM*/ DTYPE_0B127,
	    /*PAR_MOD_OSC_GAIN2_CYM*/ DTYPE_0B127,
	    /*PAR_MOD_OSC_F1*/ 		DTYPE_0B127,
	    /*PAR_MOD_OSC_F2*/ 		DTYPE_0B127,
	    /*PAR_MOD_OSC_GAIN1*/ 	DTYPE_0B127,
	    /*PAR_MOD_OSC_GAIN2*/ 	DTYPE_0B127,
	    /*PAR_FILTER_FREQ_1*/ 	DTYPE_0B127,
	    /*PAR_FILTER_FREQ_2*/ 	DTYPE_0B127,
	    /*PAR_FILTER_FREQ_3*/ 	DTYPE_0B127,
	    /*PAR_FILTER_FREQ_4*/ 	DTYPE_0B127,							//40
	    /*PAR_FILTER_FREQ_5*/ 	DTYPE_0B127,
	    /*PAR_FILTER_FREQ_6*/ 	DTYPE_0B127,
	    /*PAR_RESO_1*/ 			DTYPE_0B127,
	    /*PAR_RESO_2*/ 			DTYPE_0B127,
	    /*PAR_RESO_3*/ 			DTYPE_0B127,
	    /*PAR_RESO_4*/ 			DTYPE_0B127,
	    /*PAR_RESO_5*/ 			DTYPE_0B127,
	    /*PAR_RESO_6*/ 			DTYPE_0B127,
	    /*PAR_VELOA1*/ 			DTYPE_0B127,
	    /*PAR_VELOD1*/ 			DTYPE_0B127,							//50
	    /*PAR_VELOA2*/ 			DTYPE_0B127,
	    /*PAR_VELOD2*/ 			DTYPE_0B127,
	    /*PAR_VELOA3*/ 			DTYPE_0B127,
	    /*PAR_VELOD3*/ 			DTYPE_0B127,
	    /*PAR_VELOA4*/ 			DTYPE_0B127,
	    /*PAR_VELOD4*/ 			DTYPE_0B127,
	    /*PAR_VELOA5*/ 			DTYPE_0B127,
	    /*PAR_VELOD5*/ 			DTYPE_0B127,
	    /*PAR_VELOA6*/ 			DTYPE_0B127,
	    /*PAR_VELOD6_CLOSED*/ 	DTYPE_0B127,							//60
	    /*PAR_VELOD6_OPEN*/ 	DTYPE_0B127,
	    /*PAR_VOL_SLOPE1*/ 		DTYPE_0B127,
	    /*PAR_VOL_SLOPE2*/ 		DTYPE_0B127,
	    /*PAR_VOL_SLOPE3*/ 		DTYPE_0B127,
	    /*PAR_VOL_SLOPE4*/ 		DTYPE_0B127,
	    /*PAR_VOL_SLOPE5*/ 		DTYPE_0B127,
	    /*PAR_VOL_SLOPE6*/ 		DTYPE_0B127,
	    /*PAR_REPEAT4*/ 		DTYPE_0B127,
	    /*PAR_REPEAT5*/ 		DTYPE_0B127,
	    /*PAR_MOD_EG1*/ 		DTYPE_0B127,							//70
	    /*PAR_MOD_EG2*/ 		DTYPE_0B127,
	    /*PAR_MOD_EG3*/ 		DTYPE_0B127,
	    /*PAR_MOD_EG4*/ 		DTYPE_0B127,
	    /*PAR_MODAMNT1*/ 		DTYPE_0B127,
	    /*PAR_MODAMNT2*/ 		DTYPE_0B127,
	    /*PAR_MODAMNT3*/ 		DTYPE_0B127,
	    /*PAR_MODAMNT4*/ 		DTYPE_0B127,
	    /*PAR_PITCH_SLOPE1*/ 	DTYPE_0B127,
	    /*PAR_PITCH_SLOPE2*/ 	DTYPE_0B127,
	    /*PAR_PITCH_SLOPE3*/ 	DTYPE_0B127,							//80
	    /*PAR_PITCH_SLOPE4*/ 	DTYPE_0B127,
	    /*PAR_FMAMNT1*/ 		DTYPE_0B127,
	    /*PAR_FM_FREQ1*/ 		DTYPE_0B127,
	    /*PAR_FMAMNT2*/ 		DTYPE_0B127,
	    /*PAR_FM_FREQ2*/ 		DTYPE_0B127,
	    /*PAR_FMAMNT3*/ 		DTYPE_0B127,
	    /*PAR_FM_FREQ3*/ 		DTYPE_0B127,
	    /*PAR_VOL1*/ 			DTYPE_0B127,
	    /*PAR_VOL2*/ 			DTYPE_0B127,
	    /*PAR_VOL3*/ 			DTYPE_0B127,							//90
	    /*PAR_VOL4*/ 			DTYPE_0B127,
	    /*PAR_VOL5*/ 			DTYPE_0B127,
	    /*PAR_VOL6*/ 			DTYPE_0B127,
	    /*PAR_PAN1*/ 			DTYPE_PM63,
	    /*PAR_PAN2*/ 			DTYPE_PM63,
	    /*PAR_PAN3*/ 			DTYPE_PM63,
	    /*NRPN_FINE*/ 			DTYPE_0B127,
	    /*NRPN_COARSE*/ 		DTYPE_0B127,
	    /*PAR_PAN4*/ 			DTYPE_PM63,
	    /*PAR_PAN5*/ 			DTYPE_PM63,								//100
	    /*PAR_PAN6*/ 			DTYPE_PM63,
	    /*PAR_DRIVE1*/ 			DTYPE_0B127,
	    /*PAR_DRIVE2*/ 			DTYPE_0B127,
	    /*PAR_DRIVE3*/ 			DTYPE_0B127,
	    /*PAR_SNARE_DISTORTION*/ DTYPE_0B127,
	    /*PAR_CYMBAL_DISTORTION*/ DTYPE_0B127,
	    /*PAR_HAT_DISTORTION*/ 	DTYPE_0B127,
	    /*PAR_VOICE_DECIMATION1*/ DTYPE_0B127,
	    /*PAR_VOICE_DECIMATION2*/ DTYPE_0B127,
	    /*PAR_VOICE_DECIMATION3*/ DTYPE_0B127,							//110
	    /*PAR_VOICE_DECIMATION4*/ DTYPE_0B127,
	    /*PAR_VOICE_DECIMATION5*/ DTYPE_0B127,
	    /*PAR_VOICE_DECIMATION6*/ DTYPE_0B127,
	    /*PAR_VOICE_DECIMATION_ALL */ DTYPE_0B127,
	    /*PAR_FREQ_LFO1 */ 		DTYPE_0B127,
	    /*PAR_FREQ_LFO2*/		DTYPE_0B127,
	    /*PAR_FREQ_LFO3*/ 		DTYPE_0B127,
	    /*PAR_FREQ_LFO4*/ 		DTYPE_0B127,
	    /*PAR_FREQ_LFO5*/ 		DTYPE_0B127,
	    /*PAR_FREQ_LFO6*/ 		DTYPE_0B127,							//120
	    /*PAR_AMOUNT_LFO1*/		DTYPE_0B127,
	    /*PAR_AMOUNT_LFO2*/ 	DTYPE_0B127,
	    /*PAR_AMOUNT_LFO3*/ 	DTYPE_0B127,
	    /*PAR_AMOUNT_LFO4*/		DTYPE_0B127,
	    /*PAR_AMOUNT_LFO5*/ 	DTYPE_0B127,
	    /*PAR_AMOUNT_LFO6*/ 	DTYPE_0B127,
	    /*PAR_RESERVED4*/ 		DTYPE_0B127,
	    /*PAR_FILTER_DRIVE_1*/ 	DTYPE_0B127,
	    /*PAR_FILTER_DRIVE_2*/ 	DTYPE_0B127,
	    /*PAR_FILTER_DRIVE_3*/ 	DTYPE_0B127,							//130
	    /*PAR_FILTER_DRIVE_4*/ 	DTYPE_0B127,
	    /*PAR_FILTER_DRIVE_5*/ 	DTYPE_0B127,
	    /*PAR_FILTER_DRIVE_6*/	DTYPE_0B127,
	    /*PAR_MIX_MOD_1*/ 		DTYPE_MIX_FM,
	    /*PAR_MIX_MOD_2*/ 		DTYPE_MIX_FM,
	    /*PAR_MIX_MOD_3*/ 		DTYPE_MIX_FM,
	    /*PAR_VOLUME_MOD_ON_OFF1*/ DTYPE_ON_OFF,
	    /*PAR_VOLUME_MOD_ON_OFF2*/ DTYPE_ON_OFF,
	    /*PAR_VOLUME_MOD_ON_OFF3*/ DTYPE_ON_OFF,
	    /*PAR_VOLUME_MOD_ON_OFF4*/ DTYPE_ON_OFF,						//140
	    /*PAR_VOLUME_MOD_ON_OFF5*/ DTYPE_ON_OFF,
	    /*PAR_VOLUME_MOD_ON_OFF6*/ DTYPE_ON_OFF,
	    /*PAR_VELO_MOD_AMT_1*/ 	DTYPE_0B127,
	    /*PAR_VELO_MOD_AMT_2*/ 	DTYPE_0B127,
	    /*PAR_VELO_MOD_AMT_3*/ 	DTYPE_0B127,
	    /*PAR_VELO_MOD_AMT_4*/ 	DTYPE_0B127,
	    /*PAR_VELO_MOD_AMT_5*/ 	DTYPE_0B127,
	    /*PAR_VELO_MOD_AMT_6*/ 	DTYPE_0B127,
	    /*PAR_VEL_DEST_1*/ 		DTYPE_TARGET_SELECTION_VELO,
	    /*PAR_VEL_DEST_2*/ 		DTYPE_TARGET_SELECTION_VELO,			//150
	    /*PAR_VEL_DEST_3*/ 		DTYPE_TARGET_SELECTION_VELO,
	    /*PAR_VEL_DEST_4*/ 		DTYPE_TARGET_SELECTION_VELO,
	    /*PAR_VEL_DEST_5*/ 		DTYPE_TARGET_SELECTION_VELO,
	    /*PAR_VEL_DEST_6*/ 		DTYPE_TARGET_SELECTION_VELO,
	    /*PAR_WAVE_LFO1*/ 		DTYPE_MENU | (MENU_LFO_WAVES<<4),
	    /*PAR_WAVE_LFO2*/ 		DTYPE_MENU | (MENU_LFO_WAVES<<4),
	    /*PAR_WAVE_LFO3*/ 		DTYPE_MENU | (MENU_LFO_WAVES<<4),
	    /*PAR_WAVE_LFO4 */ 		DTYPE_MENU | (MENU_LFO_WAVES<<4),
	    /*PAR_WAVE_LFO5*/ 		DTYPE_MENU | (MENU_LFO_WAVES<<4),
	    /*PAR_WAVE_LFO6*/ 		DTYPE_MENU | (MENU_LFO_WAVES<<4),		//160
	    /*PAR_VOICE_LFO1*/ 		DTYPE_VOICE_LFO,
	    /*PAR_VOICE_LFO2*/ 		DTYPE_VOICE_LFO,
	    /*PAR_VOICE_LFO3*/ 		DTYPE_VOICE_LFO,
	    /*PAR_VOICE_LFO4*/ 		DTYPE_VOICE_LFO,
	    /*PAR_VOICE_LFO5*/ 		DTYPE_VOICE_LFO,
	    /*PAR_VOICE_LFO6*/ 		DTYPE_VOICE_LFO,
	    /*PAR_TARGET_LFO1*/ 	DTYPE_TARGET_SELECTION_LFO,
	    /*PAR_TARGET_LFO2*/ 	DTYPE_TARGET_SELECTION_LFO,
	    /*PAR_TARGET_LFO3*/ 	DTYPE_TARGET_SELECTION_LFO,
	    /*PAR_TARGET_LFO4*/ 	DTYPE_TARGET_SELECTION_LFO,				//170
	    /*PAR_TARGET_LFO5*/ 	DTYPE_TARGET_SELECTION_LFO,
	    /*PAR_TARGET_LFO6*/ 	DTYPE_TARGET_SELECTION_LFO,
	    /*PAR_RETRIGGER_LFO1*/ 	DTYPE_MENU | (MENU_RETRIGGER<<4),
	    /*PAR_RETRIGGER_LFO2*/ 	DTYPE_MENU | (MENU_RETRIGGER<<4),
	    /*PAR_RETRIGGER_LFO3*/ 	DTYPE_MENU | (MENU_RETRIGGER<<4),
	    /*PAR_RETRIGGER_LFO4*/ 	DTYPE_MENU | (MENU_RETRIGGER<<4),
	    /*PAR_RETRIGGER_LFO5*/ 	DTYPE_MENU | (MENU_RETRIGGER<<4),
	    /*PAR_RETRIGGER_LFO6*/ 	DTYPE_MENU | (MENU_RETRIGGER<<4),
	    /*PAR_SYNC_LFO1*/ 		DTYPE_MENU | (MENU_SYNC_RATES<<4),
	    /*PAR_SYNC_LFO2*/ 		DTYPE_MENU | (MENU_SYNC_RATES<<4),		//180
	    /*PAR_SYNC_LFO3*/ 		DTYPE_MENU | (MENU_SYNC_RATES<<4),
	    /*PAR_SYNC_LFO4*/ 		DTYPE_MENU | (MENU_SYNC_RATES<<4),
	    /*PAR_SYNC_LFO5*/ 		DTYPE_MENU | (MENU_SYNC_RATES<<4),
	    /*PAR_SYNC_LFO6*/ 		DTYPE_MENU | (MENU_SYNC_RATES<<4),
	    /*PAR_OFFSET_LFO1*/ 	DTYPE_0B127,
	    /*PAR_OFFSET_LFO2*/ 	DTYPE_0B127,
	    /*PAR_OFFSET_LFO3*/ 	DTYPE_0B127,
	    /*PAR_OFFSET_LFO4*/ 	DTYPE_0B127,
	    /*PAR_OFFSET_LFO5*/ 	DTYPE_0B127,
	    /*PAR_OFFSET_LFO6*/ 	DTYPE_0B127,							//190
	    /*PAR_FILTER_TYPE_1*/ 	DTYPE_MENU | (MENU_FILTER<<4),
	    /*PAR_FILTER_TYPE_2*/ 	DTYPE_MENU | (MENU_FILTER<<4),
	    /*PAR_FILTER_TYPE_3*/ 	DTYPE_MENU | (MENU_FILTER<<4),
	    /*PAR_FILTER_TYPE_4*/ 	DTYPE_MENU | (MENU_FILTER<<4),
	    /*PAR_FILTER_TYPE_5*/ 	DTYPE_MENU | (MENU_FILTER<<4),
	    /*PAR_FILTER_TYPE_6*/ 	DTYPE_MENU | (MENU_FILTER<<4),
	    /*PAR_TRANS1_VOL*/ 		DTYPE_0B127,
	    /*PAR_TRANS2_VOL*/ 		DTYPE_0B127,
	    /*PAR_TRANS3_VOL*/ 		DTYPE_0B127,
	    /*PAR_TRANS4_VOL*/ 		DTYPE_0B127,							//200
	    /*PAR_TRANS5_VOL*/ 		DTYPE_0B127,
	    /*PAR_TRANS6_VOL*/ 		DTYPE_0B127,
	    /*PAR_TRANS1_WAVE*/ 	DTYPE_MENU | (MENU_TRANS<<4),
	    /*PAR_TRANS2_WAVE*/ 	DTYPE_MENU | (MENU_TRANS<<4),
	    /*PAR_TRANS3_WAVE*/ 	DTYPE_MENU | (MENU_TRANS<<4),
	    /*PAR_TRANS4_WAVE*/ 	DTYPE_MENU | (MENU_TRANS<<4),
	    /*PAR_TRANS5_WAVE*/ 	DTYPE_MENU | (MENU_TRANS<<4),
	    /*PAR_TRANS6_WAVE*/ 	DTYPE_MENU | (MENU_TRANS<<4),
	    /*PAR_TRANS1_FREQ*/ 	DTYPE_0B127,
	    /*PAR_TRANS2_FREQ*/ 	DTYPE_0B127,							//210
	    /*PAR_TRANS3_FREQ*/ 	DTYPE_0B127,
	    /*PAR_TRANS4_FREQ*/ 	DTYPE_0B127,
	    /*PAR_TRANS5_FREQ*/ 	DTYPE_0B127,
	    /*PAR_TRANS6_FREQ*/ 	DTYPE_0B127,
	    /*PAR_AUDIO_OUT1*/ 		DTYPE_MENU | (MENU_AUDIO_OUT<<4),
	    /*PAR_AUDIO_OUT2*/ 		DTYPE_MENU | (MENU_AUDIO_OUT<<4),
	    /*PAR_AUDIO_OUT3*/ 		DTYPE_MENU | (MENU_AUDIO_OUT<<4),
	    /*PAR_AUDIO_OUT4*/ 		DTYPE_MENU | (MENU_AUDIO_OUT<<4),
	    /*PAR_AUDIO_OUT5*/ 		DTYPE_MENU | (MENU_AUDIO_OUT<<4),
	    /*PAR_AUDIO_OUT6*/ 		DTYPE_MENU | (MENU_AUDIO_OUT<<4),		//220
	    //--AS these 7
	    /*PAR_ENVELOPE_POSITION_1*/		DTYPE_0B127,
	    /*PAR_ENVELOPE_POSITION_2*/		DTYPE_0B127,
	    /*PAR_ENVELOPE_POSITION_3*/		DTYPE_0B127,
	    /*PAR_ENVELOPE_POSITION_4*/		DTYPE_0B127,
	    /*PAR_ENVELOPE_POSITION_5*/		DTYPE_0B127,
	    /*PAR_ENVELOPE_POSITION_6*/		DTYPE_0B127,
	    /*PARAM_UNUSED_01*/		DTYPE_0B127,
        /*PAR_KIT_VERSION*/     DTYPE_0B255,
	    /*PARAM_UNUSED_02*/		DTYPE_0B127,

        /*PAR_MORPH_DRUM1*/		DTYPE_0B255,
	    /*PAR_MORPH_DRUM2*/		DTYPE_0B255,
	    /*PAR_MORPH_DRUM3*/		DTYPE_0B255,
	    /*PAR_MORPH_SNARE*/		DTYPE_0B255,
	    /*PAR_MORPH_CYM*/		DTYPE_0B255,
	    /*PAR_MORPH_HIHAT*/		DTYPE_0B255,
       
       /*PARAM_UNUSED_03*/		DTYPE_0B127,
       /*PARAM_UNUSED_04*/		DTYPE_0B127, 
       /*PARAM_UNUSED_05*/		DTYPE_0B127,
       /*PARAM_UNUSED_06*/		DTYPE_0B127,
       /*PARAM_UNUSED_07*/		DTYPE_0B127,
       /*PARAM_UNUSED_08*/		DTYPE_0B127,
       /*PARAM_UNUSED_09*/		DTYPE_0B127,

	    /*PAR_ROLL*/ 			DTYPE_MENU | (MENU_ROLL_RATES<<4),
	    /*PAR_MORPH*/ 			DTYPE_0B255,
	    /*PAR_ACTIVE_STEP */ 	DTYPE_0B127,							//230
	    /*PAR_STEP_VOLUME*/ 	DTYPE_0B127,
	    /*PAR_STEP_PROB*/ 		DTYPE_0B127,
	    /*PAR_STEP_NOTE*/ 		DTYPE_NOTE_NAME,
	    /*PAR_EUKLID_LENGTH*/ 	DTYPE_1B16,
	    /*PAR_EUKLID_STEPS*/ 	DTYPE_1B16,
	    /*PAR_EUKLID_ROTATION*/	DTYPE_0B15,
       /*PAR_EUKLID_SUBSTEP_ROTATION*/  DTYPE_0B15,
	    /*PAR_AUTOM_TRACK*/ 	DTYPE_0b1,
	    /*PAR_P1_DEST*/ 		DTYPE_AUTOM_TARGET,
	    /*PAR_P2_DEST*/ 		DTYPE_AUTOM_TARGET,
	    /*PAR_P1_VAL*/ 			DTYPE_0B127,							//240
	    /*PAR_P2_VAL*/ 			DTYPE_0B127,
	    /*PAR_SHUFFLE*/ 		DTYPE_0B127,
	    /*PAR_PATTERN_BEAT*/ 	DTYPE_0B127,
	    /*PAR_PATTERN_NEXT*/ 	DTYPE_MENU | (MENU_NEXT_PATTERN<<4),
	    /*PAR_TRACK_LENGTH*/ 	DTYPE_1B16,
       /*PAR_TRACK_SCALE*/ 	DTYPE_MENU | (MENU_TRACK_SCALE<<4),
	    /*PAR_POS_X*/ 			DTYPE_0B127,
	    /*PAR_POS_Y*/ 			DTYPE_0B127,
	    /*PAR_FLUX*/ 			DTYPE_0B127,
	    /*PAR_SOM_FREQ*/ 		DTYPE_0B127,
	    /*PAR_TRACK_ROTATION*/  DTYPE_1B16,  //**PATROT this is not shown in menu, but if it were it would really be 0 to 15


       /*PAR_MAC1*/        DTYPE_0B127,
       /*PAR_MAC2*/        DTYPE_0B127,
     
      /*PAR_ROLL_NOTE*/ DTYPE_NOTE_NAME,
      /*PAR_ROLL_VELOCITY*/ DTYPE_0B127,
      /*PAR_ROLL_MODE*/  DTYPE_MENU | (MENU_MIDI<<4),
      
      /*PAR_TRANSPOSE,*/      DTYPE_PM63,
      /*PAR_TRANSPOSE_ON_OFF,*/DTYPE_ON_OFF,

	    /*PAR_BPM*/ 			DTYPE_0B255,							//251
	    /*PAR_MIDI_CHAN_1*/ 	DTYPE_0B16,
	    /*PAR_MIDI_CHAN_2*/ 	DTYPE_0B16,
	    /*PAR_MIDI_CHAN_3*/ 	DTYPE_0B16,
	    /*PAR_MIDI_CHAN_4*/ 	DTYPE_0B16,
	    /*PAR_MIDI_CHAN_5*/ 	DTYPE_0B16,
	    /*PAR_MIDI_CHAN_6*/ 	DTYPE_0B16,
	    /*PAR_FETCH*/ 			DTYPE_ON_OFF,
	    /*PAR_FOLLOW*/ 			DTYPE_ON_OFF,
	    /*PAR_QUANTISATION*/ 	DTYPE_MENU | (MENU_SEQ_QUANT<<4),
	    /*PAR_SCREENSAVER_ON_OFF*/ DTYPE_ON_OFF,						//261
	    /*PAR_MIDI_MODE*/ 		DTYPE_MENU | (MENU_MIDI<<4),			  //--AS This is now unused.
	    /*PAR_MIDI_CHAN_7*/ 	DTYPE_0B16,
	    /*PAR_MIDI_ROUTING*/	DTYPE_MENU | (MENU_MIDI_ROUTING<<4),
	    /*PAR_MIDI_FILT_TX*/    DTYPE_MENU | (MENU_MIDI_FILTERING<<4),
	    /*PAR_MIDI_FILT_RX*/    DTYPE_MENU | (MENU_MIDI_FILTERING<<4),
		/*PAR_PRESCALER_CLOCK_IN*/		DTYPE_MENU | (MENU_PPQ<<4),
		/*PAR_PRESCALER_CLOCK_OUT1*/	DTYPE_MENU | (MENU_PPQ<<4),
		/*PAR_PRESCALER_CLOCK_OUT2*/	DTYPE_MENU | (MENU_PPQ<<4),
		/*PAR_TRIGGER_GATE_MODE*/	DTYPE_ON_OFF,
	    /*PAR_BAR_RESET_MODE*/  DTYPE_ON_OFF,
	    /*PAR_MIDI_CHAN_GLOBAL*/DTYPE_0B16,		//--AS global midi channel
	   /*PAR_SEQ_PC_TIME*/  DTYPE_ON_OFF, // -bc- change patterns on sub-step instead of bar
	   /*PAR_BUT_SHIFT_MODE*/ DTYPE_ON_OFF, // -bc- make shift a toggle
      /*PAR_LOAD_PERF_ON_BANK*/  DTYPE_ON_OFF, // -bc- load perfs instead of kits on bank change cc
      /*PAR_OSC_WAVE_INTERPOLATION*/  DTYPE_ON_OFF, // previously SKIP_FIRST_ROLL
      /*PAR_FILE_LOAD_BACKGROUND*/  DTYPE_MENU | (MENU_FILE_LOAD_BACKGROUND<<4),
      
      /*PAR_GLOBAL_SETTINGS_VERSION*/  DTYPE_0B127,
      
      /*PAR_MIDI_NOTE1*/		DTYPE_NOTE_NAME,
	    /*PAR_MIDI_NOTE2*/		DTYPE_NOTE_NAME,
	    /*PAR_MIDI_NOTE3*/		DTYPE_NOTE_NAME,
	    /*PAR_MIDI_NOTE4*/		DTYPE_NOTE_NAME,
	    /*PAR_MIDI_NOTE5*/		DTYPE_NOTE_NAME,
	    /*PAR_MIDI_NOTE6*/     DTYPE_NOTE_NAME,
       /*PAR_MIDI_NOTE7*/     DTYPE_NOTE_NAME,
       /* Roll MIDI trigger offset: 0 is off, 1..127 is a positive shift. */
       /*PAR_ROLL_NOTE_OFFSET*/ DTYPE_0B127,
         

};



//-----------------------------------------------------------------------
//                            vars
//-----------------------------------------------------------------------
/** the lower 3 bit indicate the active parameter.
the upper bits indicate the active page no.
 */
static uint8_t menuIndex = 0;

static uint8_t menu_TargetVoiceGapIndex = 0xFF;

uint8_t menu_numSamples = 0;

//preset vars
#define NUM_PRESET_LOCATIONS 11 
//kit, drum1, drum2, drum3, snare, cym, hh, morph sound, pattern, performance, all

uint8_t menu_currentPresetNr[NUM_PRESET_LOCATIONS];

uint8_t menu_shownPattern = 0;
uint8_t menu_muteModeActive = 0;
uint8_t morphValue=0;
uint8_t menu_sequencerRunning = 0;
uint8_t menu_instPerfLock = 0;
uint8_t menu_instPerfLockPreset = 0;
uint8_t menu_voiceArray = 0x7f;

/** buffer to minimize the display configuration.
It holds a representation of the display content so only the changed cells have to be updated*/
char currentDisplayBuffer[2][16];	/**< what is currently shown on the LCD*/
char editDisplayBuffer[2][17];		/**< what should be shown on the LCD after the next update. length 17 to allow zero termination without overflow.*/
uint8_t visibleCursor=0;	/* cursor position and whether it's visible rightmost=visible, then row (0,1) then col: ccccrv */
/* calc cursor to location (row, col both zero based) and make visible/invisible */
#define VIS_CURS(r,c,v) (((c) << 2) | ((r) << 1) | (v))
uint8_t menu_activePage = 0;				/**< indicates which menu page is currently shown*/
uint8_t menu_activeVoice = 0;
uint8_t menu_playedPattern = 0;
uint8_t editModeActive = 0;			/**< when edit mode is active, only the currently active parameter is shown with full name*/
static uint8_t lastEncoderButton = 0;		/**< stores the state of the encoder button so the edit mode is only switched once when the button is pressed*/

static uint8_t parameterFetch = 0b00011111;	/**< the lower 4 bits define a lock for each pot, the 5 bit turns he lock on and off*/

/** array holding all the available parameter values*/
uint8_t parameter_values[NUM_PARAMS];

uint8_t parameters2[END_OF_SOUND_PARAMETERS];/**< a second array for sound x-fade to another preset*/

uint8_t menu_lastVoiceIndex=0;
uint8_t menu_lastPerfIndex=0;

uint8_t menu_lastStepSubPage=0;
uint8_t menu_selectedStepLed=LED_STEP1;

//-----------------------------------------------------------------
void menu_init()
{

	lcd_clear();

	paramToModTargetInit();

	memset(menu_currentPresetNr,0,sizeof(uint8_t) *NUM_PRESET_LOCATIONS );

	memset(parameter_values, 0, sizeof(uint8_t) * NUM_PARAMS);

   /* Global decimation should never boot as 0. The file-import clamp below
      fixes broken on-disk values, and this startup default ensures the menu
      and any no-SD / failed-load boot path start from the safe 127 value. */
   parameter_values[PAR_VOICE_DECIMATION_ALL] = 127;



	parameter_values[PAR_EUKLID_LENGTH] = 16;
	parameter_values[PAR_EUKLID_STEPS] = 16;
	parameter_values[PAR_EUKLID_ROTATION] = 0;
   
   // parameter_values[PAR_MAC1] = 0;
   // parameter_values[PAR_MAC2] = 0;

	//initialize the roll value
	parameter_values[PAR_ROLL] = 10;
   parameter_values[PAR_ROLL_NOTE] = 63;
   parameter_values[PAR_ROLL_VELOCITY] = 100;
   parameter_values[PAR_ROLL_MODE] = 4; //0=trig, 1=nte, 2=vel, 3=bth, 4=all
   /* MIDI roll notes default off. Missing old-file bytes normalize to the same
      value, so startup, old-file load, and fresh save agree on 0/off. */
   parameter_values[PAR_ROLL_NOTE_OFFSET] = 0;
   parameter_values[PAR_TRANSPOSE] = 63;
   parameter_values[PAR_TRANSPOSE_ON_OFF] = 0;
   parameter_values[PAR_ACTIVE_STEP] = 0;
   // parameter_values[PAR_SKIP_FIRST_ROLL] = 0;
   
	//avrComms_sendData(SEQ_CC,SEQ_ROLL_RATE,8); //value is initialized in cortex firmware

	parameter_values[PAR_BPM] = 120;

	// --AS todo I tried to move everything below here to main.c before the call
	// to copyClear_clearCurrentPattern (see 7b0932f20b948ff29037bf6c2b1ecb7eba8bb89e)
	// but then the global data loaded from glo.cfg seemed like it was not being sent to the back.
	// need to figure out why as it's likely to crop up again at some point.

	avrComms_sendData(SEQ_CC,SEQ_SET_ACTIVE_TRACK,0);
	//the currently active button is lit
	led_setActiveVoice(0);

   //set voice 1 active
	menu_switchPage(0);
   menu_switchSubPage(0);
   led_setActiveSelectButton(0);
   menu_enterVoiceMode();
   
   
}
//-----------------------------------------------------------------
/** compare the currentDisplayBuffer with the editDisplayBuffer and send all differences to the display*/
void sendDisplayBuffer()
{
	// turn off the cursor
	lcd_turnOn(1,0);
	for(uint8_t i=0;i<2;i++)
	{
		for(uint8_t j=0;j<16;j++)
		{
			if(currentDisplayBuffer[i][j] != editDisplayBuffer[i][j])
			{
				//something has changed

				//we have to omit the zero termination sign
				if(editDisplayBuffer[i][j] == '\0')
				{
					//make a clear space out of zero termination
					editDisplayBuffer[i][j] = ' ';
				}

				//set cursor to current position
				lcd_setcursor(j,(uint8_t)(i+1));
				//send the new character
				lcd_data(editDisplayBuffer[i][j]);
				//update currentDisplayBuffer
				currentDisplayBuffer[i][j] = editDisplayBuffer[i][j];
			}	
		}

	}

	// set cursor position and make visible or not
	lcd_setcursor((uint8_t)(visibleCursor >> 2), (uint8_t)(((visibleCursor & 2) >> 1)+1));
	lcd_turnOn(1,(visibleCursor & 1));

}
//-----------------------------------------------------------------
void menu_debug(char* displayText, size_t length, uint8_t value1, uint8_t value2, int delayTime)
{
   if (length>16)
      return;
   uint8_t i;
   
   lcd_clear();
   memset(editDisplayBuffer,' ',2*17);	
   for(i=0;i<length;i++)
   {
      editDisplayBuffer[0][i]=displayText[i];
   }

   numtostru(&editDisplayBuffer[1][0],(uint8_t)(value1));
   numtostru(&editDisplayBuffer[1][7],(uint8_t)(value2));

   sendDisplayBuffer();
   if(delayTime>=5000)
      _delay_ms(5000);
   else if(delayTime>=4000)
      _delay_ms(4000);
   else if(delayTime>=3000)
      _delay_ms(3000);
   else if(delayTime>=2000)
      _delay_ms(2000);
   else if(delayTime>=1000)
      _delay_ms(1000);
   else if(delayTime>=500)
      _delay_ms(500);
   else if(delayTime>=200)
      _delay_ms(500);
   else if (delayTime>=100)
      _delay_ms(100);
   else if (delayTime>=50)
      _delay_ms(50);
   else if (delayTime>=10)
      _delay_ms(10);
   else if (delayTime>=5)
      _delay_ms(5);
   else if (delayTime>=1)
      _delay_ms(1);
}
//-----------------------------------------------------------------
void menu_enterVoiceMode()
{
   //set menu to voice page mode
   //uint8_t pageNr=menu_getActiveVoice();
	//uint8_t paramNr=menu_lastVoiceParameter;
   //menu_switchSubPage(menu_lastVoiceSubPage);
   led_clearAllBlinkLeds();
   menuIndex = menu_lastVoiceIndex;
   menu_switchPage(menu_getActiveVoice());
	led_setActiveSelectButton(menu_getSubPage());
	//menu_resetActiveParameter();
   /* Use the effective shift query here rather than the old exported
      shiftState variable, so re-entering voice mode honors latched global
      shift-toggle exactly like a physically held SHIFT button. */
   if(buttonHandler_getShift())
      menu_shiftVoice(1);
}
//-----------------------------------------------------------------
void menu_enterActiveStepMode()
{
	led_clearSequencerLeds();
	led_clearSelectLeds();
   led_clearVoiceLeds();
  
   menu_switchPage(ACTIVESTEP_PAGE);
   avrComms_updateActiveStepLeds();
	menu_repaintAll();
   led_setBlinkLed(LED_MODE3,1);
}
//-----------------------------------------------------------------
void menu_enterPerfMode()
{
	led_clearSequencerLeds();
	led_clearSelectLeds();
	led_initPerformanceLeds();
   led_clearAllBlinkLeds();
   led_clearModeLeds();
   //menu_repaint();

	//set menu to perf page
   menuIndex=menu_lastPerfIndex;   
   menu_switchPage(PERFORMANCE_PAGE);
	menu_repaintAll();
   led_setValue(1,LED_MODE2); 
}
//-----------------------------------------------------------------
void menu_enterStepMode()
{
   menuIndex=menu_lastStepSubPage;
	menu_switchPage(SEQ_PAGE);   
	avrComms_updatePatternLeds();
	// set mainstep blinking
   led_setBlinkLed(menu_selectedStepLed, 1);
   // set selected substep blinking
   led_setBlinkLed((uint8_t)(LED_PART_SELECT1+(parameter_values[PAR_ACTIVE_STEP]%8)), 1);
   menu_repaint();
   /* Same effective-state rule as voice mode: shifted step paint should follow
      the latch, not the raw physical button level. */
   if(buttonHandler_getShift())
      menu_shiftStep(1);
   led_setValue(1,LED_MODE3);
}
//-----------------------------------------------------------------
void menu_enterPatgenMode()
{
   /* `menu_enterPatgenMode()` is used both for the first entry into the
      Euclid/rotation editor and for later in-page track changes. Only the
      first entry should begin a new temp-backup visit on the STM. Re-sending
      BEGIN_VISIT while merely selecting another track on the same page would
      clear the touched-track mask and leave rollback able to restore only the
      most recently edited track. */
   if(menu_activePage != EUKLID_PAGE)
      avrComms_sendData(SEQ_CC, SEQ_EUKLID_RESET, SEQ_EUKLID_RESET_BEGIN_VISIT);
   avrComms_sendData(SEQ_CC, SEQ_REQUEST_EUKLID_PARAMS,
   				menu_getActiveVoice());
   menu_switchPage(EUKLID_PAGE);
   led_clearAllBlinkLeds();
   led_setBlinkLed(LED_MODE2,1);
   led_clearSequencerLeds();
	led_clearSelectLeds();
   avrComms_updatePatternLeds();
}
//-----------------------------------------------------------------
void menu_shiftVoice(uint8_t shift)
{
   if(shift)
   {
      
      menu_switchPage(SEQ_PAGE);
      menu_resetSubPage(0);
      
      avrComms_updatePatternLeds();
   
      // set mainstep blinking
	   led_setBlinkLed(menu_selectedStepLed, 1);
      // set selected substep blinking
      led_setBlinkLed((uint8_t)(LED_PART_SELECT1+(parameter_values[PAR_ACTIVE_STEP]%8)), 1);
      
      menu_repaint();
   }
   else
   {
      menu_enterVoiceMode();
   }
}
//-----------------------------------------------------------------
void menu_shiftPerf(uint8_t shift)
{
   if(shift)
   {
      //blink selected pattern LED
      menu_switchPage(PATTERN_SETTINGS_PAGE);
      led_clearSelectLeds();
      led_clearAllBlinkLeds();
      //**PATROT The step led's are repurposed here to have the current track rotation
      // represented by a blinking led. The leftmost LED blinking means rotation is off (0)
      // and each one represents a different rotation
      led_setBlinkLed((uint8_t) (LED_STEP1 + parameter_values[PAR_TRACK_ROTATION]), 1);
      {
         uint8_t viewedPattern = menu_getViewedPattern();
         led_setBlinkLed((uint8_t)((viewedPattern == SEQ_TMP_PATTERN) ? LED_STEP16 : (LED_PART_SELECT1 + viewedPattern)), 1);
      }
      led_setBlinkLed((uint8_t)(LED_VOICE1 + menu_getActiveVoice()) ,1);
      
      // show the active voice pattern
      menu_updateMainStepDisplay();
   }
   else
   {
      led_clearAllBlinkLeds();
      led_clearSelectLeds();
      menuIndex=menu_lastPerfIndex;
      menu_switchPage(PERFORMANCE_PAGE);
      led_initPerformanceLeds();
   }
}
//-----------------------------------------------------------------
void menu_shiftStep(uint8_t shift)
{
   if(shift)
   {
      led_setBlinkLed(menu_selectedStepLed, 0);
      led_setValue(0, menu_selectedStepLed);
      
      led_clearAllBlinkLeds();
      avrComms_updatePatternLeds();
   }
   else
   {
   
      //menu_switchSubPage(0);
	   menu_switchPage(SEQ_PAGE);

	   avrComms_updatePatternLeds();
      
      // set mainstep blinking
	   led_setBlinkLed(menu_selectedStepLed, 1);
      // set selected substep blinking
      led_setBlinkLed((uint8_t)(LED_PART_SELECT1+(parameter_values[PAR_ACTIVE_STEP]%8)), 1);
   
   }
   
}
//-----------------------------------------------------------------
void menu_shiftActiveStep(uint8_t shift)
{
   if(shift)
   {  
   }
   else
   {
   }
}
//-----------------------------------------------------------------
void menu_shiftPatgen(uint8_t shift)
{
   if(shift)
   {
      menu_switchPage(SEQ_PAGE);
      menu_resetSubPage(0);
      
      avrComms_updatePatternLeds();
      
      // set mainstep blinking
	   led_setBlinkLed(menu_selectedStepLed, 1);
      // set selected substep blinking
      led_setBlinkLed((uint8_t)(LED_PART_SELECT1+(parameter_values[PAR_ACTIVE_STEP]%8)), 1);

      menu_repaint();
   }
   else
   {
      //led_clearSelectLeds();
      menu_switchPage(EUKLID_PAGE);
      led_clearAllBlinkLeds();
      led_setBlinkLed(LED_MODE2,1);
      //led_initPerformanceLeds();
      //led_setValue(1,	(uint8_t) (menu_getViewedPattern() + LED_PART_SELECT1));
   }
}
//-----------------------------------------------------------------
void menu_resetBlinkLeds()
{
   led_clearAllBlinkLeds();
   switch(menu_activePage)
   {
      case SHIFTVOICE_PAGE:
      led_setBlinkLed(LED_MODE1,1);
      break;
      case EUKLID_PAGE:
      led_setBlinkLed(LED_MODE2,1);
      break;
      case ACTIVESTEP_PAGE:
      led_setBlinkLed(LED_MODE3,1);
      break;
      case MENU_MIDI_PAGE:
      led_setBlinkLed(LED_MODE4,1);
      break;
      default:
      break;
   }   

}
//-----------------------------------------------------------------
void menu_updateMainStepDisplay()
{
   led_clearSequencerLeds();
   uint8_t trackNr = menu_getActiveVoice(); //max 6 => 0x6 = 0b110
	uint8_t patternNr = menu_shownPattern; //max 7 => 0x07 = 0b111
	uint8_t value = (uint8_t)((trackNr<<4) | (patternNr&0x0f));
	avrComms_sendData(LED_CC,LED_QUERY_SEQ_TRACK,value);
}
//-----------------------------------------------------------------
void menu_setShownPattern(uint8_t patternNr)
{
	menu_shownPattern = patternNr;
	avrComms_sendData(SEQ_CC,SEQ_SET_SHOWN_PATTERN,menu_shownPattern);
}
//-----------------------------------------------------------------
uint8_t menu_getViewedPattern()
{
	return menu_shownPattern;
}
//-----------------------------------------------------------------

//lock all 4 potentiometer values
void lockPotentiometerFetch()
{
	if(parameterFetch & PARAMETER_LOCK_ACTIVE)
	{
		//all 4 parameters locked
		parameterFetch |= 0x0F;
	}	
}
//-----------------------------------------------------------------
void menu_repaintAll()
{

	//if(copyClear_Mode == MODE_NONE)
	{
		memset(editDisplayBuffer,' ',2*17);	
		memset(currentDisplayBuffer,127,2*16);

		menu_repaint();
	}		
}

//-----------------------------------------------------------------
static void menu_repaintLoadPage()
{
// deals with encoder actions for load menu
// NB, the KNOB (shortcut actions) get dealt with in
// another function
   const char *toptxt=0;
   
//skip items that don't appear in load menu
//(there aren't any yet)
   switch(menu_saveOptions.what) {
      default:
         break;
   }
   
//Top row
   strcpy_P(&editDisplayBuffer[0][0],PSTR("Load:"));
//add menu option text to top line
   switch(menu_saveOptions.what) {
      case SAVE_TYPE_KIT: 
         toptxt=PSTR("Kit ");
         break;
      case SAVE_TYPE_DRUM1:		
         toptxt=PSTR("Drum 1  "); 
         break;
      case SAVE_TYPE_DRUM2:		
         toptxt=PSTR("Drum 2  "); 
         break;
      case SAVE_TYPE_DRUM3:		
         toptxt=PSTR("Drum 3  "); 
         break;
      case SAVE_TYPE_SNARE:		
         toptxt=PSTR("Snare   "); 
         break;
      case SAVE_TYPE_CYM:		   
         toptxt=PSTR("Clap/Cym"); 
         break;
      case SAVE_TYPE_HIHAT:		
         toptxt=PSTR("HiHat   ");
         break;
      case SAVE_TYPE_PATTERN: 
         toptxt=PSTR("Pattern "); 
         break;
      case SAVE_TYPE_MORPH: 
         toptxt=PSTR("MorphKit"); 
         break;
      case SAVE_TYPE_GLO: 
         toptxt=PSTR("Settings"); 
         break;
      case SAVE_TYPE_PERFORMANCE: 
         toptxt=PSTR("Perform "); 
         break;
      case SAVE_TYPE_ALL: 
         toptxt=PSTR("All "); 
         break;
      case SAVE_TYPE_SAMPLES: 
         toptxt=PSTR("Samples "); 
         break;
   }
   // send concatenated top text to display
   strcpy_P(&editDisplayBuffer[0][6],toptxt);
   
   // if a save option has been selected, put it
   // in brackets, otherwise put an '>' next to 
   // the selected menu item
   if(menu_saveOptions.state == SAVE_STATE_EDIT_TYPE) {
      if(editModeActive) {
         editDisplayBuffer[0][5]='[';
         editDisplayBuffer[0][14]=']';
      } 
      // 
      else {
      //arrow before parameter
         editDisplayBuffer[0][5]= ARROW_SIGN;
      }
   }

//Display Bottom row for load page - name and number if applicable
   if( menu_saveOptions.what < SAVE_TYPE_GLO) { 
   //no name and number for global settings or samples
   
      numtostrpu(&editDisplayBuffer[1][1], menu_currentPresetNr[menu_saveOptions.what],' ');
      if(menu_saveOptions.state == SAVE_STATE_EDIT_PRESET_NR)
      // user has selected a load type that has a preset number
      // and is on the preset number selection item
      {
         if(editModeActive) {
         // user has clicked into the preset number, bracket it
            editDisplayBuffer[1][0]='[';
            editDisplayBuffer[1][4]=']';
         } 
         else {
         //arrow before parameter
            editDisplayBuffer[1][0]= ARROW_SIGN;
         //visibleCursor = VIS_CURS(1,1,1);
         }
      }
   // write the full preset name
      //strcpy_P(&preset_currentName,preset_loadName(menu_currentPresetNr[menu_saveOptions.what],menu_saveOptions.what));
      DISABLE_CONV_WARNING
      memcpy(&editDisplayBuffer[1][5],(const void*)preset_loadName(menu_currentPresetNr[menu_saveOptions.what],menu_saveOptions.what,0),8);
      END_DISABLE_CONV_WARNING
   }
   
   // bottom row - Load page is active
   //ok button shown for loading everything except kit and morph - which are loaded instantaneously
   
   if(menu_saveOptions.what<SAVE_TYPE_PATTERN) 
   { //clear ok text
      editDisplayBuffer[1][14] = 0;
      editDisplayBuffer[1][15] = 0;
   }
   
   else
   {
      memcpy_P(&editDisplayBuffer[1][14],menuText_ok,2);
      if((menu_saveOptions.state==SAVE_STATE_OK) ||
         (menu_saveOptions.what >= SAVE_TYPE_GLO && menu_saveOptions.state > SAVE_STATE_EDIT_TYPE)) {
         //arrow before parameter
         editDisplayBuffer[1][13]= ARROW_SIGN;
         //visibleCursor = VIS_CURS(1,14,1);
      }
   } 
   
}


//-----------------------------------------------------------------


static void menu_repaintSavePage()
{
// deals with encoder actions for 
   const char *toptxt=0;
   
//Top row
   strcpy_P(&editDisplayBuffer[0][0],
      menu_activePage == SAVE_PAGE ?
      PSTR("Save:") :
      PSTR("Load:"));
   switch(menu_saveOptions.what) {
      case SAVE_TYPE_KIT: toptxt=PSTR("Kit "); 
         break;
      case SAVE_TYPE_MORPH: toptxt=PSTR("MorphMix "); 
         break;
      case SAVE_TYPE_PATTERN: toptxt=PSTR("Pattern "); 
         break;
      case SAVE_TYPE_GLO: toptxt=PSTR("Settings"); 
         break;
      case SAVE_TYPE_PERFORMANCE: toptxt=PSTR("Perform "); 
         break;
      case SAVE_TYPE_ALL: toptxt=PSTR("All "); 
         break;
     default: // we are on a page that the save screen doesn't
              // recognise, return to 'kit' page
         toptxt=PSTR("Kit ");
         menu_saveOptions.what=SAVE_TYPE_KIT;
         break;
   }
   strcpy_P(&editDisplayBuffer[0][6],toptxt);
   
   
   if(menu_saveOptions.state == SAVE_STATE_EDIT_TYPE) {
      if(editModeActive) {
         editDisplayBuffer[0][5]='[';
         editDisplayBuffer[0][14]=']';
      } 
      else {
      //arrow before parameter
         editDisplayBuffer[0][5]= ARROW_SIGN;
      }
   }
//Bottom row - name and number if applicable
   if( menu_saveOptions.what < SAVE_TYPE_GLO) { //no name and number for global settings or samples
   //the preset number
      numtostrpu(&editDisplayBuffer[1][1], menu_currentPresetNr[menu_saveOptions.what],' ');
      if(menu_saveOptions.state == SAVE_STATE_EDIT_PRESET_NR)
      {
         if(editModeActive) {
            editDisplayBuffer[1][0]='[';
            editDisplayBuffer[1][4]=']';
         } 
         else {
         //arrow before parameter
            editDisplayBuffer[1][0]= ARROW_SIGN;
         //visibleCursor = VIS_CURS(1,1,1);
         }
      }
   // write the full preset name
      //strcpy_P(&preset_currentName,preset_loadName(menu_currentPresetNr[menu_saveOptions.what],menu_saveOptions.what));
      memcpy(&editDisplayBuffer[1][5],(const void*)preset_currentSaveMenuName,8);
   }
// bottom row - for save page, we show editing box or underline cursor as applicable
   if(menu_activePage == SAVE_PAGE){
   //the preset number
      if( menu_saveOptions.what < SAVE_TYPE_GLO) //not saving global settings, so print name and number
      {
      // if we are editing the name
         if( (menu_saveOptions.state >= SAVE_STATE_EDIT_NAME1) && (menu_saveOptions.state <= SAVE_STATE_EDIT_NAME8) )
         {
            if(editModeActive) { // draw a box around active character (encoder changes value)
               editDisplayBuffer[1][4+menu_saveOptions.state-SAVE_STATE_EDIT_NAME1]='[';
               editDisplayBuffer[1][6+menu_saveOptions.state-SAVE_STATE_EDIT_NAME1]=']';
            } 
            else { // using encoder to move left and right, using knob to change letter
            //underline under current char
               visibleCursor =(uint8_t)( VIS_CURS(1,5+menu_saveOptions.state-SAVE_STATE_EDIT_NAME1,1));
            }
         }
      } // not saving global settings
   //ok button - shown in all cases
      memcpy_P(&editDisplayBuffer[1][14],menuText_ok,2);
      if((menu_saveOptions.state==SAVE_STATE_OK) ||
      (menu_saveOptions.what >= SAVE_TYPE_GLO && menu_saveOptions.state > SAVE_STATE_EDIT_TYPE))
      {
      //arrow before parameter
         editDisplayBuffer[1][13]= ARROW_SIGN;
      //visibleCursor = VIS_CURS(1,14,1);
      }
   } 
   else { // bottom row 
   //ok button shown for loading everything except kit and morph - which are loaded instantaneously
      if(menu_saveOptions.what != SAVE_TYPE_KIT && menu_saveOptions.what != SAVE_TYPE_MORPH) {
         memcpy_P(&editDisplayBuffer[1][14],menuText_ok,2);
         if((menu_saveOptions.state==SAVE_STATE_OK) ||
         (menu_saveOptions.what >= SAVE_TYPE_GLO && menu_saveOptions.state > SAVE_STATE_EDIT_TYPE)) {
         //arrow before parameter
            editDisplayBuffer[1][13]= ARROW_SIGN;
         //visibleCursor = VIS_CURS(1,14,1);
         }
      } 
      else { //clear ok text
         editDisplayBuffer[1][14] = 0;
         editDisplayBuffer[1][15] = 0;
      }
   }
}

//-----------------------------------------------------------------
static uint8_t has2ndPage(uint8_t menuPage)
{
	const uint8_t textType = pgm_read_byte(&menuPages[menu_activePage][menuPage].top1 + 4);
	if(textType != TEXT_EMPTY)
	{
		return 1;
	}
	else return 0;	

}
//-----------------------------------------------------------------
static uint8_t checkScrollSign(uint8_t activePage, uint8_t activeParameter)
{
	const uint8_t is2ndPage = (uint8_t)(activeParameter>3);

	//**GMENU show '*' when both left and right movement are possible in the global settings menu
	// show > or < as appropriate
	if(menu_activePage==MENU_MIDI_PAGE||menu_activePage==PERFORMANCE_PAGE) {
		if(is2ndPage) {
			//if we are on 2nd screen, and there are more sub-pages after this show "*" to signify both ways are available
			if((activePage < NUM_SUB_PAGES-1) && (pgm_read_byte(&menuPages[menu_activePage][activePage+1].top1) != TEXT_EMPTY))
				return '*';
			else // on 2nd screen, no sub-pages after this
				return '<';
		}
      else { // on 1st screen
			if(has2ndPage(activePage)) { // have a second screen
				if(activePage > 0)  // on first screen and there are sub-pages before this and a second screen after
					return '*';
				else
					return '>';
			} else { // don't have a second screen
				if(activePage > 0) // on first screen and have sub-pages before this but not second screen after
					return '<';
				else // on first screen, no sub-pages before... would not happen
					return 0;
			}
		}
	}

	// normal handling for other menus
	if(has2ndPage(activePage))
		return is2ndPage?'<':'>';
	else
		return 0;

}
//-----------------------------------------------------------------
void menu_setNumSamples(uint8_t num)
{
	menu_numSamples = num;
}
//-----------------------------------------------------------------
void menu_showSampleUploadProgress(uint8_t looped, uint8_t index)
{
	char number[4] = {0,0,0,0};

	/* This is called both before the command is sent and from UART progress
	   packets. Keeping it as a small shared renderer makes the initial "Started"
	   page and numbered updates use identical LCD cursor positions. A numbered
	   packet means the visible file-import phase is alive, so reset any flash
	   spinner and refresh the idle timer. index==0 is only the pre-command
	   local screen; it must not arm the idle-to-flash fallback. */
	menu_sampleFlashProgressScreenActive = 0u;
	menu_sampleFlashProgressAnimate = 0u;
	menu_sampleUploadLastProgressTick = time_sysTick;
	menu_sampleUploadProgressSeen = (uint8_t)(index != 0u);
	lcd_clear();
	lcd_home();
	if(looped)
		lcd_string_F(PSTR("Loop upload"));
	else
		lcd_string_F(PSTR("Sample upload"));

	lcd_setcursor(0,2);
	lcd_string_F(PSTR("Started"));
	if(index != 0u)
	{
		lcd_string_F(PSTR(" "));
		numtostru(number, index);
		lcd_string(number);
	}
}
//-----------------------------------------------------------------
static void menu_drawSampleUploadFlashDots(uint8_t phase)
{
	uint8_t i;
	uint8_t dotCount = (uint8_t)((phase % 3u) + 1u);

	/* Inputs: local AVR animation phase 0..2. Output: only the first three
	   cells of LCD line two are rewritten, so the wait loop can animate without
	   clearing the whole screen. */
	lcd_setcursor(0,2);
	for(i = 0u; i < 3u; i++)
	{
		if(i < dotCount)
			lcd_string_F(PSTR("."));
		else
			lcd_string_F(PSTR(" "));
	}
}
//-----------------------------------------------------------------
static void menu_showSampleUploadFlashProgress(uint8_t phase)
{
	/* Display-only transition into the final import wait screen. Input phase
	   seeds the local dot animation; output is the "Writing Flash" title plus
	   local spinner state. This function never acknowledges, cancels, or
	   completes the STM import. */
	if(!menu_sampleFlashProgressScreenActive)
	{
		lcd_clear();
		lcd_home();
		lcd_string_F(PSTR("Writing Flash"));
		menu_sampleFlashProgressScreenActive = 1u;
	}

	menu_sampleFlashProgressPhase = (uint8_t)(phase % 3u);
	menu_sampleFlashProgressLastTick = time_sysTick;
	menu_sampleFlashProgressAnimate = 1u;
	menu_drawSampleUploadFlashDots(menu_sampleFlashProgressPhase);
}
//-----------------------------------------------------------------
static void menu_tickSampleUploadFlashProgress(void)
{
	uint16_t now;

	if(!menu_sampleFlashProgressAnimate)
	{
		return;
	}

	now = time_sysTick;
	if((uint16_t)(now - menu_sampleFlashProgressLastTick) >= MENU_SAMPLE_FLASH_ANIM_TICKS)
	{
		menu_sampleFlashProgressLastTick = now;
		menu_sampleFlashProgressPhase = (uint8_t)((menu_sampleFlashProgressPhase + 1u) % 3u);
		menu_drawSampleUploadFlashDots(menu_sampleFlashProgressPhase);
	}
}
//-----------------------------------------------------------------
static void menu_maybeShowSampleUploadFlashIdle(void)
{
	uint16_t now;

	/* AVR-side fallback for the silent end of sample import.
	   Inputs: last numbered progress tick and current Timer2 tick.
	   Output: display-only transition to "Writing Flash" if progress has
	   started, no flash screen is active, and no numbered progress has arrived
	   for the idle threshold while menu_waitSampleUploadResult() is still
	   waiting. This does not acknowledge, cancel, or complete the STM import. */
	if(menu_sampleFlashProgressAnimate || !menu_sampleUploadProgressSeen)
	{
		return;
	}

	now = time_sysTick;
	if((uint16_t)(now - menu_sampleUploadLastProgressTick) >= MENU_SAMPLE_FLASH_IDLE_TICKS)
	{
		menu_showSampleUploadFlashProgress(0u);
	}
}
//-----------------------------------------------------------------
static void menu_waitSampleUploadResult(uint8_t *statusFlags)
{

	/* Wait for the explicit SAMPLE_UPLOAD_RESULT protocol packet while
	   continuing to parse progress packets and animate the local final-flash
	   screen. There is intentionally no timeout: large sorted imports and flash
	   commits can exceed a fixed AVR wall-clock limit even though the STM is
	   still working correctly. */
	while(1)
	{
		uart_checkAndParse();
		menu_maybeShowSampleUploadFlashIdle();
		menu_tickSampleUploadFlashProgress();
		if(avrComms_sampleUploadDone)
		{
			menu_sampleFlashProgressAnimate = 0u;
			menu_sampleUploadProgressSeen = 0u;
			*statusFlags = avrComms_sampleUploadStatus;
			return;
		}
	}
}
//-----------------------------------------------------------------

static void menu_displayModTargetFull(uint8_t curParmVal)
{

	//**AUTOM **VELO - determine the full name to display for edit mode
	// a letters for the category and 8 letters for the name

	if( pgm_read_word(&modTargets[curParmVal].param)==PAR_NONE ) {
		strcpy_P(&editDisplayBuffer[1][0],menuText_off);
		return;
	}

	uint8_t nameidx = pgm_read_byte(&modTargets[curParmVal].nameIdx);

	strcpy_P(&editDisplayBuffer[1][0],
		(const char PROGMEM *)(&catNames[
					 pgm_read_byte(&valueNames[nameidx].category)
										]));
	strcpy_P(&editDisplayBuffer[1][8],
		(const char PROGMEM *)(&longNames[
					pgm_read_byte(&valueNames[nameidx].longName)
										 ]));
}

static void menu_displayModTargetShort(uint8_t curParmVal, char *valueAsText, char inclVoice)
{
	//**AUTOM **VELO - render a short name for non edit mode (encoder mode)

	if( pgm_read_word(&modTargets[curParmVal].param)==PAR_NONE ) {
		memcpy_P(valueAsText,menuText_off,3);
	} else {
		uint8_t off=0;
		if(inclVoice){
			// first digit is voice #, subsequent two letters are part of short name
			// remember that curParmVal is our index into modTargets, not an actual parameter number
			valueAsText[off++]=(char)(voiceFromModTargValue(curParmVal) + '0');
		}

		const uint8_t name = pgm_read_byte(&modTargets[curParmVal].nameIdx);
		memcpy_P(&valueAsText[off],shortNames[pgm_read_byte(&valueNames[name].shortName)],(size_t)(3-off));

	}
}

//-----------------------------------------------------------------
// paint the screen when in normal parameter edit mode
// (as opposed to repainting the load save screen)
void menu_repaintGeneric()
{
	//first get the active sub-page and parameter from the menuIndex
	const uint8_t activeParameter	= menuIndex & MASK_PARAMETER; // this will be 0 to 7 for the 4 columns
	const uint8_t activePage		= (menuIndex&MASK_PAGE)>>PAGE_SHIFT; // this is the active sub page
	uint8_t curParmVal;

	// the active page
	const Page *ap=&menuPages[menu_activePage][activePage];

	if(editModeActive) //show single parameter with full name
	{
		//get address from top1-4 from activeParameter (base adress top1 + offset)
		uint8_t parName = pgm_read_byte(&ap->top1 + activeParameter);
		uint16_t parNr = pgm_read_word(&ap->bot1 + activeParameter);
      /* The single-parameter editor must display the same normal-vs-morph
         value that the encoder will edit; both now use buttonHandler_getShift()
         as the effective latched state. */
      if (buttonHandler_getShift() && parNr < END_OF_SOUND_PARAMETERS)
         curParmVal = parameters2[parNr];
      else    
		   curParmVal = parameter_values[parNr];

		// just clear the whole thing
		memset(&editDisplayBuffer[0][0],' ',16);
		memset(&editDisplayBuffer[1][0],' ',16);

		if((pgm_read_byte(&parameter_dtypes[parNr]) & 0x0f) == DTYPE_AUTOM_TARGET)
		{
			//**AUTOM --AS this is an index into modTargets now
			//Top row (which destination (1 or 2) and which voice it's targeting)
			if(menu_activePage==PERFORMANCE_PAGE){
            //Top row left -> category
			   strcpy_P(&editDisplayBuffer[0][0],
				(const char PROGMEM *)(&catNames[pgm_read_byte(&valueNames[parName].category)]));

         }
         else {
            memcpy_P(&editDisplayBuffer[0][0],PSTR("AutDst"),6);
			   numtostru(&editDisplayBuffer[0][7],(uint8_t)( parNr - PAR_P1_DEST + 1));
         }
			// bottom row is the category and long name for the parameter being targeted
			if( pgm_read_word(&modTargets[curParmVal].param)==PAR_NONE ) {
				//  OFF
				strcpy_P(&editDisplayBuffer[1][0], menuText_off);
			} else {
				memcpy_P(&editDisplayBuffer[0][9],PSTR("Voice"),5);
				//**AUTOM - determine voice number to display for edit mode
				// remember that curParmVal is our index into modTargets, not an actual parameter number
				numtostru(&editDisplayBuffer[0][15], voiceFromModTargValue(curParmVal));
				// display the full mod target
				menu_displayModTargetFull(curParmVal);

			}

		} // parameter type is automation target

		else // parameter type is not automation target
		{
			//Top row left -> category
			strcpy_P(&editDisplayBuffer[0][0],
				(const char PROGMEM *)(&catNames[pgm_read_byte(&valueNames[parName].category)]));
			//Top row right -> long name
			strcpy_P(&editDisplayBuffer[0][8],
				(const char PROGMEM *)(&longNames[pgm_read_byte(&valueNames[parName].longName)]));

			//Bottom row -> parameter value (set below)
			switch(pgm_read_byte(&parameter_dtypes[parNr]) & 0x0F)
			{
			case DTYPE_TARGET_SELECTION_VELO: //switch(parameters_dtypes[parNr] & 0x0F)
			case DTYPE_TARGET_SELECTION_LFO:
				//**LFO **VELO edit mode - display the full target (Cat and long name)
				// curParmVal is an index into modTargets
				menu_displayModTargetFull(curParmVal);
				break;
			case DTYPE_MIX_FM: //switch(parameters_dtypes[parNr] & 0x0F)
				if(curParmVal == 1)
					memcpy_P(&editDisplayBuffer[1][13], menuText_mix, 3);
				else
					memcpy_P(&editDisplayBuffer[1][13],menuText_fm,3);
				break;
			case DTYPE_ON_OFF: //switch(parameters_dtypes[parNr] & 0x0F)
				if(curParmVal == 1)
					memcpy_P(&editDisplayBuffer[1][13],menuText_on,3);
				else
					memcpy_P(&editDisplayBuffer[1][13],menuText_off,3);
				break;

			case DTYPE_MENU: //switch(parameters_dtypes[parNr] & 0x0F)
			{
				//get the used menu (upper 4 bit)
				const uint8_t menuId = (pgm_read_byte(&parameter_dtypes[parNr]) >> 4);
				// get the value name
				getMenuItemNameForValue(menuId, curParmVal, &editDisplayBuffer[1][13]);
			}
				break; // switch(parameters[parNr].dtype&0x0F) case DTYPE_MENU

			case DTYPE_PM63: //switch(parameters_dtypes[parNr] & 0x0F)
				// -63 to +63
				numtostrps(&editDisplayBuffer[1][13],(int8_t)(curParmVal - 63));
				break;

			case DTYPE_NOTE_NAME: //switch(parameters_dtypes[parNr] & 0x0F)
				//--AS note names TODO we could create another dtype but we don't want to waste them
				if(parNr >= PAR_MIDI_NOTE1 && parNr <= PAR_MIDI_NOTE7 && curParmVal==0)
					memcpy_P(&editDisplayBuffer[1][13],menuText_any,3);
				else
					setNoteName(curParmVal, &editDisplayBuffer[1][13]);
				break;

			case DTYPE_0b1: //--AS the only 0/1 is automation track, make it look 1 based
				numtostrpu(&editDisplayBuffer[1][13],(uint8_t)(curParmVal+1),' ');
				break;
			default: //switch(parameters_dtypes[parNr] & 0x0F)
				// unsigned numeric value: just print the value
			case DTYPE_0B127:
			case DTYPE_0B255:
			case DTYPE_1B16:
         case DTYPE_0B16:
			case DTYPE_0B15:
			case DTYPE_VOICE_LFO:
				if(parNr == PAR_ROLL_NOTE_OFFSET && curParmVal == 0)
					memcpy_P(&editDisplayBuffer[1][13],menuText_off,3);
				else
					numtostrpu(&editDisplayBuffer[1][13],(uint8_t)(curParmVal),' ');
				break;
			} //switch(parameters_dtypes[parNr] & 0x0F) end

		} // parameter type is not automation target

	} // if editmode active
   
   
   
   
   
	else
	{ // editmode not active - show regular menu parameters

		//check if parameters 1-4 or 5-8 are shown
		const uint8_t is2ndPage = (activeParameter>3) * 4;

		// top texts
		memcpy_P(&editDisplayBuffer[0][0],
			&shortNames[pgm_read_byte(&valueNames[pgm_read_byte(&ap->top1+is2ndPage)].shortName)],3);
		memcpy_P(&editDisplayBuffer[0][4],
			&shortNames[pgm_read_byte(&valueNames[pgm_read_byte(&ap->top2+is2ndPage)].shortName)],3);
		memcpy_P(&editDisplayBuffer[0][8],
			&shortNames[pgm_read_byte(&valueNames[pgm_read_byte(&ap->top3+is2ndPage)].shortName)],3);
		memcpy_P(&editDisplayBuffer[0][12],
			&shortNames[pgm_read_byte(&valueNames[pgm_read_byte(&ap->top4+is2ndPage)].shortName)],3);

		// capitalize selected item
		upr_three(&editDisplayBuffer[0][(activeParameter%4)*4]);

		//check if scroll sign needs to be shown
		uint8_t showScrollSign = checkScrollSign(activePage, activeParameter);
		editDisplayBuffer[0][15] = showScrollSign;

		// bottom values

		// some place to store the parameter value as text
		char valueAsText[3];

		for(uint8_t i=0;i<4;i++) { // for each of the four columns of values
			const uint16_t parNr = pgm_read_word(&ap->bot1 + i +is2ndPage);
			curParmVal = parameter_values[parNr];

			//convert the parameter uint8_t value to a 3 place char
			if(parNr==PAR_NONE) {
				memcpy_P(valueAsText,menuText_blank,3);
			} else { //not if(parNr==PAR_NONE)

				switch(pgm_read_byte(&parameter_dtypes[parNr]) & 0x0F) {
				case DTYPE_TARGET_SELECTION_VELO: //switch(parameters[parNr].dtype&0x0F)
				case DTYPE_TARGET_SELECTION_LFO: //switch(parameters[parNr].dtype&0x0F)
					//**LFO **VELO non-edit mode display short name with no numeric prefix
					menu_displayModTargetShort(curParmVal,valueAsText,0 /*dont include voice number */);
					break;
				case DTYPE_AUTOM_TARGET: //switch(parameters[parNr].dtype&0x0F)
					//**AUTOM - determine the name to display for non-edit mode
					menu_displayModTargetShort(curParmVal,valueAsText,1 /*include voice number */);
					break;
				case DTYPE_PM63: //switch(parameters[parNr].dtype&0x0F)
					// -63 to 64
					numtostrps(valueAsText,(int8_t)(curParmVal - 63));
					break;

				case DTYPE_NOTE_NAME: //switch(parameters[parNr].dtype&0x0F)
					//--AS note names
					if(parNr >= PAR_MIDI_NOTE1 && parNr <= PAR_MIDI_NOTE7 && curParmVal==0)
						// for the voice note setting, the value 0 designates Any, whereas for the
						// step note name it does not. (Any means that the step note value is sent, as opposed to
						// the voice note being sent for all triggered notes)
						memcpy_P(valueAsText,menuText_any,3);
					else
						setNoteName(curParmVal, valueAsText);
					break;

				case DTYPE_MIX_FM: //switch(parameters[parNr].dtype&0x0F)
					if(curParmVal == 1)
						memcpy_P(valueAsText,menuText_mix,3);
					else
						memcpy_P(valueAsText,menuText_fm,3);
					break;
				case DTYPE_ON_OFF: //switch(parameters[parNr].dtype&0x0F)
					if(curParmVal == 1)
						memcpy_P(valueAsText,menuText_on,3);
					else
						memcpy_P(valueAsText,menuText_off,3);
					break;
				case DTYPE_MENU: //switch(parameters[parNr].dtype&0x0F)
				{
					//get the used menu (upper 4 bit)
					const uint8_t menuId = pgm_read_byte(&parameter_dtypes[parNr]) >> 4;
					getMenuItemNameForValue(menuId, curParmVal, valueAsText);
				}
					break;

				case DTYPE_0b1: // switch(parameters[parNr].dtype&0x0F)
					//automation track 0/1 value, make it look 1 based
					numtostrpu(valueAsText,(uint8_t)(curParmVal+1),' ');
					break;
				default: //switch(parameters[parNr].dtype&0x0F)
				case DTYPE_0B127:
				case DTYPE_0B255:
				case DTYPE_1B16:
            case DTYPE_0B16:
				case DTYPE_0B15:
				case DTYPE_VOICE_LFO:
					// fallthrough for the rest of the unsigned values
					if(parNr == PAR_ROLL_NOTE_OFFSET && curParmVal == 0)
						memcpy_P(valueAsText,menuText_off,3);
					else
						numtostrpu(valueAsText,curParmVal,' ');
					break;
				} //switch(parameters[parNr].dtype&0x0F) end

			} // if(parNr==PAR_NONE)

			//write the text to the edit buffer
			memcpy(&editDisplayBuffer[1][4*i],valueAsText,3);

		}	// for 1 to 4 .. each menu item
	} // if editmode not active
}
//-----------------------------------------------------------------
// repaint the screen
void menu_repaint()
{

	visibleCursor = 0;
	if(menu_activePage == LOAD_PAGE)
	{
		//this is a special case because the load/save page differs from all the other pages
		menu_repaintLoadPage();

	} 
   else if(menu_activePage == SAVE_PAGE)
   {
   
      menu_repaintSavePage();
   }
   
   else {
		menu_repaintGeneric();
	}

	//now send the changes from the edit buffer to the display
	sendDisplayBuffer();
};
//-----------------------------------------------------------------
//save screen knob shortcuts



void menu_handleSaveScreenKnobValue(uint8_t potNr, uint8_t value)
{
   uint8_t x=value;
   switch(potNr)
   {       
      case 0: // knob 1 actions // selects menu_saveOptions.what
            // kit, pattern, perform, all, settings
         x = value/(256/NUM_SAVE_TYPES);
         switch(x){
            case SAVE_TYPE_DRUM1:
            case SAVE_TYPE_DRUM2:
            case SAVE_TYPE_DRUM3:
            case SAVE_TYPE_SNARE:
            case SAVE_TYPE_CYM:
            case SAVE_TYPE_HIHAT:
            case SAVE_TYPE_KIT:
               menu_saveOptions.what=SAVE_TYPE_KIT;
               DISABLE_CONV_WARNING
               preset_loadName(menu_currentPresetNr[menu_saveOptions.what], menu_saveOptions.what,1);
               END_DISABLE_CONV_WARNING
               break;
            case SAVE_TYPE_MORPH:
               menu_saveOptions.what=SAVE_TYPE_MORPH;
               DISABLE_CONV_WARNING
               preset_loadName(menu_currentPresetNr[menu_saveOptions.what], menu_saveOptions.what,1);
               END_DISABLE_CONV_WARNING
               break;
            case SAVE_TYPE_PATTERN:
               menu_saveOptions.what=SAVE_TYPE_PATTERN;
               DISABLE_CONV_WARNING
               preset_loadName(menu_currentPresetNr[menu_saveOptions.what], menu_saveOptions.what,1);
               END_DISABLE_CONV_WARNING
               break;
            case SAVE_TYPE_SAMPLES:
            case SAVE_TYPE_GLO:
               menu_saveOptions.what=SAVE_TYPE_GLO;
               break;
            case SAVE_TYPE_PERFORMANCE:
               menu_saveOptions.what=SAVE_TYPE_PERFORMANCE;
               DISABLE_CONV_WARNING
               preset_loadName(menu_currentPresetNr[menu_saveOptions.what], menu_saveOptions.what,1);
               END_DISABLE_CONV_WARNING
               break;
            case SAVE_TYPE_ALL:
               menu_saveOptions.what=SAVE_TYPE_ALL;
               DISABLE_CONV_WARNING
               preset_loadName(menu_currentPresetNr[menu_saveOptions.what], menu_saveOptions.what,1);
               END_DISABLE_CONV_WARNING
               break;
            default:
               break;
         }
         break;
         
            
      case 1: // knob 2 actions // selects menu_saveOptions.state
                    // choose to edit type, preset number, alphanums
         switch(menu_saveOptions.what) {
                  case SAVE_TYPE_KIT:
                  case SAVE_TYPE_MORPH:
                  case SAVE_TYPE_PERFORMANCE:
                  case SAVE_TYPE_ALL:
                  case SAVE_TYPE_PATTERN:
               x=value/(256/SAVE_STATE_OK);
               break;
            case SAVE_TYPE_GLO: // global doesn't have a name
               if(value<127)
               {
                  x=SAVE_STATE_EDIT_TYPE;
               }
               else
               {
                  x=SAVE_STATE_OK;
               }
               break;
            default:
               break;
                     
         }
         DISABLE_CONV_WARNING
         menu_saveOptions.state=x;
         END_DISABLE_CONV_WARNING
         break;
               
         
      case 2: // knob 3 actions  // edit the .state
         switch(menu_saveOptions.state) {
            case SAVE_STATE_EDIT_TYPE:
                  // type is kit, pattern, perf, all, settings
               editModeActive=1;
               x=value/(256/6); // there are 6 options for save type
               switch(x){
                  case 0:
                     menu_saveOptions.what=SAVE_TYPE_KIT;
                     DISABLE_CONV_WARNING
               preset_loadName(menu_currentPresetNr[menu_saveOptions.what], menu_saveOptions.what,1);
               END_DISABLE_CONV_WARNING
                     break;
                  case 1:
                     menu_saveOptions.what=SAVE_TYPE_MORPH;
                     DISABLE_CONV_WARNING
               preset_loadName(menu_currentPresetNr[menu_saveOptions.what], menu_saveOptions.what,1);
               END_DISABLE_CONV_WARNING
                     break;
                  case 2:
                     menu_saveOptions.what=SAVE_TYPE_PATTERN;
                     DISABLE_CONV_WARNING
               preset_loadName(menu_currentPresetNr[menu_saveOptions.what], menu_saveOptions.what,1);
               END_DISABLE_CONV_WARNING
                     break;
                  case 3:
                     menu_saveOptions.what=SAVE_TYPE_PERFORMANCE;
                     DISABLE_CONV_WARNING
               preset_loadName(menu_currentPresetNr[menu_saveOptions.what], menu_saveOptions.what,1);
               END_DISABLE_CONV_WARNING
                     break;
                  case 4:
                     menu_saveOptions.what=SAVE_TYPE_ALL;
                     DISABLE_CONV_WARNING
               preset_loadName(menu_currentPresetNr[menu_saveOptions.what], menu_saveOptions.what,1);
               END_DISABLE_CONV_WARNING
                     break;
                  case 5:
                     menu_saveOptions.what=SAVE_TYPE_GLO;
                     break;
               }
               break;
            case SAVE_STATE_EDIT_PRESET_NR:
                     // global has no preset number
               if(menu_saveOptions.what!=SAVE_TYPE_GLO)
               {
                  x=value/2;
                  editModeActive=1;
                  menu_currentPresetNr[menu_saveOptions.what]=x;
                  DISABLE_CONV_WARNING
                  preset_loadName(menu_currentPresetNr[menu_saveOptions.what], menu_saveOptions.what,1);
                  END_DISABLE_CONV_WARNING
               }
               else // if it is global, how did we get here?
                          // put the cursor back on type, please
               {
                  editModeActive=0;
                  menu_saveOptions.state=SAVE_STATE_EDIT_TYPE;
               }
               break;
                  case SAVE_STATE_EDIT_NAME1:
                  case SAVE_STATE_EDIT_NAME2:
                  case SAVE_STATE_EDIT_NAME3:
                  case SAVE_STATE_EDIT_NAME4:
                  case SAVE_STATE_EDIT_NAME5:
                  case SAVE_STATE_EDIT_NAME6:
                  case SAVE_STATE_EDIT_NAME7:
                  case SAVE_STATE_EDIT_NAME8:
               editModeActive=1;
               //x=(uint8_t)((value/(256/128))+128);
               
               x=(uint8_t)(value/(256/128));
               if(x<32) // put upper case letters at the front of the list
               x=(uint8_t)(x+65);
               else if(x<65) // put common characters, then numbers next
               x=(uint8_t)(x-32+32);
               else if(x<95)
               x=(uint8_t)(x-64);// the weird characters from the start of the list
               else
               x=(uint8_t)(x-95+224);
               
               DISABLE_CONV_WARNING
               preset_currentSaveMenuName[menu_saveOptions.state - SAVE_STATE_EDIT_NAME1] = x;
               END_DISABLE_CONV_WARNING
               break;
            default:
               break; // does nothing on 'ok' - you have to click the encoder
                     
         }
         break;
               
      case 3: // knob 4 actions  // for alphanums only - second bank 
         switch(menu_saveOptions.state) {
                  case SAVE_STATE_EDIT_NAME1:
                  case SAVE_STATE_EDIT_NAME2:
                  case SAVE_STATE_EDIT_NAME3:
                  case SAVE_STATE_EDIT_NAME4:
                  case SAVE_STATE_EDIT_NAME5:
                  case SAVE_STATE_EDIT_NAME6:
                  case SAVE_STATE_EDIT_NAME7:
                  case SAVE_STATE_EDIT_NAME8:
               editModeActive=1;
               //x=(uint8_t)(value/(256/128));
               
               x=(uint8_t)(value/(256/128));
               if (x<127)//(x<26) // put lower case letters at the front of the list
               x=(uint8_t)(x+97);
               else
               x=x;
               
               
               DISABLE_CONV_WARNING
               preset_currentSaveMenuName[menu_saveOptions.state - SAVE_STATE_EDIT_NAME1] = x;
               END_DISABLE_CONV_WARNING
               break;
            default:
                     
               break; // does nothing otherwise
                     
         }
         break;
   }	
   
}


   
//-----------------------------------------------------------------  

// load screen knob shortcuts


void menu_handleLoadScreenKnobValue(uint8_t potNr, uint8_t value)
{
   uint8_t x=value;
   switch(potNr)
   {
      case 0: // selects menu_saveOptions.what
           // kit, pattern, perform, all, settings
         x = value/(256/NUM_SAVE_TYPES);
         switch(x){
            case SAVE_TYPE_KIT:
               menu_saveOptions.what=SAVE_TYPE_KIT;
               break;
            case SAVE_TYPE_MORPH:
               menu_saveOptions.what=SAVE_TYPE_MORPH;
               break;
            case SAVE_TYPE_DRUM1:
               menu_saveOptions.what=SAVE_TYPE_DRUM1;
               break;
            case SAVE_TYPE_DRUM2:
               menu_saveOptions.what=SAVE_TYPE_DRUM2;
               break;
            case SAVE_TYPE_DRUM3:
               menu_saveOptions.what=SAVE_TYPE_DRUM3;
               break;
            case SAVE_TYPE_SNARE:
               menu_saveOptions.what=SAVE_TYPE_SNARE;
               break;
            case SAVE_TYPE_CYM:
               menu_saveOptions.what=SAVE_TYPE_CYM;
               break;
            case SAVE_TYPE_HIHAT:
               menu_saveOptions.what=SAVE_TYPE_HIHAT;
               break;
            case SAVE_TYPE_PATTERN:
               menu_saveOptions.what=SAVE_TYPE_PATTERN;
               break;
            case SAVE_TYPE_GLO:
               menu_saveOptions.what=SAVE_TYPE_GLO;
               break;
            case SAVE_TYPE_PERFORMANCE:
               menu_saveOptions.what=SAVE_TYPE_PERFORMANCE;
               break;
            case SAVE_TYPE_ALL:
               menu_saveOptions.what=SAVE_TYPE_ALL;
               break;
            case SAVE_TYPE_SAMPLES:
               menu_saveOptions.what=SAVE_TYPE_SAMPLES;
               break;
            default:
               break;
         }
         break;
      case 1: // on load screen, always selects preset number
              // if this is an option, auto-load if a voice thing
              // and the preset number is bracketed
         x=value/2;
         switch(menu_saveOptions.what) {
            case SAVE_TYPE_KIT:
               menu_currentPresetNr[menu_saveOptions.what]=x;
               if(editModeActive){
                  preset_loadDrumset(menu_currentPresetNr[menu_saveOptions.what],0x7f, 0);
                  menu_TargetVoiceGapIndex = getModTargetGapIndex(parameter_values[PAR_TARGET_LFO1 + menu_activeVoice]);
               }
               break;
            case SAVE_TYPE_DRUM1:
               menu_currentPresetNr[menu_saveOptions.what]=x;
               if(editModeActive){
                  preset_loadDrumset(menu_currentPresetNr[menu_saveOptions.what],0x01,0);
               }
               break;
            case SAVE_TYPE_DRUM2:
               menu_currentPresetNr[menu_saveOptions.what]=x;
               if(editModeActive){
                  preset_loadDrumset(menu_currentPresetNr[menu_saveOptions.what],0x02,0);
               }
               break;
            case SAVE_TYPE_DRUM3:
               menu_currentPresetNr[menu_saveOptions.what]=x;
               if(editModeActive){
                  preset_loadDrumset(menu_currentPresetNr[menu_saveOptions.what],0x04,0);
               }
               break;
            case SAVE_TYPE_SNARE:
               menu_currentPresetNr[menu_saveOptions.what]=x;
               if(editModeActive){
                  preset_loadDrumset(menu_currentPresetNr[menu_saveOptions.what],0x08,0);
               }
               break;
            case SAVE_TYPE_CYM:
               menu_currentPresetNr[menu_saveOptions.what]=x;
               if(editModeActive){
                  preset_loadDrumset(menu_currentPresetNr[menu_saveOptions.what],0x10,0);
               }
               break;
            case SAVE_TYPE_HIHAT:
                     // special case - instead of a bank we send literal bits to change both
                     // closed and open hi-hat voices
               menu_currentPresetNr[menu_saveOptions.what]=x;
               if(editModeActive){
                  preset_loadDrumset(menu_currentPresetNr[menu_saveOptions.what],0x60,0);
               }
               break;
            case SAVE_TYPE_MORPH:
                     //load to morph buffer
               menu_currentPresetNr[menu_saveOptions.what]=x;
               if(editModeActive){
                  preset_loadDrumset(menu_currentPresetNr[menu_saveOptions.what],0x7f,1);
               }
               break;
            case SAVE_TYPE_PERFORMANCE:
            case SAVE_TYPE_ALL:
            case SAVE_TYPE_PATTERN:
               menu_currentPresetNr[menu_saveOptions.what]=x;
         }
               
         switch(menu_saveOptions.what){
            case SAVE_TYPE_KIT:
            case SAVE_TYPE_DRUM1:
            case SAVE_TYPE_DRUM2:
            case SAVE_TYPE_DRUM3:
            case SAVE_TYPE_SNARE:
            case SAVE_TYPE_CYM:
            case SAVE_TYPE_HIHAT:
            case SAVE_TYPE_PERFORMANCE:
            case SAVE_TYPE_ALL:
            case SAVE_TYPE_PATTERN:
               menu_currentPresetNr[menu_saveOptions.what]=x;
               break;
            default:
               break;
         }
         break;
      case 2: // this does nothing but disable quickload on the load screen
         editModeActive=0;
         break;
      case 3: // this jumps between .what and 'ok' if it exists
         editModeActive=0;
         x=value/127;
         switch(menu_saveOptions.what) {
            case SAVE_TYPE_PATTERN:
            case SAVE_TYPE_PERFORMANCE:
            case SAVE_TYPE_ALL:
            case SAVE_TYPE_GLO:
            case SAVE_TYPE_SAMPLES:
               if (x){
                  menu_saveOptions.state=SAVE_STATE_OK;
               }
               else{
                  menu_saveOptions.state=SAVE_STATE_EDIT_TYPE;
               }
               break;
            default:
               menu_saveOptions.state=SAVE_STATE_EDIT_TYPE;
               break;   
         }
         break;
   }	

}




//-----------------------------------------------------------------
void menu_handleLoadMenu(int8_t inc, uint8_t btnClicked)
{
	//this is a special case because the load/save page differs from all the other pages
	// when this is called, edit mode will have already been set or cleared. this is only
	// called when something has changed

	//---- handle the button ----
   if(btnClicked) {
   	//if the ok button is active or we are in global save and ok is active, 
      // save/load the preset on click
      if( (editModeActive && menu_saveOptions.state == SAVE_STATE_OK) ||
      	(menu_saveOptions.what >= SAVE_TYPE_GLO && menu_saveOptions.state > SAVE_STATE_EDIT_TYPE) ) {
      	//load page 
         switch(menu_saveOptions.what) {
            
            
            case SAVE_TYPE_PATTERN:
               preset_loadPattern(menu_currentPresetNr[SAVE_TYPE_PATTERN],menu_voiceArray);
               menu_resetSaveParameters();
               break;
            
            case SAVE_TYPE_PERFORMANCE:
               //preset_loadAll(menu_currentPresetNr[SAVE_TYPE_ALL],0,0,menu_voiceArray);//last 0 is don't release kit lock
               preset_loadPerf(menu_currentPresetNr[SAVE_TYPE_PERFORMANCE],menu_voiceArray);//last 0 is don't release kit lock
               menu_resetSaveParameters();
               // buttonHandler_handleModeButtons(SELECT_MODE_PERF);
               break;
            
            case SAVE_TYPE_ALL:
               preset_loadAll(menu_currentPresetNr[SAVE_TYPE_ALL],menu_voiceArray);
               //preset_loadAll(menu_currentPresetNr[SAVE_TYPE_ALL],1,0,menu_voiceArray);//last 0 is don't release kit lock
               menu_resetSaveParameters();
               break;
            
            case SAVE_TYPE_GLO:
               preset_loadGlobals();
               menu_resetSaveParameters();						
               break;
            
            case SAVE_TYPE_SAMPLES:
               /* The SD card is normally AVR-owned. Release AVR SPI before the
                  STM import command so the STM sample loader can temporarily
                  access the card without bus contention. */
               spi_deInit();
               
               //Display load message
               menu_showSampleUploadProgress(0u, 0u);
               /* Let the LCD settle before the STM begins the blocking flash
                  operation; without this, the final character in "Sample" could
                  be missed visually on the first screen. */
               _delay_ms(250);
               //send load sample command to mainboard
               /* Clear the result latch before sending START_UPLOAD so a stale
                  result from an earlier import cannot end the wait immediately. */
               avrComms_sampleUploadDone = 0u;
               avrComms_sampleUploadStatus = SAMPLE_UPLOAD_STATUS_OK;
               avrComms_sendData(SAMPLE_CC,SAMPLE_START_UPLOAD,0x00);
               {
                  uint8_t sampleStatus = SAMPLE_UPLOAD_STATUS_OK;
                  menu_waitSampleUploadResult(&sampleStatus);

                  if(sampleStatus & SAMPLE_UPLOAD_STATUS_COUNT_LIMIT)
                  {
                     lcd_clear();
                     lcd_home();
                     lcd_string_F(PSTR("Sample upload"));
                     lcd_setcursor(0,2);
                     lcd_string_F(PSTR("248 max exceeded"));
                     _delay_ms(1000);
                  }

                  if(sampleStatus & SAMPLE_UPLOAD_STATUS_FLASH_LIMIT)
                  {
                     lcd_clear();
                     lcd_home();
                     lcd_string_F(PSTR("Sample upload"));
                     lcd_setcursor(0,2);
                     lcd_string_F(PSTR("Flash exceeded"));
                     _delay_ms(1000);
                  }

                  if(sampleStatus & SAMPLE_UPLOAD_STATUS_READ_ERROR)
                  {
                     lcd_clear();
                     lcd_home();
                     lcd_string_F(PSTR("Sample upload"));
                     lcd_setcursor(0,2);
                     lcd_string_F(PSTR("Read error"));
                     _delay_ms(1000);
                  }
               }
               //re-initialize SD-Card
               preset_init();
               //redraw screen
               // menu_repaintAll(); --AS screen will be repainted later, relax!
               
               avrComms_sendData(SAMPLE_CC,SAMPLE_COUNT,0x00);
               break;
            
            
            default:
               break;
         }
      
      } // if ok button active
   } // btnClicked

	//---- handle the encoder ----
   if(editModeActive) {
   	//encoder changes value
   	///**< 0=edit kit/pat, 1=edit preset nr, 2 = edit name*/
      switch(menu_saveOptions.state) {
      
         case 0: //switch(menu_saveOptions.state) - edit kit/Pat
          // all the items are active on the load menu, don't have to exclude any
            if(inc<0) {
               if(menu_saveOptions.what!=0) {
                  menu_saveOptions.what--;
               }
            } 
            else if(inc>0) {
               if(menu_saveOptions.what < SAVE_TYPE_SAMPLES) {
                  menu_saveOptions.what++;
               }
            }
            DISABLE_CONV_WARNING
            preset_loadName(menu_currentPresetNr[menu_saveOptions.what],menu_saveOptions.what,0);
            END_DISABLE_CONV_WARNING
            break;
      
         case 1: //switch(menu_saveOptions.state) - edit preset nr
            if(inc<0) {
               if(menu_currentPresetNr[menu_saveOptions.what]+inc>=0)	{
               DISABLE_CONV_WARNING
                  menu_currentPresetNr[menu_saveOptions.what] += inc;
               END_DISABLE_CONV_WARNING
               }
            } 
            else if(inc>0) {
               if(menu_currentPresetNr[menu_saveOptions.what]<=125) {
               DISABLE_CONV_WARNING
                  menu_currentPresetNr[menu_saveOptions.what] += inc;
               END_DISABLE_CONV_WARNING
               }
            }
         //if on load page, load the new preset when the preset number is changed
            if((inc!=0)) {
            
            // always load the name regardless of what else
               DISABLE_CONV_WARNING
               preset_loadName(menu_currentPresetNr[menu_saveOptions.what], menu_saveOptions.what,0);
               END_DISABLE_CONV_WARNING
            
          
               // if loading and type is kit and morph do an insta-load
                  switch(menu_saveOptions.what) {
                     case SAVE_TYPE_KIT:
                        preset_loadDrumset(menu_currentPresetNr[menu_saveOptions.what],0x7f, 0);
                        menu_TargetVoiceGapIndex = getModTargetGapIndex(parameter_values[PAR_TARGET_LFO1 + menu_activeVoice]);
                        break;
                     case SAVE_TYPE_DRUM1:
                        preset_loadDrumset(menu_currentPresetNr[menu_saveOptions.what],0x01,0);
                        break;
                     case SAVE_TYPE_DRUM2:
                        preset_loadDrumset(menu_currentPresetNr[menu_saveOptions.what],0x02,0);
                        break;
                     case SAVE_TYPE_DRUM3:
                        preset_loadDrumset(menu_currentPresetNr[menu_saveOptions.what],0x04,0);
                        break;
                     case SAVE_TYPE_SNARE:
                        preset_loadDrumset(menu_currentPresetNr[menu_saveOptions.what],0x08,0);
                        break;
                     case SAVE_TYPE_CYM:
                        preset_loadDrumset(menu_currentPresetNr[menu_saveOptions.what],0x10,0);
                        break;
                     case SAVE_TYPE_HIHAT:
                     // special case - instead of a bank we send literal bits to change both
                     // closed and open hi-hat voices
                        preset_loadDrumset(menu_currentPresetNr[menu_saveOptions.what],0x60,0);
                        break;
                     case SAVE_TYPE_MORPH:
                     //load to morph buffer
                        preset_loadDrumset(menu_currentPresetNr[menu_saveOptions.what],0x7f, 1);
                        break;
                  }
               
            }
            break; //switch(menu_saveOptions.state) - edit preset nr
    
      
         default: //switch(menu_saveOptions.state)
            break;
      } //switch(menu_saveOptions.state)
   } 
   else { //editModeActive is false
   	//encoder selects value to change
      if(inc<0) {
         if(menu_saveOptions.state != SAVE_STATE_EDIT_TYPE) {
            menu_saveOptions.state --;
         	//no name edit on load page
            if(menu_saveOptions.state >= SAVE_STATE_EDIT_NAME1) {
               menu_saveOptions.state = SAVE_STATE_EDIT_PRESET_NR;
            }
         }
      } 
      else if(inc>0) {
         if(menu_saveOptions.state < SAVE_STATE_OK) {
            menu_saveOptions.state ++;
            	//no name edit on load page
            if(menu_saveOptions.state >= SAVE_STATE_EDIT_NAME1 ) {
               menu_saveOptions.state = SAVE_STATE_OK;
            }
            
            	// ok button doesnt exist on kit and morph
            if(menu_saveOptions.state == SAVE_STATE_OK ) {
               if(menu_saveOptions.what == SAVE_TYPE_KIT ||
                  	menu_saveOptions.what == SAVE_TYPE_DRUM1||
                  	menu_saveOptions.what == SAVE_TYPE_DRUM2||
                  	menu_saveOptions.what == SAVE_TYPE_DRUM3||
                  	menu_saveOptions.what == SAVE_TYPE_SNARE||
                  	menu_saveOptions.what == SAVE_TYPE_CYM||
                  	menu_saveOptions.what == SAVE_TYPE_HIHAT||
                  	menu_saveOptions.what == SAVE_TYPE_MORPH)
                  menu_saveOptions.state = SAVE_STATE_EDIT_PRESET_NR;
            }
            
            	//ok button only for load pattern
            	//if( (menu_saveOptions.what != SAVE_TYPE_PATTERN) &&
            	//	(menu_saveOptions.what >= SAVE_TYPE_GLO)  && menu_saveOptions.state == SAVE_STATE_OK ) {
            	//	menu_saveOptions.state = SAVE_STATE_EDIT_PRESET_NR;
            	//}
            
         }
      } // inc<0
   } //editModeActive
}


void menu_handleSaveMenu(int8_t inc, uint8_t btnClicked)
{
	//this is a special case because the load/save page differs from all the other pages
	// when this is called, edit mode will have already been set or cleared. this is only
	// called when something has changed

	//---- handle the button ----
   if(btnClicked) {
   	//if the ok button is active or we are in global save and ok is active, save/load the preset on click
      if( (editModeActive && menu_saveOptions.state == SAVE_STATE_OK) ||
      	(menu_saveOptions.what >= SAVE_TYPE_GLO && menu_saveOptions.state > SAVE_STATE_EDIT_TYPE) ) {
      
         
         switch(menu_saveOptions.what) {
            case SAVE_TYPE_PATTERN:
               preset_savePattern(menu_currentPresetNr[SAVE_TYPE_PATTERN]);
               break;
            
            case SAVE_TYPE_KIT:
               preset_saveDrumset(menu_currentPresetNr[SAVE_TYPE_KIT],0);
               break;
               
            case SAVE_TYPE_MORPH:
               preset_saveDrumset(menu_currentPresetNr[SAVE_TYPE_MORPH],1);
               break;
            
            case SAVE_TYPE_GLO:
               preset_saveGlobals();
               break;
            
            case SAVE_TYPE_PERFORMANCE:
               preset_saveAll(menu_currentPresetNr[SAVE_TYPE_PERFORMANCE],0);
               break;
            
            case SAVE_TYPE_ALL:
               preset_saveAll(menu_currentPresetNr[SAVE_TYPE_ALL],1);
               break;
         }
         menu_resetSaveParameters();						
      } // if ok button active
      
   } // btnClicked

	//---- handle the encoder ----
   if(editModeActive) {
   	//encoder changes value
   	///**< 0=edit kit/pat, 1=edit preset nr, 2 = edit name*/
      switch(menu_saveOptions.state) {
         case 0: //switch(menu_saveOptions.state) - edit kit/Pat
            if(inc<0) {
               if(menu_saveOptions.what!=0) {
                  switch (menu_saveOptions.what){
                     case SAVE_TYPE_MORPH:
                        
                        /*menu_saveOptions.what=SAVE_TYPE_MORPH;*/
                        // skip instruments- can't save, go straight to kit
                        menu_saveOptions.what=SAVE_TYPE_KIT;
                        break;
                     default:
                        menu_saveOptions.what--;
                        break;
                  }
               }
            } 
            else if(inc>0) {
               if(menu_saveOptions.what < SAVE_TYPE_SAMPLES) {
                  switch (menu_saveOptions.what){
                     case SAVE_TYPE_KIT:
                        // -bc- save for later
                        /*menu_saveOptions.what=SAVE_TYPE_MORPH;*/
                        // skip instruments- can't save, go straight to pat
                        menu_saveOptions.what=SAVE_TYPE_MORPH;
                        break;
                     case SAVE_TYPE_GLO:
                        // can't save samples, don't go above global settings
                        break;
                     default:
                        menu_saveOptions.what++;
                        break;
                  }
               }
            }
         
            preset_loadName(menu_currentPresetNr[menu_saveOptions.what],(uint8_t)menu_saveOptions.what,1);
            break;
      
         case 1:
         //user is on the preset (number) item 
         //switch(menu_saveOptions.state) - edit preset nr
            if(inc<0) {
               if(menu_currentPresetNr[menu_saveOptions.what]+inc>=0)	{
               DISABLE_CONV_WARNING
                  menu_currentPresetNr[menu_saveOptions.what] += inc;
               END_DISABLE_CONV_WARNING
               }
               DISABLE_CONV_WARNING
               preset_loadName(menu_currentPresetNr[menu_saveOptions.what], menu_saveOptions.what,1);
               END_DISABLE_CONV_WARNING
            } 
            else if(inc>0) {
               if(menu_currentPresetNr[menu_saveOptions.what]<=125) {
               DISABLE_CONV_WARNING
                  menu_currentPresetNr[menu_saveOptions.what] += inc;
               END_DISABLE_CONV_WARNING
               }
               DISABLE_CONV_WARNING
               preset_loadName(menu_currentPresetNr[menu_saveOptions.what], menu_saveOptions.what,1);
               END_DISABLE_CONV_WARNING
            }

            break; //switch(menu_saveOptions.state) - edit preset nr
      
      //edit name
         case 2: // switch(menu_saveOptions.state) char 1
         case 3: // switch(menu_saveOptions.state) char 2
         case 4: // switch(menu_saveOptions.state) char 3
         case 5: // switch(menu_saveOptions.state) char 4
         case 6: // switch(menu_saveOptions.state) char 5
         case 7: // switch(menu_saveOptions.state) char 6
         case 8: // switch(menu_saveOptions.state) char 7
         case 9: // switch(menu_saveOptions.state) char 8
         DISABLE_CONV_WARNING
            preset_currentSaveMenuName[menu_saveOptions.state - SAVE_STATE_EDIT_NAME1] += inc;
         END_DISABLE_CONV_WARNING
            break;
      
         default: //switch(menu_saveOptions.state)
            break;
      } //switch(menu_saveOptions.state)
   } 
   else { //editModeActive is false
   	//encoder selects value to change
      if(inc<0) {
         if(menu_saveOptions.state != SAVE_STATE_EDIT_TYPE) {
            menu_saveOptions.state --;
         }
      } 
      else if(inc>0) {
         if (menu_saveOptions.state < SAVE_STATE_OK)
         {
         menu_saveOptions.state ++;
         }
         
      } // inc<0
   } //editModeActive
}

// given a menu id returns the number of entries
static uint8_t getMaxEntriesForMenu(uint8_t menuId)
{
	switch(menuId) // clever GCC. no need to use pgm_read_byte
	{
	case MENU_TRANS:
		return transientNames[0][0];
	case MENU_AUDIO_OUT:
		return outputNames[0][0];
	case MENU_FILTER:
		return filterTypes[0][0];
	case MENU_SYNC_RATES:
		return syncRateNames[0][0];
	case MENU_LFO_WAVES:
		return lfoWaveNames[0][0];
	case MENU_RETRIGGER:
		return retriggerNames[0][0];
	case MENU_SEQ_QUANT:
		return quantisationNames[0][0];
	/*case MENU_MIDI:
		return 2;//midiModes[0][0]; - is overridden for adding roll menu*/
	case MENU_NEXT_PATTERN:
		return nextPatternNames[0][0];
	case MENU_WAVEFORM:
		return (uint8_t)(waveformNames[0][0] + menu_numSamples);
	case MENU_ROLL_RATES:
		return rollRateNames[0][0];
	case MENU_MIDI_ROUTING:
		return midiRoutingNames[0][0];
	case MENU_MIDI_FILTERING:
		return midiFilterNames[0][0];
	case MENU_PPQ:
		return ppqNames[0][0];
   case MENU_TRACK_SCALE:
      return trackScaleNames[0][0];
   case MENU_FILE_LOAD_BACKGROUND:
      return backgroundLoadNames[0][0];
   case MENU_MIDI:
      return midiModes[0][0];
	default:
		return 0;
	}
}

// given a menu id, and a value returns the name from PROGMEM
static void getMenuItemNameForValue(const uint8_t menuId, const uint8_t curParmVal, char *buf)
{
	const void *p=0;
	switch(menuId)
	{
	case MENU_TRANS:
		p = transientNames[curParmVal + 1];
		break;
	case MENU_AUDIO_OUT:
		p = outputNames[curParmVal + 1];
		break;
	case MENU_FILTER:
		p = filterTypes[curParmVal + 1];
		break;
	case MENU_WAVEFORM:
		if(curParmVal < waveformNames[0][0])
			p=waveformNames[curParmVal + 1];
		else
		{
			buf[0]='s';
			buf[2]=' ';
			numtostru(&buf[1],(uint8_t)(curParmVal - waveformNames[0][0]));
			return;
		}
		break;

	case MENU_SYNC_RATES:
		p=syncRateNames[curParmVal + 1];
		break;
	case MENU_LFO_WAVES:
		p=lfoWaveNames[curParmVal+1];
		break;
	case MENU_RETRIGGER:
		p=retriggerNames[curParmVal+1];
		break;
	case MENU_SEQ_QUANT:
		p=quantisationNames[curParmVal+1];
		break;
	/*case MENU_MIDI:
		p=midiModes[curParmVal+1];
		break;*/
   case MENU_MIDI:
		p=midiModes[curParmVal+1];
      break;
	case MENU_NEXT_PATTERN:
		p=nextPatternNames[curParmVal+1];
		break;
	case MENU_ROLL_RATES:
		p=rollRateNames[curParmVal+1];
		break;
	case MENU_MIDI_ROUTING:
		p=midiRoutingNames[curParmVal+1];
		break;
	case MENU_MIDI_FILTERING:
		p=midiFilterNames[curParmVal+1];
		break;
	case MENU_PPQ:
		p=ppqNames[curParmVal+1];
		break;
   case MENU_TRACK_SCALE:
      p=trackScaleNames[curParmVal+1];
      break;
   case MENU_FILE_LOAD_BACKGROUND:
      p=backgroundLoadNames[curParmVal+1];
      break;
	default:
		p=menuText_dash;
		break;
	}
	memcpy_P(buf,p,3);

}

//-----------------------------------------------------------------
// called when either the encoder is rotated, or the encoder button is clicked
// inc - the number of clicks the encoder has moved
// button - 1 if the button is depressed, 0 if its not
void menu_parseEncoder(int8_t inc, uint8_t button)
{
	uint8_t btnClicked=0;
	// handle the button being clicked or released
	if(button != lastEncoderButton) { // was the button clicked?
		btnClicked=button;
      screensaver_touch();
      if(btnClicked && copyClear_isClearModeActive())
      {
         lastEncoderButton = button;
         copyClear_executeClear();
         return;
      }
		if(btnClicked) // toggle edit mode
			editModeActive = (uint8_t)(1-editModeActive);
		lastEncoderButton = button;
	} else if(inc==0)
		return; // nothing has changed. do nothing

	

	inc = (int8_t)(inc * -1);
   if (inc != 0)
   {
      screensaver_touch();
   }

	if(menu_activePage == LOAD_PAGE)
   {
		menu_handleLoadMenu(inc, btnClicked);
   }
   else if (menu_activePage == SAVE_PAGE)
   {
      menu_handleSaveMenu(inc, btnClicked);
   }
	else if(inc!=0) {
		//============================= handle encoder change ==============================
		if(copyClear_isClearModeActive()) {
			//encoder selects clear target
			uint8_t target = copyClear_getClearTarget();
			if(inc<0) {
				if(target!=0) {
					target--;
				}
			} else if(inc>0) {
				if(target!=CLEAR_AUTOMATION2) {
					target++;
				}
			}
			copyClear_setClearTarget(target);
			return;

		} else if(editModeActive) {
			// edit mode is active so change the value of the current parameter
			{
				/* -bc- 027: DTYPE_MENU parameters are list indices.
				   Acceleration should skip entries one at a time,
				   so bypass the multiplier for text-list types. */
				const uint8_t activeParameter = menuIndex & MASK_PARAMETER;
				const uint8_t activePage      = (menuIndex & MASK_PAGE) >> PAGE_SHIFT;
				const uint16_t parNr = pgm_read_word(
					&menuPages[menu_activePage][activePage].bot1 + activeParameter);
				const uint8_t dtype = pgm_read_byte(&parameter_dtypes[parNr]) & 0x0F;
				if (dtype != DTYPE_MENU)
					inc = menu_applyEncoderAcceleration(inc);
			}
         if (buttonHandler_getShift())
            menu_encoderChangeShiftParameter(inc);
         else
			   menu_encoderChangeParameter(inc);
		} else {
			//edit mode not active so encoder selects active parameter
			menu_moveToMenuItem(inc);
		}

	}
	menu_repaintAll();
}


static int8_t menu_applyEncoderAcceleration(int8_t inc)
{
   const uint8_t multiplier = encode_getAccelerationMultiplier();
   int16_t accelerated;

   if(multiplier <= 1 || inc == 0)
      return inc;

   accelerated = (int16_t)inc * multiplier;

   if(accelerated > INT8_MAX)
      return INT8_MAX;
   if(accelerated < INT8_MIN)
      return INT8_MIN;

   return (int8_t)accelerated;
}


static void menu_applyEncoderDeltaToByte(uint8_t *paramValue, int8_t inc)
{
   int16_t value = *paramValue;

   value += inc;

   if(value < 0)
      value = 0;
   else if(value > 255)
      value = 255;

   *paramValue = (uint8_t)value;
}

/* ENCODER DTYPE_MENU ENTRY COUNT
   WHAT: number of selectable entries for a DTYPE_MENU parameter on the
         encoder edit path. Normally the menu table's count byte; RxFilter
         returns MIDI_FILTER_RX_ENTRIES (24) so the ¢ twins (16..23) are
         reachable (Session 039).
   WHY:  ¢ entries must be encoder-only. This helper is used ONLY by the
         encoder paths (menu_normalizeDtypeMenuEncoderValue() and the
         DTYPE_MENU clamps in menu_encoderChangeParameter() /
         menu_encoderChangeShiftParameter()); pots use getMaxEntriesForMenu()
         via getDtypeValue() and stay at 16. Without this override the
         normaliser would rescale any value >= 16 down and the clamp would
         cap it at 15. TxFilter shares the table but is not overridden, so it
         stays 0..15.
   INPUT:  paramNr - DTYPE_MENU parameter.
   OUTPUT: entry count (0 if the menu is unknown).
   ACCESSORS: menu_normalizeDtypeMenuEncoderValue() (2820),
              menu_encoderChangeParameter() (3058),
              menu_encoderChangeShiftParameter() (3291).
   AFFILIATES: getMaxEntriesForMenu(), midiFilterNames[] (MenuText.h). */
static uint8_t menu_getDtypeMenuEntryCount(uint16_t paramNr)
{
   const uint8_t menuId = pgm_read_byte(&parameter_dtypes[paramNr]) >> 4;

   if(paramNr == PAR_MIDI_FILT_RX)
      return MIDI_FILTER_RX_ENTRIES;

   return getMaxEntriesForMenu(menuId);
}


static void menu_normalizeDtypeMenuEncoderValue(uint16_t paramNr, uint8_t *paramValue)
{
   const uint8_t numEntries = menu_getDtypeMenuEntryCount(paramNr);

   if(numEntries == 0)
   {
      *paramValue = 0;
      return;
   }

   if(*paramValue < numEntries)
      return;

   *paramValue = (uint8_t)(((uint16_t)(*paramValue) * (uint16_t)(numEntries - 1)) / 255);
}

//-----------------------------------------------------------------
static uint8_t menu_isVoiceMorphAmountParam(uint16_t paramNr)
{
   return paramNr >= PAR_MORPH_DRUM1 && paramNr <= PAR_MORPH_HIHAT;
}

//-----------------------------------------------------------------
static void menu_syncVoiceMorphDisplayValues(uint8_t value)
{
   uint8_t voice;

   for(voice=0;voice<6;voice++)
      parameter_values[PAR_MORPH_DRUM1 + voice] = value;
}

//-----------------------------------------------------------------
static void menu_sendVoiceMorphAmount(uint16_t paramNr, uint8_t value)
{
   if(!menu_isVoiceMorphAmountParam(paramNr))
      return;

   avrComms_sendVoiceMorphValue((uint8_t)(paramNr - PAR_MORPH_DRUM1), value);
}

//-----------------------------------------------------------------
/* MENU SOUND-PARAMETER SEND  (Session 039 P3)
   WHAT: sends a sound-parameter change made with the menu encoder or a knob
         to the STM as MIDI_CC_MENU (raw id 0..127) or CC_2_MENU (raw id
         128..END_OF_SOUND_PARAMETERS-1). Ignores non-sound ids.
   WHY:  the STM applies these exactly like MIDI_CC/CC_2 and, when txf C is on,
         also transmits them as Global CC/NRPN. Using distinct opcodes keeps
         load, revert and restore traffic (still MIDI_CC/CC_2) from ever being
         transmitted. The armed-step and gate checks are on the STM.
   INPUT:  paramNr - raw id; value - 7-bit parameter value.
   OUTPUT: one 3-byte packet to the STM (same size as before).
   ACCESSORS: menu_encoderChangeParameter(), menu_parseKnobValue() (two branches).
   AFFILIATES: STM FRONT_CC_MENU / FRONT_CC_2_MENU handling
               (frontPanelReceivingProtocol.c), outputControl_queueMenuParameterTx(). */
static void menu_sendSoundParameter(uint16_t paramNr, uint8_t value)
{
   if(paramNr < 128)
      avrComms_sendData(MIDI_CC_MENU, (uint8_t)paramNr, value);
   else if(paramNr < END_OF_SOUND_PARAMETERS)
      avrComms_sendData(CC_2_MENU, (uint8_t)(paramNr - 128), value);
}

//-----------------------------------------------------------------
// called when edit mode is active
// encoder controls parameter value
// given a delta, will apply that to the current parameter
static void menu_encoderChangeParameter(int8_t inc)
{
   screensaver_touch();
	const uint8_t activeParameter	= menuIndex & MASK_PARAMETER;
	const uint8_t activePage		= (menuIndex&MASK_PAGE)>>PAGE_SHIFT;

	//get address from top1-8 from activeParameter (base adress top1 + offset)
	uint16_t paramNr		= pgm_read_word(&menuPages[menu_activePage][activePage].bot1 + activeParameter);
	uint8_t *paramValue = &parameter_values[paramNr];
   uint8_t isMorphParam = (paramNr<END_OF_SOUND_PARAMETERS&&buttonHandler_getShift());
   const uint8_t dtype = pgm_read_byte(&parameter_dtypes[paramNr]) & 0x0F;
   
   if (isMorphParam&&(menu_activePage<=VOICE7_PAGE))
   {
      paramValue = &parameters2[paramNr];
   }

   if(dtype == DTYPE_MENU)
      menu_normalizeDtypeMenuEncoderValue(paramNr, paramValue);

		menu_applyEncoderDeltaToByte(paramValue, inc);

	switch(dtype)
	{
	case DTYPE_TARGET_SELECTION_VELO: //parameter_dtypes[paramNr] & 0x0F
	{
		//**VELO encoder value limit to start and end of range for this voice
		// get voice, and figure valid range, translate to param number before sending
		uint8_t voiceNr=(uint8_t)(paramNr - PAR_VEL_DEST_1);
		if(*paramValue < pgm_read_byte(&modTargetVoiceOffsets[voiceNr].start)) {
			if(inc < 0) // going down, allow 0
				*paramValue=0;
			else // going up fix to start
				*paramValue = pgm_read_byte(&modTargetVoiceOffsets[voiceNr].start);
		} else if (*paramValue > pgm_read_byte(&modTargetVoiceOffsets[voiceNr].end)) {
			*paramValue = pgm_read_byte(&modTargetVoiceOffsets[voiceNr].end);
		}

		// determine the parameter id to send across
		uint8_t value = (uint8_t)pgm_read_word(&modTargets[*paramValue].param);
		uint8_t upper,lower;
		/*
		 *  upper: rightmost bit is 1 if the parameter we are targeting is in the "above 127" range
		 *         next 6 bits are the voice number (0 to 5) of which voice is being dealt with here
		 *  lower: the (0-127) value representing which parameter is being modulated
		 */
		upper = (uint8_t)( (uint8_t)((value&0x80)>>7) | ((voiceNr&0x3f)<<1) );
		lower = value&0x7f;
      if (!isMorphParam)
		avrComms_sendData(CC_VELO_TARGET,upper,lower);
		//return;
	}
		break;

	case DTYPE_VOICE_LFO://parameter_dtypes[paramNr] & 0x0F
	{
		//**LFO - limit voice number to 1-6 range. determine target selection so we can send it with voice #
		if(*paramValue < 1)
			*paramValue = 1;
		else if(*paramValue > 6)
			*paramValue = 6;

		// **LFOTARGFIX - ensure that the lfo mod target is pointing to the same type of modulation
		// on the new voice
		const uint8_t newTargVal=getModTargetIdxFromGapIdx((uint8_t)(*paramValue-1),menu_TargetVoiceGapIndex);
		// update the lfo mod target
		parameter_values[PAR_TARGET_LFO1 + (paramNr - PAR_VOICE_LFO1)] = newTargVal;

		// determine the real param value given the index into modTargets
		uint8_t value =  (uint8_t)pgm_read_word(&modTargets[newTargVal].param);

		/*  upper: rightmost bit is 1 if the parameter we are targeting is in the "above 127" range
		 *         next 6 bits are the voice number (0 to 5) of which voice is being dealt with here
		 *  lower: the (0-127) value representing which parameter is being modulated
		 */
		uint8_t upper,lower;
		upper = (uint8_t)(((value&0x80)>>7) | ((((uint8_t)(paramNr - PAR_VOICE_LFO1))&0x3f)<<1));
		lower = value&0x7f;
      if (!isMorphParam)
		avrComms_sendData(CC_LFO_TARGET,upper,lower);
		//return;
	}
		break;
	case DTYPE_TARGET_SELECTION_LFO://parameter_dtypes[paramNr] & 0x0F
	{
		//**LFO - limit encoder start and end to range for the target voice (not the lfo number)
		// this is a value from 1 to 6, so we adjust to be 0 based
		uint8_t voiceNr =  (uint8_t)(parameter_values[PAR_VOICE_LFO1+(paramNr - PAR_TARGET_LFO1)]-1);
		if(*paramValue < pgm_read_byte(&modTargetVoiceOffsets[voiceNr].start)) {
			if(inc < 0) // going down, allow 0
				*paramValue=0;
			else // going up fix to start
				*paramValue = pgm_read_byte(&modTargetVoiceOffsets[voiceNr].start);
		} else if (*paramValue > pgm_read_byte(&modTargetVoiceOffsets[voiceNr].end)) {
			*paramValue = pgm_read_byte(&modTargetVoiceOffsets[voiceNr].end);
		}

		// **LFOTARGFIX - save the gap index
		menu_TargetVoiceGapIndex = getModTargetGapIndex(*paramValue);

		uint8_t value =  (uint8_t)pgm_read_word(&modTargets[*paramValue].param);
		uint8_t upper,lower;
		upper = (uint8_t)((uint8_t)((value&0x80)>>7) | (((paramNr - PAR_TARGET_LFO1)&0x3f)<<1));
		lower = value&0x7f;
      if (!isMorphParam)
		avrComms_sendData(CC_LFO_TARGET,upper,lower);
		//--AS fall thru to update display
	}
		break;

	case DTYPE_AUTOM_TARGET: {//parameter_dtypes[paramNr] & 0x0F
   
    	const uint8_t nmt=getNumModTargets();
   	//**AUTOM - limit to valid range for encoder
   	if(*paramValue >= nmt)
   		*paramValue = (uint8_t)(nmt-1);
      if (menu_activePage == PERFORMANCE_PAGE)
      { // the only time you get a dtype autom target on the PERF page is when assigning macros
#if 0
   		uint8_t value =  (uint8_t)pgm_read_word(&modTargets[*paramValue].param); // the value of the mod target
         uint8_t lower = value&0x7f;
         uint8_t upper = (uint8_t)
                         ( ( ( (paramNr - PAR_MAC1_DST1) //  MAC1_DST1=0, M1D2=2, M2D1=4, M2D2=6
                              >>1 )                      //  MAC1_DST1=0, M1D2=1, M2D1=2, M2D2=3
                              <<2 )                      //  shift over 2 to make room for upper mod target bit
                              |(value>>7) );
                              
         
         avrComms_sendData(MACRO_CC,upper,lower);
#endif
      }      
	}
   break;

	case DTYPE_0B255:
		//if(*paramValue > 255)
		//	*paramValue = 255;
		break;

   case DTYPE_0B16://parameter_dtypes[paramNr] & 0x0F
	   if(*paramValue > 16)
			*paramValue = 16;
		break;
   
	case DTYPE_1B16://parameter_dtypes[paramNr] & 0x0F
		if(*paramValue < 1)
			*paramValue = 1;
		else if(*paramValue > 16)
			*paramValue = 16;
		break;
	case DTYPE_0B15:
		/* Session 036 fix (PROBABILITY_INVESTIGATION.md "pattern rotation" follow-up).
		   WHY: PAR_EUKLID_SUBSTEP_ROTATION shares this dtype with
		   PAR_EUKLID_ROTATION (only 16 dtype slots exist -- see the
		   16-entry limit noted on the Datatypes enum in menu.h -- so a
		   dedicated 0-7 dtype is not available), but its real range
		   must stay 0-7. euklid_rotatePattern() (EuklidGenerator.c)
		   treats any |delta| > 7 between successive sub-step rotation
		   values as spilling into a whole main-step rotation
		   (mainSteps += delta/8), which silently rotates which grid
		   button lights up for a note the user only meant to nudge
		   within its own 8-slot sub-step group.
		   WHAT: clamp PAR_EUKLID_SUBSTEP_ROTATION to a 0-7 ceiling here
		   (instead of the shared dtype's normal 0-15) so every
		   possible single-edit delta stays within +-7, which keeps
		   euklid_rotatePattern()'s normalization on the sub-step-only
		   path and never rotates the main-step bitmask as a side
		   effect. PAR_EUKLID_ROTATION keeps its real 0-15 ceiling.
		   INPUT: paramNr (which parameter is being edited), *paramValue
		   (the byte already updated by menu_applyEncoderDeltaToByte()).
		   OUTPUT: *paramValue clamped in place.
		   AFFILIATES: the identical clamp in
		   menu_encoderChangeShiftParameter() and getDtypeValue() below
		   must be kept in sync with this one; STM-side
		   euklid_setSubStepRotation()/euklid_rotatePattern()
		   (EuklidGenerator.c) is what this value ultimately drives. */
		if(paramNr == PAR_EUKLID_SUBSTEP_ROTATION)
		{
			if(*paramValue > 7)
				*paramValue = 7;
		}
		else if(*paramValue>15)
			*paramValue = 15;
		break;
	case DTYPE_MIX_FM://parameter_dtypes[paramNr] & 0x0F
	case DTYPE_ON_OFF:
	case DTYPE_0b1:
		if(*paramValue > 1)
			*paramValue = 1;
		break;



	case DTYPE_MENU://parameter_dtypes[paramNr] & 0x0F
	{
		//get the number of entries
		uint8_t numEntries = menu_getDtypeMenuEntryCount(paramNr);
      if(numEntries == 0)
         *paramValue = 0;
		else if(*paramValue >= numEntries)
			*paramValue = (uint8_t)(numEntries-1);

	} // parameter_dtypes[paramNr] & 0x0F case DTYPE_MENU
		break;

	default://parameter_dtypes[paramNr] & 0x0F
	case DTYPE_0B127:
		if(*paramValue > 127)
			*paramValue = 127;
		break;
	} //parameter_dtypes[paramNr] & 0x0F


	// --AS TODO this will also send MIDI_CC or CC_2 for the above items that have already been sent. is this desired?
	//send parameter change to uart tx
	if(!isMorphParam && menu_isVoiceMorphAmountParam(paramNr))
   {
      menu_sendVoiceMorphAmount(paramNr, *paramValue);
   }
	else if(paramNr < END_OF_SOUND_PARAMETERS) // => Sound Parameter: menu edit (Session 039 P3)
   {
      if (!isMorphParam)
      {
         menu_sendSoundParameter(paramNr, *paramValue);
      }
   }
	else // non sound parameters (ie current step data, etc)
		menu_parseGlobalParam(paramNr,parameter_values[paramNr]);
	//avrComms_sendData(0xb0,paramNr,*paramValue);
}


//-----------------------------------------------------------------
static void menu_sendMorphParameterEndpoint(uint16_t paramNr, uint8_t value)
{
   /* Morph endpoint edits send raw endpoint bytes to STM storage. They do not
      request AVR-side live morph computation. */
   if(paramNr < 128)
      avrComms_sendData(PRF_RESTORE_MORPH_CC, (uint8_t)paramNr, value);
   else if(paramNr < END_OF_SOUND_PARAMETERS)
      avrComms_sendData(PRF_RESTORE_MORPH_CC2, (uint8_t)(paramNr - 128), value);
}

//-----------------------------------------------------------------
// -bc- called when edit mode is active
// and shift is pressed - we can add some
// special functions here, primarily
// add shift+encoder changes morph parameters

static void menu_encoderChangeShiftParameter(int8_t inc)
{
   screensaver_touch();
	const uint8_t activeParameter	= menuIndex & MASK_PARAMETER;
	const uint8_t activePage		= (menuIndex&MASK_PAGE)>>PAGE_SHIFT;

	//get address from top1-8 from activeParameter (base adress top1 + offset)
	uint16_t paramNr		= pgm_read_word(&menuPages[menu_activePage][activePage].bot1 + activeParameter);
   uint8_t *paramValue;
   uint8_t isMorphParam = (paramNr < END_OF_SOUND_PARAMETERS);
   const uint8_t dtype = pgm_read_byte(&parameter_dtypes[paramNr]) & 0x0F;
      
   if (isMorphParam)
   {
      paramValue = &parameters2[paramNr];
   }
   else
   {
      paramValue = &parameter_values[paramNr];
   }
   
   if(dtype == DTYPE_MENU)
      menu_normalizeDtypeMenuEncoderValue(paramNr, paramValue);

		menu_applyEncoderDeltaToByte(paramValue, inc);

	switch(dtype)
	{
	case DTYPE_TARGET_SELECTION_VELO: //parameter_dtypes[paramNr] & 0x0F
	{
		//**VELO encoder value limit to start and end of range for this voice
		// get voice, and figure valid range, translate to param number before sending
		uint8_t voiceNr=(uint8_t)(paramNr - PAR_VEL_DEST_1);
		if(*paramValue < pgm_read_byte(&modTargetVoiceOffsets[voiceNr].start)) {
			if(inc < 0) // going down, allow 0
				*paramValue=0;
			else // going up fix to start
				*paramValue = pgm_read_byte(&modTargetVoiceOffsets[voiceNr].start);
		} else if (*paramValue > pgm_read_byte(&modTargetVoiceOffsets[voiceNr].end)) {
			*paramValue = pgm_read_byte(&modTargetVoiceOffsets[voiceNr].end);
		}

		// determine the parameter id to send across
		uint8_t value = (uint8_t)pgm_read_word(&modTargets[*paramValue].param);
		uint8_t upper,lower;
		/*
		 *  upper: rightmost bit is 1 if the parameter we are targeting is in the "above 127" range
		 *         next 6 bits are the voice number (0 to 5) of which voice is being dealt with here
		 *  lower: the (0-127) value representing which parameter is being modulated
		 */
		upper = (uint8_t)( (uint8_t)((value&0x80)>>7) | ((voiceNr&0x3f)<<1) );
		lower = value&0x7f;
      if (!isMorphParam)
		   avrComms_sendData(CC_VELO_TARGET,upper,lower);
		//return;
	}
		break;

	case DTYPE_VOICE_LFO://parameter_dtypes[paramNr] & 0x0F
	{
		//**LFO - limit voice number to 1-6 range. determine target selection so we can send it with voice #
		if(*paramValue < 1)
			*paramValue = 1;
		else if(*paramValue > 6)
			*paramValue = 6;

		// **LFOTARGFIX - ensure that the lfo mod target is pointing to the same type of modulation
		// on the new voice
		const uint8_t newTargVal=getModTargetIdxFromGapIdx((uint8_t)(*paramValue-1),menu_TargetVoiceGapIndex);
		// update the lfo mod target in the same endpoint image being edited
      if(isMorphParam)
      {
         parameters2[PAR_TARGET_LFO1 + (paramNr - PAR_VOICE_LFO1)] = newTargVal;
         menu_sendMorphParameterEndpoint((uint16_t)(PAR_TARGET_LFO1 + (paramNr - PAR_VOICE_LFO1)),
                                         newTargVal);
      }
      else
		   parameter_values[PAR_TARGET_LFO1 + (paramNr - PAR_VOICE_LFO1)] = newTargVal;

		// determine the real param value given the index into modTargets
		uint8_t value =  (uint8_t)pgm_read_word(&modTargets[newTargVal].param);

		/*  upper: rightmost bit is 1 if the parameter we are targeting is in the "above 127" range
		 *         next 6 bits are the voice number (0 to 5) of which voice is being dealt with here
		 *  lower: the (0-127) value representing which parameter is being modulated
		 */
		uint8_t upper,lower;
		upper = (uint8_t)(((value&0x80)>>7) | ((((uint8_t)(paramNr - PAR_VOICE_LFO1))&0x3f)<<1));
		lower = value&0x7f;
      if (!isMorphParam)
		avrComms_sendData(CC_LFO_TARGET,upper,lower);
		//return;
	}
		break;
	case DTYPE_TARGET_SELECTION_LFO://parameter_dtypes[paramNr] & 0x0F
	{
		//**LFO - limit encoder start and end to range for the target voice (not the lfo number)
		// this is a value from 1 to 6, so we adjust to be 0 based
		uint8_t voiceNr = isMorphParam
         ? (uint8_t)(parameters2[PAR_VOICE_LFO1+(paramNr - PAR_TARGET_LFO1)]-1)
         : (uint8_t)(parameter_values[PAR_VOICE_LFO1+(paramNr - PAR_TARGET_LFO1)]-1);
		if(*paramValue < pgm_read_byte(&modTargetVoiceOffsets[voiceNr].start)) {
			if(inc < 0) // going down, allow 0
				*paramValue=0;
			else // going up fix to start
				*paramValue = pgm_read_byte(&modTargetVoiceOffsets[voiceNr].start);
		} else if (*paramValue > pgm_read_byte(&modTargetVoiceOffsets[voiceNr].end)) {
			*paramValue = pgm_read_byte(&modTargetVoiceOffsets[voiceNr].end);
		}

		// **LFOTARGFIX - save the gap index
		menu_TargetVoiceGapIndex = getModTargetGapIndex(*paramValue);

		uint8_t value =  (uint8_t)pgm_read_word(&modTargets[*paramValue].param);
		uint8_t upper,lower;
		upper = (uint8_t)((uint8_t)((value&0x80)>>7) | (((paramNr - PAR_TARGET_LFO1)&0x3f)<<1));
		lower = value&0x7f;
      if (!isMorphParam)
		avrComms_sendData(CC_LFO_TARGET,upper,lower);
		//--AS fall thru to update display
	}
		break;

	case DTYPE_AUTOM_TARGET: {//parameter_dtypes[paramNr] & 0x0F
		const uint8_t nmt=getNumModTargets();
		//**AUTOM - limit to valid range for encoder
		if(*paramValue >= nmt)
			*paramValue = (uint8_t)(nmt-1);
	}
   break;
	case DTYPE_0B255:
		//if(*paramValue > 255)
		//	*paramValue = 255;
		break;

   case DTYPE_0B16://parameter_dtypes[paramNr] & 0x0F
	   if(*paramValue > 16)
			*paramValue = 16;
		break;
   
	case DTYPE_1B16://parameter_dtypes[paramNr] & 0x0F
		if(*paramValue < 1)
			*paramValue = 1;
		else if(*paramValue > 16)
			*paramValue = 16;
		break;
	case DTYPE_0B15:
		/* Session 036 fix: keep this in sync with the DTYPE_0B15 case in
		   menu_encoderChangeParameter() above -- PAR_EUKLID_SUBSTEP_ROTATION
		   must stay clamped to 0-7 (not the shared dtype's normal 0-15) to
		   keep euklid_rotatePattern()'s sub-step-vs-main-step rotation
		   normalization from spilling into the main-step bitmask; see the
		   full explanation there. */
		if(paramNr == PAR_EUKLID_SUBSTEP_ROTATION)
		{
			if(*paramValue > 7)
				*paramValue = 7;
		}
		else if(*paramValue>15)
			*paramValue = 15;
		break;
	case DTYPE_MIX_FM://parameter_dtypes[paramNr] & 0x0F
	case DTYPE_ON_OFF:
	case DTYPE_0b1:
		if(*paramValue > 1)
			*paramValue = 1;
		break;



	case DTYPE_MENU://parameter_dtypes[paramNr] & 0x0F
	{
		//get the number of entries
		uint8_t numEntries = menu_getDtypeMenuEntryCount(paramNr);
      if(numEntries == 0)
         *paramValue = 0;
		else if(*paramValue >= numEntries)
			*paramValue = (uint8_t)(numEntries-1);

	} // parameter_dtypes[paramNr] & 0x0F case DTYPE_MENU
		break;

	default://parameter_dtypes[paramNr] & 0x0F
	case DTYPE_0B127:
		if(*paramValue > 127)
			*paramValue = 127;
		break;
	} //parameter_dtypes[paramNr] & 0x0F


	// --AS TODO this will also send MIDI_CC or CC_2 for the above items that have already been sent. is this desired?
	//send parameter change to uart tx
	if(paramNr < 128) // => Sound Parameter
   {
      if (!isMorphParam)
		avrComms_sendData(MIDI_CC,(uint8_t)paramNr,*paramValue);
      else
         menu_sendMorphParameterEndpoint(paramNr, *paramValue);
   }
	else if(paramNr > 127 && (paramNr < END_OF_SOUND_PARAMETERS)) // => Sound Parameter above 127
   {
      if (!isMorphParam)
		avrComms_sendData(CC_2,(uint8_t)(paramNr-128),*paramValue);
      else
         menu_sendMorphParameterEndpoint(paramNr, *paramValue);
   }
	else // non sound parameters (ie current step data, etc)
		menu_parseGlobalParam(paramNr,parameter_values[paramNr]);

	//avrComms_sendData(0xb0,paramNr,*paramValue);
}


//-----------------------------------------------------------------
// given an encoder wheel change, will set the menu item to the correct new one
// or not change it if boundaries are reached
static void menu_moveToMenuItem(int8_t inc)
{
   screensaver_touch();
	int8_t activeParameter	= menuIndex & MASK_PARAMETER; // will be 0 to 7
	int8_t activePage		= (int8_t)(menuIndex >> PAGE_SHIFT); // will be 0 to 31
	uint8_t needLock=0;
	uint8_t param;
	uint8_t allowedSkips	=3; //how many skip items do we allow in a row

	// clamp encoder to +/- 1 to make logic simpler
	inc = inc>0?1:-1;

checkvalid:
	DISABLE_CONV_WARNING
	activeParameter+=inc; // set new desired active parameter value
	END_DISABLE_CONV_WARNING
	if(inc > 0) {
		if(activeParameter == 4) { // move to 2nd section of sub-page
			needLock=1;
		} else if(activeParameter == 8) { // moved past end of 2nd sub-page
			if(menu_activePage==MENU_MIDI_PAGE) { // in global menu, we can move to next sub-page
				needLock=1;
				activeParameter=0; // set to par 0 of next sub page
				activePage++; // and change page
				if(activePage >=NUM_SUB_PAGES)
					activePage=0; // wrap around to 1st (would only happen if we fill all 8. unlikely)
         }
         else if(menu_activePage==PERFORMANCE_PAGE) { // in perf menu, we can move to next sub-page
				needLock=1;
				activeParameter=0; // set to par 0 of next sub page
				activePage++; // and change page
				if(activePage >=NUM_SUB_PAGES)
					activePage=0; // wrap around to 1st (would only happen if we fill all 8. unlikely)
			} 
         else{
				return; // moved past last param on section 2. not allowed in most modes
         }
		}
	} else { // inc == -1
		if(activeParameter == 3) { // move to first section of sub-page
			needLock=1;
		} else if(activeParameter==-1) { // if we went past 0
			if(menu_activePage==MENU_MIDI_PAGE) { // in global menu, we can move to previous sub-page
				needLock=1;
				activeParameter=7; // put us on the last param of the previous sub-page
				activePage--; // and change page
				if(activePage < 0)
					activePage=NUM_SUB_PAGES-1; //wrap around to last page (would only happen if we fill all 8. unlikely)
         
			} 
         else if(menu_activePage==PERFORMANCE_PAGE) { // in global menu, we can move to previous sub-page
				needLock=1;
				activeParameter=7; // put us on the last param of the previous sub-page
				activePage--; // and change page
				if(activePage < 0)
					activePage=NUM_SUB_PAGES-1; //wrap around to last page (would only happen if we fill all 8. unlikely)
         
			}
         else{
				return; // move past first param on section 1. not allowed in most modes
         }
		}
	}

	// read the new param
	param = pgm_read_byte(&menuPages[menu_activePage][activePage].top1 + activeParameter);
	if(param == TEXT_SKIP) {
		if(allowedSkips--) // skip this one and check the next
			goto checkvalid;
		else
			return; // would never happen unless we have > 3 skips in a row
	}

	if(param == TEXT_EMPTY) // disallow the change
		return;

	// set the change
	menuIndex = (uint8_t)( (activePage << PAGE_SHIFT) | activeParameter);
	if(needLock)
		lockPotentiometerFetch();
      
   if(menu_activePage<=VOICE7_PAGE)
      menu_lastVoiceIndex=menuIndex;
   else if(menu_activePage==PERFORMANCE_PAGE)
      menu_lastPerfIndex=menuIndex;
  
}

//-----------------------------------------------------------------
// this is called when we need to reset the load/save page to it's default
// state
void menu_resetSaveParameters()
{

	//reset save params
	if(menu_saveOptions.what >= SAVE_TYPE_GLO) // global and samples only have one choice - hit ok.
	{
		// return to the load/save kit page with "type" selected
		menu_saveOptions.state = SAVE_STATE_EDIT_TYPE;
		menu_saveOptions.what = SAVE_TYPE_KIT;
	}
	else
	{
		// return to the state where preset number is selected
		editModeActive = 1;
		menu_saveOptions.state	= SAVE_STATE_EDIT_PRESET_NR;//SAVE_STATE_EDIT_TYPE;
	}		

	menu_repaintAll();
   menu_voiceArray = 0x7f;
}
//-----------------------------------------------------------------
void menu_resetSubPage() // forces menu to first sub-page, disregarding toggle
{
   menuIndex = 0;
}
//-----------------------------------------------------------------
void menu_gotoSubPage(uint8_t subPage) // forces menu to first sub-page, disregarding toggle
{
   uint8_t activeParameter	= menuIndex & MASK_PARAMETER;
	uint8_t activePage = subPage;
   menuIndex = (uint8_t)( (activePage << PAGE_SHIFT) | activeParameter);
}
//-----------------------------------------------------------------
// switches us to a different menu sub page. If that page is already active
// and has multiple screens, will toggle to the other screen
// if its the global menu, and there are multiple pages will toggle to the next page
void menu_switchSubPage(uint8_t subPageNr)
{
	//lock all parameters
	lockPotentiometerFetch();

	//leave edit mode if active
	editModeActive = 0;

	//get current position
	uint8_t activeParameter	= menuIndex & MASK_PARAMETER;
	uint8_t activePage		= (menuIndex&MASK_PAGE)>>PAGE_SHIFT;
   
   if (menu_activePage<=VOICE7_PAGE) // on a voice menu page
   {
      if(subPageNr == activePage)
      {
         if(activeParameter < 4) 
         {
   			// toggle to next if possible
   			if(has2ndPage(activePage))
   				activeParameter=4;
         }
         else
            activeParameter=0;
      }
      else
      {
         activePage=subPageNr;
         activeParameter=0;
      }  

      menuIndex = (uint8_t)( (activePage << PAGE_SHIFT) | activeParameter);
      menu_lastVoiceIndex=menuIndex;
   }
   else if (menu_activePage==MENU_MIDI_PAGE)
   {
      if(subPageNr == activePage) 
      { // in global mode, we move to next page if possible (or wrap to first)
         if(activeParameter<4)
         {
            if(pgm_read_byte(&menuPages[menu_activePage][activePage].top5) != TEXT_EMPTY) 
               activeParameter=4;
            else
               activePage=0;   
         }   
         else
         {  
   		   if(pgm_read_byte(&menuPages[menu_activePage][activePage].top1) != TEXT_EMPTY) 
            {
   				activePage=(uint8_t)((activePage+1)%8);
   			} 
            else 
            {
   				activePage=0;
   			}
            activeParameter=0;
         }
      }
      menuIndex = (uint8_t)( (activePage << PAGE_SHIFT) | activeParameter);
   }
   else if (menu_activePage==PERFORMANCE_PAGE)
   {
      if(subPageNr == activePage)
      {
         if(activeParameter < 4)
            activeParameter=4;
         else
            activeParameter=0;
      }
      else
      {
         activePage=subPageNr;
         activeParameter=0;
      }   
         
      menuIndex = (uint8_t)( (activePage << PAGE_SHIFT) | activeParameter);
      menu_lastPerfIndex=menuIndex;
   }
   else if (menu_activePage==SEQ_PAGE)
   {
      if(subPageNr == activePage)
      {
         if(activeParameter < 4)
            activeParameter=4;
         else
            activeParameter=0;
      }
      else
      {
         activePage=subPageNr;
         activeParameter=0;
      }   
         
      menuIndex = (uint8_t)( (activePage << PAGE_SHIFT) | activeParameter);
      menu_lastStepSubPage=menuIndex;
   }
   
   

}

//-----------------------------------------------------------------
void menu_resetActiveParameter()
{
	uint8_t activePage		= (menuIndex&MASK_PAGE)>>PAGE_SHIFT;
	if(!has2ndPage(activePage))
	{
		DISABLE_CONV_WARNING
		menuIndex &= ~MASK_PARAMETER;
		END_DISABLE_CONV_WARNING
	}		
};
//-----------------------------------------------------------------
int menu_getWhat()
{
	return menu_saveOptions.what;
};
//-----------------------------------------------------------------
uint8_t menu_getSubPage()
{
	return (menuIndex&MASK_PAGE)>>PAGE_SHIFT;
};
//-----------------------------------------------------------------
void menu_switchPage(uint8_t pageNr)
{
	//clear all sequencer buttons
	led_clearSequencerLeds();

	switch(pageNr) {
	case MENU_MIDI_PAGE: { // global settings page
		uint8_t toggle = (menu_activePage == MENU_MIDI_PAGE);
		menu_activePage=MENU_MIDI_PAGE;
		// leave edit mode if active
		editModeActive = 0;
		//activate the parameter lock
		lockPotentiometerFetch();

		// **GMENU TODO do we need to clear any leds?
		if (toggle) // if we are on the settings page already, toggle between all pages
			menu_switchSubPage(menu_getSubPage());

		}
		break;

	case PERFORMANCE_PAGE:
	case PATTERN_SETTINGS_PAGE:
	case SEQ_PAGE:

		menu_activePage = pageNr;
		//leave edit mode if active
		editModeActive = 0;
		//activate the parameter lock
		lockPotentiometerFetch();
		break;

	case LOAD_PAGE:
   case SAVE_PAGE:
	{
		//re-init the save page variables
		menu_resetSaveParameters();
		menu_activePage = pageNr;

		//leave edit mode if active
		//editModeActive = 0;

		// load the name of the current preset from disk
      DISABLE_CONV_WARNING
		preset_loadName(menu_currentPresetNr[menu_saveOptions.what], menu_saveOptions.what,0);
      END_DISABLE_CONV_WARNING
	}
		break;

	default: //voice pages etc
      {
   		menu_activePage = pageNr;
   		if(pageNr<7) 
         {
   		   menu_setActiveVoice(pageNr);
   	   }
   		//leave edit mode if active
   		editModeActive = 0;
   
   		//activate the parameter lock
   		lockPotentiometerFetch();
   		//query current sequencer step states and light up the corresponding leds
   
   		uint8_t trackNr = menu_getActiveVoice(); //max 6 => 0x6 = 0b110
   		uint8_t patternNr = menu_shownPattern; //max 7 => 0x07 = 0b111
         uint8_t value = (uint8_t)((trackNr<<4) | (patternNr&0x0f));
   		avrComms_sendData(LED_CC,LED_QUERY_SEQ_TRACK,value);
      
   	}
   	break;
	}		


	//re initialize the voice LEDs
	if(pageNr==PERFORMANCE_PAGE)
	{
		//light up mute leds
		buttonHandler_showMuteLEDs();
	} else
	{
		led_setActiveVoiceLeds((uint8_t)(1<<menu_getActiveVoice()));
		menu_muteModeActive = 0;
	}

	//go to 1st parameter on sub page
	menu_resetActiveParameter();

	//force complete repaint
	menu_repaintAll();
};
//-----------------------------------------------------------------
void menu_sendAllGlobals()
{
	uint16_t i;
   if(!avrComms_flowBegin(FLOW_CH_GLOBALS))
      return;

	for(i=PAR_BEGINNING_OF_GLOBALS;(i<NUM_PARAMS);i++)
	{
		menu_parseGlobalParam(i,parameter_values[i]);
      if(avrComms_flowFailed())
         break;
	}

   if(!avrComms_flowFailed())
      (void)avrComms_flowEnd(FLOW_CH_GLOBALS);
};
//-----------------------------------------------------------------
// This is used to send a global (non voice specific) parameter to the back.
// This includes values for the currently active step in the sequencer
void menu_parseGlobalParam(uint16_t paramNr, uint8_t value)
{
	switch(paramNr)
	{

	// --AS midi mode is not used anymore
	//case PAR_MIDI_MODE:
	//	avrComms_sendData(SEQ_CC,SEQ_MIDI_MODE,parameter_values[PAR_MIDI_MODE]);
	//	break;

	case PAR_MIDI_CHAN_1:
      if(value==0)
         avrComms_sendData(SEQ_CC,SEQ_MIDI_CHAN_OFF,0x00);
      else
         avrComms_sendData(SEQ_CC,SEQ_MIDI_CHAN,(uint8_t)((0x00<<4) | ((value-1)&0x0f) ));
      break;
	case PAR_MIDI_CHAN_2:
      if(value==0)
         avrComms_sendData(SEQ_CC,SEQ_MIDI_CHAN_OFF,0x01);
      else
         avrComms_sendData(SEQ_CC,SEQ_MIDI_CHAN,(uint8_t)((0x01<<4) | ((value-1)&0x0f) ));
      break;
	case PAR_MIDI_CHAN_3:
      if(value==0)
         avrComms_sendData(SEQ_CC,SEQ_MIDI_CHAN_OFF,0x02);
      else
         avrComms_sendData(SEQ_CC,SEQ_MIDI_CHAN,(uint8_t)((0x02<<4) | ((value-1)&0x0f) ));
      break;
	case PAR_MIDI_CHAN_4:
      if(value==0)
         avrComms_sendData(SEQ_CC,SEQ_MIDI_CHAN_OFF,0x03);
      else
         avrComms_sendData(SEQ_CC,SEQ_MIDI_CHAN,(uint8_t)((0x03<<4) | ((value-1)&0x0f) ));
      break;
	case PAR_MIDI_CHAN_5:
      if(value==0)
         avrComms_sendData(SEQ_CC,SEQ_MIDI_CHAN_OFF,0x04);
      else
         avrComms_sendData(SEQ_CC,SEQ_MIDI_CHAN,(uint8_t)((0x04<<4) | ((value-1)&0x0f) ));
      break;
	case PAR_MIDI_CHAN_6:
      if(value==0)
         avrComms_sendData(SEQ_CC,SEQ_MIDI_CHAN_OFF,0x05);
      else
         avrComms_sendData(SEQ_CC,SEQ_MIDI_CHAN,(uint8_t)((0x05<<4) | ((value-1)&0x0f) ));
      break;
   case PAR_MIDI_CHAN_7:
      if(value==0)
         avrComms_sendData(SEQ_CC,SEQ_MIDI_CHAN_OFF,0x06);
      else
         avrComms_sendData(SEQ_CC,SEQ_MIDI_CHAN,(uint8_t)((0x06<<4) | ((value-1)&0x0f) ));
      break;
	case PAR_MIDI_CHAN_GLOBAL:
      if(value==0)
         avrComms_sendData(SEQ_CC,SEQ_MIDI_CHAN_OFF,0x07);
      else
         avrComms_sendData(SEQ_CC,SEQ_MIDI_CHAN,(uint8_t)((0x07<<4) | ((value-1)&0x0f) ));
      break;

	case PAR_OSC_WAVE_INTERPOLATION:
		avrComms_sendData(SEQ_CC, SEQ_OSC_WAVE_INTERPOLATION, value);
		break;

	case PAR_POS_X:
		avrComms_sendData(SEQ_CC,SEQ_SET_ACTIVE_TRACK,menu_getActiveVoice());
		avrComms_sendData(SEQ_CC,SEQ_POSX,value);
		break;

	case PAR_POS_Y:
		avrComms_sendData(SEQ_CC,SEQ_SET_ACTIVE_TRACK,menu_getActiveVoice());
		avrComms_sendData(SEQ_CC,SEQ_POSY,value);
		break;

	case PAR_FLUX:
		avrComms_sendData(SEQ_CC,SEQ_SET_ACTIVE_TRACK,menu_getActiveVoice());
		avrComms_sendData(SEQ_CC,SEQ_FLUX,value);
		break;

	case PAR_SOM_FREQ:
		avrComms_sendData(SEQ_CC,SEQ_SET_ACTIVE_TRACK,menu_getActiveVoice());
		avrComms_sendData(SEQ_CC,SEQ_SOM_FREQ,value);
		break;

	case PAR_TRACK_LENGTH:
		avrComms_sendData(SEQ_CC,SEQ_SET_ACTIVE_TRACK,menu_getActiveVoice());
		avrComms_sendData(SEQ_CC,SEQ_TRACK_LENGTH,value);
		break;
      
   case PAR_TRACK_SCALE:
      avrComms_sendData(SEQ_CC,SEQ_SET_ACTIVE_TRACK,menu_getActiveVoice());
		avrComms_sendData(SEQ_CC,SEQ_TRACK_SCALE,value);
		break;
      
	case PAR_SHUFFLE:
		avrComms_sendData(SEQ_CC,SEQ_SHUFFLE,value);
		break;

	case PAR_AUTOM_TRACK:
		avrComms_sendData(SEQ_CC,SEQ_SET_AUTOM_TRACK,value);
		break;

	case PAR_P1_DEST:
	case PAR_P2_DEST:
	{ //step range 0-127 value range 0-255!
		//**AUTOM - translate back into a parameter when sending value to cortex
		uint8_t tmp=(uint8_t)pgm_read_word(&modTargets[value].param);
		avrComms_sendData(SEQ_CC, SEQ_SELECT_ACTIVE_STEP,parameter_values[PAR_ACTIVE_STEP]);
		avrComms_sendData((uint8_t)
				(paramNr==PAR_P1_DEST ? SET_P1_DEST : SET_P2_DEST),tmp>>7,tmp&0x7f);
		break;
	}

	case PAR_P1_VAL:
		avrComms_sendData(SET_P1_VAL,parameter_values[PAR_ACTIVE_STEP],value);
		break;

	case PAR_P2_VAL:
		avrComms_sendData(SET_P2_VAL,parameter_values[PAR_ACTIVE_STEP],value);
		break;

	case PAR_QUANTISATION:
		avrComms_sendData(SEQ_CC,SEQ_SET_QUANT,value);
		break;

	case PAR_SCREENSAVER_ON_OFF:

		break;

	case PAR_BPM:
		avrComms_sendData(SET_BPM,value&0x7f,(value>>7)&0x7f);
		break;
	case PAR_MORPH:
	{
		//value += (value==127)*1;
      morphValue = value;
      menu_syncVoiceMorphDisplayValues(value);
      /* Global morph is a control value now. STM caches it and copies it to all
         six per-voice morph amounts; STM performs the interpolation work.
         Send as two 7-bit-safe messages because raw values >= 128 look like
         status bytes to the STM MIDI-style parser. */
		avrComms_sendData(SEQ_CC, SEQ_SET_GLOBAL_MORPH_LSB, (uint8_t)(value & 0x7f));
		avrComms_sendData(SEQ_CC, SEQ_SET_GLOBAL_MORPH_MSB, (uint8_t)((value >> 7) & 0x01));
	}
	break;

	case PAR_FETCH:
		if(value) {
			parameterFetch |= PARAMETER_LOCK_ACTIVE;
		} else {
			//parameterFetch &= ~PARAMETER_LOCK_ACTIVE;
			parameterFetch = 0;
		}			
		break;
		/*
		case PAR_PHASE_VOICE1:
		case PAR_PHASE_VOICE2:
		case PAR_PHASE_VOICE3:
		case PAR_PHASE_VOICE4:
		case PAR_PHASE_VOICE5:
		case PAR_PHASE_VOICE6:
			avrComms_sendData(CC_2,paramNr-PAR_BEGINNING_OF_GLOBALS,value);
		break;
		 */

	case PAR_VOICE_DECIMATION1:
	case PAR_VOICE_DECIMATION2:
	case PAR_VOICE_DECIMATION3:
	case PAR_VOICE_DECIMATION4:
	case PAR_VOICE_DECIMATION5:
	case PAR_VOICE_DECIMATION6:
	case PAR_VOICE_DECIMATION_ALL:
		//select the track nr
		avrComms_sendData(SEQ_CC,SEQ_SET_ACTIVE_TRACK,((uint8_t)(paramNr-PAR_VOICE_DECIMATION1)));
		//send the new output channel
		avrComms_sendData(VOICE_CC,VOICE_DECIMATION,value);
		break;

	case PAR_ROLL:
	{
		avrComms_sendData(SEQ_CC,SEQ_ROLL_RATE,value);
	}
   break;
   case PAR_ROLL_NOTE:
	{
		avrComms_sendData(SEQ_CC,SEQ_ROLL_NOTE,value);
	}
   break;
   case PAR_ROLL_VELOCITY:
	{
		avrComms_sendData(SEQ_CC,SEQ_ROLL_VELOCITY,value);
	}
   break;
   case PAR_ROLL_MODE:
	{
      // record notes if the sequencer note lock is 0
		avrComms_sendData(SEQ_CC,SEQ_ROLL_MODE,(uint8_t)(value));
	}
	break;
   case PAR_TRANSPOSE:
   {
      avrComms_sendData(SEQ_CC,SEQ_TRANSPOSE,value);
   }
   break;
   case PAR_TRANSPOSE_ON_OFF:
   {
      avrComms_sendData(SEQ_CC,SEQ_TRANSPOSE_ON_OFF,value);
   }
   break;




	case PAR_EUKLID_LENGTH: {
		// set the length of the currently active track
		uint8_t length = (uint8_t)(value-1); // max 16
		uint8_t pattern = menu_shownPattern; //max 7
		uint8_t msg = (uint8_t)((pattern&0x7) | (length<<3));
		//select the track nr
		avrComms_sendData(SEQ_CC,SEQ_SET_ACTIVE_TRACK,menu_getActiveVoice());
		avrComms_sendData(SEQ_CC,SEQ_EUKLID_LENGTH,msg);
      
	}
	break;

	case PAR_EUKLID_STEPS:	{
		uint8_t steps =(uint8_t)( value-1); // max 16
		uint8_t pattern = menu_shownPattern; //max 7
		uint8_t msg =(uint8_t)( (pattern&0x7) | (steps<<3));

		//select the track nr
		avrComms_sendData(SEQ_CC,SEQ_SET_ACTIVE_TRACK,menu_getActiveVoice());



		avrComms_sendData(SEQ_CC,SEQ_EUKLID_STEPS,msg);
	}
	break;
	case PAR_EUKLID_ROTATION:	{
		uint8_t rotation =(uint8_t)(value); // max 15
		uint8_t pattern = menu_shownPattern; //max 7
		uint8_t msg =(uint8_t)( (pattern&0x7) | (rotation<<3));

		/* Session 036 fix (pattern-rotation retrigger-while-stopped bug).
		   WHY: this handler used to resend SEQ_SET_ACTIVE_TRACK with
		   menu_getActiveVoice() -- the track already being edited --
		   before every single rotation nudge. On STM,
		   FRONT_SEQ_SET_ACTIVE_TRACK (frontPanelReceivingProtocol.c)
		   treats "requested track == already-active track, sequencer
		   stopped" as the documented "press the already-selected voice
		   button to preview it" gesture and calls seq_triggerVoice().
		   Turning the rotation encoder is not that gesture, so every
		   nudge silently retriggered the voice while stopped.
		   WHAT: stopped sending it here. It is unnecessary: the only way
		   to reach PAR_EUKLID_ROTATION is via the Euclid page
		   (SELECT_MODE_PAT_GEN), and selecting a track for that page
		   already sends SEQ_SET_ACTIVE_TRACK exactly once, from the
		   voice-button handler (buttonHandler.c, "select active voice"
		   branch -> menu_enterPatgenMode()), before any rotation opcode
		   can be sent. STM's frontParser_activeTrack is therefore already
		   correct by the time this case runs.
		   INPUT/OUTPUT: unchanged -- still sends SEQ_EUKLID_ROTATION with
		   the same encoded pattern/rotation byte.
		   AFFILIATES: buttonHandler.c voice-button "select active voice"
		   path (still sends SEQ_SET_ACTIVE_TRACK once, unaffected);
		   frontPanelReceivingProtocol.c FRONT_SEQ_SET_ACTIVE_TRACK and
		   FRONT_SEQ_EUKLID_ROTATION handlers (unchanged). The identical
		   redundant-resend pattern still exists on PAR_EUKLID_LENGTH,
		   PAR_EUKLID_STEPS, and several non-Euclid parameters just above
		   this block; not touched here since only rotation was reported. */
		avrComms_sendData(SEQ_CC,SEQ_EUKLID_ROTATION,msg);
	}
   break;

   case PAR_EUKLID_SUBSTEP_ROTATION:	{
		uint8_t rotation =(uint8_t)(value); // max 15
		uint8_t pattern = menu_shownPattern; //max 7
		uint8_t msg =(uint8_t)( (pattern&0x7) | (rotation<<3));

		/* Session 036 fix: same reasoning as PAR_EUKLID_ROTATION above --
		   the redundant SEQ_SET_ACTIVE_TRACK resend before every
		   sub-step-rotation nudge caused the same stopped-sequencer
		   voice retrigger, for the same reason. Removed for the same
		   reason and with the same safety argument (track is already
		   selected via the Euclid-page voice-button handler before this
		   case can ever run). */
		avrComms_sendData(SEQ_CC,SEQ_EUKLID_SUBSTEP_ROTATION,msg);
	}
	break;

	case PAR_PATTERN_BEAT:
		avrComms_sendData(SEQ_CC,SEQ_SET_PAT_BEAT,value);
		break;

	case PAR_PATTERN_NEXT:
		avrComms_sendData(SEQ_CC,SEQ_SET_PAT_NEXT,value);
		break;



	case PAR_ACTIVE_STEP:
		avrComms_sendData(SEQ_CC,SEQ_REQUEST_STEP_PARAMS,value);
		break;

	case PAR_STEP_PROB:
		avrComms_sendData(SEQ_CC,SEQ_PROB,value);
		break;

	case PAR_STEP_NOTE:
		avrComms_sendData(SEQ_CC,SEQ_NOTE,value);
		break;

	case PAR_STEP_VOLUME:
		avrComms_sendData(SEQ_CC,SEQ_VOLUME,value);
		break;
	case PAR_MIDI_ROUTING:
		avrComms_sendData(SEQ_CC, SEQ_MIDI_ROUTING, value);
		break;
	case PAR_MIDI_FILT_TX:
		avrComms_sendData(SEQ_CC, SEQ_MIDI_FILT_TX, value);
		break;
	/* RX FILTER WIRE ENCODING  (Session 039)
	   WHAT: converts the RxFilter menu index into the byte STM expects:
	         0..15  -> sent unchanged (RX nibble: N 0x01, R 0x02, C 0x04, P 0x08);
	         16..23 -> ¢ twin k = value-16: RX bits 0x04|(k&3)|((k&4)<<1)
	                   (C, CN, CR, CRN, PC, PCN, PCR, all) plus
	                   MIDI_FILTER_RX_CC38_SWAP;
	         >= 24  -> corrupt (e.g. global file): repaired to "all" (15) in
	                   parameter_values and sent as 15.
	   WHY:  this is the single AVR->STM send point for RxFilter (encoder,
	         pot, global-file load via menu_sendAllGlobals(), automation
	         restore in buttonHandler.c), so encoding here covers every path.
	         Arithmetic instead of a PROGMEM table to save AVR flash (0xE000
	         bootloader limit).
	   INPUT:  value - menu index 0..23 (or a corrupt value).
	   OUTPUT: SEQ_CC / SEQ_MIDI_FILT_RX / encoded byte (7-bit safe, <= 0x1F).
	   ACCESSORS: menu_encoderChangeParameter(), menu_encoderChangeShiftParameter(),
	              menu_parseKnobValue(), menu_sendAllGlobals(),
	              buttonHandler.c original-value restore.
	   AFFILIATES: STM midi_setFilter() / MIDI_RX_FILTER_CC38_SWAP (MidiParser),
	               midiFilterNames[] ¢ rows (MenuText.h). */
	case PAR_MIDI_FILT_RX:
	{
		uint8_t wire = value;

		if(value >= MIDI_FILTER_RX_ENTRIES)
		{
			wire = 0x0F;
			parameter_values[PAR_MIDI_FILT_RX] = wire;
		}
		else if(value >= MIDI_FILTER_CENT_FIRST)
		{
			const uint8_t k = (uint8_t)(value - MIDI_FILTER_CENT_FIRST);
			wire = (uint8_t)(0x04 | (k & 0x03) | ((k & 0x04) << 1)
			                 | MIDI_FILTER_RX_CC38_SWAP);
		}
		avrComms_sendData(SEQ_CC, SEQ_MIDI_FILT_RX, wire);
	}
		break;
	case PAR_PRESCALER_CLOCK_IN:
			avrComms_sendData(SEQ_CC, SEQ_TRIGGER_IN_PPQ, value);
			break;
	case PAR_PRESCALER_CLOCK_OUT1:
			avrComms_sendData(SEQ_CC, SEQ_TRIGGER_OUT1_PPQ, value);
			break;
	case PAR_PRESCALER_CLOCK_OUT2:
			avrComms_sendData(SEQ_CC, SEQ_TRIGGER_OUT2_PPQ, value);
			break;
	case PAR_TRIG_GATE_MODE:
			avrComms_sendData(SEQ_CC, SEQ_TRIGGER_GATE_MODE, value);
			break;
	case PAR_BAR_RESET_MODE:
		avrComms_sendData(SEQ_CC, SEQ_BAR_RESET_MODE, value);
		break;
	case PAR_SEQ_PC_TIME:
		avrComms_sendData(SEQ_CC, SEQ_PC_TIME, value);
		break;
	case PAR_BUT_SHIFT_MODE:
		shiftMode=value;
		break;
   case PAR_LOAD_PERF_ON_BANK:
      parameter_values[PAR_LOAD_PERF_ON_BANK]=value;
      break;
//    case PAR_SKIP_FIRST_ROLL:
//       parameter_values[PAR_SKIP_FIRST_ROLL]=value; 
//       avrComms_sendData(SEQ_CC, SEQ_ROLL_MODE, (uint8_t)(value+5) );
//       break;
   case PAR_FILE_LOAD_BACKGROUND:
      parameter_values[PAR_FILE_LOAD_BACKGROUND]=value; 
      avrComms_sendData(SEQ_CC, SEQ_LOAD_BACKGROUND,value);
      break;
   /* Push the saved MIDI roll-note offset to STM. STM owns note routing and
      clears MIDI-held rolls when the mapping changes. */
   case PAR_ROLL_NOTE_OFFSET:
      parameter_values[PAR_ROLL_NOTE_OFFSET] = value;
      avrComms_sendData(SEQ_CC, SEQ_ROLL_NOTE_OFFSET, (uint8_t)value);
      break;
   case PAR_MIDI_NOTE1:
   case PAR_MIDI_NOTE2:
   case PAR_MIDI_NOTE3:
   case PAR_MIDI_NOTE4:
   case PAR_MIDI_NOTE5:
   case PAR_MIDI_NOTE6:
   case PAR_MIDI_NOTE7:
      avrComms_sendData(SEQ_CC, (uint8_t)(SEQ_TRACK_NOTE1+(paramNr-PAR_MIDI_NOTE1)), (uint8_t)(value) );
   
      break;
	}
}
//-----------------------------------------------------------------
void menu_vMorph(uint8_t dest, uint8_t val, uint8_t amt)
{
   (void)dest;
   (void)val;
   (void)amt;
}
//-----------------------------------------------------------------
static void menu_processSpecialCaseValues(uint16_t paramNr/*, const uint8_t *value*/)
{
   (void)paramNr;
#if 0
   if(paramNr == PAR_MAC1)
   {
      
      menu_vMorph(parameter_values[PAR_MAC1_DST1],parameter_values[PAR_MAC1],parameter_values[PAR_MAC1_DST1_AMT]);
      menu_vMorph(parameter_values[PAR_MAC1_DST2],parameter_values[PAR_MAC1],parameter_values[PAR_MAC1_DST2_AMT]);
      avrComms_sendMacro(1,parameter_values[PAR_MAC1]);
   }
   else if (paramNr == PAR_MAC2)
   {
      menu_vMorph(parameter_values[PAR_MAC2_DST1],parameter_values[PAR_MAC2],parameter_values[PAR_MAC2_DST1_AMT]);
      menu_vMorph(parameter_values[PAR_MAC2_DST2],parameter_values[PAR_MAC2],parameter_values[PAR_MAC2_DST2_AMT]);
      avrComms_sendMacro(2,parameter_values[PAR_MAC2]);
   }
	else if(paramNr == PAR_BPM)
	{
		//*value *= 2;
		//*value+=1;
	}


	//To see the generated pattern we have to update the step view
	else if( (paramNr == PAR_EUKLID_LENGTH) || (paramNr == PAR_EUKLID_STEPS) || (paramNr == PAR_EUKLID_ROTATION) )
	{
		//query current sequencer step states and light up the corresponding leds 
		//avrComms_sendData(LED_CC,LED_QUERY_SEQ_TRACK,menu_activePage);
		led_clearSequencerLeds();
	}
#endif


}
//-----------------------------------------------------------------
static uint8_t getDtypeValue(uint8_t value, uint16_t paramNr)
{
	const float frac = (value/255.f); // yield a fraction from 0 to 1

	switch(pgm_read_byte(&parameter_dtypes[paramNr]) & 0x0F)
	{
	case DTYPE_TARGET_SELECTION_VELO:
	case DTYPE_TARGET_SELECTION_LFO: {
		//**VELO **LFO limit range to start and end for the applicable voice, but allow a setting of 0 (for off)
		uint8_t voiceNr=0;
		uint8_t s, e, rng;
		if(paramNr >= PAR_VEL_DEST_1 && paramNr <= PAR_VEL_DEST_6 )
			voiceNr=(uint8_t)(paramNr-PAR_VEL_DEST_1);
		else if(paramNr >= PAR_TARGET_LFO1 && paramNr <= PAR_TARGET_LFO6) {
			// this is the voice being targeted, not the lfo number
			voiceNr=(uint8_t)(parameter_values[PAR_VOICE_LFO1+(paramNr - PAR_TARGET_LFO1)]-1);
		}
		s=pgm_read_byte(&modTargetVoiceOffsets[voiceNr].start);
		e=pgm_read_byte(&modTargetVoiceOffsets[voiceNr].end);
		// start will be a non-zero value. We need to allow setting a value of 0 (off) or
		// something within the start to end range.
		rng=(uint8_t)(frac * ((e-s)+1)); // where in the range we are
		if(!rng)
			return 0; // set to off
		else
			return (uint8_t)((s-1)+rng); // set to an offset beginning at start of range
		}
		break;

	case DTYPE_AUTOM_TARGET:
		//**AUTOM - scale to our range for pot value
		return (uint8_t)((getNumModTargets()-1) * frac);
	case DTYPE_0B255:
		return value;
		break;
	case DTYPE_VOICE_LFO:
		// the LFO target voice number is stored and displayed as 1 based
		return (uint8_t)(1 + 5*frac);
		break;
   case DTYPE_0B16:
		return (uint8_t)(16*frac);
		break;
	case DTYPE_1B16:
		return (uint8_t)(1 + 15*frac);
		break;
	case DTYPE_0B15:
		/* Session 036 fix: keep in sync with the DTYPE_0B15 clamp in
		   menu_encoderChangeParameter() -- PAR_EUKLID_SUBSTEP_ROTATION must
		   stay 0-7, not the shared dtype's normal 0-15, to avoid
		   euklid_rotatePattern() spilling a sub-step rotation into the
		   main-step bitmask. This path (absolute-position pot/knob value
		   entry) is not currently reachable for this parameter in normal
		   UI flow, but is guarded defensively for consistency. */
		if(paramNr == PAR_EUKLID_SUBSTEP_ROTATION)
			return (uint8_t)(7*frac);
		return (uint8_t)(15*frac);
		break;
		// These are 0 or 1
	case DTYPE_MIX_FM:
	case DTYPE_ON_OFF:
	case DTYPE_0b1:
		return frac>0.5f;
		break;
	case DTYPE_MENU:
	{
		//get the used menu (upper 4 bit)
		const uint8_t menuId = pgm_read_byte(&parameter_dtypes[paramNr]) >> 4;
		//get the number of entries
		uint8_t numEntries=getMaxEntriesForMenu(menuId);
		return (uint8_t)(frac * (numEntries-1));
	}

		break;
	// These ones are all in the 0-127 range
	case DTYPE_PM63:
	case DTYPE_NOTE_NAME:
	case DTYPE_0B127:
	default:
		return (uint8_t)(127*frac);
		break;
	}	
	return 0;
}

//-----------------------------------------------------------------
void menu_parseKnobValue(uint8_t potNr, uint8_t potValue)
{
	screensaver_touch();

	//leave edit mode if active
	

	if(menu_activePage == SAVE_PAGE) 
      {
      editModeActive = 0;
		menu_handleSaveScreenKnobValue(potNr, potValue);
		return;
	   }
   else if (menu_activePage == LOAD_PAGE)
      {
      menu_handleLoadScreenKnobValue(potNr, potValue);
      return;
      }
   editModeActive = 0;
   
	const uint8_t activePage		= (menuIndex&MASK_PAGE)>>PAGE_SHIFT;
	const uint8_t activeParameter	= menuIndex & MASK_PARAMETER;
	const uint8_t isOn2ndPage		= ( activeParameter > 3) * 4;
	uint16_t paramNr				= pgm_read_word(&menuPages[menu_activePage][activePage].bot1 +
										potNr + isOn2ndPage);

	/* ¢ RX-FILTER POT LOCK  (Session 039)
	   WHAT: ignores pot movement on RxFilter while a ¢ entry (value >= 16) is
	         selected.
	   WHY:  ¢ entries are encoder-only: they can be selected and left only in
	         encoder edit mode. Pots already cannot reach them (getDtypeValue()
	         scales over 16 entries); without this guard a pot nudge would
	         silently drop the CC38 workaround. The value stays visible in the
	         4-parameter view.
	   INPUT:  paramNr, parameter_values[PAR_MIDI_FILT_RX].
	   OUTPUT: early return (no value change, no STM traffic).
	   ACCESSORS: this function only.
	   AFFILIATES: menu_getDtypeMenuEntryCount() (encoder range),
	               menu_parseGlobalParam() PAR_MIDI_FILT_RX. */
	if(paramNr == PAR_MIDI_FILT_RX
	   && parameter_values[PAR_MIDI_FILT_RX] >= MIDI_FILTER_CENT_FIRST)
		return;

	//parameter fetch. given the value of the pot, and the parameter number
	// fetch the actual numeric value that the parameter should be set to
	const uint8_t dtypeValue = getDtypeValue(potValue,paramNr);


	if(parameter_values[paramNr] == dtypeValue)
	{
		//turn lock off for current pot
		DISABLE_CONV_WARNING
		parameterFetch &= ~(1<<potNr);
		END_DISABLE_CONV_WARNING
		return;
	}

	menu_processSpecialCaseValues(paramNr/*,&dtypeValue*/);

	//if parameter lock is on, do nothing
	if((parameterFetch & (1<<potNr)) != 0 )
		return;


	//make changes temporary while an automation step is armed - save original value
	if((buttonHandler_resetLock==0) && buttonHandler_getArmedAutomationStep() != NO_STEP_SELECTED)
	{
		buttonHandler_originalValue = parameter_values[paramNr];
		buttonHandler_originalParameter = paramNr;
		buttonHandler_resetLock = 1;
	}

	//update parameter value to the new value
	parameter_values[paramNr] = dtypeValue;

	switch(pgm_read_byte(&parameter_dtypes[paramNr]) & 0x0F)
	{
	case DTYPE_TARGET_SELECTION_VELO:
	{
		//**VELO knob value range. convert to param before sending
		// the value we have is an index into modTargets, we need a parameter
		const uint8_t value = (uint8_t)pgm_read_word(&modTargets[dtypeValue].param);

		uint8_t upper,lower;
		upper = (uint8_t)(((value&0x80)>>7) | ((((uint8_t)(paramNr - PAR_VEL_DEST_1))&0x3f)<<1));
		lower = value&0x7f;
		avrComms_sendData(CC_VELO_TARGET,upper,lower);
	}
		break;

	case DTYPE_VOICE_LFO:
	{
		// **LFOTARGFIX - ensure that the lfo mod target is pointing to the same type of modulation
		// on the new voice
		const uint8_t newTargVal=getModTargetIdxFromGapIdx((uint8_t)(dtypeValue-1),menu_TargetVoiceGapIndex);

		// update the lfo mod target if applicable
		parameter_values[PAR_TARGET_LFO1 + (paramNr - PAR_VOICE_LFO1)] = newTargVal;

		// determine the real param value given the index into modTargets
		const uint8_t value =  (uint8_t)pgm_read_word(&modTargets[newTargVal].param);

		uint8_t upper,lower;
		upper = (uint8_t)(((value&0x80)>>7) | ((((uint8_t)(paramNr - PAR_VOICE_LFO1))&0x3f)<<1));
		lower = value&0x7f;
		avrComms_sendData(CC_LFO_TARGET,upper,lower);
	}
		break;
	case DTYPE_TARGET_SELECTION_LFO:
	{

		// **LFOTARGFIX - save the gap index
		menu_TargetVoiceGapIndex = getModTargetGapIndex(dtypeValue);

		//**LFO convert to param value and send
		// determine the real param value given the index into modTargets
		const uint8_t value = (uint8_t)pgm_read_word(&modTargets[dtypeValue].param);
		uint8_t upper,lower;
		upper = (uint8_t)(((value&0x80)>>7) | ((((uint8_t)(paramNr - PAR_TARGET_LFO1))&0x3f)<<1));
		lower = value&0x7f;
		avrComms_sendData(CC_LFO_TARGET,upper,lower);
	}
		break;
   case DTYPE_AUTOM_TARGET:
   {
      if (menu_activePage == PERFORMANCE_PAGE)
      {
         // bc: these are only used in perf for macro automation targets
#if 0
   		const uint8_t value = (uint8_t)pgm_read_word(&modTargets[dtypeValue].param); // the value of the mod target
         uint8_t lower = value&0x7f;
         uint8_t upper = (uint8_t)
                         ( ( ( (paramNr - PAR_MAC1_DST1) //  MAC1_DST1=0, M1D2=2, M2D1=4, M2D2=6
                              >>1 )                      //  MAC1_DST1=0, M1D2=1, M2D1=2, M2D2=3
                              <<2 )                      //  shift over 2 to make room for upper mod target bit
                              |(value>>7) );
                              
         avrComms_sendData(MACRO_CC,upper,lower);
#endif
      }
      else
      {
         if(paramNr < END_OF_SOUND_PARAMETERS) // => Sound Parameter: menu edit (Session 039 P3)
            menu_sendSoundParameter(paramNr, dtypeValue);
   		else
   			menu_parseGlobalParam(paramNr,dtypeValue);
      }
   }
   break;

	default: // all other sound parameters are send as CC or CC2. anything else is handled specially
      if(menu_isVoiceMorphAmountParam(paramNr))
         menu_sendVoiceMorphAmount(paramNr, dtypeValue);
		else if(paramNr < END_OF_SOUND_PARAMETERS) // => Sound Parameter: menu edit (Session 039 P3)
			menu_sendSoundParameter(paramNr, dtypeValue);
		else
			menu_parseGlobalParam(paramNr,dtypeValue);
		break;
	}

}
//----------------------------------------------------------------
void menu_reloadKit()
{
   /* Reload now comes from STM temporary preset storage, not the AVR file-load
      snapshots. That keeps SHIFT+PLAY aligned with the STM-owned temp/normal
      storage model and prevents reload from fighting the protected .prf/.all
      background-load path. */
   if(preset_isBackgroundTempPresetPlaybackActive())
   {
      /* While a protected .prf/.all background load is still playing from
         temporary preset storage, reload must stay disabled. Copying temp back
         into normal at that point would collapse the protected old sound into
         the newly loaded normal image. */
      return;
   }

   avrComms_sendByte(PATCH_RESET);
}
//----------------------------------------------------------------
uint8_t menu_getActivePage()
{
	return menu_activePage;
}
//----------------------------------------------------------------
uint8_t menu_getActiveVoice()
{
	return menu_activeVoice;
}
//----------------------------------------------------------------
void menu_setActiveVoice(uint8_t voiceNr)
{
	// **LFOTARGFIX - save the gap index
	menu_TargetVoiceGapIndex = getModTargetGapIndex(parameter_values[PAR_TARGET_LFO1+voiceNr]);
	menu_activeVoice = voiceNr;
   avrComms_sendData(SEQ_CC, SEQ_REQUEST_EUKLID_PARAMS, voiceNr);
}
//----------------------------------------------------------------
uint8_t menu_areMuteLedsShown()
{
	return menu_muteModeActive;
}
//----------------------------------------------------------------

//--AS given a 0-127 number will return a midi note name
void setNoteName(uint8_t num, char *buf)
{
	uint8_t n = num%12; // get the note number (0 is C, 9 is A, 11 is B)
	// get the note letter ascii A=65, C=67. All repeat twice except E and B.
	// B is at the end, so only E presents a hurdle
	buf[0]=(char)((n>8 ? 60 : 67) + ((n+(n>4))/2));
	// determine natural or sharp (# if the note is 1, 3, 6, 8, 10)
	buf[1]=(n < 5 && (n&1)) || (n > 5 && !(n&1)) ? '#' : ' ';
	// determine the octave 0=48
	buf[2]=(char)(48+(num / 12));
}


/* fill up to 3 bytes of a buffer with string representation of a number */
// space padded unsigned
void numtostrpu(char *buf, uint8_t num, char pad)
{
    if(num > 99) {
        buf[0]=(char)('0'+(num / 100));
        num %= 100;
    } else {
        buf[0]=pad;
    }
    if(num > 9) {
        buf[1]=(char)('0'+(num / 10));
        num %=10;
    } else if(buf[0]==pad) {
        buf[1]=pad;
    } else
        buf[1]='0';
    buf[2]=(char)('0'+num);
}

// space padded signed
void numtostrps(char *buf, int8_t num)
{

    if(num > 99) {
        buf[0]=(char)('0'+(num / 100));
        num %= 100;
    } else if(num < 0) {
        buf[0]='-';
        num = (int8_t)(-num);
    } else {
        buf[0]=' ';
    }

    if(num > 9) {
        buf[1]=(char)('0'+(num / 10));
        num %=10;
    } else if(buf[0] < '0' /* space or minus */) {
        buf[1]=' ';
    } else {
        buf[1]='0';
    }
    buf[2]=(char)('0'+num);
}

// non space padded unsigned
void numtostru(char *buf, uint8_t num)
{
    uint8_t b=0;
    if(num > 99) {
        buf[b++]=(char)('0'+(num / 100));
        num %= 100;
        if(num < 10)
            buf[b++]='0';
    }
    if(num > 9) {
        buf[b++]=(char)('0'+(num / 10));
        num %=10;
    }
    buf[b]=(char)('0'+num);
}

// non space padded signed
#if 0
void numtostrs(char *buf, int8_t num)
{
    uint8_t b=0;
    if(num > 99) {
        buf[b++]=(char)('0'+(num / 100));
        num %= 100;
        if(num < 10)
            buf[b++]='0';
    } else if(num < 0) {
        buf[b++]='-';
        num=(int8_t)(-num);
    }

    if(num > 9) {
        buf[b++]=(char)('0'+(num / 10));
        num %=10;
    }
    buf[b]=(char)('0'+num);
}
#endif

// make 1st 3 letters of buffer uppercase
void upr_three(char *buf)
{
	DISABLE_CONV_WARNING
	if(*buf >='a' && *buf <= 'z')
		(*buf) -=32;
	buf++;
	if(*buf >='a' && *buf <= 'z')
		(*buf) -=32;
	buf++;
	if(*buf >='a' && *buf <= 'z')
		(*buf) -=32;
	buf++;
	END_DISABLE_CONV_WARNING
}
