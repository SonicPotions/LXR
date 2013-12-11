/*
 * TriggerOut.c
 *
 *  Created on: 06.05.2013
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



#include "TriggerOut.h"
#include "string.h"
#include "sequencer.h"

uint8_t trigger_dividerClock1 = 8;
uint8_t trigger_dividerClock2 = 8;
uint8_t trigger_dividerClock_Input = 1;

uint8_t trigger_clockCnt = 0;
uint8_t trigger_clockCnt_Input = 0;

uint32_t trigger_pulseTimes[NUM_PINS];
uint8_t trigger_pulseActive[NUM_PINS];

//--------------------------------------------------
void EXTI9_5_IRQHandler()
{
	//reset in
	if(EXTI_GetITStatus(EXTI_Line8) != RESET)
	{
		const uint8_t pinState = (GPIOA->IDR & GPIO_Pin_8);
		if(pinState)
		{
			//reset pin is high -> stop and reset sequencer
			seq_setRunning(0);
		} else {
			//reset pin is low -> start sequencer
			seq_setRunning(1);
		}

		EXTI_ClearITPendingBit(EXTI_Line8);
	}
	//clock in
	else if(EXTI_GetITStatus(EXTI_Line9) != RESET)
    {
        //Handle the interrupt
		if(trigger_clockCnt_Input % trigger_dividerClock_Input == 0)
		{
			if(seq_isRunning()!=0)
			{
				seq_setDeltaT(-1);
			}
		}
		trigger_clockCnt_Input++;

        EXTI_ClearITPendingBit(EXTI_Line9);
    }

}
//--------------------------------------------------
void trigger_setPin(uint8_t index, uint8_t isOn)
{

	switch(index)
	{
	case TRIGGER_1:
	case TRIGGER_2:
	case TRIGGER_3:
	case TRIGGER_4:
	case TRIGGER_5:
	case TRIGGER_6:
	case TRIGGER_7:
	case CLOCK_1:
		if(!isOn)
		{
			GPIOD->ODR |= (PIN_TRACK_1<<index) ;
		} else {
			GPIOD->ODR &= ~(PIN_TRACK_1<<index) ;
		}

		break;

	case CLOCK_2:
	case TRIGGER_RESET:
		if(!isOn)
		{
			GPIOA->ODR |= (PIN_CLOCK_2<<(index-CLOCK_2)) ;
		} else {
			GPIOA->ODR &= ~(PIN_CLOCK_2<<(index-CLOCK_2)) ;
		}
		break;

	default:
		break;
	}

}
//--------------------------------------------------
void trigger_tick()
{
	int i;
	for(i=0;i<NUM_PINS;i++)
	{
		if( (trigger_pulseActive[i]) && (systick_ticks > trigger_pulseTimes[i]) )
		{
			trigger_setPin(i,0);
			trigger_pulseActive[i] = 0;
		}
	}
}
//--------------------------------------------------
void trigger_pulsePin(uint8_t index)
{
	trigger_pulseTimes[index] = systick_ticks + PULSE_LENGTH;

	trigger_pulseActive[index] = 1;
	trigger_setPin(index,1);
}

//--------------------------------------------------
void trigger_init()
{

	memset(trigger_pulseTimes,0,NUM_PINS);
	memset(trigger_pulseActive,0,NUM_PINS);

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;


	//trigger output
	GPIO_InitStructure.GPIO_Pin = PIN_TRACK_1 | PIN_TRACK_2| PIN_TRACK_3 | PIN_TRACK_4 | PIN_TRACK_5 | PIN_TRACK_6 | PIN_TRACK_7 | PIN_CLOCK_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = PIN_CLOCK_2 | PIN_RESET;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//-------------- trigger input ---------------------------------------
	EXTI_InitTypeDef   EXTI_InitStructure;
	NVIC_InitTypeDef   NVIC_InitStructure;
	 /* Enable SYSCFG clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	//---- Reset In (PA8)------------------------------------------------------
	GPIO_InitStructure.GPIO_Pin = PIN_RESET_IN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //maybe floating?
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Connect EXTI Line8 to PA8 pin */
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource8);

	/* Configure EXTI Line8 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line8;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	/* Enable and set EXTI Line8 Interrupt to the lowest priority */
	/*//same irq as below
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	*/


	//---- Clock In (PC9)------------------------------------------------------
	GPIO_InitStructure.GPIO_Pin = PIN_CLOCK_IN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //maybe floating?
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* Connect EXTI Line9 to PC9 pin */
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource9);

	/* Configure EXTI Line9 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line9;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	/* Enable and set EXTI Line9 Interrupt to the lowest priority */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);


	//set trigger outs to low
	trigger_setPin(0,0);
	trigger_setPin(1,0);
	trigger_setPin(2,0);
	trigger_setPin(3,0);
	trigger_setPin(4,0);
	trigger_setPin(5,0);
	trigger_setPin(6,0);

}
//--------------------------------------------------
void trigger_triggerVoice(uint8_t voice)
{
	trigger_pulsePin(TRIGGER_1 + voice);
}
//--------------------------------------------------
void trigger_clockTick()
{
	if(trigger_clockCnt % trigger_dividerClock1 == 0)
	{
		trigger_pulsePin(CLOCK_1);
	}

	if(trigger_clockCnt % trigger_dividerClock2 == 0)
	{
		trigger_pulsePin(CLOCK_2);
	}

	trigger_clockCnt++;

}
//--------------------------------------------------
void trigger_reset(uint8_t value)
{
	trigger_setPin(TRIGGER_RESET,value);
	if(value) {
		trigger_clockCnt = 0;
	}
}
//--------------------------------------------------
