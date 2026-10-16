/*
 * MidiParser.c
 *
 *  Created on: 02.04.2012
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


#include "MidiParser.h"
#include "MidiNoteNumbers.h"
#include "DrumVoice.h"
#include "Snare.h"
#include "config.h"
#include "HiHat.h"
#include "CymbalVoice.h"
#include "uARTFrontSYX/Uart.h"
#include "uARTFrontSYX/frontPanelReceivingProtocol.h"
#include "ChannelMidiParser.h"
#include "GlobalMidiParser.h"
#include "clockSync.h"
#include "sequencer.h"
#include "Preset/ParameterIngress.h"
#include "mixer.h"
#include "valueShaper.h"
#include "modulationNode.h"
#include "usb_manager.h"
// front-panel opcodes are owned by uARTFrontSYX/frontPanelReceivingProtocol.h
 #define MORPH_CC        0xac
 #define VOICE_CC			0xb4
   #define BANK_GLOBAL 0x7F
// banks 1-6 plus global stack to allow for multiple voices stacked on the same
// MIDI channel to respond to the same bank change command

// above BANK_GLOBAL it doesn't matter - we reset the command anyway
#define MORPH_OP 0x81
// there is space in here to add more long operations - pattern change
// must have the highest priority
#define PATTERN_CHANGE_OP 0xAF
#define NULL_OP 0x00

/* Parser-visible caches for live MIDI, LFO, and velocity state. */
MidiMsg midi_midiCache[256];
MidiMsg midi_midiKit[256];
uint8_t midi_midiCacheAvailable[256];
uint8_t midi_midiLfoCache[6];
uint8_t midi_kitLfoCache[6];
uint8_t midi_midiLfoCacheAvailable[6];
uint8_t midi_midiVeloCache[6];
uint8_t midi_kitVeloCache[6];
uint8_t midi_midiVeloCacheAvailable[6];
uint8_t midi_unused;

static inline uint8_t midiParser_voiceMidiChannel(uint8_t voice)
{
   return (voice < 8) ? midi_MidiChannels[voice] : 0;
}

static inline uint8_t midiParser_voiceNoteOverride(uint8_t voice)
{
   return (voice < 7) ? midi_NoteOverride[voice] : 0;
}

/* PARSER-OWNED MIDI ROLL STATE
   midiParser_rollNoteOffsetRaw:
     raw 0 disables shifted roll notes; raw 1..127 is the positive semitone
     offset above the normal trigger note. Set only through
     midiParser_setRollNoteOffset() (FRONT_SEQ_ROLL_NOTE_OFFSET).

   HELD-KEY TABLE (Session 038; replaces the per-voice hold counters)
   WHAT: one slot per physically held MIDI roll key, identified by
         (chan, note), storing the voices that key claimed at note-on.
         midiParser_rollHeldVoiceMask caches the union of all slot masks,
         which equals the sequencer's seq_rollMidiHeld mask.
   WHY:  per-voice counters could not tell which press a release belonged
         to. A NOTE_ON velocity 0 release was counted as a press, releases
         recomputed their voices from the current routing, and duplicate
         presses needed duplicate releases. Each of these could leave a roll
         stuck. Keying on the MIDI key makes release exact and idempotent.
   SIZE: MIDI_ROLL_MAX_HELD_KEYS = 16 simultaneous roll keys (3 bytes each).
         If the table is full, a new press is ignored rather than starting a
         roll with no release path.
   ENCODING: chan 1..16 matches the parser's chanonly; chan 0 marks a free
             slot. Zero-initialised storage therefore starts with every slot
             free.
   ACCESSORS: midiParser_rollFindHeldKey(), midiParser_rollFindFreeKeySlot(),
              midiParser_rollHeldKeysUnion(), midiParser_rollSyncVoices(),
              midiParser_rollKeyOn(), midiParser_rollKeyOff(),
              midiParser_clearMidiRollHolds() (all in this file).
   AFFILIATES: seq_rollMidiChange() / seq_rollMidiHeld
               (Sequencer/sequencer.c). */
#define MIDI_ROLL_MAX_HELD_KEYS 16

typedef struct
{
   uint8_t chan;       /* 1..16 = held on that MIDI channel; 0 = free slot */
   uint8_t note;       /* incoming (shifted) roll note number, 0..127      */
   uint8_t voiceMask;  /* voices 0..6 claimed by this key at note-on        */
} MidiRollHeldKey;

static uint8_t midiParser_rollNoteOffsetRaw = 0;
static MidiRollHeldKey midiParser_rollHeldKeys[MIDI_ROLL_MAX_HELD_KEYS];
static uint8_t midiParser_rollHeldVoiceMask = 0;

/* Roll-note helpers. Matching is defined against the normal note that would
   trigger the voice, then shifted upward by the current positive offset. MIDI
   note bounds are checked before every shifted comparison. */
static uint8_t midiParser_rollOffsetEnabled(void)
{
   return midiParser_rollNoteOffsetRaw != 0;
}

static uint8_t midiParser_rollOffsetValue(void)
{
   return midiParser_rollNoteOffsetRaw;
}

static uint8_t midiParser_shiftedNoteInRange(uint8_t baseNote,
                                             uint8_t offset,
                                             uint8_t *shiftedNote)
{
   if(offset == 0 || baseNote > (uint8_t)(127 - offset))
      return 0;

   *shiftedNote = (uint8_t)(baseNote + offset);
   return 1;
}

static uint8_t midiParser_shiftedBaseInRange(uint8_t incomingNote,
                                             uint8_t offset,
                                             uint8_t *baseNote)
{
   if(offset == 0 || incomingNote < offset)
      return 0;

   *baseNote = (uint8_t)(incomingNote - offset);
   return 1;
}

static uint8_t midiParser_rollNoteMatches(uint8_t incomingNote,
                                           uint8_t baseNote)
{
   uint8_t shiftedNote;

   return midiParser_shiftedNoteInRange(baseNote,
                                        midiParser_rollOffsetValue(),
                                        &shiftedNote)
      && shiftedNote == incomingNote;
}

static uint8_t midiParser_chromaticRollCandidate(uint8_t incomingNote)
{
   uint8_t baseNote;

   return midiParser_rollOffsetEnabled()
      && midiParser_shiftedBaseInRange(incomingNote,
                                       midiParser_rollOffsetValue(),
                                       &baseNote);
}

/* FIND A HELD ROLL KEY
   WHAT: linear search of the held-key table for an occupied slot whose
         (chan, note) matches.
   WHY:  releases and duplicate presses are resolved by MIDI key identity,
         not by voice (Session 038).
   INPUT:  chan 1..16 (the parser's chanonly), note 0..127.
   OUTPUT: slot index 0..MIDI_ROLL_MAX_HELD_KEYS-1, or -1 if not held.
   ACCESSORS: midiParser_rollKeyOn(), midiParser_rollKeyOff().
   AFFILIATES: midiParser_rollHeldKeys[] (this file). */
static int8_t midiParser_rollFindHeldKey(uint8_t chan, uint8_t note)
{
   int8_t slot;

   for(slot = 0; slot < MIDI_ROLL_MAX_HELD_KEYS; ++slot)
   {
      if(midiParser_rollHeldKeys[slot].chan == chan
         && midiParser_rollHeldKeys[slot].note == note)
         return slot;
   }
   return -1;
}

/* FIND A FREE HELD-KEY SLOT
   WHAT: returns the first slot whose chan is 0 (free).
   WHY:  a new roll key needs storage for the voices it claims. The caller
         treats a full table as "do not start the roll", because a roll that
         cannot be recorded here would have no release path.
   INPUT:  none.
   OUTPUT: free slot index, or -1 if all MIDI_ROLL_MAX_HELD_KEYS are in use.
   ACCESSORS: midiParser_rollKeyOn().
   AFFILIATES: midiParser_rollHeldKeys[] (this file). */
static int8_t midiParser_rollFindFreeKeySlot(void)
{
   int8_t slot;

   for(slot = 0; slot < MIDI_ROLL_MAX_HELD_KEYS; ++slot)
   {
      if(midiParser_rollHeldKeys[slot].chan == 0)
         return slot;
   }
   return -1;
}

/* UNION OF ALL HELD-KEY VOICE MASKS
   WHAT: ORs the voiceMask of every occupied slot.
   WHY:  a voice is MIDI-held while *any* held key claims it. This is the
         single source of truth for the sequencer's MIDI ownership bit.
   INPUT:  none.
   OUTPUT: bit v set means voice v is claimed by at least one held key.
   ACCESSORS: midiParser_rollSyncVoices().
   AFFILIATES: midiParser_rollHeldKeys[] (this file). */
static uint8_t midiParser_rollHeldKeysUnion(void)
{
   uint8_t slot;
   uint8_t mask = 0;

   for(slot = 0; slot < MIDI_ROLL_MAX_HELD_KEYS; ++slot)
   {
      if(midiParser_rollHeldKeys[slot].chan != 0)
         mask |= midiParser_rollHeldKeys[slot].voiceMask;
   }
   return mask;
}

/* SYNCHRONISE SEQUENCER MIDI OWNERSHIP WITH THE HELD-KEY TABLE
   WHAT: recomputes the union of claimed voices. Voices that left the union
         are released with seq_rollMidiChange(v, 0). Voices in assertMask
         that are still in the union are (re)asserted with
         seq_rollMidiChange(v, 1).
   WHY:  it replaces the per-voice 0->1 / 1->0 counter transitions. The
         deliberate re-assert on every press (not only on 0->1) is what lets
         a new key press re-fire a voice that is already MIDI-held:
           - running, one-shot rate: seq_setRoll() clears seq_rollTriggered
             after its single hit, so re-asserting re-arms it;
           - stopped: seq_rollMidiChange() re-arms the stopped roll clock
             for an immediate hit (Session 038, D4).
         For a running continuous roll, re-asserting a bit that is already
         set is a no-op in seq_rollApplyAggregate().
   INPUT:  assertMask, the voices claimed by the press being processed
           (0 for a release or a clear).
   OUTPUT: midiParser_rollHeldVoiceMask updated; seq_rollMidiChange() calls
           for released and asserted voices.
   ACCESSORS: midiParser_rollKeyOn(), midiParser_rollKeyOff(),
              midiParser_clearMidiRollHolds().
   AFFILIATES: seq_rollMidiChange() / seq_rollApplyAggregate()
               (Sequencer/sequencer.c). */
static void midiParser_rollSyncVoices(uint8_t assertMask)
{
   const uint8_t newMask = midiParser_rollHeldKeysUnion();
   const uint8_t releaseMask =
      (uint8_t)(midiParser_rollHeldVoiceMask & (uint8_t)~newMask);
   uint8_t voice;

   midiParser_rollHeldVoiceMask = newMask;

   for(voice = 0; voice < 7; ++voice)
   {
      const uint8_t voiceBit = (uint8_t)(1u << voice);

      if(releaseMask & voiceBit)
         seq_rollMidiChange(voice, 0);
      else if(assertMask & newMask & voiceBit)
         seq_rollMidiChange(voice, 1);
   }
}

/* CLAIM VOICES FOR A PRESSED ROLL KEY
   WHAT: records (chan, note) as held with its claimed voiceMask, then
         synchronises sequencer ownership, asserting the captured mask.
   WHY:  the note-off for this key must later release exactly these voices,
         whatever the routing looks like by then. A repeated note-on for a
         key that is already held does not take a second slot, so one
         note-off always suffices; it only reasserts the original mask.
   INPUT:  chan 1..16, note 0..127, voiceMask (bits 0..6) produced by the
           last-consumer matcher in midiParser_parseMidiMessage().
   OUTPUT: table slot occupied for a new key, or unchanged for a duplicate;
           seq_rollMidiChange(v, 1) reasserts each captured voice. With an
           empty mask, or a full table for a new key, nothing happens and no
           roll starts.
   ACCESSORS: midiParser_parseMidiMessage() note-on branch.
   AFFILIATES: midiParser_rollFindHeldKey(), midiParser_rollFindFreeKeySlot(),
               midiParser_rollSyncVoices() (this file). */
static void midiParser_rollKeyOn(uint8_t chan, uint8_t note, uint8_t voiceMask)
{
   int8_t slot;

   voiceMask &= 0x7f;
   if(!voiceMask)
      return;

   slot = midiParser_rollFindHeldKey(chan, note);
   if(slot < 0)
   {
      slot = midiParser_rollFindFreeKeySlot();
      if(slot < 0)
         return;

      midiParser_rollHeldKeys[slot].chan = chan;
      midiParser_rollHeldKeys[slot].note = note;
      midiParser_rollHeldKeys[slot].voiceMask = 0;
   }

   if(midiParser_rollHeldKeys[slot].voiceMask == 0)
      midiParser_rollHeldKeys[slot].voiceMask = voiceMask;

   midiParser_rollSyncVoices(midiParser_rollHeldKeys[slot].voiceMask);
}

/* RELEASE A ROLL KEY
   WHAT: frees the slot for (chan, note) if it is held, then synchronises
         sequencer ownership. Only voices that no other held key still
         claims are released.
   WHY:  a release must undo exactly what its own press claimed (Session 038).
         An unmatched note-off (never held, or cleared by a stop or a mapping
         change) is ignored instead of decrementing some other press's hold.
   INPUT:  chan 1..16, note 0..127.
   OUTPUT: slot freed; seq_rollMidiChange(v, 0) for each voice that became
           unheld. No effect for an unknown key.
   ACCESSORS: midiParser_parseMidiMessage() note-off branch, which includes
              NOTE_ON velocity 0.
   AFFILIATES: midiParser_rollFindHeldKey(), midiParser_rollSyncVoices()
               (this file). */
static void midiParser_rollKeyOff(uint8_t chan, uint8_t note)
{
   const int8_t slot = midiParser_rollFindHeldKey(chan, note);

   if(slot < 0)
      return;

   midiParser_rollHeldKeys[slot].chan = 0;
   midiParser_rollHeldKeys[slot].voiceMask = 0;
   midiParser_rollSyncVoices(0);
}

void midi_clearCache()
{
   /* Clear the parser-owned live MIDI cache. */
   uint16_t i;
   midiParser_clearMidiRollHolds();
   for (i=0;i<256;i++)
   {
      midi_midiCacheAvailable[i]=0;
   }
   for(i=0;i<6;i++)
   {
      midi_midiLfoCache[i]=0;
      midi_midiLfoCacheAvailable[i]=0;
      midi_midiVeloCache[i]=0;
      midi_midiVeloCacheAvailable[i]=0;
   }
}

/* RELEASE EVERY MIDI ROLL HOLD
   WHAT: empties the held-key table, then releases every voice that MIDI was
         holding, through seq_rollMidiChange(voice, 0).
   WHY:  some events invalidate the mapping between a held key and its
         eventual note-off, or are defined as "all MIDI rolls stop". A
         cleared key's later note-off finds no entry and is ignored, so it
         can neither strand nor wrongly release a roll. Clearing the parser
         table (not only the sequencer mask) is also what makes a key
         re-triggerable after a stop: a stale entry would make the next
         press of that key look like a duplicate.
   INPUT:  none.
   OUTPUT: midiParser_rollHeldKeys[] all free; midiParser_rollHeldVoiceMask
           = 0; seq_rollMidiHeld cleared bit by bit via seq_rollMidiChange().
           Manual (front-panel) roll ownership is untouched.
   ACCESSORS (all existing unless marked):
     - midi_clearCache() (this file; reached from seq_init())
     - midiParser_setRollNoteOffset() (this file; offset change)
     - midi_setFilter() (this file; RX note filter disabled) [S038 new]
     - FRONT_SEQ_MIDI_CHAN / FRONT_SEQ_MIDI_CHAN_OFF / FRONT_SEQ_TRACK_NOTE1..7
       (uARTFrontSYX/frontPanelReceivingProtocol.c)
     - seq_setRunning(0) (Sequencer/sequencer.c; sequencer stop) [S038 new]
   AFFILIATES: midiParser_rollSyncVoices() (this file), seq_rollMidiChange()
               (Sequencer/sequencer.c). Declared in MidiParser.h. */
void midiParser_clearMidiRollHolds(void)
{
   uint8_t slot;

   for(slot = 0; slot < MIDI_ROLL_MAX_HELD_KEYS; ++slot)
   {
      midiParser_rollHeldKeys[slot].chan = 0;
      midiParser_rollHeldKeys[slot].voiceMask = 0;
   }

   midiParser_rollSyncVoices(0);
}

/* Set the global MIDI roll-note offset. A changed offset invalidates every
   outstanding shifted-note hold, so MIDI roll ownership is released before
   the new raw value is installed. */
void midiParser_setRollNoteOffset(uint8_t rawOffset)
{
   rawOffset &= 0x7f;

   if(rawOffset != midiParser_rollNoteOffsetRaw)
   {
      midiParser_clearMidiRollHolds();
      midiParser_rollNoteOffsetRaw = rawOffset;
   }
}

static union {
   uint8_t value;
   struct {
      unsigned usb2midi:1;
      unsigned usb2usb:1;   // not used
      unsigned midi2midi:1;
      unsigned midi2usb:1;
      unsigned :4;
   } route;
} midiParser_routing = {0};

/* High nibble is TX, low nibble is RX. */
uint8_t midiParser_txRxFilter = 0xFF;

enum State
{
MIDI_STATUS,  		// waiting for status byte
MIDI_DATA1,  		// waiting for data byte1
MIDI_DATA2,  		// waiting for data byte2
SYSEX_DATA,  		// read sysex data byte
IGNORE				// set when unknown status byte received, stays in ignore mode until next known status byte
};

// 2^(1/12) factor for 1 semitone
#define SEMITONE_UP 1.0594630943592952645618252949463f
#define SEMITONE_DOWN 0.94387431268169349664191315666753f

#define NUM_LFO 6
/* Track which voice each LFO display slot is bound to. */
uint8_t midiParser_selectedLfoVoice[NUM_LFO] = {0,0,0,0,0,0};

#if 0
// -- AS for debugging
void midiDebugSend(uint8_t b1, uint8_t b2)
{
uart_sendMidiByte(0xF2);
uart_sendMidiByte(b1&0x7F);
uart_sendMidiByte(b2&0x7F);
}
#endif

//----------------------------------------------------------
#if 0
inline uint16_t calcSlopeEgTime(uint8_t data2)
{
float val = (data2+1)/128.f;
return data2>0?val*val*data2*128:1;
}
#endif
//-----------------------------------------------------------
/* Voice/channel state owned by the parser and read by the split helpers. */
uint8_t midi_MidiChannels[8];	// the currently selected midi channel for each voice (element 7 is global channel)

//--AS note overrides for each voice
uint8_t midi_NoteOverride[7];
//uint8_t midi_mode; //--AS not used anymore
MidiMsg midiMsg_tmp;				// buffer message where the incoming data is stored
// these two are used only when building up a midi message
//static uint8_t msgLength;					// number of following data bytes expected for current status
static uint8_t parserState = IGNORE;	// state of the parser state machine. Set to what it's expecting next
									// we set it to ignore initially so that any random data we get before
									// a valid msg header is ignored

/* This CMSIS revision omits the DWT declarations even though STM32F407 has
   the standard Cortex-M4 DWT block. These architectural addresses are DWT
   CTRL and CYCCNT; keeping them local prevents a legacy-header gap from
   leaking into transport interfaces. */
#define MIDI_DWT_CTRL    (*(volatile uint32_t *)0xE0001000UL)
#define MIDI_DWT_CYCCNT  (*(volatile uint32_t *)0xE0001004UL)
#define MIDI_DWT_CYCCNTENA (1UL << 0)

/* The DWT cycle counter timestamps realtime MIDI at IRQ receive boundaries.
   It is deliberately independent of systick_ticks: SysTick is 250 us
   resolution and is used by transport tempo logic, while this counter records
   sub-block queue latency for diagnostics without changing event timing. */
void midiParser_initRealtimeTimestamp(void)
{
   CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
   MIDI_DWT_CYCCNT = 0;
   MIDI_DWT_CTRL |= MIDI_DWT_CYCCNTENA;
}

/* Read the enabled DWT cycle counter for one realtime transport event. The
   caller owns storage and wrap-safe latency subtraction; this function has no
   parser side effects and is safe in the short capture-only IRQ paths. */
uint32_t midiParser_captureRealtimeTimestamp(void)
{
   return MIDI_DWT_CYCCNT;
}

//-----------------------------------------------------------
/* Convert a MIDI value to the parser's detune factor. */
float midiParser_calcDetune(uint8_t value)
{
//linear interpolation between 1(no change) and semitone up/down)
   float frac = (value/127.f -0.5f);
   float cent = 1;
   if(cent>=0)
   {
      cent += frac*(SEMITONE_UP - 1);
   }
   else
   {
      cent += frac*(SEMITONE_UP - 1);
   }
   return cent;
}


//-----------------------------------------------------------
/* EXTERNAL MIDI MESSAGE DISPATCH
   WHAT: entry point for every complete external MIDI message (DIN via
         midiParser_parseUartData(), USB via usb_midi_core.c). Order:
           1. mrt routing: forward to DIN/USB outputs per midiParser_routing,
              UNFILTERED;
           2. SysEx bytes: stop here (pass-through only);
           3. system messages -> globalMidiParser_handleSystemMessage()
              (RX R bit gates clock/start/stop/MTC there);
           4. channel messages, each gated by its RX letter:
              notes N (0x01), program change P (0x08), control change C (0x04).
   WHY:  the receive filter (rxf) decides only what the LXR itself responds
         to; routing (mrt) decides what is passed through. Keeping routing
         first preserves that separation.
   INPUT:  msg - parsed message with bits.source (DIN/USB).
   OUTPUT: forwarded bytes; voice/sequencer/parameter side effects for
           accepted messages.
   ACCESSORS: midiParser_parseUartData() (this file), main.c USB dispatch,
              usb_midi_core.c.
   AFFILIATES: midi_setRouting(), midi_setFilter(), midiParser_handleDinRealtime()
               (DIN realtime takes its own route + filter path). */
void midiParser_parseMidiMessage(MidiMsg msg)
{

// route message if needed
   if(midiParser_routing.value) {
      if(msg.bits.source==midiSourceUSB) {
         if(midiParser_routing.route.usb2midi){
         // route to midi out port
            uart_sendMidi(msg);
         }
      } 
      else if(msg.bits.source==midiSourceMIDI) {
         if(midiParser_routing.route.midi2midi) {
         // route to midi out port
            uart_sendMidi(msg);
         }
         if(midiParser_routing.route.midi2usb) {
         // route to usb out port
            usb_sendMidi(msg);
         }
      }
   }

   if(msg.bits.sysxbyte)
      return; // no further action needed. we don't interpret sysex data right now

// --AS FILT filter messages here. Filter inline below to be more optimal
// we are interested in the low nibble since we are Rx here


   if((msg.status & 0xF0) == 0XF0) {
   // BC: !!!NB!!! for midi jack input, system realtime messages 
   // are dealt with at the top of midiParser_parseUartData(),
   // to avoid conflicts with channel-specific messages. These may
   // not need repeating here, or may only exist for USB messages.
   
   // non-channel specific messages (system messages)
      globalMidiParser_handleSystemMessage(msg);
      return;
   
   } 
   else { // channel specific message
      const uint8_t msgonly =msg.status & 0xF0;
      const uint8_t chanonly=(msg.status&0x0F)+1;
   
      if((msgonly & 0xE0) == 0x80) {
      // note on or note off message (one of these two only)
         if(midiParser_txRxFilter & 0x01) {
            uint8_t normalConsumed = 0;
            uint8_t rollVoiceMask = 0;
            /* NOTE-OFF CLASSIFICATION (MIDI 1.0: NOTE_ON velocity 0 == NOTE_OFF)
               WHAT: a message is a note-off if its status is NOTE_OFF, or if
                     its status is NOTE_ON and its velocity (data2) is 0.
               WHY:  many senders transmit releases as NOTE_ON velocity 0 so
                     that running status stays unbroken (the Octatrack is the
                     Session 038 reproduction). The MIDI roll feature
                     originally read statuses literally, so every such release
                     was counted as a second press and the roll stuck. That
                     "literal status" design decision is withdrawn (Session
                     038, D1). Do NOT reintroduce literal-status handling.
                     For normal notes this changes nothing observable:
                     channelMidiParser_noteOff() forces vel = 0 and delegates
                     to channelMidiParser_noteOn(), which already treats
                     vel == 0 as "no trigger, record as note-off, echo vel 0".
               INPUT:  msgonly (status high nibble), msg.data2 (velocity).
               OUTPUT: isNoteOff selects noteOff() on the normal routes and
                       the release branch on the MIDI roll path.
               ACCESSORS: read only inside this note block.
               AFFILIATES: channelMidiParser_noteOn/Off()
                           (MIDI/ChannelMidiParser.c); seq_addNote()'s
                           isNoteOff contract (Sequencer/sequencer.c) is
                           unaffected, since both routes already pass 1. */
            const uint8_t isNoteOff = (msgonly == NOTE_OFF)
                                   || (msgonly == NOTE_ON && msg.data2 == 0);
            uint8_t v;

            /* Run the existing global and voice note consumers first. A voice
               override mismatch is not a consumed note: ChannelMidiParser
               receives the call for legacy behavior but returns without a
               trigger, allowing a shifted override note to reach roll. */
            if(midiParser_voiceMidiChannel(7) == chanonly)
            {
               const uint8_t activeTrackOverride =
                  midiParser_voiceNoteOverride(frontParser_activeTrack);

               if(!activeTrackOverride)
               {
                  /* In chromatic mode, shifted roll notes must get a real
                     last-consumer lane. If the incoming note can be shifted
                     back by the roll offset, leave it unconsumed so the roll
                     matcher below can claim it; otherwise keep legacy
                     chromatic note behavior. */
                  if(!midiParser_chromaticRollCandidate(msg.data1))
                  {
                     normalConsumed = 1;
                     if(isNoteOff)
                        channelMidiParser_noteOff(frontParser_activeTrack,
                                                  msg.data1,
                                                  msg.data2,
                                                  1);
                     else
                        channelMidiParser_noteOn(frontParser_activeTrack,
                                                 msg.data1,
                                                 msg.data2,
                                                 1);
                  }
               }
               else
               {
                  for(v = 0; v < 7; ++v)
                  {
                     if(midiParser_voiceNoteOverride(v) == msg.data1)
                     {
                        normalConsumed = 1;
                        if(isNoteOff)
                           channelMidiParser_noteOff(v, msg.data1, msg.data2, 1);
                        else
                           channelMidiParser_noteOn(v, msg.data1, msg.data2, 1);
                     }
                  }
               }
            }

            /* Additionally check each assigned voice channel, preserving the
               existing active-track recording distinction. */
            for(v = 0; v < 7; ++v)
            {
               if(midiParser_voiceMidiChannel(v) == chanonly)
               {
                  const uint8_t noteOverride = midiParser_voiceNoteOverride(v);
                  const uint8_t chromaticRollCandidate =
                     (noteOverride == 0)
                        && midiParser_chromaticRollCandidate(msg.data1);

                  if((noteOverride == 0 && !chromaticRollCandidate)
                     || noteOverride == msg.data1)
                     normalConsumed = 1;

                  if(chromaticRollCandidate)
                     continue;

                  if(isNoteOff)
                  {
                     if(v == frontParser_activeTrack)
                        channelMidiParser_noteOff(v, msg.data1, msg.data2, 1);
                     else
                        channelMidiParser_noteOff(v, msg.data1, msg.data2, 0);
                  }
                  else
                  {
                     if(v == frontParser_activeTrack)
                        channelMidiParser_noteOn(v, msg.data1, msg.data2, 1);
                     else
                        channelMidiParser_noteOn(v, msg.data1, msg.data2, 0);
                  }
               }
            }

            /* MIDI ROLL-NOTE PATH (release first, then last-consumer claim)
               WHAT: a note-off (including NOTE_ON velocity 0, see isNoteOff
                     above) releases the held roll key (chanonly, note) if one
                     exists, and does nothing else on the roll path. A note-on
                     that no normal route consumed is matched against the
                     positive-offset roll map. The resulting voice mask is
                     claimed by that key in the held-key table.
               WHY:  Session 038 stuck-roll bug. Releases used to recompute a
                     voice mask from the *current* routing and decrement
                     per-voice counters, so any sender using NOTE_ON velocity 0
                     as note-off (e.g. the Octatrack) added a second hold
                     instead of releasing. A routing drift between press and
                     release (such as an active-track change on the global
                     chromatic route) also released the wrong voice. A release
                     is now a lookup of what the press claimed, and it is
                     deliberately not gated by normalConsumed, the offset
                     state, or routing: any accepted roll key must always be
                     releasable.
               INPUT:  chanonly (1..16), msg.data1 (incoming note), isNoteOff,
                       normalConsumed, and the offset/channel/override tables
                       read by the unchanged mask builder below.
               OUTPUT: held-key table updated; seq_rollMidiChange() called for
                       each voice whose MIDI hold state changes (or, on a
                       press, is re-asserted).
               ACCESSORS: the only caller of midiParser_rollKeyOn() and
                          midiParser_rollKeyOff().
               AFFILIATES: midiParser_rollKeyOn/Off(),
                           midiParser_rollSyncVoices() (this file);
                           seq_rollMidiChange() (Sequencer/sequencer.c). */
            if(isNoteOff)
            {
               midiParser_rollKeyOff(chanonly, msg.data1);
            }
            else if(!normalConsumed && midiParser_rollOffsetEnabled())
            {
               const uint8_t offset = midiParser_rollOffsetValue();
               uint8_t baseNote;

               if(midiParser_voiceMidiChannel(7) == chanonly)
               {
                  const uint8_t activeTrackOverride =
                     midiParser_voiceNoteOverride(frontParser_activeTrack);

                  if(!activeTrackOverride)
                  {
                     if(frontParser_activeTrack < 7
                        && midiParser_shiftedBaseInRange(msg.data1,
                                                         offset,
                                                         &baseNote))
                     {
                        rollVoiceMask |= (uint8_t)(1u << frontParser_activeTrack);
                     }
                  }
                  else
                  {
                     for(v = 0; v < 7; ++v)
                     {
                        const uint8_t noteOverride =
                           midiParser_voiceNoteOverride(v);
                        if(noteOverride
                           && midiParser_rollNoteMatches(msg.data1,
                                                         noteOverride))
                        {
                           rollVoiceMask |= (uint8_t)(1u << v);
                        }
                     }
                  }
               }

               for(v = 0; v < 7; ++v)
               {
                  const uint8_t noteOverride = midiParser_voiceNoteOverride(v);

                  if(midiParser_voiceMidiChannel(v) != chanonly)
                     continue;

                  if(!noteOverride)
                  {
                     if(midiParser_shiftedBaseInRange(msg.data1,
                                                      offset,
                                                      &baseNote))
                     {
                        rollVoiceMask |= (uint8_t)(1u << v);
                     }
                  }
                  else if(midiParser_rollNoteMatches(msg.data1, noteOverride))
                  {
                     rollVoiceMask |= (uint8_t)(1u << v);
                  }
               }

               /* Claim the computed voices for this key. An empty mask (no
                  voice maps to this shifted note) claims nothing. */
               midiParser_rollKeyOn(chanonly, msg.data1, rollVoiceMask);
            }
         } // check midi filter
         
      } 
      else if(msgonly==PROG_CHANGE) 
      {
         
      // --AS respond to prog change and change patterns. This responds only when global channel matches the PC message's channel.
         //send the ack message to tell the front that a new pattern starts playing
         if(midiParser_txRxFilter & 0x08)
         {
            if(chanonly == midiParser_voiceMidiChannel(7))
            {
               if(msg.data1<16)
               {
                  seq_setNextPattern(msg.data1&0x07,0x7f);
                  if(msg.data1>7)
                  {
                     seq_newVoiceAvailable=0x7f;
                  }
               }   
            }
            uint8_t i;
            for(i=0;i<NUM_TRACKS;i++) // set individual track patterns with PC on that channel
            {
               if(chanonly == midiParser_voiceMidiChannel(i))
               {
                  seq_setNextPattern(msg.data1&0x07,i);
                  seq_newVoiceAvailable&=(0x01<<i);
               }
            }   
         }
      } 
      else if(msgonly==MIDI_CC){
         /* RX CONTROLLER FILTER GATE  (Session 039 P2)
            WHAT: passes an incoming Control Change to the CC router only
                  when the RX filter's C bit (0x04) is set. This covers every
                  channel (Global and voice) and every CC number: CC0 bank/kit
                  change, CC1 morph, NRPN select/data/RPN-null, and both CC
                  parameter tables.
            WHY:  rxf "C" means "controller messages are received". Before this
                  gate the CC router ran unconditionally, and 0x04 was only
                  tested deep inside the CC ladders, where it suppressed the
                  front-panel display echo and automation recording. With C off,
                  CCs still changed the sound and the stored kit while the
                  display silently desynchronised.
                  Routing (mrt) is deliberately unaffected: forwarding to the
                  DIN/USB outputs already happened at the top of this function,
                  before any RX filtering, so a filtered CC is still passed
                  through but does not affect the LXR.
            INPUT:  msg - external MIDI_CC (DIN or USB, any channel);
                    midiParser_txRxFilter RX nibble bit 0x04.
            OUTPUT: midiParser_MIDIccHandler(msg, 1) when C is on (1 = store
                    to kit / record automation; 0 would be DSP-only); nothing
                    when C is off. NRPN selection state is left untouched.
            ACCESSORS: this function only. It is the single external CC
                       ingress; internal automation and front-panel edits
                       reach DSP via frontParser_applyParameterCommand() and
                       never pass here.
            AFFILIATES: midiParser_MIDIccHandler() (this file),
                        channelMidiParser_MIDIccHandler() (ChannelMidiParser.c),
                        globalMidiParser_MIDIccHandler() and its NRPN handler
                        (GlobalMidiParser.c), midi_setFilter() (RX nibble
                        writer), AVR PAR_MIDI_FILT_RX (C and ¢ entries).
                        The inner (midiParser_txRxFilter & 0x04) tests in
                        ChannelMidiParser.c and GlobalMidiParser.c are now
                        always true for external CCs. They are kept as
                        harmless guards; do not read them as the RX C
                        implementation. */
         if(midiParser_txRxFilter & 0x04)
            midiParser_MIDIccHandler(msg,1);
      }
      else {
      // anything else
      // TODO MIDI_PITCH_WHEEL ?
      }
   } // channel specific vs non channel specific

}

//-----------------------------------------------------------


/* Split CC router. Global CC0/CC1 intentionally keep the existing channel
   parser bank/morph behavior; Global CC2-127 use the alternate global table. */
void midiParser_MIDIccHandler(MidiMsg msg, uint8_t updateOriginalValue)
{
   const uint8_t chanonly = (msg.status & 0x0F) + 1;

   if(chanonly == midiParser_voiceMidiChannel(7))
   {
      if(msg.data1 == BANK || msg.data1 == MOD_WHEEL)
         channelMidiParser_MIDIccHandler(msg, updateOriginalValue);
      else
         globalMidiParser_MIDIccHandler(msg, updateOriginalValue);

      return;
   }

   channelMidiParser_MIDIccHandler(msg, updateOriginalValue);
}
/* Cache a new status byte and prime the running-status state machine. */
void midiParser_handleStatusByte(unsigned char data)
{
// we received a channel voice/mode byte. set the status as appropriate
   switch(data&0xF0) {
   // 2 databyte messages
      case NOTE_OFF:
      case NOTE_ON:
      case MIDI_CC:
      case MIDI_PITCH_WHEEL:
      case MIDI_AT:
         midiMsg_tmp.status = data;	// store the new status byte
         parserState = MIDI_DATA1;	//status received, next should be data byte 1
         midiMsg_tmp.bits.length=2;// status is followed by 2 data bytes
         break;
   
   // 1 databyte messages
      case PROG_CHANGE:
      case CHANNEL_PRESSURE:
         midiMsg_tmp.status = data;	// store the new status byte
         parserState = MIDI_DATA1;	//status received, next should be data byte 1
         midiMsg_tmp.bits.length=1;// status is followed by 1 data bytes
         break;
   
   // messages we don't care about right now, and don't know how to handle or passthru (Are there any?).
      default:
         parserState = IGNORE;	// throw away any data bytes until next message
         midiMsg_tmp.bits.length=0;
      
         break;
   }
}
//-----------------------------------------------------------
/* Consume one DIN system-realtime byte in main-loop context. Input is a
   complete status byte captured by the USART realtime queue; output is the
   same external-sync transport action and optional DIN/USB forwarding that
   the legacy raw-byte parser performed. This helper intentionally owns no
   queue state and performs no timing capture, allowing the UART transport to
   prioritize arrival without moving sequencer or TX work into an ISR. */
void midiParser_handleDinRealtime(uint8_t data)
{
   switch(data)
   {
      case MIDI_START:
      case MIDI_CONTINUE:
         if((midiParser_txRxFilter & 0x02) && seq_getExtSync())
            sync_midiStartStop(1);
         break;

      case MIDI_STOP:
         if((midiParser_txRxFilter & 0x02) && seq_getExtSync())
            sync_midiStartStop(0);
         break;

      case MIDI_CLOCK:
         if((midiParser_txRxFilter & 0x02) && seq_getExtSync())
            seq_sync();
         break;

      default:
         break;
   }

   if(midiParser_routing.value)
   {
      MidiMsg rtMsg = {0};
      rtMsg.status = data;
      if(midiParser_routing.route.midi2midi)
         uart_sendMidi(rtMsg);
      if(midiParser_routing.route.midi2usb)
         usb_sendMidi(rtMsg);
   }
}

// This will build up the midi message and hand it off to
// parseMidiMessage when it's complete
/* Byte-stream parser for raw MIDI UART input. */
void midiParser_parseUartData(unsigned char data)
{

   if(data&0x80) { // High bit is set -  its either a status or a system message.
   // regardless of current state, we blindly start a new message without questioning it
      if((data&0xf8)==0xf8) // direct callers retain legacy realtime handling
      {
         midiParser_handleDinRealtime(data);
         return; // realtime has no follow-up data and must not alter parser state
      }
      midiMsg_tmp.bits.sysxbyte=0;
      if( (data&0xf0) == 0xf0) { // system message
         midiMsg_tmp.status = data;
         switch(data) {
            case SYSEX_START: // get into sysex receive mode. any more bytes received until this status changes are considered
            		  // to be sysex data. we still need to parse this sysex start, in case we are routing it
               parserState = SYSEX_DATA;
               midiMsg_tmp.bits.length=0;
               goto parseMsg; // we will still parse it in case we are doing a passthru
            case SYSEX_END:	  // get out of sysex mode
               if(parserState==SYSEX_DATA) {
                  parserState=MIDI_STATUS;
                  midiMsg_tmp.bits.length=0;
                  goto parseMsg; // we will still parse it in case we are doing a passthru
               } 
               else {
               // spurious sysex end msg received. ignore it
               }
               break; //
         // 1 byte payload messages
            case MIDI_SONG_SEL:		// passthru only
            case MIDI_MTC_QFRAME: 	// mtc chunk
               parserState = MIDI_DATA1; 	// we expect the nugget of mtc frame info
               midiMsg_tmp.bits.length=1;// we expect 1 data byte
               break;
         
         // 0 byte payload messages (we will assume that any system message
         // other than those above has 0 byte payload)
            default:
               midiMsg_tmp.bits.length=0;
               goto parseMsg;
         }
      } 
      else { //status byte (channel specific message containing a channel number)
         midiParser_handleStatusByte(data);
      }
   } 
   else { // high bit is not set - it's a data byte
   
      switch(parserState)	{
         case MIDI_STATUS: // we are expecting status msg, but got data, so running status may be in effect
            if(midiMsg_tmp.bits.length) {
               midiMsg_tmp.data1 = data;
               if(midiMsg_tmp.bits.length==2)
                  parserState=MIDI_DATA2;
               else {
                  midiMsg_tmp.data2=0;
                  goto parseMsg;
               }
            } 
            else {
               break; // last msg had 0 payload, so wtf is this? just ignore it
            }
            break;
      
         case MIDI_DATA1:
            midiMsg_tmp.data1 = data;
            if(midiMsg_tmp.bits.length==2) {
            // we need another byte before we can do anything meaningful
               parserState = MIDI_DATA2;
            } 
            else { // it must be 1
               goto parseMsg;
            }
            break;
         case MIDI_DATA2:
            midiMsg_tmp.data2 = data;
            goto parseMsg; // message complete
         case SYSEX_DATA: // we are in sysex mode
            midiMsg_tmp.bits.sysxbyte=1;
            midiMsg_tmp.status = data; // status will contain the sysex byte
            midiMsg_tmp.bits.length=0;
            goto parseMsg;
         default: //we are expecting no data byte, but we got one.
         // ignore it
            break;
      } // switch parserState
   } // if high bit is set

   return;

parseMsg:
// we get here if we have proudly received a message that we want to do something with
   if(parserState != SYSEX_DATA) // we are still in sysex receive mode
      parserState = MIDI_STATUS; // next byte should be a new message, or we don't care about it
   midiMsg_tmp.bits.source=midiSourceMIDI;
   midiParser_parseMidiMessage(midiMsg_tmp);

}

// 0 - Off - nothing to nothing
// 1 - U2M - usb in to midi out
// 2 - M2M - midi in to midi out
// 3 - A2M - usb in and midi in to midi out
// 4 - M2U - midi in to usb out
// 5 - M2A - midi in to usb out and midi out

/* Configure the MIDI routing matrix. */
void midi_setRouting(uint8_t value)
{
   midiParser_routing.value=0;

   switch(value) {
      case 1:
         midiParser_routing.route.usb2midi=1;
         break;
      case 2:
         midiParser_routing.route.midi2midi=1;
         break;
      case 3:
         midiParser_routing.route.usb2midi=1;
         midiParser_routing.route.midi2midi=1;
         break;
      case 4:
         midiParser_routing.route.midi2usb=1;
         break;
      case 5:
         midiParser_routing.route.midi2midi=1;
         midiParser_routing.route.midi2usb=1;
         break;
      default:
      case 0:
      
         break;
   }

}

/* Configure the TX/RX MIDI filter bitfields; RX also decodes the ¢ flag. See MidiParser.h for the full contract. */
void midi_setFilter(uint8_t is_tx, uint8_t value)
{

   if(is_tx) // set the high nibble to value
      midiParser_txRxFilter = (value << 4) | (midiParser_txRxFilter & 0x0F);
   else // set the low nibble to value
   {
      /* MIDI ROLL HOLD RELEASE ON RX NOTE-FILTER DISABLE
         WHAT: when the RX nibble changes so that bit 0 (receive notes) goes
               from enabled to disabled, every parser-owned MIDI roll hold is
               released before the new filter value is installed.
         WHY:  midiParser_parseMidiMessage() only enters its note block while
               (midiParser_txRxFilter & 0x01) is set. A roll key held at the
               moment note reception is switched off would never see its
               note-off, so its roll would stick until the next stop or
               mapping change. Releasing here keeps the rule "every accepted
               roll key has a guaranteed release path".
         INPUT:  value, the new RX nibble (low 4 bits) plus the optional ¢ flag in bit 4 (MIDI_RX_FILTER_CC38_SWAP), decoded below.
         OUTPUT: midiParser_txRxFilter updated. MIDI-owned rolls released
                 through midiParser_clearMidiRollHolds() when bit 0 turns off.
         ACCESSORS: called from the front-panel MIDI filter setting path.
         AFFILIATES: midiParser_clearMidiRollHolds() (this file),
                     seq_rollMidiChange() (Sequencer/sequencer.c). */
      if((midiParser_txRxFilter & 0x01) && !(value & 0x01))
         midiParser_clearMidiRollHolds();

      /* RX ¢ FLAG DECODE  (Session 039)
         WHAT: forwards bit 4 of the RX value to the Global parser as the CC38
               swap state, then lets the existing line below store only the
               low nibble.
         WHY:  the ¢ RxFilter entries are ordinary RX filters plus "CC38 off,
               NRPN 96 = Drum 1 cutoff"; the flag must not leak into
               midiParser_txRxFilter, whose high nibble is TX.
         INPUT:  value bit MIDI_RX_FILTER_CC38_SWAP.
         OUTPUT: globalMidiParser_cc38Swap set or cleared.
         ACCESSORS: this function (RX branch) only.
         AFFILIATES: globalMidiParser_setCc38Swap(),
                     globalMidiParser_handleNrpnControl() and
                     globalMidiParser_applySpecialNrpn() (GlobalMidiParser.c). */
      globalMidiParser_setCc38Swap((uint8_t)((value & MIDI_RX_FILTER_CC38_SWAP) != 0));

      midiParser_txRxFilter = (value & 0x0F) | (midiParser_txRxFilter & 0xF0);
   }

}
