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
	/*
	//led
	SIGNAL_LED_DDR		|= (1<<SIGNAL_LED); //LED pin as output
	SIGNAL_LED_PORT		&= ~(1<<SIGNAL_LED);//deactivated init
	*/

	//button
	SIGNAL_BUTTON_DDR	&= ~(1<<SIGNAL_BUTTON); //button pin as input
	SIGNAL_BUTTON_PORT	|= (1<<SIGNAL_BUTTON);	 //pull up active
}
//-------------------------------------------------------
/*
void ui_ledToggle()
{
	if(SIGNAL_LED_PORT&(1<<SIGNAL_LED))
	{
		SIGNAL_LED_PORT &= ~((1<<SIGNAL_LED));
	}
	else
	{
		SIGNAL_LED_PORT |= ((1<<SIGNAL_LED));
	}		
};
*/
//-------------------------------------------------------
uint8_t ui_isButtonPressed()
{
	return (ENCODER_BUTTON==0);
}