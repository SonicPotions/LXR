/*
 * TriggerOut.c
 *
 *  Created on: 06.05.2013
 *      Author: Julian
 */


#include "TriggerOut.h"
#include "string.h"

uint8_t trigger_dividerClock1 = 1;
uint8_t trigger_dividerClock2 = 1;
uint8_t trigger_clockCnt = 0;

uint32_t trigger_pulseTimes[NUM_PINS];
uint8_t trigger_pulseActive[NUM_PINS];
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
		if(isOn)
		{
			GPIOD->ODR |= (PIN_TRACK_1<<index) ;
		} else {
			GPIOD->ODR &= ~(PIN_TRACK_1<<index) ;
		}

		break;

	case CLOCK_2:
	case TRIGGER_RESET:
		if(isOn)
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

	GPIO_InitTypeDef GPIO_InitStructure;

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
