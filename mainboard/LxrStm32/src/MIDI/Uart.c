/*
 * MidiUart.c
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
#include "frontPanelParser.h"
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

static Fifo fifo_frontTx;
static FifoBig fifo_frontRx; //we use a bigger fifo here because we have lots of data coming in for the preset
						//todo test if necessary!
//-----------------------------------------------------------------------------
void uart_clearFrontFifo()
{
	fifo_clear(&fifo_frontTx);
	fifoBig_clear(&fifo_frontRx);

}
//-----------------------------------------------------------------------------
//the midi interrupt handler
void USART2_IRQHandler(void)
{
	uint8_t data;
	//if Receive interrupt (rx not empty)
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		data=(uint8_t)USART_ReceiveData(USART2);

		//put the received data in the RX midi buffer
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
	uint8_t data;
	if(fifo_bufferOut(&fifo_midiRx,&data))
	{
		midiParser_parseUartData(data);
	}
}

void uart_sendMidi(MidiMsg msg)
{
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
	//put data in the output fifo
	fifo_bufferIn(&fifo_midiTx,data);

	//enable Transmit Data Register empty interrupt
	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
};
//-----------------------------------------------------------------------------
void uart_processFront()
{
#if UART_DEBUG_ECHO_MODE
	uint8_t data;
	if(fifoBig_bufferOut(&fifo_frontRx,&data))
	{
		//echo back received data
		uart_sendFrontpanelByte(data);
	}
#else
	uint8_t data;
	if(fifoBig_bufferOut(&fifo_frontRx,&data))
	{
		frontParser_parseUartData(data);
	}
#endif
}
//-----------------------------------------------------------------------------
void uart_sendFrontpanelByte(uint8_t data)
{
	//do not send anything besides sysex data while sysex mode is active!
	if(frontParser_sysexActive == 0)
	{
		//put data in the output fifo
		fifo_bufferIn(&fifo_frontTx,data);

		//enable Transmit Data Register empty interrupt
		USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
	}
};
//-----------------------------------------------------------------------------
void uart_sendFrontpanelSysExByte(uint8_t data)
{
	//put data in the output fifo
	fifo_bufferIn(&fifo_frontTx,data);

	//enable Transmit Data Register empty interrupt
	USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
}
//-----------------------------------------------------------------------------
void initMidiUart()
{

	//init the fifo
	fifo_init(&fifo_midiTx);
	fifo_init(&fifo_midiRx);
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
	//set priority to lowest
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	//set subpriority to lowest
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	//enable IRQ channel
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	//update NVIC registers
	NVIC_Init(&NVIC_InitStructure);
	//disable Transmit Data Register empty interrupt
	USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
	//enable Receive Data register not empty interrupt
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
}
//-------------------------------------------------------------------------------------------

void initFrontpanelUart()
{
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
