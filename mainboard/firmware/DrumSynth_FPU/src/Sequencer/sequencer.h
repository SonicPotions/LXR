/*
 * sequencer.h
 *
 *  Created on: 11.04.2012
 *      Author: Julian
 */

#ifndef SEQUENCER_H_
#define SEQUENCER_H_


/*
 * sequencer.c
 *
 *  Created on: 11.04.2012
 *      Author: Julian
 */

#include "stm32f4xx.h"
#include "globals.h"

 /**<
  * we have 6 voices
  * 3 drums
  * 1 snare/claps
  * 1 cymbal/snare
  * 1 hiHat
  * track 7 is the open hh... it triggers the highhat voice but with longer decay. it chokes the closed hihat*/
#define NUM_TRACKS 7
#define NUM_PATTERN 8
#define NUM_STEPS 128

#define SEQ_DEFAULT_NOTE 63

#define STEP_ACTIVE_MASK 0x80
#define STEP_VOLUME_MASK 0x7f

#define PATTERN_END_MASK 0x7f
#define PATTERN_END 0x80

#define SEQ_NEXT_RANDOM 		0x08
#define SEQ_NEXT_RANDOM_PREV 	0x09

#define ROLL_VOLUME 100

enum Seq_QuantisationEnum
{
	NO_QUANTISATION,
	QUANT_8,
	QUANT_16,
	QUANT_32,
	QUANT_64,
};

typedef struct StepStruct
{
	//uint8_t  	active;		// bit 0: on/off
	uint8_t 	volume;		// 0-127 volume -> 0x7f => lower 7 bit, upper bit => active
	uint8_t  	prob;		//step probability
	uint8_t		note;		//midi note value 0-127 -> 0x7f, upper bit signals pattern end

	//parameter automation
	uint8_t 	param1Nr;
	uint8_t 	param1Val;

	uint8_t 	param2Nr;
	uint8_t 	param2Val;

}Step;

typedef struct PatternSettingsStruct
{
	uint8_t 	changeBar;		// change on every Nth bar to the next pattern
	uint8_t  	nextPattern;	// [0:9] (0-7) are the 8 patterns, (8) is random previous, (9) is random all
}PatternSetting;

typedef struct PatternSetStruct
{
	Step seq_subStepPattern[NUM_PATTERN][NUM_TRACKS][NUM_STEPS];
	uint16_t seq_mainSteps[NUM_PATTERN][NUM_TRACKS];
	PatternSetting seq_patternSettings[NUM_PATTERN];
}PatternSet;

typedef struct TempPatternStruct
{
	Step seq_subStepPattern[NUM_TRACKS][NUM_STEPS];
	uint16_t seq_mainSteps[NUM_TRACKS];
	PatternSetting seq_patternSettings;
}TempPattern;

extern uint8_t seq_activePattern;
extern uint8_t seq_newPatternAvailable;

//extern PatternSet* seq_activePatternSetPtr;
extern PatternSet seq_patternSet;
extern TempPattern seq_tmpPattern;

//extern uint8_t seq_midiChannel;
/*
extern volatile Step seq_subStepPattern[NUM_PATTERN][NUM_TRACKS][NUM_STEPS];
extern volatile uint16_t seq_mainSteps[NUM_PATTERN][NUM_TRACKS];
extern PatternSetting seq_patternSettings[NUM_PATTERN];
*/

extern uint8_t seq_syncStepCnt;
extern uint8_t seq_selectedStep;
//if != 0 the recording mode is active
//extern uint8_t seq_recordActive;

//------------------------------------------------------------------------------
//PatternSet* seq_getActivePatternSet();
//------------------------------------------------------------------------------
//PatternSet* seq_getInactivePatternSet();
void seq_triggerVoice(uint8_t voiceNr, uint8_t vol, uint8_t note);
//------------------------------------------------------------------------------
void seq_setShuffle(float shuffle);
//------------------------------------------------------------------------------
void seq_setTrackLength(uint8_t trackNr, uint8_t length);
//------------------------------------------------------------------------------
uint8_t seq_getTrackLength(uint8_t trackNr);
//------------------------------------------------------------------------------
//void seq_switchActivePatternSet();
void seq_activateTmpPattern();
//------------------------------------------------------------------------------
void seq_init();
//------------------------------------------------------------------------------
/** call periodically to check if the next step has to be processed */
void seq_tick();
//------------------------------------------------------------------------------
void seq_armAutomationStep(uint8_t stepNr, uint8_t track,uint8_t isArmed);
//------------------------------------------------------------------------------
void seq_resetDeltaAndTick();
//------------------------------------------------------------------------------
void seq_setBpm(uint16_t bpm);
//------------------------------------------------------------------------------
uint16_t seq_getBpm();
//------------------------------------------------------------------------------
void seq_sync();
//------------------------------------------------------------------------------
void seq_nextStep();
//------------------------------------------------------------------------------
uint8_t seq_getExtSync();
//------------------------------------------------------------------------------
void seq_setQuantisation(uint8_t value);
//------------------------------------------------------------------------------
void seq_setExtSync(uint8_t isExt);
//------------------------------------------------------------------------------
/** switch to pattern patNr after the current pattern has finished*/
void seq_setNextPattern(const uint8_t patNr);
//------------------------------------------------------------------------------
void seq_toggleStep(uint8_t voice, uint8_t stepNr, uint8_t patternNr);
//------------------------------------------------------------------------------
void seq_toggleMainStep(uint8_t voice, uint8_t stepNr, uint8_t patternNr);
//------------------------------------------------------------------------------
void seq_setStep(uint8_t voice, uint8_t stepNr, uint8_t onOff);
//------------------------------------------------------------------------------
//uint8_t seq_isStepActive(uint8_t voice, uint8_t stepNr);
//------------------------------------------------------------------------------
//void seq_setStepProbability(uint8_t voice, uint8_t stepNr, uint8_t value);
//------------------------------------------------------------------------------
void seq_setRunning(uint8_t isRunning);
//------------------------------------------------------------------------------
uint8_t seq_isRunning();
//------------------------------------------------------------------------------
inline uint8_t seq_isStepActive(uint8_t voice, uint8_t stepNr, uint8_t patternNr);
//------------------------------------------------------------------------------
inline uint8_t seq_isMainStepActive(uint8_t voice, uint8_t mainStepNr, uint8_t pattern);
//------------------------------------------------------------------------------
/** mutes and unmutes a track [0..maxTrack]*/
void seq_setMute(uint8_t trackNr, uint8_t isMuted);
//------------------------------------------------------------------------------
/** send step data to front panel. the whole step struct for one step is transmitted*/
void seq_sendStepInfoToFront(uint16_t stepNr);
//------------------------------------------------------------------------------
void seq_sendMainStepInfoToFront(uint16_t stepNr);
//------------------------------------------------------------------------------
void seq_setRoll(uint8_t voice, uint8_t onOff);
//------------------------------------------------------------------------------
void seq_setRollRate(uint8_t rate);
//------------------------------------------------------------------------------
/** add a note to the current pattern position*/
void seq_addNote(uint8_t trackNr,uint8_t vel);
//------------------------------------------------------------------------------
void seq_setRecordingMode(uint8_t active);
//------------------------------------------------------------------------------
void seq_clearTrack(uint8_t trackNr, uint8_t pattern);
//------------------------------------------------------------------------------
void seq_clearAutomation(uint8_t trackNr, uint8_t pattern, uint8_t automTrack);
//------------------------------------------------------------------------------
void seq_clearPattern(uint8_t pattern);
//------------------------------------------------------------------------------
void seq_copyTrack(uint8_t srcNr, uint8_t dstNr, uint8_t pattern);
//------------------------------------------------------------------------------
void seq_copyPattern(uint8_t src, uint8_t dst);
//------------------------------------------------------------------------------
//selects the automation track (0:1) that is recorded to
void seq_setActiveAutomationTrack(uint8_t trackNr);
//------------------------------------------------------------------------------
void seq_recordAutomation(uint8_t voice, uint8_t dest, uint8_t value);
//------------------------------------------------------------------------------
uint8_t seq_isNextStepSyncStep();
//------------------------------------------------------------------------------

#endif /* SEQUENCER_H_ */
