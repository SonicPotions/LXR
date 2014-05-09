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

enum {
	USB_NOT_DETECTED,
	USB_DETECTED,
};

USB_OTG_CORE_HANDLE           USB_OTG_dev;
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
/*
 * flush FIFO and send out next block of messages stored in the usb_MidiInBuff
 * called periodically by usb_tick()
 * TODO:  maybe there is a better way to call this automatically by the USB driver when it is ready to TX some more data?
 */
void usb_flushMidi()
{
	const uint8_t len = usb_MidiInWrPtr - usb_MidiInRdPtr;
	if((len != 0) && (USB_OTG_dev.dev.device_status != USB_OTG_SUSPENDED))
	{
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
