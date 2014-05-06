/*
 * userInterface.h
 *
 * Created: 08.07.2012 16:06:40
 *  Author: Julian
 */ 


#ifndef USERINTERFACE_H_
#define USERINTERFACE_H_
#include <avr/io.h>

#define SIGNAL_LED			PD2		//start stop button led
#define SIGNAL_LED_PORT		PORTD
#define SIGNAL_LED_DDR		DDRD

#define SIGNAL_BUTTON		PC2	//encoder button
#define SIGNAL_BUTTON_PORT	PORTC
#define SIGNAL_BUTTON_DDR	DDRC

#define ENCODER_BUTTON	(PINC & (1<<PC2))


void ui_init();
uint8_t ui_isButtonPressed();


#endif /* USERINTERFACE_H_ */