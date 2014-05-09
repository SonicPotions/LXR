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
void usb_sendMidi(MidiMsg msg);
uint8_t usb_getMidi(MidiMsg* msg);
void usb_flushMidi();

#endif /* USB_MANAGER_H_ */
