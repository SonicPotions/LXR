/*
 * din.c
 *
 * Created: 22.04.2012 17:15:25
 *  Author: Julian
 */ 

#include "din.h"
#include <string.h>
#include <util/delay.h>
#include "../buttonHandler.h"

//---------------------------------------------------------------------
//1 bit for every digital input, so we can store 8 input signals in one uint8_t
uint8_t din_inputData[NUM_INPUTS/8+1];
//indicates which input was read last. if NUM_INPUTS is reached, a new load has to occur
static uint8_t inputPos = NUM_INPUTS; 
//---------------------------------------------------------------------
void din_init()
{
	uint8_t outPins = DIN_LOAD_PIN | DIN_CLK_PIN;
	uint8_t inPins  = DIN_INPUT_PIN;
	
	//configure as input
	DIN_DDR &= (uint8_t)(~inPins);
	
	//configure as outputs
	DIN_DDR |= outPins; 
	
	// disable internal pull ups
	DIN_PORT &= (uint8_t)(~inPins);
	
	//set outputs to high
	DIN_PORT |= outPins;
	
	//clear the input data array
	memset(din_inputData,0,NUM_INPUTS/8+1);
	
	din_load();
};
//---------------------------------------------------------------------
void din_shift()
{
	DIN_PORT &= (uint8_t)~DIN_CLK_PIN; //clock low
	DIN_PORT |= DIN_CLK_PIN;  //clock high
}
//---------------------------------------------------------------------
void din_load()
{
	//set load pin LOW to get new data
	DIN_PORT &= (uint8_t)~DIN_LOAD_PIN;
	//set load pin HIGH again to enable clock
	DIN_PORT |= DIN_LOAD_PIN;
}
//---------------------------------------------------------------------
void din_readNextInput()
{
	
	int i;
	for(i=0;i<10;i++)
	{
		//check if a new load is necessary
		if(inputPos>=NUM_INPUTS)
		{
			//reset inputPos
			inputPos = 0;
			//load new data into register
			din_load();
		}
		else
		{
			//increment position counter		
			inputPos++;
			//shift data
			din_shift();
		}
	
		//store input value in inputData[]
		uint8_t arrayPos = inputPos/8;
		uint8_t bitPos	 = inputPos&7;

		//read pin
		uint8_t value;
		if((DIN_INPUT & DIN_INPUT_PIN) >0)
		{
			value = 0;
		}		
		else
		{
			value = 1;
		}
	
		//check if button was pressed
		if(!value)
		{
			if( (din_inputData[arrayPos] & (1<<bitPos)) !=  (value << bitPos))
			{
				buttonHandler_buttonPressed(inputPos);
			}
		}		
	
		//check if button was release
		if(value)
		{
			if( (din_inputData[arrayPos] & (1<<bitPos)) !=  (value << bitPos))
			{
				buttonHandler_buttonReleased(inputPos);
			}
		}		
	
		//clear bit
		din_inputData[arrayPos] &= (uint8_t)(~(1<<bitPos));
		//set new bit value
		din_inputData[arrayPos] |= (uint8_t)(value << bitPos);
	}		
};
