/*
 * MidiVoiceControl.h
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


#ifndef MIDIVOICECONTROL_H_
#define MIDIVOICECONTROL_H_

#include "stm32f4xx.h"
#include "config.h"


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
uint8_t voiceStatus[NUM_VOICES];
//---------------------------------------------------
//------------- Functions ---------------------------
void voiceControl_noteOn(uint8_t voice, uint8_t note, uint8_t vel);
void voiceControl_noteOff(uint8_t voice);//0xff == all voices
uint8_t voiceControl_isVoicePlaying(uint8_t voice);

#endif /* MIDIVOICECONTROL_H_ */
