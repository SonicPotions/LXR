//---------------------------------------------------
/*! \file menu.h
    \brief handles the menu strukture on the LCD
    
    
*/

// Created:      2012-04-21 (13:47), by Julian Schmidt
// Coypright(c): Julian Schmidt
//---------------------------------------------------

#ifndef __LCD_MENU_MANAGER__
#define __LCD_MENU_MANAGER__

#include <avr/io.h>
#include "../Parameters.h"
//-----------------------------------------------------------------
//#define NUM_PAGES 12		
#define NUM_SUB_PAGES 8	// a maximum of 8 subpages 
#define MASK_PARAMETER 0x07

#define MASK_PAGE 0xf8
#define PAGE_SHIFT 3
//-----------------------------------------------------------------

#define PATTERN_END_MARKER 127
//the currently shown page from the LCD
extern uint8_t menu_activePage;
extern uint8_t menu_activeVoice;
extern uint8_t menu_playedPattern;
extern uint8_t menu_shownPattern;

void menu_setShownPattern(uint8_t patternNr);
uint8_t menu_getViewedPattern();


enum PageNames
{
	VOICE1_PAGE,
	VOICE2_PAGE,
	VOICE3_PAGE,
	VOICE4_PAGE,
	VOICE5_PAGE,
	VOICE6_PAGE,
	VOICE7_PAGE,
	MENU_MIDI_PAGE,
	LOAD_PAGE,
	SAVE_PAGE,
	/*
	LFO1_PAGE, //TODO load und savepage verschwenken speicher... nicht gefüllt. ans ende und aus dem array nehmen?
	LFO2_PAGE,
	LFO3_PAGE,
	LFO4_PAGE,
	LFO5_PAGE,
	LFO6_PAGE,
	*/
	
	PERFORMANCE_PAGE,
	SEQ_PAGE,
	EUKLID_PAGE,
	PATTERN_SETTINGS_PAGE,
	RECORDING_PAGE,
	SOM_PAGE,
	
	//The Voice settings -> Midi Channel, Audio Out...
	/*
	VOICE1_SETTINGS_PAGE,
	VOICE2_SETTINGS_PAGE,
	VOICE3_SETTINGS_PAGE,
	VOICE4_SETTINGS_PAGE,
	VOICE5_SETTINGS_PAGE,
	VOICE6_SETTINGS_PAGE,
	*/
	
	NUM_PAGES
};
//-----------------------------------------------------------------
enum NamesEnum
{
	TEXT_EMPTY = 0,
	TEXT_COARSE,
	TEXT_FINE,
	TEXT_ATTACK,
	TEXT_DECAY,
	TEXT_PITCH_DECAY,
	TEXT_MOD_AMOUNT,
	TEXT_FM_AMOUNT,
	TEXT_FM_FREQ,
	TEXT_DRIVE,
	TEXT_VOLUME,
	TEXT_PAN,
	TEXT_NOISE,
	TEXT_MIX,
	TEXT_REPEAT,
	TEXT_FILTER_F,
	TEXT_FILTER_RESO,
	TEXT_FILTER_TYPE,
	TEXT_MOD_OSC1_FREQ,
	TEXT_MOD_OSC2_FREQ,
	TEXT_MOD_OSC1_GAIN,
	TEXT_MOD_OSC2_GAIN,
	
	TEXT_FREQ_LFO,
	TEXT_MOD_LFO,
	TEXT_WAVE_LFO,
	TEXT_TARGET_LFO,
	TEXT_SYNC_LFO,
	TEXT_RETRIGGER_LFO,
	TEXT_OFFSET_LFO,
	TEXT_TARGET_VOICE_LFO,
	TEXT_EG_SLOPE,
	TEXT_DECAY_CLOSED,
	TEXT_DECAY_OPEN,
	TEXT_WAVEFORM,
	
	TEXT_TRANSIENT_WAVE,
	TEXT_TRANSIENT_VOLUME,
	TEXT_TRANSIENT_FREQ,
	
	TEXT_EQ_GAIN,
	TEXT_EQ_FREQ,
#if USE_CODEC_EQ	
	TEXT_CODEC_EQ_ENABLE,
	TEXT_CODEC_EQ_FB,
	TEXT_CODEC_EQ_FT,
	TEXT_CODEC_EQ_GB,
	TEXT_CODEC_EQ_GT,
	
	TEXT_CODEC_LIMIT_ENABLE,
	TEXT_CODEC_LIMIT_ATTACK,
	TEXT_CODEC_LIMIT_RELEASE,
	TEXT_CODEC_LIMIT_MAX,
	TEXT_CODEC_LIMIT_MIN,
#endif
	TEXT_ROLL_SPEED,
	TEXT_X_FADE,
	
	TEXT_STEP_VELOCITY,
	TEXT_NOTE,
	TEXT_PROBABILITY,
	TEXT_ACTIVE_STEP,
	TEXT_PAT_LENGTH,
	TEXT_NUM_STEPS,
	
	//Global Parameters
	TEXT_BPM,
	TEXT_MIDI_CHANNEL,
	TEXT_AUDIO_OUT,
	TEXT_SAMPLE_RATE,
	TEXT_PATTERN_BEAT,
	TEXT_PATTERN_NEXT,
	
	
	//TEXT_OSC_PHASE,
	TEXT_MODE,
	TEXT_OSC_VOLUME,
	TEXT_FILTER_DRIVE,
	
	TEXT_VEL_DEST,
	TEXT_VEL_AMT,
	TEXT_VEL_MOD_VOL,
	TEXT_FETCH,
	TEXT_FOLLOW,
	TEXT_QUANTISATION,
	TEXT_AUTOMATION_TRACK,
	TEXT_PARAM_DEST,	
	TEXT_PARAM_VAL,
	TEXT_SHUFFLE,
	TEXT_SCREENSAVER_ON_OFF,
	
	TEXT_SKIP, //spacer to insert if 1st of 2 pages has only 3 parameters
	
	TEXT_POS_X,
	TEXT_POS_Y,
	TEXT_FLUX,
	TEXT_SOM_FREQ,
	TEXT_MIDI_MODE,
	
	NUM_NAMES
};
//-----------------------------------------------------------------
enum WaveformNamesEnum
{
	WAVEFORM_SINE,
	WAVEFORM_TRI,
	WAVEFORM_SAW,
	WAVEFORM_REC,
	WAVEFORM_NOISE,
	WAVEFORM_CRASH,
};	
//-----------------------------------------------------------------
enum shortNamesEnum
{
	SHORT_EMPTY,
	SHORT_COARSE,
	SHORT_FINE,
	SHORT_ATTACK,
	SHORT_DECAY,
	SHORT_EG2,
	SHORT_MOD,
	SHORT_FM_AMNT,
	SHORT_FREQ,
	SHORT_DRIVE,
	SHORT_VOL,
	SHORT_PAN,
	SHORT_NOISE,
	SHORT_REPEAT,
	SHORT_MIX,
	SHORT_FIL_RESO,
	SHORT_FIL_TYPE,
	SHORT_MOD_OSC1_FREQ,
	SHORT_MOD_OSC2_FREQ,
	SHORT_MOD_OSC1_GAIN,
	SHORT_MOD_OSC2_GAIN,
	SHORT_WAVE,
	SHORT_DEST,
	SHORT_SYNC,
	SHORT_RETRIGGER,
	SHORT_OFFSET,
	SHORT_VOICE,
	SHORT_SLOPE,
	SHORT_DECAY1,
	SHORT_DECAY2,
	SHORT_EQ_GAIN,
	SHORT_EQ_FREQ,
#if USE_CODEC_EQ	
	SHORT_EQ,
	SHORT_LIMITER,
	SHORT_RELEASE,
	SHORT_MAX,
	SHORT_MIN,
#endif	
	SHORT_ROLL,
	SHORT_MORPH,
	SHORT_NOTE,
	SHORT_PROBABILITY,
	SHORT_STEP,
	SHORT_LENGTH,
	
	SHORT_BPM,
	SHORT_CHANNEL,
	SHORT_OUT,
	SHORT_SR,
	
	SHORT_NXT,

	SHORT_MODE,
	SHORT_VELOCITY,
	SHORT_FETCH,
	SHORT_FOLLOW,
	SHORT_QUANT,
	SHORT_TRACK,
	SHORT_VALUE,
	SHORT_SHUFFLE,
	SHORT_SCREEN_SAVER,
	SHORT_X,
	SHORT_Y,
	SHORT_FLUX,
	SHORT_MIDI,
	

	
};
//-----------------------------------------------------------------
enum catNamesEnum
{
	CAT_EMPTY,
	CAT_OSC,
	CAT_VELO_EG,
	CAT_PITCH_EG,
	CAT_PITCH_MOD,
	CAT_FM,
	CAT_VOICE,
	CAT_NOISE,
	CAT_NOISE_OSC,
	CAT_FILTER,
	CAT_MOD_OSC,
	CAT_LFO,
	CAT_TRANS,
	CAT_EQ,
#if USE_CODEC_EQ	
	CAT_LIMIT,
#endif
	CAT_PATTERN,
	CAT_SOUND,
	CAT_STEP,
	CAT_EUKLID,
	CAT_GLOBAL,
	CAT_VELOCITY,
	CAT_PARAMETER,
	CAT_SEQUENCER,
	CAT_GENERATOR,
	CAT_MIDI,
	
};
//-----------------------------------------------------------------
enum longNamesEnum
{
	LONG_EMPTY,
	LONG_COARSE,
	LONG_FINE,
	LONG_ATTACK,
	LONG_DECAY,
	LONG_AMOUNT,
	LONG_FREQ,
	LONG_DRIVE,
	LONG_VOL,
	LONG_PAN,
	LONG_MIX,
	LONG_REPEAT_CNT,
	LONG_RESONANCE,
	LONG_TYPE,
	LONG_GAIN,
	LONG_WAVE,
	LONG_DEST_PARAM,
	LONG_CLOCKSYNC,
	LONG_RETRIGGER,
	LONG_OFFSET,
	LONG_DEST_VOICE,
	LONG_SLOPE,
	LONG_DECAY_CLOSED,
	LONG_DECAY_OPEN,
#if USE_CODEC_EQ	
	LONG_ENABLE,
	LONG_RELEASE,
	LONG_THRESH_MAX,
	LONG_THRESH_MIN,
#endif
	LONG_ROLLRATE,
	LONG_MORPH,
	LONG_NOTE,
	LONG_PROBABILITY,
	LONG_NUMBER,
	LONG_LENGTH,
	LONG_STEPS,
	LONG_TEMPO,
	LONG_AUDIO_OUT,
	LONG_MIDI_CHANNEL,
	LONG_SAMPLE_RATE,
	LONG_NEXT_PAT,
	LONG_PHASE,
	LONG_MODE,
	LONG_VOLUME_MOD,
	LONG_FETCH,
	LONG_FOLLOW,
	LONG_QUANTISATION,
	LONG_AUTOMATION_TRACK,
	LONG_AUTOMATION_DEST,
	LONG_AUTOMATION_VAL,
	LONG_SHUFFLE,
	LONG_SCREENSAVER,
	LONG_X,
	LONG_Y,
	LONG_FLUX,
	LONG_VELOCITY,
	
	
};
//-----------------------------------------------------------------

//-----------------------------------------------------------------
/** struct describing a display page.
top values are from the 'NamesEnum' selecting a text to display.
bottom values are from the 'ParamEnums' selecting the parameter value to show*/
typedef struct PageStruct
{
	uint8_t top1;
	uint8_t top2;
	uint8_t top3;
	uint8_t top4;
	
	uint8_t top5;
	uint8_t top6;
	uint8_t top7;
	uint8_t top8;
	
	uint8_t bot1;
	uint8_t bot2;
	uint8_t bot3;
	uint8_t bot4;
		
	uint8_t bot5;
	uint8_t bot6;
	uint8_t bot7;
	uint8_t bot8;
}Page;
//-----------------------------------------------------------------
typedef struct NameStruct
{
	const uint8_t shortName; /**< 3 letter short name id*/
	const uint8_t category; /**< category name id e.g. Filter, Noise, Velocity etc...*/
	const uint8_t longName; /**< full name id of the parameter*/
}Name;
//-----------------------------------------------------------------
enum Datatypes
{
	DTYPE_0B255,	//0:255
	DTYPE_0B127,	//0-127
	DTYPE_PM100,	//-100:100
	DTYPE_MENU,		//text type outputs 
	DTYPE_PM63,		//-63 <-> 63
	DTYPE_1B16,		//1-16
	DTYPE_ON_OFF,	//0-1
	DTYPE_MIX_FM,	//0-1
	DTYPE_TARGET_SELECTION_LFO,
	DTYPE_TARGET_SELECTION_VELO,	
	DTYPE_VOICE_LFO,
	DTYPE_AUTOM_TARGET,
	DTYPE_0b1,

};

//-----------------------------------------------------------------
typedef struct ParameterStruct
{
	uint8_t value;
	enum Datatypes dtype;	/**< The data type of the parameter -> 0x0F = 16 Data Types field; 0xF0 = 16 extra parameter fields*/
}Parameter;
//-----------------------------------------------------------------
extern Parameter parameters[NUM_PARAMS];
//TODO parameters2 doesn't need a dtype field -> save memory
extern Parameter parameters2[END_OF_SOUND_PARAMETERS];

extern const Page menuPages[NUM_PAGES][NUM_SUB_PAGES];
//-----------------------------------------------------------------
/** forces a complete repaint of the display*/
void menu_repaintAll();
//-----------------------------------------------------------------
//repaint the screen. show all changes*/
void menu_repaint();
//-----------------------------------------------------------------
/**set all save state parameters to 0*/
void menu_resetSaveParameters();
//-----------------------------------------------------------------
void menu_init();
//-----------------------------------------------------------------
/** takes the values received from the encoder and translates it to menu changes*/
void menu_parseEncoder(int8_t inc, uint8_t button);
//-----------------------------------------------------------------
void menu_switchPage(uint8_t pageNr);
//-----------------------------------------------------------------
void menu_switchSubPage(uint8_t subPageNr);
//-----------------------------------------------------------------
void menu_resetActiveParameter();
//-----------------------------------------------------------------
uint8_t menu_getSubPage();
//-----------------------------------------------------------------
void menu_parseKnobValue(uint8_t potNr, uint8_t value);
//-----------------------------------------------------------------
void menu_parseGlobalParam(uint8_t paramNr, uint8_t value);
//-----------------------------------------------------------------
/** all parameters are send to the cortex soundchip. called after loading a preset into the avr ram*/
void menu_sendAllParameters();
//-----------------------------------------------------------------
/** get the currently active page nr*/
uint8_t menu_getActivePage();
//-----------------------------------------------------------------
/** get the currently active voice nr*/
uint8_t menu_getActiveVoice();
void menu_setActiveVoice(uint8_t voiceNr);
//-----------------------------------------------------------------
/** used to upodate all global parameters that need processing after a preset is loaded*/
void menu_sendAllGlobals();
//-----------------------------------------------------------------
uint8_t getModTargetValue(uint8_t value, uint8_t voiceNr);
#endif
