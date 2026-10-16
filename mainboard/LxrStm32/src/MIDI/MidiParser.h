/*
 * MidiParser.h
 *
 *  Created on: 13.04.2012
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


#ifndef MIDIPARSER_H_
#define MIDIPARSER_H_

#include "stm32f4xx.h"
#include "globals.h"
#include "MidiMessages.h"
#include "MidiOutputControl.h"

/* Stream parser entry point. This remains the top-level MIDI byte coordinator
   while channel/global ownership lives in the split helper modules. */
void midiParser_parseUartData(unsigned char data);
/* Consume one DIN system-realtime byte in main-loop context. Input is a
   complete status byte captured by the USART realtime queue; output is the
   same external-sync transport action and optional DIN/USB forwarding that
   the legacy raw-byte parser performed. This helper intentionally owns no
   queue state and performs no timing capture, allowing the UART transport to
   prioritize arrival without moving sequencer or TX work into an ISR. */
void midiParser_handleDinRealtime(uint8_t data);
/* Enable the Cortex-M cycle counter used to timestamp realtime MIDI at each
   transport's first firmware-visible receive boundary. The counter is a
   diagnostic/ordering clock only; audio sample-offset scheduling remains out
   of scope for this interface. */
void midiParser_initRealtimeTimestamp(void);
/* Return the current 32-bit DWT cycle count for a realtime transport event.
   Callers store it with a queue entry and later derive queue-to-dispatch
   latency by unsigned subtraction, which remains valid across counter wrap. */
uint32_t midiParser_captureRealtimeTimestamp(void);

void midiParser_parseMidiMessage(MidiMsg msg);
void midiParser_MIDIccHandler(MidiMsg msg, uint8_t updateOriginalValue);
float midiParser_calcDetune(uint8_t value);
/* Watchdog for the MIDI timecode start/stop path. */
void midiParser_checkMtc();

#if 0
void midiDebugSend(uint8_t b1, uint8_t b2);
#endif

/* Routing selection for the MIDI I/O bridge. */
void midi_setRouting(uint8_t value);

/* MIDI TX/RX FILTER SETTER
   WHAT: installs the TX (high nibble) or RX (low nibble) MIDI filter of
         midiParser_txRxFilter. Bit layout per nibble: 0x01 notes, 0x02
         realtime/clock, 0x04 control change (RX: gates all incoming
         CC/NRPN at the midiParser_parseMidiMessage() entry, Session 039 P2;
         TX: reserved, as the LXR transmits no CCs), 0x08 program change.
         The RX filter never affects mrt routing, which forwards first.
         RX only: bit 0x10 (MIDI_RX_FILTER_CC38_SWAP) is the Session 039 ¢ flag.
         It is stripped before the nibble is stored and forwarded to
         globalMidiParser_setCc38Swap().
   WHY:  one front-panel opcode (FRONT_SEQ_MIDI_RX_FILTER) carries both the RX
         nibble and the ¢ flag, so ¢ entries need no new protocol traffic.
   INPUT:  is_tx - 1 for TX, 0 for RX; value - filter bits (RX may carry 0x10).
   OUTPUT: midiParser_txRxFilter updated; RX also updates the CC38 swap state
           and may release MIDI roll holds (note bit turning off).
   ACCESSORS: frontPanelReceivingProtocol.c FRONT_SEQ_MIDI_TX/RX_FILTER case.
   AFFILIATES: globalMidiParser_setCc38Swap() (GlobalMidiParser.c),
               midiParser_clearMidiRollHolds() (MidiParser.c),
               AVR menu_parseGlobalParam() PAR_MIDI_FILT_RX (sender). */
void midi_setFilter(uint8_t is_tx, uint8_t value);

/* RX FILTER ¢ FLAG  (Session 039)
   WHAT: bit 4 of the RX filter byte sent by the AVR. Set only by the ¢ twins
         of RxFilter (AVR menu values 16..23).
   WHY:  Elektron devices append CC38 (data-entry LSB) to every NRPN, which
         lands on Global CC38 = Drum 1 filter cutoff. While this flag is set,
         Global CC38 is consumed and NRPN 96 controls Drum 1 cutoff instead.
   ACCESSORS: midi_setFilter() (decode). The AVR encodes the same value as
              MIDI_FILTER_RX_CC38_SWAP in Menu/MenuText.h; keep them equal. */
#define MIDI_RX_FILTER_CC38_SWAP 0x10

/* Last seen CC values used by automation and live-apply feedback. */
extern uint8_t frontParser_originalCcValues[0xff];

/* MIDI parser caches shared with the receive and voice-control code. */
extern MidiMsg midi_midiCache[256];
extern MidiMsg midi_midiKit[256];
extern uint8_t midi_midiCacheAvailable[256];
extern uint8_t midi_midiLfoCache[6];
extern uint8_t midi_kitLfoCache[6];
extern uint8_t midi_midiLfoCacheAvailable[6];
extern uint8_t midi_midiVeloCache[6];
extern uint8_t midi_kitVeloCache[6];
extern uint8_t midi_midiVeloCacheAvailable[6];

extern uint8_t midi_envPosition[6];
extern uint8_t midi_unused;

void midi_clearCache();

/* MIDI ROLL-NOTE OFFSET AND HOLD OWNERSHIP
   MidiParser owns these because it is the only layer that knows which
   incoming note/channel pairs map to shifted roll triggers.

   Hold model (Session 038): each pressed roll key (MIDI channel + note) is
   stored with the voices it claimed. Its note-off, which may be NOTE_OFF or
   NOTE_ON velocity 0, releases exactly those voices. A voice stays
   MIDI-held while any held key claims it. MIDI ownership is reported to the
   sequencer through seq_rollMidiChange() and is kept separate from manual
   front-panel roll ownership.

   midiParser_setRollNoteOffset()
     INPUT:  rawOffset. 0 = off, 1..127 = positive semitone offset (masked to
             7 bits).
     OUTPUT: the stored offset. If the value changed, every MIDI roll hold is
             released first.
     ACCESSOR: FRONT_SEQ_ROLL_NOTE_OFFSET
               (uARTFrontSYX/frontPanelReceivingProtocol.c).

   midiParser_clearMidiRollHolds()
     WHAT: releases every MIDI roll hold and empties the held-key table.
           Later note-offs for cleared keys are ignored, and a fresh press of
           any key starts a new roll.
     ACCESSORS: offset change, MIDI channel change or off, track note
                override change (frontPanelReceivingProtocol.c);
                midi_clearCache() and the RX note-filter disable
                (MidiParser.c); sequencer stop, seq_setRunning(0)
                (Sequencer/sequencer.c). */
void midiParser_setRollNoteOffset(uint8_t rawOffset);
void midiParser_clearMidiRollHolds(void);

/* Voice MIDI channel table. Element 7 is the global channel. */
extern uint8_t midi_MidiChannels[8]; // last element is global channel
extern uint8_t midi_NoteOverride[7];
extern uint8_t midi_KitChange[6];
//extern uint8_t midi_mode; --AS not used anymore

//enum MIDI_modeEnum
//{
//	MIDI_MODE_TRIGGER,
//	MIDI_MODE_NOTE,
//} MidiModes;

/* MIDI FILTER STATE
   WHAT: high nibble TX, low nibble RX; see midi_setFilter() for the bit
         layout. The RX ¢ flag (MIDI_RX_FILTER_CC38_SWAP) is NOT stored here;
         it lives in GlobalMidiParser.c (globalMidiParser_cc38Swap).
   WHY:  one byte read on every MIDI message; nibble split keeps TX and RX
         independent.
   WRITER: midi_setFilter() only. Default 0xFF (everything on) until the AVR
           sends its globals at boot.
   READERS: RX - midiParser_parseMidiMessage() (N 0x01, P 0x08, C 0x04 gate
            at entry since Session 039 P2), midiParser_handleDinRealtime()
            and globalMidiParser_handleSystemMessage() (R 0x02), plus legacy
            0x04 echo/record guards in the CC ladders (now always true for
            external CCs). TX - seq_sendMidiNoteOn() 0x10, seq_sendRealtime()
            0x20, seq_sendProgChg() 0x80; 0x40 has no transmitter.
   AFFILIATES: midi_setRouting() (routing is independent of these bits). */
extern uint8_t midiParser_txRxFilter;

#endif /* MIDIPARSER_H_ */
