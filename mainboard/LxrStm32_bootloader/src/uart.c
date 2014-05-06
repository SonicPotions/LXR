/*
 * uart.c
 *
 *  Created on: 11.07.2012
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


#include "uart.h"
#include "BootloaderCommands.h"


Fifo fifo_Rx;

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
		fifo_bufferIn(&fifo_Rx,data);
	}
}

//--------------------------------------------------
uint8_t uart_waitForInitCommand()
{
	const int16_t data = uart_rxTimeout(INIT_TIMEOUT);

	//If timeout or wrong data is received
	if(data != INIT_BOOTLOADER)
	{
		//abort bootloader
		return START_PROGRAM;
	}
	//INIT_BOOTLOADER message received
	//start upload
	fifo_clear(&fifo_Rx);


	return START_BOOTLOADER;

}
//--------------------------------------------------
uint8_t uart_rx()
{
	uint8_t data;
	//wait until data is available in the fifo
	while(!(fifo_bufferOut(&fifo_Rx,&data)))
	{

	}

	return data;


}
//--------------------------------------------------
int16_t uart_rxTimeout(uint16_t timeout)
{
	uint32_t timeoutValue = systick_ticks + timeout;
	//wait until USART_FLAG_RXNE flag is set (=> new data received)
	uint8_t data;
	while(!(fifo_bufferOut(&fifo_Rx,&data)))
	{
		if(systick_ticks>=timeoutValue)
		{
			//timeout occured
			return TIMEOUT;
		}
	}
	return data;
}
//--------------------------------------------------
void uart_tx(uint8_t data)
{
	while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET)
	{

	}
	USART_SendData(USART3, data);
}
//--------------------------------------------------
void uart_deinit()
{
	USART_InitTypeDef USART_InitStructure;
		GPIO_InitTypeDef GPIO_InitStructure;
		USART_ClockInitTypeDef USART_ClockInitStructure;
    //uart clock
    /*
    USART_ClockStructInit(&USART_ClockInitStructure);
    USART_ClockInit(USART3, &USART_ClockInitStructure);
    */
    USART_ClockStructInit(&USART_ClockInitStructure);
    USART_ClockInit(USART3, &USART_ClockInitStructure);


	USART_Cmd(USART3, DISABLE);



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
		USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
}
//--------------------------------------------------

void uart_init()
{
	/*
	 * UART3, APB1
	 * (PD8 TX, PD9 RX)
	 *
	 */

	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_ClockInitTypeDef USART_ClockInitStructure;

	/*
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE); // Enable GPIOD on APB1 peripheral clock
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE); // Enable USART3 Clock
    */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); // Enable GPIOD on APB1 peripheral clock
	    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE); // Enable USART3 Clock

/*
    //Set USART3 Rx (PD.09) as AF push-pull
    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF ;
    GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;


    GPIO_Init(GPIOD, &GPIO_InitStructure);
    */
    //Set USART2 Rx (PA.03) as AF push-pull
    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF ;
    GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    //GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_DOWN;


    GPIO_Init(GPIOB, &GPIO_InitStructure);

    //TX pin
    /*
    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF ;
    GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    */
    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF ;
    GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    //configure AF
    /*
    GPIO_PinAFConfig(GPIOD,GPIO_PinSource8 ,GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOD,GPIO_PinSource9,GPIO_AF_USART3);
    */
    GPIO_PinAFConfig(GPIOB,GPIO_PinSource11 ,GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_USART3);

    //uart clock
    /*
    USART_ClockStructInit(&USART_ClockInitStructure);
    USART_ClockInit(USART3, &USART_ClockInitStructure);
    */
    USART_ClockStructInit(&USART_ClockInitStructure);
    USART_ClockInit(USART3, &USART_ClockInitStructure);

    //init the uart
    USART_InitStructure.USART_BaudRate = 500000UL;

	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART3, &USART_InitStructure); // Now do the setup

	//Enable USART3

	USART_Cmd(USART3, ENABLE);


	//Enable interrupt

	//configure NVIC
	/*
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
	*/
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
//--------------------------------------------------
