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
#include "limits.h"
#include "config.h"
#include "../DSPAudio/medianFilter.h"

uint8_t trigger_dividerClockOut1 = PRE_4_PPQ;
uint8_t trigger_dividerClockOut2 = PRE_4_PPQ;
uint8_t trigger_prescalerClockInput = PRE_4_PPQ;

uint8_t trigger_nextPulseOut1 = 0xff;
uint8_t trigger_nextPulseOut2 = 0xff;


uint32_t trigger_pulseTimes[NUM_PINS];
uint8_t trigger_pulseActive[NUM_PINS];
uint8_t trigger_gateMode = 0;




volatile float trigger_phase = 0;
volatile float trigger_phaseInc = 0;
static uint8_t trigger_phaseWrapCounter = 1;	// count how many times the phase increased by 1.0
												// with a preascaler of 8 (4ppq) 1 clock pulse
												// has to increment the sequencer by 8 steps
#define COUNTER_DURATION (1.0f/42000000l)

//--------------------------------------------------
void EXTI9_5_IRQHandler()
{
	//reset in
	if(EXTI_GetITStatus(EXTI_Line8) != RESET)
	{
		if(seq_getExtSync())
		{
			const uint16_t pinState = (GPIOA->IDR & GPIO_Pin_8);
			if(!pinState)
			{
				//reset pin is high -> stop and reset sequencer
				seq_setRunning(0);
			} else {
				//reset pin is low -> start sequencer
				seq_setRunning(1);
			}
		}

		EXTI_ClearITPendingBit(EXTI_Line8);
	}
	//clock in falling edge (inverted due to input transistor)
	else if(EXTI_GetITStatus(EXTI_Line9) != RESET)
    {
		if(seq_getExtSync())
		{
			uint32_t counter = medianFilter(TIM_GetCounter(TIM2));

			//time for a 128th substep
			float t =  (counter) * COUNTER_DURATION / (trigger_prescalerClockInput);
			const float f = (1.f/t);
			trigger_phaseInc = (1/(REAL_FS/f)) ;

			//divide incoming clocks with prescaler
			//if(trigger_prescaleCounterClockInput % trigger_prescalerClockInput == 0)
			{
				if(seq_isRunning()!=0)
				{
					seq_triggerNextMasterStep(trigger_prescalerClockInput);
					//reset phase counter
					trigger_phase = 0;
					trigger_phaseWrapCounter = 1;

				}
			}
			//reset measurement timer
		}
		TIM_SetCounter(TIM2,0);
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

	case TRIGGER_ALL:
		if(!isOn)
		{
			GPIOD->ODR |= (PIN_TRACK_1) ;
			GPIOD->ODR |= (PIN_TRACK_1<<1) ;
			GPIOD->ODR |= (PIN_TRACK_1<<2) ;
			GPIOD->ODR |= (PIN_TRACK_1<<3) ;
			GPIOD->ODR |= (PIN_TRACK_1<<4) ;
			GPIOD->ODR |= (PIN_TRACK_1<<5) ;
			GPIOD->ODR |= (PIN_TRACK_1<<6) ;
		} else {
			GPIOD->ODR &= ~(PIN_TRACK_1) ;
			GPIOD->ODR &= ~(PIN_TRACK_1<<1) ;
			GPIOD->ODR &= ~(PIN_TRACK_1<<2) ;
			GPIOD->ODR &= ~(PIN_TRACK_1<<3) ;
			GPIOD->ODR &= ~(PIN_TRACK_1<<4) ;
			GPIOD->ODR &= ~(PIN_TRACK_1<<5) ;
			GPIOD->ODR &= ~(PIN_TRACK_1<<6) ;
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
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);



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

	/* Enable and set EXTI Line9 Interrupt  */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	//---- Timer 2 ------------------------------------------------------
	//Timer 2 (32-bit) for time measuring of external clock pulses (trigger IO)
	TIM_TimeBaseInitTypeDef TIM_TimeBase_InitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	//HSE 168MHz, APB1 Prescaler = 4 => 42MHz
	TIM_TimeBase_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBase_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBase_InitStructure.TIM_Period = 0xffffffff;
	TIM_TimeBase_InitStructure.TIM_Prescaler = 1;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBase_InitStructure);

	TIM_Cmd(TIM2, ENABLE);

	//set trigger outs to low
	int i;
	for(i=0;i<NUM_PINS;i++)
	{
		trigger_setPin(i,0);
	}
}
//--------------------------------------------------
void trigger_triggerVoice(uint8_t voice, triggerMode mode)
{
	switch(mode)
	{
	case TRIGGER_PULSE:
		trigger_pulsePin(TRIGGER_1 + voice);
		break;

	case TRIGGER_ON:
		trigger_setPin(TRIGGER_1 + voice,1);
		break;

	case TRIGGER_OFF:
		trigger_setPin(TRIGGER_1 + voice,0);
		break;
	}

}
//--------------------------------------------------
void trigger_clockTick(uint8_t pos)
{
	if(pos==1)
	{
		trigger_nextPulseOut1 = 0;
		trigger_nextPulseOut2 = 0;
		//trigger_setPin(CLOCK_1,0);
		//trigger_setPin(CLOCK_2,0);
	}

	if(pos >= trigger_nextPulseOut1)
	{
		//trigger_pulsePin(CLOCK_1);
		trigger_setPin(CLOCK_1,1);
		trigger_nextPulseOut1 += trigger_dividerClockOut1;
	} else trigger_setPin(CLOCK_1,0);

	if(pos >= trigger_nextPulseOut2)
	{
		trigger_setPin(CLOCK_2,1);
		trigger_nextPulseOut2 += trigger_dividerClockOut2;
	}else trigger_setPin(CLOCK_2,0);

}
//--------------------------------------------------
void trigger_reset(uint8_t value)
{
	trigger_setPin(TRIGGER_RESET,1-value);

	if(value) {
		trigger_nextPulseOut1 = 0;
		trigger_nextPulseOut2 = 0;
	}
}
//--------------------------------------------------
void trigger_tickPhaseCounter()
{
	int i;
	for(i=0;i<OUTPUT_DMA_SIZE;i++)
	{
		//only run one cycle of the phase counter
		// it is reset by the next incoming master clock pulse
		if( (trigger_phase + trigger_phaseInc) < trigger_prescalerClockInput)
		{
			trigger_phase += trigger_phaseInc;
		}
		if(trigger_phase >= trigger_phaseWrapCounter)
		{
			trigger_phaseWrapCounter++;
			// trigger a step
			// -> set time to next step to immediately (trigger next step)
			seq_setDeltaT(-1);
		}
	}
}
//--------------------------------------------------
uint8_t trigger_isGateModeOn()
{
	return trigger_gateMode;
}
//--------------------------------------------------
void trigger_setGatemode(uint8_t onOff)
{
	trigger_gateMode = onOff;
}
//--------------------------------------------------
void trigger_allOff()
{
	trigger_setPin(TRIGGER_ALL, 0);
}

