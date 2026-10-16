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

/* Shared MTC watchdog used by the sequencer and parser coordination layer. */
void midiParser_checkMtc(void);

#endif /* GLOBALMIDIPARSER_H_ */
