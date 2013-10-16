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
//#include "LCD_driver.h"

//static uint8_t c = 0;
//------------------------------------------------------
void voiceControl_noteOn(uint8_t note, uint8_t vel)
{

	uint8_t voice = note-NOTE_VOICE1;
	if(seq_isTrackMuted(voice))
	{
		return;
	}

	switch(note)
	{
	case NOTE_VOICE1:
	case NOTE_VOICE2:
	case NOTE_VOICE3:

		Drum_trigger(note-NOTE_VOICE1, vel,SEQ_DEFAULT_NOTE);
		break;

	case NOTE_VOICE4:
		Snare_trigger(vel,SEQ_DEFAULT_NOTE);
		break;
	case NOTE_VOICE5:
		Cymbal_trigger(vel,SEQ_DEFAULT_NOTE);
		break;

	case NOTE_VOICE6:
		HiHat_trigger(vel,0,SEQ_DEFAULT_NOTE);
		break;

	case NOTE_VOICE7:
		HiHat_trigger(vel,1,SEQ_DEFAULT_NOTE);
		break;
	default:

		return;
		break;
	}

	//Recording Mode
	seq_addNote(note-NOTE_VOICE1,vel);
}
//------------------------------------------------------
void voiceControl_noteOff(uint8_t note, uint8_t vel)
{

}
//------------------------------------------------------
