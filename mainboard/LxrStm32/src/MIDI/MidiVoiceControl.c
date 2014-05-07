/*
 * MidiVoiceControl.c
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


#include "MidiVoiceControl.h"
#include "DrumVoice.h"
#include "Snare.h"
#include "HiHat.h"
#include "MidiMessages.h"
#include "CymbalVoice.h"
#include "sequencer.h"
#include "TriggerOut.h"
#include "Uart.h"
//#include "LCD_driver.h"

static uint8_t active_voices=0;	// which voices are currently playing a note
//----------------------------------------------------------------
// this fn assumes a valid voice is sent
void voiceControl_noteOn(uint8_t voice, uint8_t note, uint8_t vel)
{
	active_voices |= (1<<voice);

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
	} else {
		trigger_triggerVoice(voice, TRIGGER_PULSE);
	}

	// Send to front panel so it can pulse the LED
	uart_sendFrontpanelByte(NOTE_ON);
	uart_sendFrontpanelByte(voice);
	uart_sendFrontpanelByte(0);
}
//----------------------------------------------------------------
void voiceControl_noteOff(uint8_t voice)
{
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
		midiChan = midi_MidiChannels[voice];
		seq_midiNoteOff(midiChan);
	}
}
//----------------------------------------------------------------
uint8_t voiceControl_isVoicePlaying(uint8_t voice)
{
	return (active_voices & (1<<voice));
}
