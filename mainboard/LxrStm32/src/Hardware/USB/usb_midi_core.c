/*
 * usb_midi_core.c
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


#include "usb_midi_core.h"
#include "MidiParser.h"

/* Includes ------------------------------------------------------------------*/

//buffer for parsed midi messages
MidiMsg usb_MidiMessages[USB_MIDI_INPUT_BUFFER_SIZE];
uint8_t usb_MidiMessagesRead = 0;
uint8_t usb_MidiMessagesWrite = 0;



/*********************************************
   AUDIO Device library callbacks
 *********************************************/
static uint8_t  usbd_midi_Init       (void  *pdev, uint8_t cfgidx);
static uint8_t  usbd_midi_DeInit     (void  *pdev, uint8_t cfgidx);
static uint8_t  usbd_midi_Setup      (void  *pdev, USB_SETUP_REQ *req);
static uint8_t  usbd_midi_EP0_RxReady(void *pdev);
static uint8_t  usbd_midi_EP0_TxSent (void  *pdev);
static uint8_t  usbd_midi_DataIn     (void *pdev, uint8_t epnum);
static uint8_t  usbd_midi_DataOut    (void *pdev, uint8_t epnum);
static uint8_t  usbd_midi_SOF        (void *pdev);
//static uint8_t  usbd_midi_OUT_Incplt (void  *pdev);

/*********************************************
   AUDIO Requests management functions
 *********************************************/
/*
static void AUDIO_Req_GetCurrent(void *pdev, USB_SETUP_REQ *req);
static void AUDIO_Req_SetCurrent(void *pdev, USB_SETUP_REQ *req);
*/
static uint8_t  *USBD_midi_GetCfgDesc (uint8_t speed, uint16_t *length);
/**
  * @}
  */

/** @defgroup usbd_midi_Private_Variables
  * @{
  */
/* Main Buffer for Audio Data Out transfers and its relative pointers */
/*
uint8_t  IsocOutBuff [TOTAL_OUT_BUF_SIZE * 2];
uint8_t* IsocOutWrPtr = IsocOutBuff;
uint8_t* IsocOutRdPtr = IsocOutBuff;
*/

//buffers for midi IO
uint8_t  usb_MidiOutBuff [TOTAL_MIDI_BUF_SIZE * NUM_SUB_BUFFERS];
uint8_t* usb_MidiOutWrPtr = usb_MidiOutBuff;
uint8_t* usb_MidiOutRdPtr = usb_MidiOutBuff;

uint8_t  usb_MidiInBuff [TOTAL_MIDI_BUF_SIZE * NUM_SUB_BUFFERS];
uint8_t* usb_MidiInWrPtr = usb_MidiInBuff;
uint8_t* usb_MidiInRdPtr = usb_MidiInBuff;


/* Main Buffer for Audio Control Rrequests transfers and its relative variables */
/*
uint8_t  AudioCtl[64];
uint8_t  AudioCtlCmd = 0;
uint32_t AudioCtlLen = 0;
uint8_t  AudioCtlUnit = 0;
*/

static uint32_t PlayFlag = 0;

static uint32_t  usbd_midi_AltSet = 0;
static uint8_t usbd_midi_CfgDesc[AUDIO_CONFIG_DESC_SIZE];
//------------------------------------------------------------------------------------------------
/* AUDIO interface class callbacks structure */
USBD_Class_cb_TypeDef  MIDI_cb =
{
	/*this callback is called when the device receives the set configuration request; in
	this function the endpoints used by the class interface are open*/
  usbd_midi_Init,
  /*This callback is called when the clear configuration request has been received;
this function closes the endpoints used by the class interface*/
  usbd_midi_DeInit,
  /*------------ Control Endpoints ------------*/
  /*This callback is called to handle the specific class setup requests*/
  usbd_midi_Setup,
  /*This callback is called when the send status is finished*/
 usbd_midi_EP0_TxSent, /* EP0_TxSent */
  /*This callback is called when the receive status is finished*/
  usbd_midi_EP0_RxReady,
  /*This callback is called to perform the data in stage relative to the non-control
endpoints.*/
  NULL,//usbd_midi_DataIn,
  /*This callback is called to perform the data out stage relative to the non-control endpoints.*/
  usbd_midi_DataOut,
  /*This callback is called when a SOF interrupt is received; this callback can be
used to synchronize some processes with the Start of frame*/
  usbd_midi_SOF,
  /*IsoINIncomplete: This callback is called when the last isochronous IN transfer is
incomplete.*/
  NULL,
  /*IsoOUTIncomplete: This callback is called when the last isochronous OUT transfer is
incomplete.*/
  NULL,//usbd_midi_OUT_Incplt,
  /*GetConfigDescriptor: This callback returns the USB Configuration descriptor*/
  USBD_midi_GetCfgDesc,
#ifdef USB_OTG_HS_CORE
  /*GetConfigDescriptor: This callback returns the USB Configuration descriptor*/
  USBD_midi_GetCfgDesc, /* use same config as per FS */
#endif
  /*GetUsrStrDescriptor: This callback returns the user defined string descriptor.*/
};
//------------------------------------------------------------------------------------------------
/* USB AUDIO device Configuration Descriptor */
static uint8_t usbd_midi_CfgDesc[AUDIO_CONFIG_DESC_SIZE] =
{
	/* Configuration 1 */
	0x09,                                 	/* bLength */
	USB_CONFIGURATION_DESCRIPTOR_TYPE,    	/* bDescriptorType */
	0x65,      	/* wTotalLength  109 bytes*/
	0x00,
	0x02,                                 	/* bNumInterfaces */
	0x01,                                 	/* bConfigurationValue */
	0x00,                                 	/* iConfiguration */
	0xC0,                                 	/* bmAttributes  BUS Powred*/
	0x00,                                 	/* bMaxPower = 100 mA*/
	/* 09 byte*/


	//--- MIDI AdapterStandard AC Interface Descriptor
	0x09,									//  / SIZE		[B.3.1]
	0x04,0x00,								// INTERFACE / Index Interface
	0x00,0x00,								// AlternateSetting / NumEndpoints
	0x01,									// Interface Class (0xFF=vendor specific) = AUDIO 0x01
	0x01,		 							// Interface Sub Class = AUDIOCONTROL = 0x01 (AudioStreaming = 0x02, MIDISTREAMING = 0x03)
	0x00,0x00,								// Interface Protocol / Interface (String)

	//--- MIDI AdapterClass-specific AC Interface Descriptor
	0x09,AUDIO_INTERFACE_DESCRIPTOR_TYPE,	// SIZE / CS_INTERFACE		[B.3.2]
	AUDIO_CONTROL_HEADER,0x00,				// HEADER / rev
	0x01,0x09,								// rev / len
	0x00,0x01,								// len / number of streaming id
	0x01,									// interface 1

	//--- MIDI AdapterStandard MS Interface Descriptor
	0x09,									// SIZE		[B.4.1]
	0x04,0x01,								// INTERFACE / index
	0x00,0x02,								// alt set / #endpoints
	0x01,0x03,								// AUDIO / MIDISTREAMING
	0x00,0x00,								// unused / unused

	//--- MIDI Adapter Class-specific MS Interface Descriptor - ok
	0x07,AUDIO_INTERFACE_DESCRIPTOR_TYPE,	// SIZE / CS_INTERFACE		[B.4.2]
	0x01,0x00,								// HEADER / rev
	0x01,0x41,								// rev / total size
	0x00,									// total size

	//--- MIDI IN Jack Descriptor -> embedded (endpoint) jack id 1
	0x06,									// SIZE		[B.4.3]
	AUDIO_INTERFACE_DESCRIPTOR_TYPE,0x02,	// CS_INTERFACE / MIDI IN JACK
	0x01,0x01,								// EMBEDDED / ID
	0x00,									// unused

	//--- MIDI IN Jack Descriptor -> external(physical) jack id 2
	0x06,									// SIZE			[B.4.3 part 2]
	AUDIO_INTERFACE_DESCRIPTOR_TYPE,0x02,	// CS_INTERFACE / MIDI IN JACK
	0x02,0x02,								// EXT / ID
	0x00,									// String

	//--- MIDI OUT Jack Descriptor -> embedded (endpoint) out jack id 3
	0x09,									// SIZE
	AUDIO_INTERFACE_DESCRIPTOR_TYPE,0x03,	// CS_INTERFACE / MIDI OUT JACK	[B.4.4]
	0x01,0x03,								// EMBEDDED / ID			//embedded midi jacks are endpoints, external are physical jacks
	0x01,0x02,								// NrInputPins / connected with ID
	0x01,0x00,								// Output Pin # / String

	//--- MIDI OUT Jack Descriptor -> external (physical) out jack id 4
	0x09,AUDIO_INTERFACE_DESCRIPTOR_TYPE,	// SIZE / CS_INTERFACE		[B.4.4 part 2]
	0x03,0x02,								// MIDI OUT JACK / EXT
	0x04,0x01,								// ID / NrInputPins
	0x01,0x01,								// connected with ID / Output Pin #
	0x00,									// unused

	//--- Endpoint Descriptor
	0x09,		// SIZE			[B.5.1]
	USB_ENDPOINT_DESCRIPTOR_TYPE,			// desc. type =  ENDPOINT (Constant 	Endpoint Descriptor (0x05))
	/*
	Endpoint Address
	Bits 0..3b Endpoint Number.
	Bits 4..6b Reserved. Set to Zero
	Bits 7 Direction 0 = Out, 1 = In (Ignored for Control Endpoints)
	*/
	MIDI_OUT_EP,							// OUT 2
	0x02,0x40,								// BULK / packetsize
	0x00,0x00,								// packetsize / ignored
	0x00,0x00,								// unused / unused


	//--- class specific endpoint descriptor
	0x05,AUDIO_ENDPOINT_DESCRIPTOR_TYPE,	// SIZE / CS_ENDPOINT		[B.5.2]
	0x01,0x01,								// MS_GENERAL / Number of embedded IN JACKS
	0x01,									// ID of embedded IN JACK


	//--- Endpoint Descriptor IN 2
	0x09,									// SIZE	[B.6.1]
	USB_ENDPOINT_DESCRIPTOR_TYPE,			// desc. type = ENDPOINT (Constant 	Endpoint Descriptor (0x05))
	MIDI_IN_EP,								// ENDPOINT / IN 2
	0x02,0x40,								// BULK / packetsize
	0x00,0x00,								// packetsize / ignored
	0x00,0x00,								// unused / unused

	//--- class specific endpoint descriptor
	0x05,AUDIO_ENDPOINT_DESCRIPTOR_TYPE,	// SIZE / CS_ENDPOINT		[B.6.2]
	0x01,0x01,								// MS_GENERAL / Number of embedded OUT JACKS
	0x03, 0									// ID of embedded OUT JACK
} ;
//------------------------------------------------------------------------------------------------
/**
* @brief  usbd_midi_Init
*         Initilaizes the MIDI interface.
* @param  pdev: device instance
* @param  cfgidx: Configuration index
* @retval status
*/
static uint8_t  usbd_midi_Init (void  *pdev,
                                 uint8_t cfgidx)
{
  /* Open EP OUT */
	/**
	* @brief  Configure an EP
	* @param pdev : Device instance
	* @param epdesc : Endpoint Descriptor
	* @retval : status
	*/
  DCD_EP_Open(pdev,
		  	  MIDI_OUT_EP,
		  	  MIDI_PACKET_SIZE,
		  	  USB_OTG_EP_BULK);

  DCD_EP_Open(pdev,
  		  	  MIDI_IN_EP,
  		  	  MIDI_PACKET_SIZE,
  		  	  USB_OTG_EP_BULK);

  /* Prepare Out endpoint to receive midi data */
  DCD_EP_PrepareRx(pdev,
		  	  	   MIDI_OUT_EP,
                   (uint8_t*)usb_MidiOutBuff,
                   MIDI_PACKET_SIZE);

  //No Hardware to init
  return USBD_OK;
}
//------------------------------------------------------------------------------------------------
/**
* @brief  usbd_midi_Init
*         DeInitializes the AUDIO layer.
* @param  pdev: device instance
* @param  cfgidx: Configuration index
* @retval status
*/
static uint8_t  usbd_midi_DeInit (void  *pdev,
                                   uint8_t cfgidx)
{
  DCD_EP_Close (pdev , MIDI_OUT_EP);
  DCD_EP_Close (pdev , MIDI_IN_EP);

  //No Hardware to de-init
  return USBD_OK;
}
//------------------------------------------------------------------------------------------------
/**
  * @brief  usbd_midi_Setup
  *         Handles the Audio control request parsing.
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  usbd_midi_Setup (void  *pdev,
                                  USB_SETUP_REQ *req)
{
  uint16_t len=USB_AUDIO_DESC_SIZ;
  uint8_t  *pbuf=usbd_midi_CfgDesc + 9;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
    /* Standard Requests -------------------------------*/
  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_DESCRIPTOR:
      if( (req->wValue >> 8) == AUDIO_DESCRIPTOR_TYPE)
      {
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
        pbuf = usbd_midi_Desc;
#else
        pbuf = usbd_midi_CfgDesc + 9;
#endif
        len = MIN(USB_AUDIO_DESC_SIZ , req->wLength);
      }

      USBD_CtlSendData (pdev,
                        pbuf,
                        len);
      break;

    case USB_REQ_GET_INTERFACE :
      USBD_CtlSendData (pdev,
                        (uint8_t *)&usbd_midi_AltSet,
                        1);
      break;

    case USB_REQ_SET_INTERFACE :
      if ((uint8_t)(req->wValue) < 0x02)
      {
        usbd_midi_AltSet = (uint8_t)(req->wValue);
      }
      else
      {
        /* Call the error management function (command will be nacked */
        USBD_CtlError (pdev, req);
      }
      break;
    }
    break;
  }
  return USBD_OK;
}

//------------------------------------------------------------------------------------------------
static uint8_t  usbd_midi_EP0_TxSent (void  *pdev)
{


  return USBD_OK;
}
//------------------------------------------------------------------------------------------------
/**
  * @brief  usbd_midi_EP0_RxReady
  *         Handles audio control requests data.
  * @param  pdev: device device instance
  * @retval status
  */
static uint8_t  usbd_midi_EP0_RxReady (void  *pdev)
{

  return USBD_OK;
}
//------------------------------------------------------------------------------------------------
/**
  * @brief  usbd_midi_DataIn
  *         Handles the MIDI IN data stage. (INto the host(pc) out of the Device)
  * @param  pdev: instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  usbd_midi_DataIn (void *pdev, uint8_t epnum)
{

	DCD_EP_Flush (pdev,epnum);
	//DCD_EP_Tx (pdev, MIDI_IN_EP, usb_MidiInBuff, 0x40);
#if 0
	if (epnum == 0x02)
	{
		const uint8_t len = usb_MidiInWrPtr-usb_MidiInRdPtr;
		DCD_EP_Tx (pdev, MIDI_IN_EP, usb_MidiInRdPtr, len);

		usb_MidiInRdPtr += TOTAL_MIDI_BUF_SIZE;
		if(usb_MidiInRdPtr >= (TOTAL_MIDI_BUF_SIZE * NUM_SUB_BUFFERS))
		{
			usb_MidiInRdPtr = usb_MidiInBuff;
		}

		usb_MidiInWrPtr=usb_MidiInRdPtr;
	}
#endif
  return USBD_OK;
}
//------------------------------------------------------------------------------------------------
/**
  * @brief  usbd_midi_DataOut
  *         Handles the Midi Out data stage (out of host and into this device).
  * @param  pdev: instance
  * @param  epnum: endpoint number
  * @retval status
  */

static uint8_t  usbd_midi_DataOut (void *pdev, uint8_t epnum)
{
  if (epnum == MIDI_OUT_EP)
  {
    /* Increment the Buffer pointer or roll it back when all buffers are full */
	  usb_MidiOutWrPtr += MIDI_PACKET_SIZE;

    if (usb_MidiOutWrPtr >= (usb_MidiOutBuff + (TOTAL_MIDI_BUF_SIZE*NUM_SUB_BUFFERS)))
    {/* All buffers are full: roll back */
      usb_MidiOutWrPtr = usb_MidiOutBuff;
    }

    /* Toggle the frame index */
    ((USB_OTG_CORE_HANDLE*)pdev)->dev.out_ep[epnum].even_odd_frame =
      (((USB_OTG_CORE_HANDLE*)pdev)->dev.out_ep[epnum].even_odd_frame)? 0:1;

    /* Prepare Out endpoint to receive next audio packet */
    DCD_EP_PrepareRx(pdev,
                     MIDI_OUT_EP,
                     (uint8_t*)(usb_MidiOutWrPtr),
                     MIDI_PACKET_SIZE);

    //process buffer
    int i;


    MidiDataUsb usbData;
    usbData.cin = 1;
    for(i=0;i< 0x40 && usbData.cin!=0x00;i+=4)
    {

    	usbData.cin = usb_MidiOutRdPtr[i];
    	usbData.status = usb_MidiOutRdPtr[i+1];
    	usbData.data1 = usb_MidiOutRdPtr[i+2];
    	usbData.data2 = usb_MidiOutRdPtr[i+3];

    	uint8_t length=0;

    	//TODO propper midi parsing
    	// we only have cable 0
    	if ((usbData.cin&0xf0) == 0)
    	{
			// get length and type of message
			switch(usbData.cin&0x0f) {
				case CIN_TWO_BYTE_MESSAGE:
				case CIN_PROG_CHANGE:
				case CIN_CHAN_PRESS:
					length = 2;
					break;

				case CIN_THREE_BYTE_MESSAGE:
				case CIN_NOTE_OFF:
				case CIN_NOTE_ON:
				case CIN_POLY_KEY_PRESS:
				case CIN_CC:
				case CIN_PITCH_BEND:
					length = 3;
					break;

				case CIN_SYSEX_START:
					// SYSEX
					length = 3;
					break;

				case CIN_SYSEX_END_1:
					// SYSEX end
					length = 1;
					break;

				case CIN_SYSEX_END_2:
					// SYSEX end
					length = 2;
					break;

				case CIN_SYSEX_END_3:
					// SYSEX end
					length = 3;
					break;

				case CIN_SINGLE_BYTE:
					length = 1;
					break;

				default:
					break;
			}
		}
		else
		{
			//not our cable
			//todo: are all messages in this chunk the same cable?
			//then we could return; here
		}
    	if(length != 0)
    	{
    		/*
    		midiParser_parseUartData(usbData.status);
    		midiParser_parseUartData(usbData.data1);
    		midiParser_parseUartData(usbData.data2);
    		*/
    		usb_MidiMessages[usb_MidiMessagesWrite].status	= usbData.status;
    		usb_MidiMessages[usb_MidiMessagesWrite].data1 	= usbData.data1;
    		usb_MidiMessages[usb_MidiMessagesWrite].data2 	= usbData.data2;
    		usb_MidiMessages[usb_MidiMessagesWrite].bits.length = length-1; // we don't count the status byte in our length
    		// --AS todo keep track of sysex mode
    		// if we are in sysex mode, this will be set, and status will contain the current sysex message byte
    		// is it possible for there to be more than one sysex message byte received?
    		usb_MidiMessages[usb_MidiMessagesWrite].bits.sysxbyte = 0;
    		usb_MidiMessages[usb_MidiMessagesWrite].bits.source = midiSourceUSB;

    		//increment and wrap write pointer
    		usb_MidiMessagesWrite++;
    		usb_MidiMessagesWrite &= USB_MIDI_INPUT_BUFFER_MASK;
    	}
 }
	usb_MidiOutRdPtr += TOTAL_MIDI_BUF_SIZE;
    /* Increment the Buffer pointer or roll it back when all buffers are full */
    if (usb_MidiOutRdPtr >= (usb_MidiOutBuff + (TOTAL_MIDI_BUF_SIZE*NUM_SUB_BUFFERS)))
    {/* All buffers are full: roll back */
    	usb_MidiOutRdPtr = usb_MidiOutBuff;
    }



  }

  return USBD_OK;
}
//------------------------------------------------------------------------------------------------
/**
  * @brief  usbd_midi_SOF
  *         Handles the SOF event (data buffer update and synchronization).
  * @param  pdev: instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  usbd_midi_SOF (void *pdev)
{
	//TODO was passiert hier?
  /* Check if there are available data in stream buffer.
    In this function, a single variable (PlayFlag) is used to avoid software delays.
    The play operation must be executed as soon as possible after the SOF detection. */
  if (PlayFlag)
  {
    /* Start playing received packet */
#if 0
    AUDIO_OUT_fops.AudioCmd((uint8_t*)(IsocOutRdPtr),  /* Samples buffer pointer */
                            AUDIO_OUT_PACKET,          /* Number of samples in Bytes */
                            AUDIO_CMD_PLAY);           /* Command to be processed */
#endif
    /* Increment the Buffer pointer or roll it back when all buffers all full */
    if (usb_MidiOutRdPtr >= (usb_MidiOutBuff + TOTAL_MIDI_BUF_SIZE*NUM_SUB_BUFFERS))
    {/* Roll back to the start of buffer */
      usb_MidiOutRdPtr = usb_MidiOutBuff;
    }
    else
    {/* Increment to the next sub-buffer */
    	usb_MidiOutRdPtr += MIDI_PACKET_SIZE;
    }

    /* If all available buffers have been consumed, stop playing */
    if (usb_MidiOutRdPtr == usb_MidiOutWrPtr)
    {
      /* Pause the audio stream */
#if 0
      AUDIO_OUT_fops.AudioCmd((uint8_t*)(IsocOutBuff),   /* Samples buffer pointer */
    		  	  	  	  	  MIDI_PACKET_SIZE,          /* Number of samples in Bytes */
                              AUDIO_CMD_PAUSE);          /* Command to be processed */
#endif
      /* Stop entering play loop */
      PlayFlag = 0;

      /* Reset buffer pointers */
      usb_MidiOutRdPtr = usb_MidiOutBuff;
      usb_MidiOutWrPtr = usb_MidiOutBuff;
    }
  }

  return USBD_OK;
}
//------------------------------------------------------------------------------------------------
/**
  * @brief  usbd_midi_OUT_Incplt
  *         Handles the iso out incomplete event.
  * @param  pdev: instance
  * @retval status
  */
/*
static uint8_t  usbd_midi_OUT_Incplt (void  *pdev)
{
  return USBD_OK;
}
*/
//------------------------------------------------------------------------------------------------
/******************************************************************************
     AUDIO Class requests management
******************************************************************************/
#if 0
/**
  * @brief  AUDIO_Req_GetCurrent
  *         Handles the GET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_Req_GetCurrent(void *pdev, USB_SETUP_REQ *req)
{
  /* Send the current mute state */
  USBD_CtlSendData (pdev,
                    AudioCtl,
                    req->wLength);
}
//------------------------------------------------------------------------------------------------
/**
  * @brief  AUDIO_Req_SetCurrent
  *         Handles the SET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_Req_SetCurrent(void *pdev, USB_SETUP_REQ *req)
{
  if (req->wLength)
  {
    /* Prepare the reception of the buffer over EP0 */
    USBD_CtlPrepareRx (pdev,
                       AudioCtl,
                       req->wLength);

    /* Set the global variables indicating current request and its length
    to the function usbd_midi_EP0_RxReady() which will process the request */
    AudioCtlCmd = AUDIO_REQ_SET_CUR;     /* Set the request value */
    AudioCtlLen = req->wLength;          /* Set the request data length */
    AudioCtlUnit = HIBYTE(req->wIndex);  /* Set the request target unit */
  }
}
#endif
//------------------------------------------------------------------------------------------------
/**
  * @brief  USBD_midi_GetCfgDesc
  *         Returns configuration descriptor.
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_midi_GetCfgDesc (uint8_t speed, uint16_t *length)
{
  *length = sizeof (usbd_midi_CfgDesc);
  return usbd_midi_CfgDesc;
}
