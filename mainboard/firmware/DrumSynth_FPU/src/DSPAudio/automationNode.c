/*
 * automationNode.c
 *
 *  Created on: 12.02.2013
 *      Author: Julian
 */
#include "automationNode.h"

//-------------------------------------------------------------
void autoNode_init(AutomationNode* node)
{
	node->destination = NO_AUTOMATION;
}
//-------------------------------------------------------------
void autoNode_setDestination(AutomationNode* node, uint16_t dest)
{
	//reset lastDest
	if(node->destination != NO_AUTOMATION)
	{
		MidiMsg msg;

		if(node->destination > 127) {
			msg.status = MIDI_CC2;
			msg.data1 = node->destination - 127 -1;
		} else {
			msg.status = MIDI_CC;
			msg.data1 = node->destination;
		}
		msg.data2 = midiParser_originalCcValues[node->destination];
		midiParser_ccHandler(msg,0);
	}

	//set new destination
	node->destination = dest;
}
//-------------------------------------------------------------
void autoNode_updateValue(AutomationNode* node, uint8_t val)
{
	if(node->destination != NO_AUTOMATION) {
		MidiMsg msg;

		if(node->destination > 127) {
			msg.status = MIDI_CC2;
			msg.data1 = node->destination - 127 -1; //todo why +1 offset?
		} else {
			msg.status = MIDI_CC;
			msg.data1 = node->destination;
		}
		msg.data2 = val;
		midiParser_ccHandler(msg,0);
	}
}
//-------------------------------------------------------------
