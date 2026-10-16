/*
 * usb_manager.c
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
 *      Author: Julian Schmidt
 */


#include "usb_manager.h"
#include <string.h>
#include "globals.h"	/* systick_ticks (Session 039 P3 USB timeout) */

enum {
	USB_NOT_DETECTED,
	USB_DETECTED,
};

USB_OTG_CORE_HANDLE           USB_OTG_dev;

/* USB MIDI OUT DIAGNOSTICS AND TIMING  (Session 039 P3)
   usb_midiTxStartTick: systick_ticks when the current IN transfer started;
     read only by usb_flushMidi() for the 5 ms timeout.
   usb_midiTxDropCount: whole USB-MIDI events dropped because the current
     ping-pong half was full (debugger-visible; wraps).
   usb_midiTxTimeoutCount: IN transfers abandoned after the busy timeout. */
static uint32_t usb_midiTxStartTick = 0;
static uint16_t usb_midiTxDropCount = 0;
static uint16_t usb_midiTxTimeoutCount = 0;
//-------------------------------------------------------------------------------
void usb_init()
{
	/*
	//init usb detect pin
	  GPIO_InitTypeDef  GPIO_InitStructure;
	  // GPIOD Periph clock enable
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	  // Configure PD12, PD13, PD14 and PD15 in output push-pull mode
	  GPIO_InitStructure.GPIO_Pin = USB_DETECT_PIN;
	  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;

	  // standard output pin
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	  GPIO_Init(USB_DETECT_PORT, &GPIO_InitStructure);
	  */

	  usb_start();
}
//-------------------------------------------------------------------------------
void usb_stop()
{
	//stop usb port
	USBD_DeInit(&USB_OTG_dev);
}
//-------------------------------------------------------------------------------
void usb_tick()
{
	usb_flushMidi();
}
//-------------------------------------------------------------------------------
void usb_start()
{
	//zero init the usb receive buffer
	memset(usb_MidiMessages,0, sizeof(MidiMsg)*USB_MIDI_INPUT_BUFFER_SIZE);

	//start usb port
	 USBD_Init(&USB_OTG_dev,
	#ifdef USE_USB_OTG_HS
	            USB_OTG_HS_CORE_ID,
	#else
	            USB_OTG_FS_CORE_ID,
	#endif
	            &USR_desc,
	            &MIDI_cb,
	            &USR_cb);
}
//-------------------------------------------------------------------------------
/*
 * writes a byte to the Usb Tx buffer
 * Will be send out by the usb_flushMidi() call
 */
void usb_sendByte(uint8_t byte)
{
	*usb_MidiInWrPtr++ = byte;
	if(usb_MidiInWrPtr >= (usb_MidiInBuff+(TOTAL_MIDI_BUF_SIZE * NUM_SUB_BUFFERS)))
	{
		usb_MidiInWrPtr = usb_MidiInBuff;
	}
}
//------------------------------------------------------------------------------
/* USB MIDI OUT FREE SPACE (Session 039 P3); see usb_manager.h. The write
   pointer always stays within [RdPtr, RdPtr + TOTAL_MIDI_BUF_SIZE] because
   usb_sendMidi() checks this before writing. */
uint8_t usb_midiTxFree(void)
{
	const uint8_t used = (uint8_t)(usb_MidiInWrPtr - usb_MidiInRdPtr);
	return (used >= TOTAL_MIDI_BUF_SIZE) ? 0u : (uint8_t)(TOTAL_MIDI_BUF_SIZE - used);
}
//------------------------------------------------------------------------------
/*
 * flush FIFO and send out next block of messages stored in the usb_MidiInBuff
 * called periodically by usb_tick()
 * TODO:  maybe there is a better way to call this automatically by the USB driver when it is ready to TX some more data?
 */
/*
 * flush FIFO and send out next block of messages stored in the usb_MidiInBuff
 * called periodically by usb_tick()
 * Session 039 P3:
 *   WHAT: starts a DCD_EP_Tx() of the filled ping-pong half only when the
 *         previous transfer has completed (usb_midiTxBusy cleared by
 *         usbd_midi_DataIn()), or after USB_MIDI_TX_TIMEOUT_TICKS (5 ms).
 *         When the device is not USB_OTG_CONFIGURED, pending bytes are
 *         discarded.
 *   WHY:  the old code flushed and restarted the endpoint on every main-loop
 *         pass, aborting data still in flight. With no host reading, it kept
 *         accumulating into the unbounded ring.
 *   INPUT:  usb_MidiInWrPtr/RdPtr, usb_midiTxBusy, systick_ticks,
 *           USB_OTG_dev.dev.device_status.
 *   OUTPUT: one IN transfer started and halves swapped; or nothing; or the
 *           pending half discarded.
 *   ACCESSORS: usb_tick() (main loop).
 *   AFFILIATES: usb_sendMidi(), usb_midiTxFree(), usbd_midi_DataIn().
 */
void usb_flushMidi()
{
	if(USB_OTG_dev.dev.device_status != USB_OTG_CONFIGURED)
	{
		/* No host is reading: drop the backlog so the ring cannot fill. */
		usb_MidiInWrPtr = usb_MidiInRdPtr;
		usb_midiTxBusy = 0;
		return;
	}

	if(usb_midiTxBusy)
	{
		if((uint32_t)(systick_ticks - usb_midiTxStartTick) < USB_MIDI_TX_TIMEOUT_TICKS)
			return;
		/* Host stopped polling: abandon the in-flight half and continue. */
		usb_midiTxBusy = 0;
		usb_midiTxTimeoutCount++;
	}

	const uint8_t len = (uint8_t)(usb_MidiInWrPtr - usb_MidiInRdPtr);
	if(len != 0)
	{
		usb_midiTxStartTick = systick_ticks;
		usb_midiTxBusy = 1;	/* set before Tx: DataIn may fire immediately */
		DCD_EP_Flush (&USB_OTG_dev,MIDI_IN_EP);
		DCD_EP_Tx (&USB_OTG_dev, MIDI_IN_EP, usb_MidiInRdPtr, len);

		usb_MidiInRdPtr += TOTAL_MIDI_BUF_SIZE;
		if(usb_MidiInRdPtr >= (usb_MidiInBuff+(TOTAL_MIDI_BUF_SIZE * NUM_SUB_BUFFERS)))
		{
			usb_MidiInRdPtr = usb_MidiInBuff;
		}
		usb_MidiInWrPtr=usb_MidiInRdPtr;
	}
}
//------------------------------------------------------------------------------
void usb_sendMidi(MidiMsg msg)
{
	/* WHOLE-EVENT BOUNDS CHECK  (Session 039 P3)
	   WHAT: drops the complete 4-byte event (and counts it) when it does not
	         fit in the ping-pong half being filled.
	   WHY:  usb_sendByte() has no bounds check: more than 64 bytes between
	         flushes ran into the half being transmitted, and more than 128
	         wrapped, corrupting usb_flushMidi()'s length. Whole-event drop
	         never produces a partial event.
	   INPUT:  msg; current half fill (usb_midiTxFree()).
	   OUTPUT: 4 bytes queued, or usb_midiTxDropCount++.
	   ACCESSORS: outputControl_sendMidi(), midiParser_parseMidiMessage()
	              routing, midiParser_handleDinRealtime() routing (main loop).
	   AFFILIATES: usb_flushMidi(), usb_midiTxFree(). */
	if(usb_midiTxFree() < 4u)
	{
		usb_midiTxDropCount++;
		return;
	}

	//MIDI byte[0] = 4 bits cable number + 4 bits event type code
	usb_sendByte(msg.status>>4);

	//then the 3 midi message bytes
	//USB MIDI msg has to be ALWAYS 4 bytes long
	//even if it is just a single clock byte
	usb_sendByte(msg.status);
	usb_sendByte(msg.data1);
	usb_sendByte(msg.data2);
}
//-------------------------------------------------------------------------------
uint8_t usb_getMidi(MidiMsg* msg)
{
	if(usb_MidiMessagesRead != usb_MidiMessagesWrite)
	{
		//we have unprocessed messages in the queue
		*msg = usb_MidiMessages[usb_MidiMessagesRead];

		usb_MidiMessagesRead++;
		usb_MidiMessagesRead &= USB_MIDI_INPUT_BUFFER_MASK;

		return 1; //return OK
	}
return 0; //no messages
}
