/*
 * sequencer.c
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


#include "stm32f4xx.h"
#include "globals.h"
#include "DrumVoice.h"
#include "Snare.h"
#include "HiHat.h"
#include "random.h"
#include "Uart.h"
#include "MidiMessages.h"
#include "MidiVoiceControl.h"
#include "CymbalVoice.h"
#include "frontPanelParser.h"
#include "sequencer.h"
#include <string.h>
#include "usb_manager.h"
#include "clockSync.h"
#include "MidiParser.h"
#include "automationNode.h"
#include "SomGenerator.h"
#include "TriggerOut.h"


#define SEQ_PRESCALER_MASK 	0x03
#define MIDI_PRESCALER_MASK	0x04
static uint8_t seq_prescaleCounter = 0;

uint8_t seq_masterStepCnt=0;				/** keeps track of the played steps between 0 and 127 indipendend from the track counters*/
uint8_t seq_rollRate = 0x08;				//start with roll rate = 1/16
uint8_t seq_rollState = 0;					/**< each bit represents a voice. if bit is set, roll is active*/

static int8_t 	seq_stepIndex[NUM_TRACKS];	/**< we have 16 steps consisting of 8 sub steps = 128 steps. each track has its own counter to allow different pattern lengths*/

static uint16_t seq_tempo = 120;			/**< seq speed in bpm*/

static uint32_t	seq_lastTick = 0;			/**< stores the time the last step change occured*/
static float	seq_deltaT;					/**< time in [ms] until the next step
 	 	 	 	 	 	 	 	 	 	 	 1000ms = 1 sec
 	 	 	 	 	 	 	 	 	 	 	 1 min = 60 sec*/
uint8_t seq_activeAutomTrack=0;

uint8_t seq_delayedSyncStepFlag = 0;		//normally sync steps will only be advanced by external midi clocks in ext. sync mode
											//if the shuffle needs a delayed sync step, it is indicated here.

uint8_t seq_isSyncExternal = 0;

float seq_shuffle = 0;

static uint8_t seq_SomModeActive = 0;

static int8_t seq_armedArmedAutomationStep = -1;
static int8_t seq_armedArmedAutomationTrack = -1;

static uint8_t seq_mutedTracks=0;			/**< indicate which tracks are muted */
uint8_t seq_running = 0;					/**< 1 if running, 0 if stopped*/

uint8_t seq_activePattern = 0;				/**< the currently playing pattern*/
uint8_t seq_pendingPattern = 0;				/**< next pattern to play*/

uint8_t seq_selectedStep = 0;

uint8_t seq_recordActive = 0;				/**< set to 1 to activate the reording mode*/

uint8_t seq_quantisation = QUANT_16;

uint8_t seq_rndValue[NUM_TRACKS];			/**< random value for probability function*/

uint8_t seq_barCounter;						/**< counts the absolute position in bars since the seq was started */

static uint8_t seq_loadPendigFlag = 0;

// --AS keep track of which midi notes are playing
static uint8_t midi_chan_notes[16];		    /**< what note is playing on each channel */
static uint16_t midi_notes_on=0;		    /**< which channels have a note currently playing */

const float seq_shuffleTable[16] =
{
		0.f,
		0.015625f,
		0.0625f,
		0.140625f,
		0.25f,
		0.390625f,
		0.5625f,
		0.765625f,
		1.f,
		0.984375f,
		0.9375f,
		0.859375f,
		0.75f,
		0.609375f,
		0.4375f,
		0.234375f,
};

float seq_lastShuffle = 0;

PatternSet seq_patternSet;

TempPattern seq_tmpPattern;

uint8_t seq_newPatternAvailable = 0; //indicate that a new pattern has loaded in the background and we should switch

//for the automation tracks each track needs 2 modNodes
static AutomationNode seq_automationNodes[NUM_TRACKS][2];
//------------------------------------------------------------------------------
void seq_init()
{
	int i,j,k;

	for(i=0;i<NUM_TRACKS;i++) {
		autoNode_init(&seq_automationNodes[i][0]);
		autoNode_init(&seq_automationNodes[i][1]);
	}

	memset(seq_stepIndex,0,NUM_TRACKS);


	for(i=0;i<NUM_PATTERN;i++)
	{
		seq_patternSet.seq_patternSettings[i].changeBar 	= 0;	//default setting: zero repeats (play once then change)
		seq_patternSet.seq_patternSettings[i].nextPattern 	= i;	//default setting: repeat same pattern

		for(j=0;j<NUM_TRACKS;j++)
		{
			for(k=0;k<128;k++)
			{
				seq_patternSet.seq_subStepPattern[i][j][k].note 		= SEQ_DEFAULT_NOTE;
				seq_patternSet.seq_subStepPattern[i][j][k].param1Nr 	= NO_AUTOMATION;
				seq_patternSet.seq_subStepPattern[i][j][k].param1Val 	= 0;
				seq_patternSet.seq_subStepPattern[i][j][k].param2Nr	= NO_AUTOMATION;
				seq_patternSet.seq_subStepPattern[i][j][k].param2Val	= 0;
				seq_patternSet.seq_subStepPattern[i][j][k].prob		= 127;
				seq_patternSet.seq_subStepPattern[i][j][k].volume		= 100;

				//every 1st step in a substep pattern active
				if( (k%8) == 0)
				{
					seq_patternSet.seq_subStepPattern[i][j][k].volume		|= STEP_ACTIVE_MASK ;
				}
			}
			//all 16 main steps off
			seq_patternSet.seq_mainSteps[i][j] = 0;

		}
	}
}
//------------------------------------------------------------------------------
void seq_activateTmpPattern()
{
	memcpy(&seq_patternSet.seq_subStepPattern[seq_activePattern],&seq_tmpPattern.seq_subStepPattern,sizeof(Step)*NUM_TRACKS*NUM_STEPS);
	memcpy(&seq_patternSet.seq_mainSteps[seq_activePattern],&seq_tmpPattern.seq_mainSteps,sizeof(uint16_t)*NUM_TRACKS);
	memcpy(&seq_patternSet.seq_patternSettings[seq_activePattern],&seq_tmpPattern.seq_patternSettings,sizeof(PatternSetting));
}
//------------------------------------------------------------------------------
void seq_setShuffle(float shuffle)
{
	seq_shuffle = shuffle;
}
//------------------------------------------------------------------------------
void seq_setTrackLength(uint8_t trackNr, uint8_t length)
{
	int i;
	for(i=0;i<128;i++)
	{
		//clear all end markers
		if(seq_patternSet.seq_subStepPattern[seq_activePattern][trackNr][i].note & PATTERN_END)
		{
			seq_patternSet.seq_subStepPattern[seq_activePattern][trackNr][i].note &= ~PATTERN_END;
		}
	}
	//set new end marker
	if(length < 16)
	{
		seq_patternSet.seq_subStepPattern[seq_activePattern][trackNr][length*8].note |= PATTERN_END;
	}
}
//------------------------------------------------------------------------------
uint8_t seq_getTrackLength(uint8_t trackNr)
{
	int i;
	for(i=0;i<128;i++)
	{
		//clear all end markers
		if(seq_patternSet.seq_subStepPattern[seq_activePattern][trackNr][i].note & PATTERN_END)
		{
			return i/8;
		}
	}

	return 16;
}
//------------------------------------------------------------------------------
void seq_calcDeltaT(uint16_t bpm)
{
	//--- calc deltaT ----
	// für 4/4tel takt -> 1 beat = 4 main steps = 4*8 = 32 sub steps
	// 120 bpm 4/4tel = 120 * 1 beat / 60sec = 120 * 32 in 60 sec;
	seq_deltaT 	= (1000*60)/bpm; 	//bei 12 = 500ms = zeit für einen beat
	seq_deltaT /= 96.f; //we run the internal clock at 96ppq -> seq clock 32 ppq == prescaler 3, midi clock 24 ppq == prescale 4
	seq_deltaT *= 4;
	//32.f;					// zeit für einen beat / anzahl steps pro beat

	//--- calc shuffle ---
	if(seq_shuffle != 0)
	{
		//every 2nd and 4th 16th note in a beat is shifted full
		//=> step 8 and step 24
		//every 2nd 16th note in half a beat
		//every beat has 32 steps => half = 16
		uint8_t stepInHalfBeat = seq_masterStepCnt&0xf;
		const float shuffleFactor = seq_shuffleTable[stepInHalfBeat] * seq_shuffle;
		const float originalDeltaT = seq_deltaT;

		seq_deltaT += shuffleFactor * originalDeltaT * 16.f;
		seq_deltaT -= seq_lastShuffle * originalDeltaT * 16.f;

		seq_lastShuffle = shuffleFactor;
	}
}
//------------------------------------------------------------------------------
void seq_setBpm(uint16_t bpm)
{
	seq_tempo 	= bpm;
	//seq_calcDeltaT(bpm);
	lfo_recalcSync();
}
//------------------------------------------------------------------------------
uint16_t seq_getBpm()
{
	return seq_tempo;
}
//------------------------------------------------------------------------------
void seq_sync()
{
	sync_tick();
}
//------------------------------------------------------------------------------
void seq_setNextPattern(const uint8_t patNr)
{
	seq_pendingPattern = patNr;
	seq_loadPendigFlag = 1;
}
//------------------------------------------------------------------------------
void seq_sendMidi(uint8_t status, uint8_t data1, uint8_t data2)
{
	//TODO midi out seq mode
	//TODO usb und uart einjzeln aktivierbar

	//send to usb midi
	usb_sendMidi(status,data1,data2);

	//send to hardware midi out jack
	uart_sendMidiByte(status);
	uart_sendMidiByte(data1);
	uart_sendMidiByte(data2);
}
//------------------------------------------------------------------------------
void seq_parseAutomationNodes(uint8_t track, Step* stepData)
{
	//set new destination
	autoNode_setDestination(&seq_automationNodes[track][0], stepData->param1Nr);
	autoNode_setDestination(&seq_automationNodes[track][1], stepData->param2Nr);
	//set new mod value
	autoNode_updateValue(&seq_automationNodes[track][0], stepData->param1Val);
	autoNode_updateValue(&seq_automationNodes[track][1], stepData->param2Val);
}
//------------------------------------------------------------------------------
void seq_triggerVoice(uint8_t voiceNr, uint8_t vol, uint8_t note)
{
	uint8_t midiChan; // which midi channel to send a note on
	uint8_t midiNote; // which midi note to send

	if(voiceNr > 6) return;

	seq_parseAutomationNodes(voiceNr, &seq_patternSet.seq_subStepPattern[seq_activePattern][voiceNr][seq_stepIndex[voiceNr]]);

	if(voiceNr < 3)
	{
		Drum_trigger(voiceNr,vol, note);
	}
	else if(voiceNr<4)
	{
		Snare_trigger(vol,note);
	}
	else if(voiceNr<5)
	{
		Cymbal_trigger(vol,note);
	}
	else
	{
		HiHat_trigger(vol,voiceNr-5,note);
	}

	trigger_triggerVoice(voiceNr);

	//to retrigger the LFOs the Frontpanel AVR needs to know about note ons
	uart_sendFrontpanelByte(0x90);
	uart_sendFrontpanelByte(voiceNr);
	uart_sendFrontpanelByte(0);

	/* --AS getting ride of the midi mode
	if(midi_mode == MIDI_MODE_TRIGGER)
	{
		 midiChan = midi_MidiChannels[0];
		 midiNote=NOTE_VOICE1+voiceNr;
	}
	else
	{*/

	midiChan = midi_MidiChannels[voiceNr];

	//--AS the note that is played will be whatever is received unless we have a note override set
	// A note override is any non-zero value for this parameter
	if(midi_NoteOverride[voiceNr] == 0)
		midiNote = note;
	else
		midiNote = midi_NoteOverride[voiceNr];
	//}

	//--AS if a note is on for that channel send note-off first
	// The proper way to do a note off is with 0x80. 0x90 with velocity 0 is also used, however I think there is still
	// synth gear out there that doesn't recognize that properly.
	if((1<<midiChan) & midi_notes_on)
		seq_sendMidi(0x80 | midiChan,midi_chan_notes[midiChan],0);
	//send the new note to midi out
	seq_sendMidi(0x90 | midiChan,midiNote,
			seq_patternSet.seq_subStepPattern[seq_activePattern][voiceNr][seq_stepIndex[voiceNr]].volume&STEP_VOLUME_MASK);
	// finally, keep track of which notes are on so we can turn them off later
	midi_chan_notes[midiChan]=midiNote;
	midi_notes_on |= (1 << midiChan);



}
//------------------------------------------------------------------------------
void seq_nextStep()
{
	if(seq_running)
	{
		trigger_clockTick();
		seq_masterStepCnt++;

		//---- calc master step position. max value is 127. also take in regard the pattern end bit from the note value -----
		uint8_t masterStepPos;
		//if( (((seq_stepIndex[0]+1) &0x7f) == 0) || (((seq_patternSet.seq_subStepPattern[seq_activePattern][0][seq_stepIndex[0]+1]).note & PATTERN_END_MASK)>=PATTERN_END_MASK) )
		if( (((seq_stepIndex[0]+1) &0x7f) == 0) || (((seq_patternSet.seq_subStepPattern[seq_activePattern][0][seq_stepIndex[0]+1]).note & PATTERN_END)) )
		{
			masterStepPos = 0;
			//a bar has passed
			seq_barCounter++;
		}
		else
		{
			masterStepPos = seq_stepIndex[0]+1;
		}

		//-------- check if the master track has ended and check if a pattern switch is necessary --------
		if(masterStepPos == 0)
		{
			if(seq_activePattern == seq_pendingPattern)
			{
				//check pattern settings if we have to auto change patterns
				if(seq_barCounter%(seq_patternSet.seq_patternSettings[seq_activePattern].changeBar+1) == 0)
				{
					//we have to change to the next pattern
					if(seq_patternSet.seq_patternSettings[seq_activePattern].nextPattern < SEQ_NEXT_RANDOM)
					{
						seq_pendingPattern = seq_patternSet.seq_patternSettings[seq_activePattern].nextPattern;
					}
					else if(seq_patternSet.seq_patternSettings[seq_activePattern].nextPattern >= SEQ_NEXT_RANDOM)
					{
						uint8_t limit = seq_patternSet.seq_patternSettings[seq_activePattern].nextPattern - SEQ_NEXT_RANDOM +2;
						uint8_t rnd = GetRngValue() % limit;
						seq_pendingPattern = rnd;
					}
				}
			}

			// a new pattern is about to start
			// set pendingPattern active
			if((seq_activePattern != seq_pendingPattern) || seq_loadPendigFlag)
			{

				seq_loadPendigFlag = 0;
				//first check if 2 new pattern is available
				if(seq_newPatternAvailable)
				{
					seq_newPatternAvailable = 0;
					seq_activateTmpPattern();
				}

				seq_activePattern = seq_pendingPattern;

				//send the ack message to tell the front that a new pattern starts playing
				uart_sendFrontpanelByte(FRONT_SEQ_CC);
				uart_sendFrontpanelByte(FRONT_SEQ_CHANGE_PAT);
				uart_sendFrontpanelByte(seq_activePattern);
			}
		}

		//---------- now check if the master track is at a full beat position to flash the start/stop button --------
		if((masterStepPos&31) == 0)
		{
			//&32 <=> %32
			//a quarter beat occured (multiple of 32 steps in the 128 step pattern)
			//turn on the start/stop led (beat indicator)
			uart_sendFrontpanelByte(FRONT_STEP_LED_STATUS_BYTE);
			uart_sendFrontpanelByte(FRONT_LED_PULSE_BEAT);
			uart_sendFrontpanelByte(1);
		}
		else if ((masterStepPos&31) == 1)
		{
			//TODO datenmenge zur front reduzieren
			//turn it of again on the next step
			uart_sendFrontpanelByte(FRONT_STEP_LED_STATUS_BYTE);
			uart_sendFrontpanelByte(FRONT_LED_PULSE_BEAT);
			uart_sendFrontpanelByte(0);
		}

		//--------- Now its time to process the single tracks -------------------------

		int i;
		for(i=0;i<NUM_TRACKS;i++)
		{
			//increment the step index
			seq_stepIndex[i]++;
			//check if track end is reached
			if(((seq_patternSet.seq_subStepPattern[seq_activePattern][i][seq_stepIndex[i]].note & PATTERN_END)) || ((seq_stepIndex[i] & 0x7f) == 0))
			{
				//if end is reached reset track to step 0
				seq_stepIndex[i] = 0;
			}


			if(seq_SomModeActive)
			{
				som_tick(seq_stepIndex[0],seq_mutedTracks);

			} else {
				//if track is not muted
				if(!(seq_mutedTracks & (1<<i) ) )
				{
					//if main step + sub step is active
					if(seq_isMainStepActive(i,seq_stepIndex[i]/8,seq_activePattern) && (seq_isStepActive(i,seq_stepIndex[i],seq_activePattern)))
					{
						//PROBABILITY
						//every 8th step a new random value is generated
						//thus every sub step block has only one random value to compare against
						//allows randomisation of rolls by chance

						if((seq_stepIndex[i] & 0x07) == 0x00) //every 8th step
						{
							seq_rndValue[i] = GetRngValue()&0x7f;
						}

						if( (seq_rndValue[i]) <= seq_patternSet.seq_subStepPattern[seq_activePattern][i][seq_stepIndex[i]].prob )
						{
							const uint8_t vol = seq_patternSet.seq_subStepPattern[seq_activePattern][i][seq_stepIndex[i]].volume&STEP_VOLUME_MASK;
							const uint8_t note = seq_patternSet.seq_subStepPattern[seq_activePattern][i][seq_stepIndex[i]].note;
							seq_triggerVoice(i,vol,note);
						}
					}

				}
			}

			//---- check if the roll mode has to trigger the voice
			if(seq_rollRate!=0xff) //not in oneshot mode
			{
				if(seq_rollState & (1<<i))
				{
					//check if roll is active
					{
						if((seq_stepIndex[i]%seq_rollRate)==0)
						{
							const uint8_t vol = ROLL_VOLUME;

							const uint8_t note = seq_patternSet.seq_subStepPattern[seq_activePattern][i][seq_stepIndex[i]].note;
							seq_triggerVoice(i,vol,note);

							seq_addNote(i,vol);
						}
					}
				}
			}//end oneshot

		}

		//send message to frontpanel
		//to display the current step
		uart_sendFrontpanelByte(FRONT_STEP_LED_STATUS_BYTE);
		uart_sendFrontpanelByte(FRONT_CURRENT_STEP_NUMBER_CC);
		uart_sendFrontpanelByte(seq_stepIndex[frontParser_activeTrack]);
	}
}
//------------------------------------------------------------------------------
uint8_t seq_getExtSync()
{
	return seq_isSyncExternal;
}
//------------------------------------------------------------------------------
void seq_setExtSync(uint8_t isExt)
{
	seq_isSyncExternal = isExt;
}
//------------------------------------------------------------------------------
void seq_armAutomationStep(uint8_t stepNr, uint8_t track,uint8_t isArmed)
{
	if(isArmed) {
		seq_armedArmedAutomationStep 	= stepNr;
		seq_armedArmedAutomationTrack 	= track;
	} else {
		seq_armedArmedAutomationStep 	= -1;
		seq_armedArmedAutomationTrack 	= -1;
	}
}
//------------------------------------------------------------------------------
void seq_setDeltaT(float delta)
{
	seq_deltaT = delta;
}
//------------------------------------------------------------------------------
void seq_resetDeltaAndTick()
{
	//if there are unplayed steps jump over them
	while(!seq_isNextStepSyncStep())
	{
		seq_nextStep();
	}

	//is shuffle delay necessary?

	if(seq_shuffle != 0)
	{
		//seq_deltaT = 0;^

		seq_deltaT 	= (1000*60)/seq_tempo; 	//bei 12 = 500ms = zeit für einen beat
		seq_deltaT /= 96.f; //we run the internal clock at 96ppq -> seq clock 32 ppq == prescaler 3, midi clock 24 ppq == prescale 4
		seq_deltaT *= 4;


		seq_lastTick = systick_ticks;

		uint8_t stepInHalfBeat = seq_masterStepCnt&0xf;
		const float shuffleFactor = seq_shuffleTable[stepInHalfBeat] * seq_shuffle;
		const float originalDeltaT = seq_deltaT;

		seq_deltaT = shuffleFactor * originalDeltaT * 1.f;
		//seq_deltaT -= seq_lastShuffle * originalDeltaT * 8.f;

		seq_lastShuffle = shuffleFactor;

		//seq_calcDeltaT(seq_tempo);

		if(seq_deltaT <= 0)
		{
			seq_nextStep();
			seq_lastTick = systick_ticks;
			seq_calcDeltaT(seq_tempo);
		} else
		{
			seq_delayedSyncStepFlag =1;
		}

		seq_prescaleCounter = 0;

	}
	else
	{
		//play next sync step
		seq_nextStep();

		seq_lastTick = systick_ticks;
		seq_calcDeltaT(seq_tempo);

		seq_prescaleCounter = 0;
	}


}
//------------------------------------------------------------------------------
/** call periodically to check if the next step has to be processed */
void seq_tick()
{
	if(seq_deltaT == -1)
	{
		seq_deltaT = 32000;
		seq_nextStep();
		return;
	}
	if(systick_ticks-seq_lastTick >= seq_deltaT)
	{

		float rest = systick_ticks-seq_lastTick - seq_deltaT;
		seq_lastTick = systick_ticks;
		seq_calcDeltaT(seq_tempo);
		seq_deltaT = seq_deltaT - rest;

		if((seq_prescaleCounter%SEQ_PRESCALER_MASK) == 0)
		{
			//for external sync we have a ratio of 3/4 ppq/steps
			//so when the 3rd ppq is received we have to activate the 4th step etc
			//advance only 2 steps automatically, then wait for sync message

			if(seq_getExtSync()) {
				if(seq_isNextStepSyncStep()==0) {
					seq_delayedSyncStepFlag = 0;

					seq_nextStep();
				}
			} else {
				seq_nextStep();
			}
		}

		if(!seq_getExtSync()) //only send internal MIDI clock to output when external sync is off
		{
			if((seq_prescaleCounter%MIDI_PRESCALER_MASK) == 0)
			{
				uart_sendMidiByte(MIDI_CLOCK);
			}
		}
		seq_prescaleCounter++;
		if(seq_prescaleCounter>=12)seq_prescaleCounter=0;
	}
}
//------------------------------------------------------------------------------
void seq_setQuantisation(uint8_t value)
{
	seq_quantisation = value;
}
//------------------------------------------------------------------------------
void seq_toggleStep(uint8_t voice, uint8_t stepNr, uint8_t patternNr)
{
	if((seq_patternSet.seq_subStepPattern[patternNr][voice][stepNr].volume&STEP_ACTIVE_MASK)==0)
	{
		seq_patternSet.seq_subStepPattern[patternNr][voice][stepNr].volume |= STEP_ACTIVE_MASK;
	} else {
		seq_patternSet.seq_subStepPattern[patternNr][voice][stepNr].volume &= ~STEP_ACTIVE_MASK;
	}
}
//------------------------------------------------------------------------------
void seq_toggleMainStep(uint8_t voice, uint8_t stepNr, uint8_t patternNr)
{
	seq_patternSet.seq_mainSteps[patternNr][voice] ^= (1<<stepNr);
}
//------------------------------------------------------------------------------
void seq_setMainStep(uint8_t voice, uint8_t stepNr, uint8_t onOff)
{
	if(onOff)
	{
		seq_patternSet.seq_mainSteps[seq_activePattern][voice] |= (1<<stepNr);
	}
	else
	{
		seq_patternSet.seq_mainSteps[seq_activePattern][voice] &= ~(1<<stepNr);
	}
}
//------------------------------------------------------------------------------
void seq_setStep(uint8_t voice, uint8_t stepNr, uint8_t onOff)
{
	if(onOff)
	{
		seq_patternSet.seq_subStepPattern[seq_activePattern][voice][stepNr].volume |= STEP_ACTIVE_MASK;
	}
	else
	{
		seq_patternSet.seq_subStepPattern[seq_activePattern][voice][stepNr].volume &= ~STEP_ACTIVE_MASK;
	}
}
//------------------------------------------------------------------------------
uint8_t seq_isRunning() {
	return seq_running;
}
//------------------------------------------------------------------------------
void seq_setRunning(uint8_t isRunning)
{
	seq_running = isRunning;
	//jump to 1st step if sequencer is stopped
	if(!seq_running)
	{
		uint8_t i;
		for(i=0;i<NUM_TRACKS;i++) {
			seq_stepIndex[i] = -1;
		}

		//reset song position bar counter
		seq_lastShuffle = 0;
		seq_barCounter = 0;
		seq_masterStepCnt = 0;
		//so the next seq_tick call will trigger the next step immediately
		seq_deltaT = 0;
		uart_sendMidiByte(MIDI_STOP);

		//--AS send notes off on all channels that have notes playing and reset our bitmap to reflect that
		for(i=0;i<16;i++)
			if((1<<i) & midi_notes_on)
				seq_sendMidi(0x80 | i, midi_chan_notes[i],0);
		midi_notes_on=0;

		trigger_reset(1);
	} else {
		seq_prescaleCounter = 0;
		uart_sendMidiByte(MIDI_START);
		trigger_reset(0);
	}
}
//------------------------------------------------------------------------------
inline uint8_t seq_isStepActive(uint8_t voice, uint8_t stepNr, uint8_t patternNr)
{
	return ((seq_patternSet.seq_subStepPattern[patternNr][voice][stepNr].volume & STEP_ACTIVE_MASK) > 0);
}
//------------------------------------------------------------------------------
inline uint8_t seq_isMainStepActive(uint8_t voice, uint8_t mainStepNr, uint8_t pattern)
{
	return (seq_patternSet.seq_mainSteps[pattern][voice] & (1<<mainStepNr)) > 0;
}
//------------------------------------------------------------------------------
void seq_setMute(uint8_t trackNr, uint8_t isMuted)
{
	if(trackNr==7)
	{
		//unmute all
		seq_mutedTracks = 0;
	} else {
		//mute/unmute tracks
		if(isMuted) {
			//mute track
			seq_mutedTracks |= (1<<trackNr);
		} else {
			//unmute track
			seq_mutedTracks &= ~(1<<trackNr);
		}
	}
};
//------------------------------------------------------------------------------
uint8_t seq_isTrackMuted(uint8_t trackNr)
{
	if(seq_mutedTracks & (1<<trackNr) )
	{
		return 1;
	}
	return 0;
}
//------------------------------------------------------------------------------
void seq_sendMainStepInfoToFront(uint16_t stepNr)
{
	//the absolute number of patterns
	const uint8_t currentPattern	= stepNr / 7;
	const uint8_t currentTrack  	= stepNr - currentPattern*7;

	uint16_t dataToSend = seq_patternSet.seq_mainSteps[currentPattern][currentTrack];

	uart_sendFrontpanelSysExByte(  dataToSend	  & 0x7f); //1st 7 bit
	uart_sendFrontpanelSysExByte( (dataToSend>>7) & 0x7f); //2nd 7 bit
	uart_sendFrontpanelSysExByte( (dataToSend>>14)& 0x7f); //last 2 bit
}
//--------------------------------------------------------------------
/** this one is more complicated than the 14 bit upper/lower nibble when transmitting the requested step number via SysEx.
 * because we have to transmit seven 8-bit values that make up the StepStruct we have to pack the data clever into the 7-bit
 * SysEx packets.
 *
 * first we send the lower 7-bit of all 7 values
 * then we transmit an additional 7-bit value containing all the MSBs from the previous seven values.
 *
 * 1st - (value1 & 0x7f)
 * 2nd - (value2 & 0x7f)
 * 3rd - (value3 & 0x7f)
 * 4th - (value4 & 0x7f)
 * 5th - (value5 & 0x7f)
 * 6th - (value6 & 0x7f)
 * 7th - (value7 & 0x7f)
 *
 * 8th - (0 MSB7 MSB6 MSB5 MSB4 MSB3 MSB2 MSB1)
 */
void seq_sendStepInfoToFront(uint16_t stepNr)
{

	//decode the step number
	//the step number is between 0 and NUMBER_STEPS*NUM_TRACKS*NUM_PATTERN = 128*7*8 = 7168 steps
	// stepNr / 128 = absolute number of patterns (each track has 8 patterns * 7 tracks)
	// numPatterns / 8 = the current track

	//the absolute number of patterns
	const uint8_t absPat 			= stepNr/128;
	const uint8_t currentTrack 		= absPat / 8;
	const uint8_t currentPattern 	= absPat - currentTrack*8;
	const uint8_t currentStep		= stepNr - absPat*128;

	//encode the data and send it back
	Step *dataToSend = &seq_patternSet.seq_subStepPattern[currentPattern][currentTrack][currentStep];

	uart_sendFrontpanelSysExByte(dataToSend->volume	& 0x7f);
	uart_sendFrontpanelSysExByte(dataToSend->prob	& 0x7f);
	uart_sendFrontpanelSysExByte(dataToSend->note	& 0x7f);

	uart_sendFrontpanelSysExByte(dataToSend->param1Nr	& 0x7f);
	uart_sendFrontpanelSysExByte(dataToSend->param1Val	& 0x7f);

	uart_sendFrontpanelSysExByte(dataToSend->param2Nr	& 0x7f);
	uart_sendFrontpanelSysExByte(dataToSend->param2Val	& 0x7f);

	//now the MSBs from all 7 values
	uart_sendFrontpanelSysExByte( 	(((dataToSend->volume 	& 0x80)>>7) |
									((dataToSend->prob	 	& 0x80)>>6) |
									((dataToSend->note	 	& 0x80)>>5) |
									((dataToSend->param1Nr	& 0x80)>>4) |
									((dataToSend->param1Val	& 0x80)>>3) |
									((dataToSend->param2Nr	& 0x80)>>2) |
									((dataToSend->param2Val	& 0x80)>>1))&0x7f
									);
}
//-------------------------------------------------------------------------------
void seq_setRoll(uint8_t voice, uint8_t onOff)
{
	if(voice >= 7) return;

	if(onOff) {
		seq_rollState |= (1<<voice);
		if(seq_rollRate == 0xff) {
			//trigger one shot
			seq_triggerVoice(voice,ROLL_VOLUME,SEQ_DEFAULT_NOTE);
			//record roll notes
			seq_addNote(voice,ROLL_VOLUME);
		}
	} else {
		seq_rollState &= ~(1<<voice);
	}
};
//--------------------------------------------------------------------------------
void seq_setRollRate(uint8_t rate)
{
	/*
	0 - one shot immediate trigger
	1 - 1/1
	2 - 1/2
	3 - 1/3
	4 - 1/4
	5 - 1/6
	6 - 1/8
	7 - 1/12
	8 - 1/16
	9 - 1/24
	10 - 1/32
	11 - 1/48
	12 - 1/64
	13 - 1/128
				*/

	switch(rate)
	{
	case 0:
		seq_rollRate = 0xfe;
		break;
	case 1: // 1/1
		seq_rollRate = 0x7f;
		break;

	case 2: // 1/2
		seq_rollRate = 0x3f;
		break;

	case 3:// 1/3
		seq_rollRate = 0x2a;
			break;

	case 4:// 1/4
		seq_rollRate = 0x1f;
			break;

	case 5:// 1/6
		seq_rollRate = 0x31;
			break;

	case 6:// 1/8
			seq_rollRate = 0x0f;
			break;

	case 7:// 1/12
		seq_rollRate = 0x0a;
			break;

	case 8:// 1/16
		seq_rollRate = 0x07;
			break;

	case 9: // 1/24
		seq_rollRate = 0x05;
		break;

	case 10:// 1/32
		seq_rollRate = 0x03;
		break;

	case 11:// 1/48
		seq_rollRate = 0x02;
		break;

	case 12://1/64
		seq_rollRate = 0x01;
		break;

	case 13://1/128
		seq_rollRate = 0x00;
		break;
	}
	seq_rollRate +=1; //is there a reason for this offset here? seems the value could be assigned directly!?!

}
//------------------------------------------------------------------------
/** quantize a step to the seq_quantisation value*/
#define QUANT(x) (NUM_STEPS/x)
int8_t seq_quantize(int8_t step)
{
	uint8_t quantisationMultiplier=1;
	switch(seq_quantisation)
	{
	case QUANT_8:
		quantisationMultiplier = QUANT(8);
		break;

	case QUANT_16:
		quantisationMultiplier = QUANT(16);
		break;

	case QUANT_32:
		quantisationMultiplier = QUANT(32);
		break;

	case QUANT_64:
		quantisationMultiplier = QUANT(64);
		break;

	case NO_QUANTISATION:
	default:
		return step;
		break;
	}

	//now calc the quantisation
	float frac = step/(float)quantisationMultiplier;
	int8_t itg = (int8_t)frac;
	frac = frac - itg;

	if(frac>=0.5f)
	{
		return ((itg + 1)*quantisationMultiplier)&0x7f;
	}
	return itg*quantisationMultiplier;
}
//------------------------------------------------------------------------
void seq_recordAutomation(uint8_t voice, uint8_t dest, uint8_t value)
{
	if(seq_recordActive)
	{
		uint8_t quantizedStep = seq_quantize(seq_stepIndex[voice]);

		//only record to active steps
		if( seq_isMainStepActive(voice,quantizedStep/8,seq_activePattern) &&
				seq_isStepActive(voice,quantizedStep,seq_activePattern))
		{
			if(seq_activeAutomTrack == 0) {
				seq_patternSet.seq_subStepPattern[seq_activePattern][voice][quantizedStep].param1Nr = dest;
				seq_patternSet.seq_subStepPattern[seq_activePattern][voice][quantizedStep].param1Val = value;
			} else {
				seq_patternSet.seq_subStepPattern[seq_activePattern][voice][quantizedStep].param2Nr = dest;
				seq_patternSet.seq_subStepPattern[seq_activePattern][voice][quantizedStep].param2Val = value;
			}
		}
	}

	if( (seq_armedArmedAutomationStep	!= -1) && (seq_armedArmedAutomationTrack != -1) )
	{
		//step button is held down
		//-> set step automation parameters
		if(seq_activeAutomTrack == 0) {
			seq_patternSet.seq_subStepPattern[seq_activePattern]
			                                  [seq_armedArmedAutomationTrack]
			                                   [seq_armedArmedAutomationStep].param1Nr = dest;
			seq_patternSet.seq_subStepPattern[seq_activePattern]
			                                  [seq_armedArmedAutomationTrack]
			                                   [seq_armedArmedAutomationStep].param1Val = value;
		} else {
			seq_patternSet.seq_subStepPattern[seq_activePattern]
			                                  [seq_armedArmedAutomationTrack]
			                                   [seq_armedArmedAutomationStep].param2Nr = dest;
			seq_patternSet.seq_subStepPattern[seq_activePattern]
			                                  [seq_armedArmedAutomationTrack]
			                                   [seq_armedArmedAutomationStep].param2Val = value;
		}
	}
}
//------------------------------------------------------------------------
void seq_addNote(uint8_t trackNr,uint8_t vel)
{
	//only record notes when seq is running and recording
	if(seq_running && seq_recordActive)
	{
		const int8_t quantizedStep = seq_quantize(seq_stepIndex[trackNr]);

		//special care must be taken when recording midi notes!
		//since per default the 1st substep of a mainstep cluster is always active
		//we will get double notes when a substep other than ss1 is recorded
		if(!seq_isMainStepActive(trackNr, quantizedStep/8, seq_activePattern))
		{
			//if the mainstep is not active, we clear the 1st substep
			//to prevent double notes while recording
			seq_patternSet.seq_subStepPattern[seq_activePattern][trackNr][(quantizedStep/8)*8].volume 	&= ~STEP_ACTIVE_MASK;
		}

		//set the current step in the requested track active
		seq_patternSet.seq_subStepPattern[seq_activePattern][trackNr][quantizedStep].note 		= SEQ_DEFAULT_NOTE;	// default note
		seq_patternSet.seq_subStepPattern[seq_activePattern][trackNr][quantizedStep].volume		= vel;				// new velocity
		seq_patternSet.seq_subStepPattern[seq_activePattern][trackNr][quantizedStep].prob		= 127;				// 100% probability

		seq_patternSet.seq_subStepPattern[seq_activePattern][trackNr][quantizedStep].volume 	|= STEP_ACTIVE_MASK;

		//activate corresponding main step
		seq_setMainStep(trackNr, quantizedStep/8,1);

		if( (frontParser_shownPattern == seq_activePattern) && ( frontParser_activeTrack == trackNr) )
		{
			//update front sub step LED
			uart_sendFrontpanelByte(FRONT_STEP_LED_STATUS_BYTE);
			uart_sendFrontpanelByte(FRONT_LED_SEQ_SUB_STEP);
			uart_sendFrontpanelByte(quantizedStep);
			//update front main step LED
			uart_sendFrontpanelByte(FRONT_STEP_LED_STATUS_BYTE);
			uart_sendFrontpanelByte(FRONT_LED_SEQ_BUTTON);
			uart_sendFrontpanelByte(quantizedStep);
		}
	}
}
//------------------------------------------------------------------------
void seq_setRecordingMode(uint8_t active)
{
	seq_recordActive = active;
}
//------------------------------------------------------------------------------
void seq_clearTrack(uint8_t trackNr, uint8_t pattern)
{
	int k;
	for(k=0;k<128;k++)
	{
		seq_patternSet.seq_subStepPattern[pattern][trackNr][k].note 		= SEQ_DEFAULT_NOTE;
		seq_patternSet.seq_subStepPattern[pattern][trackNr][k].param1Nr 	= NO_AUTOMATION;
		seq_patternSet.seq_subStepPattern[pattern][trackNr][k].param1Val 	= 0;
		seq_patternSet.seq_subStepPattern[pattern][trackNr][k].param2Nr	= NO_AUTOMATION;
		seq_patternSet.seq_subStepPattern[pattern][trackNr][k].param2Val	= 0;
		seq_patternSet.seq_subStepPattern[pattern][trackNr][k].prob		= 127;
		seq_patternSet.seq_subStepPattern[pattern][trackNr][k].volume		= 100;//STEP_VOLUME_MASK;

		//every 1st step in a substep pattern active
		if( (k%8) == 0)
		{
			seq_patternSet.seq_subStepPattern[pattern][trackNr][k].volume		|= STEP_ACTIVE_MASK ;
		}

		seq_patternSet.seq_mainSteps[pattern][trackNr] = 0;
	}
}
//------------------------------------------------------------------------------
void seq_clearPattern(uint8_t pattern)
{
	int k,i;
	for(k=0;k<128;k++)
	{
		for(i=0;i<NUM_TRACKS;i++)
		{
			seq_patternSet.seq_subStepPattern[pattern][i][k].note 		= SEQ_DEFAULT_NOTE;
			seq_patternSet.seq_subStepPattern[pattern][i][k].param1Nr 	= NO_AUTOMATION;
			seq_patternSet.seq_subStepPattern[pattern][i][k].param1Val 	= 0;
			seq_patternSet.seq_subStepPattern[pattern][i][k].param2Nr	= NO_AUTOMATION;
			seq_patternSet.seq_subStepPattern[pattern][i][k].param2Val	= 0;
			seq_patternSet.seq_subStepPattern[pattern][i][k].prob		= 127;
			seq_patternSet.seq_subStepPattern[pattern][i][k].volume		= 100;//STEP_VOLUME_MASK;

			//every 1st step in a substep pattern active
			if( (k%8) == 0)
			{
				seq_patternSet.seq_subStepPattern[pattern][i][k].volume		|= STEP_ACTIVE_MASK ;
			}

			seq_patternSet.seq_mainSteps[pattern][i] = 0;
		}
	}
}
//------------------------------------------------------------------------------
void seq_clearAutomation(uint8_t trackNr, uint8_t pattern, uint8_t automTrack)
{
	int k;

	if(automTrack==0)
	{
		for(k=0;k<128;k++)
		{
			seq_patternSet.seq_subStepPattern[pattern][trackNr][k].param1Nr 	= NO_AUTOMATION;
			seq_patternSet.seq_subStepPattern[pattern][trackNr][k].param1Val 	= 0;
		}
	} else {
		for(k=0;k<128;k++)
		{
			seq_patternSet.seq_subStepPattern[pattern][trackNr][k].param2Nr		= NO_AUTOMATION;
			seq_patternSet.seq_subStepPattern[pattern][trackNr][k].param2Val	= 0;
		}
	}
}
//------------------------------------------------------------------------------
void seq_copyTrack(uint8_t srcNr, uint8_t dstNr, uint8_t pattern)
{
	int k;
	for(k=0;k<128;k++)
	{
		seq_patternSet.seq_subStepPattern[pattern][dstNr][k].note 		= seq_patternSet.seq_subStepPattern[pattern][srcNr][k].note;
		seq_patternSet.seq_subStepPattern[pattern][dstNr][k].param1Nr 	= seq_patternSet.seq_subStepPattern[pattern][srcNr][k].param1Nr;
		seq_patternSet.seq_subStepPattern[pattern][dstNr][k].param1Val 	= seq_patternSet.seq_subStepPattern[pattern][srcNr][k].param1Val;
		seq_patternSet.seq_subStepPattern[pattern][dstNr][k].param2Nr		= seq_patternSet.seq_subStepPattern[pattern][srcNr][k].param2Nr;
		seq_patternSet.seq_subStepPattern[pattern][dstNr][k].param2Val	= seq_patternSet.seq_subStepPattern[pattern][srcNr][k].param2Val;
		seq_patternSet.seq_subStepPattern[pattern][dstNr][k].prob			= seq_patternSet.seq_subStepPattern[pattern][srcNr][k].prob;
		seq_patternSet.seq_subStepPattern[pattern][dstNr][k].volume		= seq_patternSet.seq_subStepPattern[pattern][srcNr][k].volume;
	}

	seq_patternSet.seq_mainSteps[pattern][dstNr] = seq_patternSet.seq_mainSteps[pattern][srcNr];
}
//------------------------------------------------------------------------------
void seq_copyPattern(uint8_t src, uint8_t dst)
{
	int k,j;
	for(j=0;j<NUM_TRACKS;j++)
	{
		for(k=0;k<128;k++)
		{
			seq_patternSet.seq_subStepPattern[dst][j][k].note 		= seq_patternSet.seq_subStepPattern[src][j][k].note;
			seq_patternSet.seq_subStepPattern[dst][j][k].param1Nr 	= seq_patternSet.seq_subStepPattern[src][j][k].param1Nr;
			seq_patternSet.seq_subStepPattern[dst][j][k].param1Val 	= seq_patternSet.seq_subStepPattern[src][j][k].param1Val;
			seq_patternSet.seq_subStepPattern[dst][j][k].param2Nr		= seq_patternSet.seq_subStepPattern[src][j][k].param2Nr;
			seq_patternSet.seq_subStepPattern[dst][j][k].param2Val	= seq_patternSet.seq_subStepPattern[src][j][k].param2Val;
			seq_patternSet.seq_subStepPattern[dst][j][k].prob			= seq_patternSet.seq_subStepPattern[src][j][k].prob;
			seq_patternSet.seq_subStepPattern[dst][j][k].volume		= seq_patternSet.seq_subStepPattern[src][j][k].volume;
		}

		seq_patternSet.seq_mainSteps[dst][j] = seq_patternSet.seq_mainSteps[src][j];
	}
}
//------------------------------------------------------------------------------
void seq_setActiveAutomationTrack(uint8_t trackNr)
{
	seq_activeAutomTrack = trackNr;
}
//------------------------------------------------------------------------------
uint8_t seq_isNextStepSyncStep()
{
	if(seq_delayedSyncStepFlag)
	{
		seq_delayedSyncStepFlag = 0;
		seq_prescaleCounter = 0;
		return 0;
	}
	if( ((seq_stepIndex[0] & 0x3) % 4) == 3) {
		return 1;
	}
	return 0;
}
//------------------------------------------------------------------------------

