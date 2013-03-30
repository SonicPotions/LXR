/*
 * usb_midi_core.h
 *
 *------------------------------------------------------------------------------
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
 */

#ifndef USB_MIDI_CORE_H_
#define USB_MIDI_CORE_H_





#include "usbd_ioreq.h"
#include "usbd_req.h"
#include "usbd_desc.h"
#include "MidiMessages.h"

typedef struct UsbMidiStruct
{
	uint8_t cin;
	uint8_t status;
	uint8_t data1;
	uint8_t data2;
}MidiDataUsb;



#define TOTAL_MIDI_BUF_SIZE                            0x40 // each buffer is 64 bytes
#define NUM_SUB_BUFFERS									0x2	//double buffering


#define USB_MIDI_INPUT_BUFFER_SIZE 	256
#define USB_MIDI_INPUT_BUFFER_MASK	0xFF	/**< mask to wrap the read/wrt pointers*/
/*
extern uint8_t  usb_MidiOutBuff [TOTAL_MIDI_BUF_SIZE * 2];
extern uint8_t* usb_MidiOutWrPtr;
extern uint8_t* usb_MidiOutRdPtr;
*/
extern uint8_t  usb_MidiInBuff [TOTAL_MIDI_BUF_SIZE * NUM_SUB_BUFFERS];
extern uint8_t* usb_MidiInWrPtr;
extern uint8_t* usb_MidiInRdPtr;


//buffer for parsed midi messages
extern MidiMsg usb_MidiMessages[USB_MIDI_INPUT_BUFFER_SIZE];
extern uint8_t usb_MidiMessagesRead;
extern uint8_t usb_MidiMessagesWrite;


//USB Code Index Number Classifications
#define CIN_RESERVED 			0x00
#define CIN_CABLE_EVENT 		0x01
#define CIN_TWO_BYTE_MESSAGE	0x02	/**<Two-byte System Common messages like  MTC, SongSelect, etc.*/
#define CIN_THREE_BYTE_MESSAGE	0x03	/**<Three-byte System Common messages like SPP, etc*/
#define CIN_SYSEX_START			0x04	/**<SysEx starts or continues*/
#define CIN_SYSEX_END_1			0x05	/**<Single-byte System Common Message or SysEx ends with following single byte.*/
#define CIN_SYSEX_END_2			0x06	/**<SysEx ends with following two bytes.*/
#define CIN_SYSEX_END_3			0x07	/**<SysEx ends with following three bytes.*/
#define CIN_NOTE_OFF			0x08
#define CIN_NOTE_ON				0x09
#define CIN_POLY_KEY_PRESS		0x0A
#define CIN_CC					0x0B
#define CIN_PROG_CHANGE			0x0C
#define CIN_CHAN_PRESS			0x0D
#define CIN_PITCH_BEND			0x0E
#define CIN_SINGLE_BYTE			0x0F


/* Number of sub-packets in the audio transfer buffer. You can modify this value but always make sure
  that it is an even number and higher than 3 */
//#define OUT_PACKET_NUM                                   4
/* Total size of the midi transfer buffers */


#define AUDIO_CONFIG_DESC_SIZE                        109
#define AUDIO_INTERFACE_DESC_SIZE                     9
#define USB_AUDIO_DESC_SIZ                            0x09
#define AUDIO_STANDARD_ENDPOINT_DESC_SIZE             0x09
#define AUDIO_STREAMING_ENDPOINT_DESC_SIZE            0x07

#define AUDIO_DESCRIPTOR_TYPE                         0x21
//#define USB_DEVICE_CLASS_AUDIO                        0x01
//#define AUDIO_SUBCLASS_AUDIOCONTROL                   0x01
//#define AUDIO_SUBCLASS_AUDIOSTREAMING                 0x02
//#define AUDIO_PROTOCOL_UNDEFINED                      0x00
//#define AUDIO_STREAMING_GENERAL                       0x01
//#define AUDIO_STREAMING_FORMAT_TYPE                   0x02

/* Audio Descriptor Types */
#define AUDIO_INTERFACE_DESCRIPTOR_TYPE               0x24
#define AUDIO_ENDPOINT_DESCRIPTOR_TYPE                0x25

/* Audio Control Interface Descriptor Subtypes */
#define AUDIO_CONTROL_HEADER                          0x01
#define AUDIO_CONTROL_INPUT_TERMINAL                  0x02
#define AUDIO_CONTROL_OUTPUT_TERMINAL                 0x03
#define AUDIO_CONTROL_FEATURE_UNIT                    0x06

#define AUDIO_INPUT_TERMINAL_DESC_SIZE                0x0C
#define AUDIO_OUTPUT_TERMINAL_DESC_SIZE               0x09
#define AUDIO_STREAMING_INTERFACE_DESC_SIZE           0x07

//#define AUDIO_CONTROL_MUTE                            0x0001

//#define AUDIO_FORMAT_TYPE_I                           0x01
//#define AUDIO_FORMAT_TYPE_III                         0x03

//#define USB_ENDPOINT_TYPE_ISOCHRONOUS                 0x01
//#define AUDIO_ENDPOINT_GENERAL                        0x01

//#define AUDIO_REQ_GET_CUR                             0x81
//#define AUDIO_REQ_SET_CUR                             0x01

//#define AUDIO_OUT_STREAMING_CTRL                      0x02

/*
typedef struct _Audio_Fops
{
    uint8_t  (*Init)         (uint32_t  AudioFreq, uint32_t Volume, uint32_t options);
    uint8_t  (*DeInit)       (uint32_t options);
    uint8_t  (*AudioCmd)     (uint8_t* pbuf, uint32_t size, uint8_t cmd);
    uint8_t  (*VolumeCtl)    (uint8_t vol);
    uint8_t  (*MuteCtl)      (uint8_t cmd);
    uint8_t  (*PeriodicTC)   (uint8_t cmd);
    uint8_t  (*GetState)     (void);
}AUDIO_FOPS_TypeDef;
*/
/**
  * @}
  */


extern USBD_Class_cb_TypeDef  MIDI_cb;


#endif /* USB_MIDI_CORE_H_ */

