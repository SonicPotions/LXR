/*
 * automationNode.c
 *
 *  Created on: 12.02.2013
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
