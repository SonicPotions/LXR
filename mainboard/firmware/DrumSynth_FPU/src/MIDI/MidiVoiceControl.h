/*
 * MidiVoiceControl.h
 *
 *  Created on: 03.04.2012
 *      Author: Julian
 */

#ifndef MIDIVOICECONTROL_H_
#define MIDIVOICECONTROL_H_

#include "stm32f4xx.h"
#include "config.h"


//-------------- DEFINES ----------------------------


//Note numbers to trigger drum channels
#define NOTE_VOICE1		36
#define NOTE_VOICE2		37
#define NOTE_VOICE3		38
#define NOTE_VOICE4		39
#define NOTE_VOICE5		40
#define NOTE_VOICE6		41
#define NOTE_VOICE7		42
//-------------- ENUMS ------------------------------

//--------------- VARS ------------------------------
uint8_t voiceStatus[NUM_VOICES];
//---------------------------------------------------
//------------- Functions ---------------------------
void voiceControl_noteOn(uint8_t note, uint8_t vel);
void voiceControl_noteOff(uint8_t note, uint8_t vel);

#endif /* MIDIVOICECONTROL_H_ */
