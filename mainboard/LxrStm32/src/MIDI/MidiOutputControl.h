/*
 * MidiOutputControl.h
 *
 *  Created on: 03.04.2012
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


#ifndef MIDIOUTPUTCONTROL_H_
#define MIDIOUTPUTCONTROL_H_

#include "stm32f4xx.h"
#include "config.h"
#include "MidiMessages.h"


//-------------- DEFINES ----------------------------
//Note numbers to trigger drum channels
// --AS these are not used anymore. the midi_note setting on each voice controls
// what note that voice responds to now
//#define NOTE_VOICE1		36 // C2
//#define NOTE_VOICE2		37
//#define NOTE_VOICE3		38
//#define NOTE_VOICE4		39
//#define NOTE_VOICE5		40
//#define NOTE_VOICE6		41
//#define NOTE_VOICE7		42 // F#2
//-------------- ENUMS ------------------------------

//--------------- VARS ------------------------------
/* Live voice activity state owned by the voice-control module and consumed by
   the parser / LED feedback paths. */
extern uint8_t voiceStatus[NUM_VOICES];
//---------------------------------------------------
//------------- Functions ---------------------------
/* Send a MIDI message to the configured sequencer-originated output targets. */
void outputControl_sendMidi(MidiMsg msg);
/* Send a MIDI realtime status byte through the same output boundary. */
void outputControl_sendRealtime(uint8_t status);

/* MENU TX PERF SLOTS  (Session 039 P3)
   Identify PERF-page controls that have a Global MIDI address but are not raw
   sound parameters: roll rate/note/velocity (NRPN 93/94/95), global morph
   (CC1). Passed to outputControl_queueMenuPerfTx(). */
#define OUTPUT_MENU_TX_ROLL_RATE       0
#define OUTPUT_MENU_TX_ROLL_NOTE       1
#define OUTPUT_MENU_TX_ROLL_VELOCITY   2
#define OUTPUT_MENU_TX_GLOBAL_MORPH    3

/* QUEUE A MENU SOUND-PARAMETER CHANGE FOR MIDI TX  (Session 039 P3)
   WHAT: records the latest value of a raw sound parameter changed in the
         menu, to be sent on the Global channel as its CC/NRPN by
         outputControl_serviceMenuTx(). Repeated edits coalesce: the latest
         value wins.
   WHY:  "txf C" = menu parameter changes are transmitted (user spec), so knob
         motions can be recorded in a DAW and played back into the LXR.
   GATE: TX filter C (0x40) on, Global channel 1..16, no automation step
         armed; unmapped or excluded parameters are ignored.
   INPUT:  rawParam - AVR/Preset raw id; value - 7-bit value as applied.
   OUTPUT: slot marked dirty (or nothing when the gate is closed).
   ACCESSORS: frontParser_handleMidiMessage() FRONT_CC_MENU / FRONT_CC_2_MENU
              cases (frontPanelReceivingProtocol.c) only.
   AFFILIATES: globalMidiParser_getTxAddress(), seq_isAutomationStepArmed(),
               midiParser_txRxFilter, midi_MidiChannels[7]. */
void outputControl_queueMenuParameterTx(uint16_t rawParam, uint8_t value);

/* QUEUE A MENU PERF-CONTROL CHANGE FOR MIDI TX  (Session 039 P3)
   WHAT: as outputControl_queueMenuParameterTx() for the PERF slots above.
         Roll rate is the 0..15 selector; roll note/velocity 0..127; global
         morph 0..255, converted to CC1 as amount >> 1 (the inverse of
         receive preset_morphAutomationValueToAmount(): v*2, 127 -> 255).
   INPUT:  perfSlot - OUTPUT_MENU_TX_*; value - control value as applied.
   OUTPUT: slot marked dirty (or nothing when the gate is closed).
   ACCESSORS: frontParser_handleMidiMessage() FRONT_SEQ_ROLL_RATE/_NOTE/
              _VELOCITY and FRONT_SEQ_SET_GLOBAL_MORPH_MSB cases (menu-only
              senders on the AVR).
   AFFILIATES: GLOBAL_MIDI_NRPN_ROLL_* (GlobalMidiParser.h), MOD_WHEEL. */
void outputControl_queueMenuPerfTx(uint8_t perfSlot, uint8_t value);

/* MENU TX SERVICE  (Session 039 P3)
   WHAT: emits at most one queued menu change per call, round-robin, as a
         Global-channel CC (3 bytes) or NRPN (CC99 0, CC98 n, CC6 v; 9 bytes),
         only when both DIN and USB outputs can take the whole message.
         Drops everything queued if the gate has closed.
   WHY:  bounded, paced output: knob sweeps cannot saturate DIN (31,250 baud)
         or split messages, and sequencer notes/clock keep their direct path.
   OUTPUT: MIDI bytes via outputControl_sendMidi(); queue state updated.
   ACCESSORS: main.c main loop, immediately before usb_tick().
   AFFILIATES: uart_midiTxFree(), usb_midiTxFree(), outputControl_sendMidi(). */
void outputControl_serviceMenuTx(void);

/* Trigger a voice and update the live voice bookkeeping. */
void voiceControl_noteOn(uint8_t voice, uint8_t note, uint8_t vel);
/* Release a voice, or all voices when passed 0xff. */
void voiceControl_noteOff(uint8_t voice);//0xff == all voices
/* Query whether a voice is currently marked active. */
uint8_t voiceControl_isVoicePlaying(uint8_t voice);

#endif /* MIDIOUTPUTCONTROL_H_ */
