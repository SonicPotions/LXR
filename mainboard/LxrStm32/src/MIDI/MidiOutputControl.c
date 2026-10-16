/*
 * MidiOutputControl.c
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


#include "MidiOutputControl.h"
#include "DrumVoice.h"
#include "Snare.h"
#include "HiHat.h"
#include "MidiMessages.h"
#include "CymbalVoice.h"
#include "MidiParser.h"
#include "sequencer.h"
#include "uARTFrontSYX/frontPanelReceivingProtocol.h"
#include "uARTFrontSYX/Uart.h"
#include "ChannelMidiParser.h"
#include "TriggerOut.h"
#include "Preset/MorphEngine.h"
#include "usb_manager.h"
#include "GlobalMidiParser.h"      /* globalMidiParser_getTxAddress(), GLOBAL_MIDI_NRPN_* (Session 039 P3) */
#include "Preset/ParameterArray.h" /* END_OF_SOUND_PARAMETERS (menu TX slots) */
#include <string.h>                /* memset (menu TX queue) */
//#include "LCD_driver.h"

/* Live voice activity state consumed by the parser and LED feedback paths. */
uint8_t voiceStatus[NUM_VOICES];

/* Tracks which voices are currently active for note-off and LED logic. */
static uint8_t active_voices=0;
//----------------------------------------------------------------
void outputControl_sendMidi(MidiMsg msg)
{
   /* Sequencer-originated MIDI output is mirrored to USB and DIN MIDI. */
   usb_sendMidi(msg);
   uart_sendMidi(msg);
}

//----------------------------------------------------------------
void outputControl_sendRealtime(uint8_t status)
{
   MidiMsg msg;

   msg.status = status;
   msg.data1 = 0;
   msg.data2 = 0;
   msg.bits.length = 0;
   msg.bits.sysxbyte = 0;

   outputControl_sendMidi(msg);
}

//----------------------------------------------------------------
/* MENU TX QUEUE STATE  (Session 039 P3)
   WHAT: one slot per raw sound parameter plus 4 PERF slots
         (OUTPUT_MENU_TX_*). value[] holds the latest 7-bit value; dirty[] is
         a bitmap of slots waiting to be sent; pending counts the dirty bits;
         cursor is the round-robin scan position.
   WHY:  coalescing (user decision 8): a knob sweep keeps only its latest
         value per parameter, and the service emits at a bounded rate.
   WRITERS: outputControl_menuTxMark(), outputControl_serviceMenuTx().
   READERS: outputControl_serviceMenuTx(). About 280 bytes of STM RAM. */
#define MENU_TX_PERF_SLOTS   4
#define MENU_TX_NUM_SLOTS    (END_OF_SOUND_PARAMETERS + MENU_TX_PERF_SLOTS)
#define MENU_TX_FILTER_BIT   0x40   /* TX nibble C (midi_setFilter() layout) */

static uint8_t  outputControl_menuTxValue[MENU_TX_NUM_SLOTS];
static uint8_t  outputControl_menuTxDirty[(MENU_TX_NUM_SLOTS + 7) / 8];
static uint16_t outputControl_menuTxPending = 0;
static uint16_t outputControl_menuTxCursor = 0;

/* MENU TX GATE  (Session 039 P3)
   WHAT: returns the Global MIDI channel (1..16) when menu TX is allowed, else 0.
   WHY:  user decisions: TX only with txf C on, and only on the Global channel,
         which must not be 0 ("off").
   INPUT:  midiParser_txRxFilter, midi_MidiChannels[7].
   OUTPUT: channel 1..16, or 0 = closed.
   ACCESSORS: queue functions and outputControl_serviceMenuTx(). */
static uint8_t outputControl_menuTxChannel(void)
{
   const uint8_t channel = midi_MidiChannels[7];

   if(!(midiParser_txRxFilter & MENU_TX_FILTER_BIT))
      return 0;
   if(channel == 0 || channel > 16)
      return 0;
   return channel;
}

/* MENU TX MARK  (Session 039 P3)
   WHAT: stores value in slot and sets its dirty bit, counting newly dirty
         slots. INPUT: slot < MENU_TX_NUM_SLOTS, value 0..127.
   ACCESSORS: outputControl_queueMenuParameterTx(), outputControl_queueMenuPerfTx(). */
static void outputControl_menuTxMark(uint16_t slot, uint8_t value)
{
   const uint8_t mask = (uint8_t)(1u << (slot & 7u));

   outputControl_menuTxValue[slot] = (uint8_t)(value & 0x7f);
   if(!(outputControl_menuTxDirty[slot >> 3] & mask))
   {
      outputControl_menuTxDirty[slot >> 3] |= mask;
      outputControl_menuTxPending++;
   }
}

/* MENU TX ADDRESS RESOLVE  (Session 039 P3)
   WHAT: maps a slot to its Global address at emit time (so a ¢ toggle
         between queue and emit is honoured). Sound slots use
         globalMidiParser_getTxAddress(); PERF slots are fixed: roll ->
         NRPN 93/94/95, global morph -> CC1 (MOD_WHEEL).
   INPUT:  slot; number - out. OUTPUT: GLOBAL_MIDI_TX_NONE/_CC/_NRPN.
   ACCESSORS: outputControl_serviceMenuTx(). */
static uint8_t outputControl_menuTxResolve(uint16_t slot, uint8_t *number)
{
   if(slot < END_OF_SOUND_PARAMETERS)
      return globalMidiParser_getTxAddress(slot, number);

   switch(slot - END_OF_SOUND_PARAMETERS)
   {
      case OUTPUT_MENU_TX_ROLL_RATE:     *number = GLOBAL_MIDI_NRPN_ROLL_RATE;     return GLOBAL_MIDI_TX_NRPN;
      case OUTPUT_MENU_TX_ROLL_NOTE:     *number = GLOBAL_MIDI_NRPN_ROLL_NOTE;     return GLOBAL_MIDI_TX_NRPN;
      case OUTPUT_MENU_TX_ROLL_VELOCITY: *number = GLOBAL_MIDI_NRPN_ROLL_VELOCITY; return GLOBAL_MIDI_TX_NRPN;
      case OUTPUT_MENU_TX_GLOBAL_MORPH:  *number = MOD_WHEEL;                      return GLOBAL_MIDI_TX_CC;
      default:                           return GLOBAL_MIDI_TX_NONE;
   }
}

/* MENU TX CC EMITTER  (Session 039 P3)
   WHAT: sends one control change on channel (1..16) to DIN and USB.
   ACCESSORS: outputControl_serviceMenuTx(). AFFILIATES: outputControl_sendMidi(). */
static void outputControl_emitCc(uint8_t channel, uint8_t cc, uint8_t value)
{
   MidiMsg msg = {0};

   msg.status = (uint8_t)(MIDI_CC | ((channel - 1u) & 0x0F));
   msg.data1 = (uint8_t)(cc & 0x7f);
   msg.data2 = (uint8_t)(value & 0x7f);
   msg.bits.length = 2;
   outputControl_sendMidi(msg);
}

//----------------------------------------------------------------
/* QUEUE A MENU SOUND-PARAMETER CHANGE (Session 039 P3); see MidiOutputControl.h. */
void outputControl_queueMenuParameterTx(uint16_t rawParam, uint8_t value)
{
   uint8_t number;

   if(rawParam >= END_OF_SOUND_PARAMETERS)
      return;
   if(!outputControl_menuTxChannel() || seq_isAutomationStepArmed())
      return;
   if(globalMidiParser_getTxAddress(rawParam, &number) == GLOBAL_MIDI_TX_NONE)
      return;

   outputControl_menuTxMark(rawParam, value);
}

//----------------------------------------------------------------
/* QUEUE A MENU PERF-CONTROL CHANGE (Session 039 P3); see MidiOutputControl.h. */
void outputControl_queueMenuPerfTx(uint8_t perfSlot, uint8_t value)
{
   if(perfSlot >= MENU_TX_PERF_SLOTS)
      return;
   if(!outputControl_menuTxChannel() || seq_isAutomationStepArmed())
      return;

   if(perfSlot == OUTPUT_MENU_TX_GLOBAL_MORPH)
      value = (uint8_t)(value >> 1);   /* 0..255 -> 0..127; receive maps 127 -> 255 */

   outputControl_menuTxMark((uint16_t)(END_OF_SOUND_PARAMETERS + perfSlot), value);
}

//----------------------------------------------------------------
/* MENU TX SERVICE (Session 039 P3); see MidiOutputControl.h.
   RPN-NULL GUARD: CC100/CC101 = 127 directly after an NRPN data entry is an
   NRPN deselect for an LXR receiver (Session 039 P1). Before emitting
   CC100/101 = 127 a harmless CC99 = 0 is sent, which closes the receiver's
   null window, so recorded Snare/Cymbal hard-pan plays back. */
void outputControl_serviceMenuTx(void)
{
   uint8_t channel;
   uint16_t scanned;

   if(outputControl_menuTxPending == 0)
      return;

   channel = outputControl_menuTxChannel();
   if(channel == 0)
   {
      /* Gate closed after queuing (txf C off or channel 0): drop all. */
      memset(outputControl_menuTxDirty, 0, sizeof(outputControl_menuTxDirty));
      outputControl_menuTxPending = 0;
      return;
   }

   for(scanned = 0; scanned < MENU_TX_NUM_SLOTS; scanned++)
   {
      const uint16_t slot = outputControl_menuTxCursor;
      const uint8_t mask = (uint8_t)(1u << (slot & 7u));

      if(outputControl_menuTxDirty[slot >> 3] & mask)
      {
         uint8_t number = 0;
         const uint8_t value = outputControl_menuTxValue[slot];
         const uint8_t kind = outputControl_menuTxResolve(slot, &number);

         if(kind != GLOBAL_MIDI_TX_NONE)
         {
            const uint8_t guard = (uint8_t)((kind == GLOBAL_MIDI_TX_CC)
                                  && (number == RPN_LSB || number == RPN_MSB)
                                  && (value == 127));
            const uint8_t dinNeed = (uint8_t)(((kind == GLOBAL_MIDI_TX_CC) ? 3u : 9u) + (guard ? 3u : 0u));
            const uint8_t usbNeed = (uint8_t)(((kind == GLOBAL_MIDI_TX_CC) ? 4u : 12u) + (guard ? 4u : 0u));

            if(uart_midiTxFree() < dinNeed || usb_midiTxFree() < usbNeed)
               return;   /* retry this slot on a later pass */

            if(kind == GLOBAL_MIDI_TX_NRPN)
            {
               outputControl_emitCc(channel, NRPN_COARSE, 0);
               outputControl_emitCc(channel, NRPN_FINE, number);
               outputControl_emitCc(channel, NRPN_DATA_ENTRY_COARSE, value);
            }
            else
            {
               if(guard)
                  outputControl_emitCc(channel, NRPN_COARSE, 0);
               outputControl_emitCc(channel, number, value);
            }
         }

         outputControl_menuTxDirty[slot >> 3] &= (uint8_t)~mask;
         outputControl_menuTxPending--;
         if(++outputControl_menuTxCursor >= MENU_TX_NUM_SLOTS)
            outputControl_menuTxCursor = 0;
         return;   /* at most one message (or one drop) per pass */
      }

      if(++outputControl_menuTxCursor >= MENU_TX_NUM_SLOTS)
         outputControl_menuTxCursor = 0;
   }
}

//----------------------------------------------------------------
// this fn assumes a valid voice is sent
void voiceControl_noteOn(uint8_t voice, uint8_t note, uint8_t vel)
{
   /* Trigger the voice engine and mirror the LED pulse to the front panel. */
   active_voices |= (1<<voice);
   preset_applyVelocityVoiceMorphOnTrigger(voice, vel);

   if(voice < 3)
      Drum_trigger(voice, vel, note);
   else if(voice < 4)
      Snare_trigger(vel, note);
   else if(voice < 5)
      Cymbal_trigger(vel, note);
   else
      HiHat_trigger(vel,voice-5,note);
	
	//Send trigger out signal	
   if(trigger_isGateModeOn())
   {
      if(vel)
         trigger_triggerVoice(voice, TRIGGER_ON);
   } 
   else {
      trigger_triggerVoice(voice, TRIGGER_PULSE);
   }

	// Send to front panel so it can pulse the LED
   channelMidiParser_sendVoiceActivity(voice);
}
//----------------------------------------------------------------
void voiceControl_noteOff(uint8_t voice)
{
   /* Release one voice, or all voices when passed 0xff. */
   uint8_t midiChan; // which midi channel to send a note on

   if(voice==0xff)
   {
      active_voices = 0;
      seq_midiNoteOff(0xff);
      return;
   }

	//only set voice inactive and send MIDI off when voice is currently playing
   if(active_voices & (1<<voice))
   {
      active_voices &= (~(1<<voice));
   
   	//send midi note off
      uint8_t midiChannel = midi_MidiChannels[voice];
      if(midiChannel)
      {
         midiChan = midiChannel-1;
         seq_midiNoteOff(midiChan);
      }
   }
}
//----------------------------------------------------------------
uint8_t voiceControl_isVoicePlaying(uint8_t voice)
{
   /* Query the live activity bitmask for a voice. */
   return (active_voices & (1<<voice));
}
