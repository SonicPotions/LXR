/*
 * sequencer.h
 *
 *  Created on: 11.04.2012
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


#ifndef SEQUENCER_H_
#define SEQUENCER_H_



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

// **PATROT these are not used anymore
//#define PATTERN_END_MASK 0x7f
//#define PATTERN_END 0x80

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
	uint8_t 	volume;		// 0-127 volume -> 0x7f => lower 7 bit, upper bit => active
	uint8_t  	prob;		//step probability (--AS todo we have one free bit here)
	uint8_t		note;		//midi note value 0-127 -> 0x7f, --AS todo upper bit is now free for other usages

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

// --AS **PATROT
typedef union {
	uint8_t value;
	struct {
		unsigned length:4;	// length (0 = default 16 steps)
		unsigned rotate:4;	// 0 means not rotated, 15 is max
	};
} LengthRotate;

typedef struct PatternSetStruct
{
	Step seq_subStepPattern[NUM_PATTERN][NUM_TRACKS][NUM_STEPS];
	uint16_t seq_mainSteps[NUM_PATTERN][NUM_TRACKS];
	PatternSetting seq_patternSettings[NUM_PATTERN];
	LengthRotate seq_patternLengthRotate[NUM_PATTERN][NUM_TRACKS];
}PatternSet;

typedef struct TempPatternStruct
{
	Step seq_subStepPattern[NUM_TRACKS][NUM_STEPS];
	uint16_t seq_mainSteps[NUM_TRACKS];
	PatternSetting seq_patternSettings;
	LengthRotate seq_patternLengthRotate[NUM_TRACKS]; // only used for length
}TempPattern;

extern uint8_t seq_activePattern;
extern uint8_t seq_newPatternAvailable;

//extern PatternSet* seq_activePatternSetPtr;
extern PatternSet seq_patternSet;
extern TempPattern seq_tmpPattern;


extern uint8_t seq_selectedStep;

extern uint8_t seq_resetBarOnPatternChange;

//------------------------------------------------------------------------------
void seq_triggerVoice(uint8_t voiceNr, uint8_t vol, uint8_t note);
//------------------------------------------------------------------------------
void seq_setShuffle(float shuffle);
//------------------------------------------------------------------------------
void seq_setTrackLength(uint8_t trackNr, uint8_t length);
//------------------------------------------------------------------------------
uint8_t seq_getTrackLength(uint8_t trackNr);
//------------------------------------------------------------------------------
void seq_setTrackRotation(uint8_t trackNr, const uint8_t rot);
//------------------------------------------------------------------------------
uint8_t seq_getTrackRotation(uint8_t trackNr);
//------------------------------------------------------------------------------
//void seq_activateTmpPattern();
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
void seq_setDeltaT(float delta);
//------------------------------------------------------------------------------
void seq_triggerNextMasterStep(uint8_t stepSize);
//------------------------------------------------------------------------------
void seq_setBpm(uint16_t bpm);
//------------------------------------------------------------------------------
uint16_t seq_getBpm();
//------------------------------------------------------------------------------
void seq_sync();
//------------------------------------------------------------------------------
//void seq_nextStep();
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
//void seq_setStep(uint8_t voice, uint8_t stepNr, uint8_t onOff);
//------------------------------------------------------------------------------
void seq_setRunning(uint8_t isRunning);
//------------------------------------------------------------------------------
uint8_t seq_isRunning();
//------------------------------------------------------------------------------
uint8_t seq_isStepActive(uint8_t voice, uint8_t stepNr, uint8_t patternNr);
//------------------------------------------------------------------------------
uint8_t seq_isMainStepActive(uint8_t voice, uint8_t mainStepNr, uint8_t pattern);
//------------------------------------------------------------------------------
/** mutes and unmutes a track [0..maxTrack]*/
void seq_setMute(uint8_t trackNr, uint8_t isMuted);
//------------------------------------------------------------------------------
uint8_t seq_isTrackMuted(uint8_t trackNr);
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
void seq_addNote(uint8_t trackNr,uint8_t vel, uint8_t note);
//------------------------------------------------------------------------------
void seq_setRecordingMode(uint8_t active);
//------------------------------------------------------------------------------
void seq_setErasingMode(uint8_t active);
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
//uint8_t seq_isNextStepSyncStep();
//------------------------------------------------------------------------------
// send a note off for a channel if there is a note playing on that channel
// if 0xff is specified, send a note off on all channels that have a note playing
void seq_midiNoteOff(uint8_t chan);
void seq_sendMidiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t veloc);

#endif /* SEQUENCER_H_ */
