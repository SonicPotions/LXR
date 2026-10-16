/*
 * Uart.c
 *
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

#include "stm32f4xx.h"
#include "MidiParser.h"
#include "Uart.h"
#include "FIFO.h"
#include "frontPanelReceivingProtocol.h"
#include "config.h"

 /* USART2 MIDI configured as follow:
	 - BaudRate = 31250 baud
	 - Word Length = 8 Bits
	 - One Stop Bit
	 - No parity
	 - Hardware flow control disabled (RTS and CTS signals)
	 - Receive and transmit enabled
*/
//-----------------------------------------------------------------------------
//we need a TX and RX FIFO for every uart

static Fifo fifo_midiTx;
static Fifo fifo_midiRx;
/* Whole DIN MIDI messages dropped by uart_sendMidi() because fifo_midiTx was
   full (Session 039 P3 diagnostics; debugger-visible; wraps). */
static uint16_t uart_midiTxDropCount = 0;

#define MIDI_REALTIME_QUEUE_SIZE 16
#define MIDI_REALTIME_QUEUE_MASK (MIDI_REALTIME_QUEUE_SIZE - 1)

typedef struct
{
	uint32_t timestamp;
	uint8_t status;
} MidiRealtimeEvent;

/* DIN system-realtime SPSC queue. USART2 RX is its sole producer and the
   main-loop audio-deadline service is its sole consumer. Realtime status
   bytes may legally interrupt MIDI running status, so keeping them out of the
   ordinary byte FIFO cannot alter channel-message assembly. The queue holds
   timestamped one-byte events in arrival order; overflow is counted rather
   than discarding or coalescing a clock silently. */
static volatile MidiRealtimeEvent midiRealtimeQueue[MIDI_REALTIME_QUEUE_SIZE];
static volatile uint8_t midiRealtimeRead;
static volatile uint8_t midiRealtimeWrite;
static volatile uint16_t midiRealtimeOverflowCount;
static volatile uint32_t midiRealtimeLastDispatchLatencyCycles;
static volatile uint32_t midiRealtimeMaxDispatchLatencyCycles;

/* Capture one realtime byte without doing sequencer, parser, or TX work in
   USART interrupt context. Input is a complete MIDI realtime status byte;
   output is an ordered queue entry, or an explicit overflow count when the
   bounded queue is full. */
static void uart_queueMidiRealtime(uint8_t data)
{
	const uint8_t nextWrite = (uint8_t)((midiRealtimeWrite + 1)
	                                 & MIDI_REALTIME_QUEUE_MASK);

	if(nextWrite == midiRealtimeRead)
	{
		midiRealtimeOverflowCount++;
		return;
	}

	midiRealtimeQueue[midiRealtimeWrite].status = data;
	midiRealtimeQueue[midiRealtimeWrite].timestamp =
		midiParser_captureRealtimeTimestamp();
	midiRealtimeWrite = nextWrite;
}

static Fifo fifo_frontTx;
static FifoBig fifo_frontRx; //we use a bigger fifo here because we have lots of data coming in for the preset
						//todo test if necessary!
//-----------------------------------------------------------------------------
void uart_clearFrontFifo()
{
	/* Transport-side FIFO reset for the front-panel RX/TX queues. */
	fifo_clear(&fifo_frontTx);
	fifoBig_clear(&fifo_frontRx);

}
//-----------------------------------------------------------------------------
//the midi interrupt handler
void USART2_IRQHandler(void)
{
	/* Raw USART2 MIDI IRQ plumbing. */
	uint8_t data;
	//if Receive interrupt (rx not empty)
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		data=(uint8_t)USART_ReceiveData(USART2);

		/* Keep the USART IRQ bounded: classify and enqueue only. Clock transport
		   is prioritized here so USB/front-panel/main-loop work cannot delay its
		   capture, but sequencer state and MIDI routing remain main-loop work and
		   therefore never execute at USART interrupt priority. */
		if((data & 0xf8) == 0xf8)
			uart_queueMidiRealtime(data);
		else
			fifo_bufferIn(&fifo_midiRx,data);
	}

	if (USART_GetITStatus(USART2, USART_IT_TXE) != RESET)
	{
			if (fifo_bufferOut(&fifo_midiTx,&data) == SUCCESS)//if there is data in the output buffer to send
			{
				USART_SendData(USART2, data);
			}
			else//if buffer empty
			{
				//disable Transmit Data Register empty interrupt
				USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
			}
	}

}
//-----------------------------------------------------------------------------
//the front panel interrupt handler
void USART3_IRQHandler(void)
{
	/* Raw USART3 front-panel IRQ plumbing. */
	uint8_t data;
	//if Receive interrupt (rx not empty)
	if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
	{
		data=(uint8_t)USART_ReceiveData(USART3);

		//put the received data in the RX midi buffer
		fifoBig_bufferIn(&fifo_frontRx,data);
	}

	if (USART_GetITStatus(USART3, USART_IT_TXE) != RESET)
	{
			if (fifo_bufferOut(&fifo_frontTx,&data) == SUCCESS)//if there is data in the output buffer to send
			{
				USART_SendData(USART3, data);
			}
			else//if buffer empty
			{
				//disable Transmit Data Register empty interrupt
				USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
			}
	}

}
//-----------------------------------------------------------------------------
void uart_processMidi()
{
	/* Drain one ordinary MIDI byte into the stream parser. System-realtime
	   statuses are captured separately at USART RX, so they cannot wait behind
	   this FIFO or alter channel-message running-status assembly. */
	uint8_t data;
	if(fifo_bufferOut(&fifo_midiRx,&data))
	{
		midiParser_parseUartData(data);
	}
}

//-----------------------------------------------------------------------------
void uart_serviceMidiRealtime()
{
	/* Drain captured DIN realtime events before an audio render. Input is the
   bounded SPSC queue written by USART2 RX; output is ordered calls to the
   MIDI realtime dispatcher. This service deliberately excludes ordinary
   MIDI bytes, preventing parameter/SysEx bursts from extending the
   clock-to-render critical section. */
	MidiRealtimeEvent event;

	while(midiRealtimeRead != midiRealtimeWrite)
	{
		event.status = midiRealtimeQueue[midiRealtimeRead].status;
		event.timestamp = midiRealtimeQueue[midiRealtimeRead].timestamp;
		midiRealtimeRead = (uint8_t)((midiRealtimeRead + 1)
		                           & MIDI_REALTIME_QUEUE_MASK);
		midiRealtimeLastDispatchLatencyCycles =
			midiParser_captureRealtimeTimestamp() - event.timestamp;
		if(midiRealtimeLastDispatchLatencyCycles
		   > midiRealtimeMaxDispatchLatencyCycles)
		{
			midiRealtimeMaxDispatchLatencyCycles =
				midiRealtimeLastDispatchLatencyCycles;
		}
		midiParser_handleDinRealtime(event.status);
	}
}

void uart_sendMidi(MidiMsg msg)
{
	/* Serialize a MIDI message onto the raw MIDI transport FIFO. */
	/* WHOLE-MESSAGE-OR-DROP  (Session 039 P3, user decision)
	   WHAT: queues status + msg.bits.length data bytes only if all of them
	         fit; otherwise drops the whole message and counts it.
	   WHY:  fifo_bufferIn() drops single bytes when full, which could put a
	         partial message on the DIN wire (e.g. a status without its data,
	         or an NRPN triplet missing CC6), corrupting the receiver's
	         running status. Applies to every DIN output: sequencer notes,
	         clock, routing and menu CC/NRPN TX.
	   INPUT:  msg (status, data1, data2, bits.length 0..2).
	   OUTPUT: 1-3 bytes queued and TXE enabled, or uart_midiTxDropCount++.
	   ACCESSORS: outputControl_sendMidi(), midiParser_parseMidiMessage()
	              routing, midiParser_handleDinRealtime() routing (all main
	              loop; single producer).
	   AFFILIATES: fifo_freeSpace(), uart_sendMidiByte(), USART2 TXE ISR. */
	if(fifo_freeSpace(&fifo_midiTx) < (uint8_t)(1u + msg.bits.length))
	{
		uart_midiTxDropCount++;
		return;
	}
	uart_sendMidiByte(msg.status);
	if(msg.bits.length) {
		uart_sendMidiByte(msg.data1);
		if(msg.bits.length > 1)
			uart_sendMidiByte(msg.data2);
	}

}
//-----------------------------------------------------------------------------
void uart_sendMidiByte(uint8_t data)
{
	/* Queue a raw MIDI byte and enable TX. */
	//put data in the output fifo
	fifo_bufferIn(&fifo_midiTx,data);

	//enable Transmit Data Register empty interrupt
	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
};
//-----------------------------------------------------------------------------
/* DIN MIDI OUT FREE SPACE (Session 039 P3); see Uart.h. */
uint8_t uart_midiTxFree(void)
{
	return fifo_freeSpace(&fifo_midiTx);
}
//-----------------------------------------------------------------------------
void uart_processFront()
{
	/* Drain the front-panel RX FIFO into the front-panel parser. */
#if UART_DEBUG_ECHO_MODE
	uint8_t data;
	if(fifoBig_bufferOut(&fifo_frontRx,&data))
	{
		//echo back received data
		uart_sendFrontpanelByte(data);
	}
#else
	uint8_t data;
   while(fifoBig_bufferOut(&fifo_frontRx,&data))
	{
		frontParser_parseUartData(data);
	}
#endif
}
//-----------------------------------------------------------------------------
void uart_sendFrontpanelByte(uint8_t data)
{
	/* Lowest-level front-panel command byte writer. */
	//do not send anything besides sysex data while sysex mode is active!
	if((frontParser_sysexActive == 0) && !frontParser_isQuietUi())
	{
		//put data in the output fifo
		fifo_bufferIn(&fifo_frontTx,data);

		//enable Transmit Data Register empty interrupt
		USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
	}
};
//-----------------------------------------------------------------------------
void uart_sendFrontpanelPriorityByte(uint8_t data)
{
	/* Priority front-panel byte writer used by flow-control and restore paths. */
	//put data in the output fifo
	fifo_bufferIn(&fifo_frontTx,data);

	//enable Transmit Data Register empty interrupt
	USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
}
//-----------------------------------------------------------------------------
void uart_sendFrontpanelPriorityByteWait(uint8_t data)
{
	/* Blocking priority front-panel byte writer. */
	while(!fifo_bufferIn(&fifo_frontTx,data))
	{
		USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
	}

	USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
}
//-----------------------------------------------------------------------------
void uart_sendFrontpanelSysExByte(uint8_t data)
{
	/* Front-panel SysEx byte writer that bypasses the normal quiet-UI gate. */
	//put data in the output fifo
	fifo_bufferIn(&fifo_frontTx,data);

	//enable Transmit Data Register empty interrupt
	USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
}
//-----------------------------------------------------------------------------
void initMidiUart()
{
	/* Configure the MIDI UART transport and FIFOs. */

	//init the fifo
	fifo_init(&fifo_midiTx);
	fifo_init(&fifo_midiRx);
	/* Reset both SPSC queue indices before USART2 RX is enabled. The overflow
	   counter is diagnostic-only and does not alter transport behaviour. */
	midiRealtimeRead = 0;
	midiRealtimeWrite = 0;
	midiRealtimeOverflowCount = 0;
	midiRealtimeLastDispatchLatencyCycles = 0;
	midiRealtimeMaxDispatchLatencyCycles = 0;
	/*
	 * UART2, APB1
	 *
	 */

	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_ClockInitTypeDef USART_ClockInitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); // Enable GPIOA on APB1 peripheral clock
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); // Enable USART2 Clock

    //Set USART2 Rx (PA.03) as AF push-pull
    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF ;
    GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_DOWN;

    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //TX pin
    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF ;
    GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //configure AF
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource2 ,GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource3 ,GPIO_AF_USART2);

    //uart clock
    USART_ClockStructInit(&USART_ClockInitStructure);
    USART_ClockInit(USART2, &USART_ClockInitStructure);

    //init the uart
	USART_InitStructure.USART_BaudRate = 31250;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART2, &USART_InitStructure); // Now do the setup

	//Enable USART2
	USART_Cmd(USART2, ENABLE);

	//Enable inmterrupt
	//configure NVIC
	NVIC_InitTypeDef NVIC_InitStructure;
	//select NVIC channel to configure
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	/* MIDI capture outranks USB but remains below a pending audio DMA IRQ.
	   The IRQ only timestamps and queues, so this cannot become DSP work. */
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = MIDI_UART_IRQ_PREPRIO;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = MIDI_UART_IRQ_SUBRIO;
	//enable IRQ channel
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	//update NVIC registers
	NVIC_Init(&NVIC_InitStructure);
	//disable Transmit Data Register empty interrupt
	USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
	//enable Receive Data register not empty interrupt
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
   
   // kick the midi parser
   midiParser_parseUartData(0xB0);
}
//-------------------------------------------------------------------------------------------

void initFrontpanelUart()
{
	/* Configure the front-panel UART transport and FIFOs. */
	//fifo init
	fifo_init(&fifo_frontTx);
	fifoBig_init(&fifo_frontRx);
	/*
	 * UART3, APB1
	 * (PB10 TX, PB11 RX)
	 *
	 */

	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_ClockInitTypeDef USART_ClockInitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); // Enable GPIOD on APB1 peripheral clock
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE); // Enable USART3 Clock

    //Set USART3 Rx (PB.11) as AF push-pull
    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF ;
    GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_DOWN;

    GPIO_Init(GPIOB, &GPIO_InitStructure);

    //TX pin
    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF ;
    GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    //configure AF
    GPIO_PinAFConfig(GPIOB,GPIO_PinSource11 ,GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_USART3);

    //uart clock
    USART_ClockStructInit(&USART_ClockInitStructure);
    USART_ClockInit(USART3, &USART_ClockInitStructure);

    //init the uart
	//USART_InitStructure.USART_BaudRate = 38400UL;
    //USART_InitStructure.USART_BaudRate = 1000000UL;
    USART_InitStructure.USART_BaudRate = 500000UL;

	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART3, &USART_InitStructure); // Now do the setup

	//Enable USART3
	USART_Cmd(USART3, ENABLE);

	//Enable inmterrupt
	//configure NVIC
	NVIC_InitTypeDef NVIC_InitStructure;
	//select NVIC channel to configure
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	//set priority to lowest
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	//set subpriority to lowest
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	//enable IRQ channel
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	//update NVIC registers
	NVIC_Init(&NVIC_InitStructure);
	//disable Transmit Data Register empty interrupt
	USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
	//enable Receive Data register not empty interrupt
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
}
