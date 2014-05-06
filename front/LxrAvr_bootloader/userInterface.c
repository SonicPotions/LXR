/*
 * userInterface.c
 *
 * Created: 08.07.2012 16:45:48
 *  Author: Julian
 */ 
#include "userInterface.h"
//-------------------------------------------------------
void ui_init()
{
	//button
	SIGNAL_BUTTON_DDR	&= ~(1<<SIGNAL_BUTTON); //button pin as input
	SIGNAL_BUTTON_PORT	|= (1<<SIGNAL_BUTTON);	 //pull up active
}
//-------------------------------------------------------
uint8_t ui_isButtonPressed()
{
	return (ENCODER_BUTTON==0);
}