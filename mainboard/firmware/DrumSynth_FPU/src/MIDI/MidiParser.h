/*
 * MidiParser.h
 *
 *  Created on: 13.04.2012
 *      Author: Julian
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

// a place to store all the incoming CC values
//needed to know to which value the automation node should return
extern uint8_t midiParser_originalCcValues[0xff];

extern uint8_t midi_globalMidiChannel;

#endif /* MIDIPARSER_H_ */
