/*
 * usb_manager.h
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
 *  Created on: 26.10.2012
 *      Author: Julian
 */

#ifndef USB_MANAGER_H_
#define USB_MANAGER_H_

#include "stm32f4xx.h"
#include "usbd_conf.h"
#include "usbd_usr.h"
#include "usb_midi_core.h"
#include "MidiMessages.h"

#define USB_DETECT_PIN GPIO_Pin_2
#define USB_DETECT_PORT GPIOB

void usb_init();
void usb_stop();
void usb_start();
void usb_tick();
/* Queue one 4-byte USB-MIDI event into the current ping-pong half. Session 039
   P3: whole-event-or-drop; never writes past the current half. */
void usb_sendMidi(MidiMsg msg);
uint8_t usb_getMidi(MidiMsg* msg);
/* Drain timestamped USB system-realtime events before an audio render. USB
   packet receive is the producer and the main loop is the consumer; ordinary
   USB MIDI remains available through usb_getMidi(). */
void usb_serviceMidiRealtime(void);
void usb_flushMidi();

/* USB MIDI OUT FREE SPACE  (Session 039 P3)
   WHAT: bytes still free in the ping-pong half currently being filled
         (0..TOTAL_MIDI_BUF_SIZE). One USB-MIDI event is 4 bytes.
   WHY:  callers that pace output (outputControl_serviceMenuTx()) only emit
         whole messages that fit; usb_sendMidi() uses it as its bounds check.
   OUTPUT: free byte count.
   ACCESSORS: usb_sendMidi(), outputControl_serviceMenuTx().
   AFFILIATES: usb_flushMidi(), usb_MidiInWrPtr/RdPtr. */
uint8_t usb_midiTxFree(void);

/* USB MIDI busy timeout: 20 SysTicks x 250 us = 5 ms (Session 039 P3,
   user decision). After this, usb_flushMidi() assumes the host stopped
   polling, abandons the in-flight half and continues. */
#define USB_MIDI_TX_TIMEOUT_TICKS 20u

#endif /* USB_MANAGER_H_ */
