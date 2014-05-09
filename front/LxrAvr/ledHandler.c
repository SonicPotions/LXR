/*
 * ledHandler.c
 *
 * Created: 25.04.2012 16:04:30
 *  Author: Julian
 */ 

#include "ledHandler.h"
#include "IO/dout.h"
#include <string.h>
#include "Hardware/timebase.h"
#include "Menu/menu.h"

//since we need an array to store the start time we have to limit the number of simultanousely pulsable LEDs
#define NUM_OF_PULSABLE_LEDS 8	//MAX 8 at the moment (because of led_pulsingLeds)
#define LED_PULSE_TIME_MS 50		/**<time a pulsed LED stays on in [ms]*/
#define LED_PULSE_TIME ((uint16_t)(LED_PULSE_TIME_MS/16.384f))

#define NUM_OF_BLINKABLE_LEDS 4
#define LED_BLINK_TIME_MS 250
#define LED_BLINK_TIME ((uint16_t)(LED_BLINK_TIME_MS/16.384f))

volatile uint8_t led_currentStepLed = 0;

/** we have to distinguish between 2 led modes.
there are the nomral set value functions, and there are temporary operations.
For example the menu state or step indicator lights are normal mode
stuff like the chaselight or pulsing a select LED on voice trigger are temporary.
to make it possible that the temporary led control does not influence the normal/default LED state
the normal/default state is also saved in this array, while the temporary functuions like flashing only alter the dout register directly.
this way a temporary led action can be undone by resetting its value to the led_originalLedState[] value
*/
volatile uint8_t led_originalLedState[NUM_OUTS/8];
volatile uint16_t led_pulseEndTime[NUM_OF_PULSABLE_LEDS];		/**< stores the time a LED was activated*/
volatile uint8_t led_pulseLedNumber[NUM_OF_PULSABLE_LEDS];	/**< stores the corresponding LED number*/
volatile uint8_t led_pulsingLeds;


//volatile uint16_t led_nextBlinkTime[NUM_OF_BLINKABLE_LEDS];		/**< stores the time a LED was activated*/
uint16_t led_nextBlinkTime=0;
volatile uint8_t led_blinkLedNumber[NUM_OF_BLINKABLE_LEDS];	/**< stores the corresponding LED number*/
volatile uint8_t led_blinkingLeds;
//--------------------------------------------
void led_init()
{
	//init bitfield to zero
	led_pulsingLeds = 0;	
	memset((void*)led_originalLedState,0,NUM_OUTS/8);
};
//--------------------------------------------
void led_clearSelectLeds()
{
	uint8_t arrayPos = LED_PART_SELECT1/8;
	dout_outputData[arrayPos] = 0;
	led_originalLedState[arrayPos] = 0; 
}
//--------------------------------------------
void led_initPerformanceLeds()
{
	led_setValue(1,(uint8_t)(menu_playedPattern + LED_PART_SELECT1));
	// a blinking LED shows the viewed pattern if different from the played pattern
	if(menu_playedPattern != menu_getViewedPattern())
	{
		led_setBlinkLed((uint8_t)(LED_PART_SELECT1 + menu_getViewedPattern()) ,1);
	}		
};
//--------------------------------------------
void led_clearVoiceLeds()
{
	uint8_t arrayPos = LED_VOICE1/8;
	dout_outputData[arrayPos] &= 0x80;
	led_originalLedState[arrayPos] &= 0x80;
}
//--------------------------------------------
void led_setActivePage(uint8_t pageNr)
{
	pageNr =(uint8_t)(pageNr + LED_PART_SELECT1);
	uint8_t arrayPos = pageNr/8;
	uint8_t bitPos = pageNr&0x7;
	
	//set the whole byte to clear the other leds
	dout_outputData[arrayPos] = (uint8_t)(1<<bitPos);
	//since this is not a temp. change, store in led_originalLedState as well
	led_originalLedState[arrayPos] = (uint8_t)(1<<bitPos);
};
//---------------------------------------------
void led_setActiveSelectButton(uint8_t butNr)
{
	butNr = (uint8_t)(butNr + LED_PART_SELECT1);
	const uint8_t arrayPos = butNr/8;
	const uint8_t bitPos = butNr&0x7;
	
	//set the whole byte to clear the other leds
	dout_outputData[arrayPos] = (uint8_t)(1<<bitPos);
	//since this is not a temp. change, store in led_originalLedState as well
	led_originalLedState[arrayPos] = (uint8_t)(1<<bitPos);
};
//---------------------------------------------
void led_setActiveVoiceLeds(uint8_t pattern)
{
	uint8_t arrayPos = LED_VOICE1/8;
	//clear lower 7 bits
	dout_outputData[arrayPos] &= 0x80;
	led_originalLedState[arrayPos] &= 0x80;
	//set the lower 7 bits
	dout_outputData[arrayPos] =  (uint8_t)(dout_outputData[arrayPos] | (pattern&0x7f));
	//since this is not a temp. change, store in led_originalLedState as well
	led_originalLedState[arrayPos] =  (uint8_t)(led_originalLedState[arrayPos] | (pattern&0x7f));
}
//---------------------------------------------
void led_setActiveVoice(uint8_t voiceNr)
{
	voiceNr =  (uint8_t)(voiceNr + LED_VOICE1);
	uint8_t arrayPos = voiceNr/8;
	uint8_t bitPos = voiceNr&0x7;
	
	//clear lower 7 bit (8th bit is copy led)
	dout_outputData[arrayPos] &= 0x80;
	led_originalLedState[arrayPos] &= 0x80;
	//set the whole byte to clear the other leds
	dout_outputData[arrayPos] |= (uint8_t)(1<<bitPos);
	//since this is not a temp. change, store in led_originalLedState as well
	led_originalLedState[arrayPos] |= (uint8_t)(1<<bitPos);
	
	//reset mute mode
	menu_muteModeActive = 0;
};
//---------------------------------------------
void led_setValue(uint8_t val, uint8_t ledNr)
{
	uint8_t arrayPos = ledNr/8;
	uint8_t bitPos = ledNr&0x7;
	//clear the led
	dout_outputData[arrayPos] &= (uint8_t)~(1<<bitPos);
	led_originalLedState[arrayPos] &= (uint8_t)~(1<<bitPos);

	//set new value
	if(val)
	{
		dout_outputData[arrayPos] |= (uint8_t)(1<<bitPos);
		led_originalLedState[arrayPos] |= (uint8_t)(1<<bitPos);
			
	}	
	
	return;
}
//-----------------------------------------------
void led_setValueTemp(uint8_t val, uint8_t ledNr)
{
	uint8_t arrayPos = ledNr/8;
	uint8_t bitPos = ledNr&0x7;
	//clear the led
	dout_outputData[arrayPos] &= (uint8_t)~(1<<bitPos);
	
	//set new value
	if(val)
	{
		dout_outputData[arrayPos] |= (uint8_t)(1<<bitPos);
	}	
	
	return;
};
//-----------------------------------------------
void led_clearAll()
{
	memset(dout_outputData,0,NUM_OUTS/8);
	memset((void*)led_originalLedState,0,NUM_OUTS/8);
};
//-----------------------------------------------
void led_toggle(uint8_t ledNr)
{
	uint8_t arrayPos = ledNr/8;
	uint8_t bitPos = ledNr&0x7;

	uint8_t oldValue = (uint8_t)(dout_outputData[arrayPos]&(1<<bitPos));
	//clear the led
	dout_outputData[arrayPos] &= (uint8_t)~(1<<bitPos);
	led_originalLedState[arrayPos] &= (uint8_t)~(1<<bitPos);

	//set new value
	if(!oldValue)
	{
		dout_outputData[arrayPos] |= (uint8_t)(1<<bitPos);
		led_originalLedState[arrayPos] |= (uint8_t)(1<<bitPos);
			
	}	
	return;	
};
//--------------------------------------------------------------------------
void led_toggleTemp(uint8_t ledNr)
{
	uint8_t arrayPos = ledNr/8;
	uint8_t bitPos = ledNr&0x7;
	uint8_t oldValue = (uint8_t)(dout_outputData[arrayPos]&(1<<bitPos));
	//clear the led
	dout_outputData[arrayPos] &= (uint8_t)~(1<<bitPos);
	//set new value
	if(!oldValue)
	{
		dout_outputData[arrayPos] |= (uint8_t)(1<<bitPos);
	}		
	return;
};
//--------------------------------------------------
void led_setActive_step(uint8_t stepNr)
{
	stepNr /= 8; //128 steps auf 16 reduzieren
	uint8_t ledNr;
	ledNr = (uint8_t)(stepNr+LED_STEP1);
		
	if(led_currentStepLed != ledNr)
	{
		led_reset(led_currentStepLed);
		led_currentStepLed = ledNr;
		led_toggleTemp(ledNr);
	}		
};
//-------------------------------------------------
void led_clearActive_step()
{
	led_reset(led_currentStepLed);
};
//-------------------------------------------------
void led_clearSequencerLeds()
{
	for(int i=0;i<16;i++)
	{
		led_setValue(0,(uint8_t)(LED_STEP1 + i));
	}
};
//-------------------------------------------------
void led_clearSequencerLeds9_16()
{
	for(uint8_t i=0;i<8;i++)
	{
		led_setValue(0,(uint8_t)(LED_STEP8 + i));
	}
};
//-------------------------------------------------
void led_clearSequencerLeds1_8()
{
	for(uint8_t i=0;i<8;i++)
	{
		led_setValue(0,(uint8_t)(LED_STEP1 + i));
	}
};
//-------------------------------------------------
void led_setMode2(uint8_t status)
{
	led_setValue(0,LED_MODE1);
	led_setValue(0,LED_MODE2);
	led_setValue(0,LED_MODE3);
	led_setValue(0,LED_MODE4);	
	
	led_setBlinkLed(LED_MODE1,0);
	led_setBlinkLed(LED_MODE2,0);
	led_setBlinkLed(LED_MODE3,0);
	led_setBlinkLed(LED_MODE4,0);
			
	switch(status)
	{
		case 0:
			led_setValue(1,LED_MODE1);
		break;
		
		case 1:
			led_setValue(1,LED_MODE2);
		break;
		
		case 2:
			led_setValue(1,LED_MODE3);
		break;
		
		case 3:
			led_setValue(1,LED_MODE4);		
		break;
		
		case 4:
			led_setValue(1,LED_MODE4);	
		break;
		
		case 5:
			led_setBlinkLed(LED_MODE2,1);
		break;
		
		case 6:
			
		break;
		
		case 7:
			led_setBlinkLed(LED_MODE4,1);
		break;
		
		default:
		break;
	}
};
//-------------------------------------------------------
void led_setMode2Leds(uint8_t value)
{
	//set the whole byte to clear the other leds
	dout_outputData[LED_PART_SELECT1/8] = (uint8_t)~value;//(1<<bitPos);
	led_originalLedState[LED_PART_SELECT1/8] = (uint8_t)~value;
};
//-------------------------------------------------------
void led_pulseLed(uint8_t ledNr)
{
	//search for a free pulse slot
	for(int i=0;i<NUM_OF_PULSABLE_LEDS;i++)
	{
		if( !(led_pulsingLeds & (1<<i)))
		{
			//we found a free slot	
			
			//store led number
			led_pulseLedNumber[i] = ledNr;
			//set slot to active
			led_pulsingLeds |= (uint8_t)(1<<i);
			//set light up time
			led_pulseEndTime[i] = time_sysTick  +LED_PULSE_TIME;
			//turn on LED
			led_toggleTemp(ledNr);
			
			break;
		}
	}		
};
//-------------------------------------------------------
void led_clearAllBlinkLeds()
{
	for(int i=0;i<NUM_OF_BLINKABLE_LEDS;i++)
	{
		//reset led
		led_reset(led_blinkLedNumber[i]);
		//set slot to inactive
		led_blinkingLeds &= (uint8_t)~(1<<i);
	}
};
//-------------------------------------------------------
void led_setBlinkLed(const uint8_t ledNr, const uint8_t onOff)
{
	if(onOff)
	{
		//--- turn on blinking ---
		//search for a free blink slot
		for(int i=0;i<NUM_OF_BLINKABLE_LEDS;i++)
		{
			if( !(led_blinkingLeds & (1<<i)))
			{
				//we found a free slot	
			
				//store led number
				led_blinkLedNumber[i] = ledNr;
				//set slot to active
				led_blinkingLeds |= (uint8_t)(1<<i);
				//turn on LED
				led_toggleTemp(ledNr);
			
				break;		
			}
		}	
	}		
	else
	{
		//--- turn off blinking ---
		//search the slot with matching ledNr
		for(int i=0;i<NUM_OF_BLINKABLE_LEDS;i++)
		{
			if(led_blinkLedNumber[i] == ledNr)
			{
				//we found the matching slot
				//set slot to inactive
				led_blinkingLeds &= (uint8_t)~(1<<i);
				led_reset(ledNr);
			}
		}
	}
};
//-------------------------------------------------------
void led_tickHandler()
{
	for(int i=0;i<NUM_OF_PULSABLE_LEDS;i++)
	{
		//if the led is active at the moment
		if(led_pulsingLeds & (1<<i))
		{
			//if the on time has passed
			if(time_sysTick > led_pulseEndTime[i])
			{
					//set slot to inactive
					led_pulsingLeds &= (uint8_t)~(1<<i);
					// reset LED
					led_reset(led_pulseLedNumber[i]);
			}
		}			
	}
	
	//second condition is to prevent hanging LEDs if a systick overflow occurs
	if( (time_sysTick > led_nextBlinkTime) || ( (led_nextBlinkTime-time_sysTick) > LED_BLINK_TIME*2) )
	{
		led_nextBlinkTime = time_sysTick + LED_BLINK_TIME;
		
		for(int i=0;i<NUM_OF_BLINKABLE_LEDS;i++)
		{
			//if the led is active at the moment
			if(led_blinkingLeds & (1<<i))
			{
				//toggle led
				led_toggleTemp(led_blinkLedNumber[i]);
				//set next toggle time			
			}	
		}
					
	}		
}

//--------------------------------------------------------------------------
void led_reset(uint8_t ledNr)
{
	uint8_t arrayPos = ledNr/8;
	uint8_t bitPos = ledNr&0x7;
		
	const uint8_t ledValue = (uint8_t)(led_originalLedState[arrayPos] & (1<<bitPos));
		
	if(ledValue!=0)
	{
		dout_outputData[arrayPos] |= (uint8_t)(1<<bitPos);
	}	
	else
	{
		dout_outputData[arrayPos] &= (uint8_t)~(1<<bitPos);
	}	 
};
//--------------------------------------------------------------------------
