/*
 * GlobalMidiParser.h
 *
 * MIDI-wide system message helpers split out of MidiParser.c.
 */

#ifndef GLOBALMIDIPARSER_H_
#define GLOBALMIDIPARSER_H_

#include <stdint.h>

#include "MidiMessages.h"

/* Global MIDI system ownership. This module handles clock/MTC start-stop
   behavior that is shared across all voices. */
uint8_t globalMidiParser_handleSystemMessage(MidiMsg msg);

/* Global-channel CC router. The global channel owns the bank/morph/reset
   ladder and its sequencer side effects. */
void globalMidiParser_MIDIccHandler(MidiMsg msg, uint8_t updateOriginalValue);

/* GLOBAL CC38 SWAP (¢ RX-FILTER MODE) SETTER  (Session 039)
   WHAT: enables or disables the ¢ behaviour: while enabled, Global-channel
         CC38 is consumed as an NRPN data-entry LSB and NRPN 96 controls
         Drum 1 filter cutoff (PAR_FILTER_FREQ_1); while disabled, CC38 keeps
         its published Drum 1 cutoff mapping and NRPN 96 is ignored.
   WHY:  Elektron senders append CC38 to every NRPN, which otherwise corrupts
         Drum 1 cutoff (and the stored kit) on every NRPN send. Opt-in keeps
         the published Global CC38 mapping for everyone else.
   INPUT:  enabled - nonzero to enable.
   OUTPUT: internal GlobalMidiParser.c state only.
   ACCESSORS: midi_setFilter() RX branch (MidiParser.c).
   AFFILIATES: MIDI_RX_FILTER_CC38_SWAP (MidiParser.h),
               globalMidiParser_MIDIccHandler(), AVR RxFilter ¢ entries. */
void globalMidiParser_setCc38Swap(uint8_t enabled);

/* GLOBAL SPECIAL NRPN NUMBERS  (Session 038 / 039; moved here in P3)
   WHAT: NRPN numbers outside globalMidiParser_nrpnToRawParam[] (0..92):
         93 roll rate (0..15), 94 roll note, 95 roll velocity (PERF),
         96 Drum 1 filter cutoff (¢ RX-filter mode only; replaces CC38).
   WHY:  shared by receive (globalMidiParser_applySpecialNrpn()) and menu TX
         (outputControl_serviceMenuTx(), globalMidiParser_getTxAddress()) so
         both directions use one definition.
   AFFILIATES: MIDI_TABLE.md, manual/latex/midi_tables.tex. */
#define GLOBAL_MIDI_NRPN_ROLL_RATE      93
#define GLOBAL_MIDI_NRPN_ROLL_NOTE      94
#define GLOBAL_MIDI_NRPN_ROLL_VELOCITY  95
#define GLOBAL_MIDI_NRPN_FILTER_FREQ_1  96

/* GLOBAL TX ADDRESS KINDS  (Session 039 P3)
   Return values of globalMidiParser_getTxAddress(). */
#define GLOBAL_MIDI_TX_NONE  0
#define GLOBAL_MIDI_TX_CC    1
#define GLOBAL_MIDI_TX_NRPN  2

/* GLOBAL TX ADDRESS LOOKUP  (Session 039 P3)
   WHAT: returns how a raw sound parameter is addressed on the Global MIDI
         channel for transmit: CC number, NRPN number, or none. It is the
         inverse of the receive tables globalMidiParser_ccToRawParam[] and
         globalMidiParser_nrpnToRawParam[]; the CC wins if both map a
         parameter. ¢ mirror: PAR_FILTER_FREQ_1 returns NRPN 96 while the RX
         filter is a ¢ entry, otherwise CC38.
   WHY:  menu TX must reproduce exactly what receive understands, so a DAW
         recording of the LXR's output plays back into the LXR. Deriving the
         inverse from the receive tables means the directions cannot diverge.
   EXCLUDED (returns NONE): parameters with no Global mapping (envelope
         position 1-6, per-voice morph, kit version, reserved) and the target
         selectors PAR_VEL_DEST_1..6, PAR_VOICE_LFO1..6, PAR_TARGET_LFO1..6
         (user decision: their receive value domain is unverified).
   INPUT:  rawParam - AVR/Preset raw id; number - out: CC or NRPN number.
   OUTPUT: GLOBAL_MIDI_TX_NONE / _CC / _NRPN.
   ACCESSORS: outputControl_queueMenuParameterTx(),
              outputControl_serviceMenuTx() (MidiOutputControl.c).
   AFFILIATES: globalMidiParser_setCc38Swap(), receive tables in
               GlobalMidiParser.c, MIDI_TABLE.md. */
uint8_t globalMidiParser_getTxAddress(uint16_t rawParam, uint8_t *number);

/* Shared MTC watchdog used by the sequencer and parser coordination layer. */
void midiParser_checkMtc(void);

#endif /* GLOBALMIDIPARSER_H_ */
