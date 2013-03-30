/*
 * frontPanelParser.h
 *
 *  Created on: 27.04.2012
 *      Author: Julian
 */

#ifndef FRONTPANELPARSER_H_
#define FRONTPANELPARSER_H_

#include "MidiMessages.h"
#include "modulationNode.h"
#include "valueShaper.h"

extern  uint8_t frontParser_activeTrack;
extern uint8_t frontParser_shownPattern;
extern  uint8_t frontParser_sysexActive;

extern uint8_t frontParser_activeFrontTrack;

void frontParser_parseUartData(unsigned char data);

void frontParser_updateTrackLeds(const uint8_t trackNr, uint8_t patternNr);

#endif /* FRONTPANELPARSER_H_ */
