/*
 * MidiUart.h
 *
 *
 * ------------------------------------------------------------------------------
 *
 *  This file is part of the "Cortex Hardware Audio Operating System (CHAOS)".
 *
 *	Copyright 2012 Julian Schmidt / Sonic Potions
 *	http://www.sonic-potions.com
 *
 *  ------------------------------------------------------------------------------
 *
 *	CHAOS is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	CHAOS is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with CHAOS.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	------------------------------------------------------------------------------
 *
 *
 * handles the uart init and communication
 * usart2 is midi IO
 * usart3 is front panel communication (PD8 TX, PD9 RX)
 *
 *  Created on: 13.04.2012
 *      Author: Julian
 */

#ifndef MIDIUART_H_
#define MIDIUART_H_


void initMidiUart();
/*
uint8_t Usart2Get(void);
*/

void uart_sendMidiByte(uint8_t data);
//check if there is some received data in the Rx buffer and send it to the midi parser
void uart_processMidi();

void initFrontpanelUart();
//check if there is data in the front panel Rx buffer and send it to the front panel parser
void uart_processFront();
//sends a byte to the frontpanel
void uart_sendFrontpanelByte(uint8_t data);
//send sysex data to the frontpanel
//unlike the sendFrontPanelByte function, this function is allowed to send data whilke sysex mode is active
void uart_sendFrontpanelSysExByte(uint8_t data);

void uart_clearFrontFifo();
#endif /* MIDIUART_H_ */
