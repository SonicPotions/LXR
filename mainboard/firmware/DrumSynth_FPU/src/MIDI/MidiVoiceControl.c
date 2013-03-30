/*
 * MidiVoiceControl.c
 *
 *  Created on: 03.04.2012
 *      Author: Julian
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
	//uint8_t voiceNr;
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
	//	lcdFifo_bufferIn(c++);
		Cymbal_trigger(vel,SEQ_DEFAULT_NOTE);
		break;

	case NOTE_VOICE6:
	//	lcdFifo_bufferIn(c++);
		HiHat_trigger(vel,0,SEQ_DEFAULT_NOTE);
		break;

	case NOTE_VOICE7:
		HiHat_trigger(vel,1,SEQ_DEFAULT_NOTE);
		break;
	default:

		return;
		break;
	}
/*
	uart_sendFrontpanelByte(FRONT_STEP_LED_STATUS_BYTE);
	uart_sendFrontpanelByte(FRONT_LED_TRIGGER_VOICE);
	uart_sendFrontpanelByte(note-NOTE_VOICE1);
	*/

	//Recording Mode
	seq_addNote(note-NOTE_VOICE1,vel);


}
//------------------------------------------------------
void voiceControl_noteOff(uint8_t note, uint8_t vel)
{

}
//------------------------------------------------------
