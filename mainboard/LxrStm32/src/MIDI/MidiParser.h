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
#include "MidiVoiceControl.h"


void midiParser_parseUartData(unsigned char data);
void midiParser_ccHandler(MidiMsg msg, uint8_t updateOriginalValue);
void midiParser_parseMidiMessage(MidiMsg msg);
float midiParser_calcDetune(uint8_t value);
// check mtc status, might stop the sequencer
void midiParser_checkMtc();

#if 0
void midiDebugSend(uint8_t b1, uint8_t b2);
#endif

// 0 - Off - nothing to nothing
// 1 - U2M - usb in to midi out
// 2 - M2M - midi in to midi out
// 3 - A2M - usb in and midi in to midi out
// 4 - M2U - midi in to usb out
// 5 - M2A - midi in to usb out and midi out

void midiParser_setRouting(uint8_t value);

// it's either tx or rx. value is a bitmap value from 0 to 15 where (lsb first):
// bit 1 - Note on/off
// bit 2 - Realtime on/off
// bit 3 - CC on/off (right now only applies to Rx)
// bit 4 - Prog chg on/off
void midiParser_setFilter(uint8_t is_tx, uint8_t value);

// a place to store all the incoming CC values
//needed to know to which value the automation node should return
extern uint8_t midiParser_originalCcValues[0xff];


extern uint8_t midi_MidiChannels[8]; // last element is global channel
extern uint8_t midi_NoteOverride[7];
//extern uint8_t midi_mode; --AS not used anymore

//enum MIDI_modeEnum
//{
//	MIDI_MODE_TRIGGER,
//	MIDI_MODE_NOTE,
//} MidiModes;

// high nibble is TX low nibble is RX. see above (midiParser_setFilter) for bitmap
extern uint8_t midiParser_txRxFilter;

#endif /* MIDIPARSER_H_ */
