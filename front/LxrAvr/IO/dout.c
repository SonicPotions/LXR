/*
 * dout.c
 *
 * Created: 25.04.2012 15:30:25
 *  Author: Julian
 */ 

#include "dout.h"
#include <string.h>

#define USE_BRIGHTNESS 0

#if USE_BRIGHTNESS
static uint8_t dout_ledBrightness[NUM_OUTS];
static uint8_t dout_brightnessCounter = 0;
#endif

uint8_t dout_outputData[NUM_OUTS/8];
//---------------------------------------------------------------------------------
void dout_init()
{
	uint8_t outPins = DOUT_LOAD_PIN | DOUT_CLK_PIN | DOUT_OUTPUT_PIN;
	
	//configure as outputs
	DOUT_DDR |= outPins; 
	
	//set outputs to low
	DOUT_PORT &= (uint8_t)(~outPins);
	
	//clear the input data array
	memset(dout_outputData,0,NUM_OUTS/8);
	
#if USE_BRIGHTNESS	
	for(int i=0;i<NUM_OUTS;i++)
	{
		if(i&0x1)
			dout_ledBrightness[i] = 0x08;
		else
			dout_ledBrightness[i] = 0x00;
	}
#endif	
}
//---------------------------------------------------------------------------------
void dout_updateOutputs()
{
#if USE_BRIGHTNESS
	dout_brightnessCounter++;
#endif
	
	for(int i=0;i<NUM_OUTS;i++)
	{
		//clear the output pin
		DOUT_PORT &= (uint8_t)(~DOUT_OUTPUT_PIN);
#if USE_BRIGHTNESS
		if((dout_brightnessCounter &  dout_ledBrightness[i]) == 0x00)
		{
			//put bit on output
			if((dout_outputData[i/8]&(1<<(i&7)))!=0)
			{
				DOUT_PORT |= DOUT_OUTPUT_PIN;
			}		
		}
#else		
		//put bit on output
		if((dout_outputData[i/8]&(1<<(i&7)))!=0)
		{
			DOUT_PORT |= DOUT_OUTPUT_PIN;
		}		
#endif
		//shift output into register
		dout_shift();
	}
	//output the data on the register pins
	dout_load();
};
//---------------------------------------------------------------------------------
void dout_shift()
{
	//clock high
	DOUT_PORT |= DOUT_CLK_PIN;
	//clock low
	DOUT_PORT &= (uint8_t)(~DOUT_CLK_PIN);
};
//---------------------------------------------------------------------------------
void dout_load()
{
	DOUT_PORT |= DOUT_LOAD_PIN;
	DOUT_PORT &= (uint8_t)(~DOUT_LOAD_PIN);
};
//---------------------------------------------------------------------------------
