/*
 * din.h
 *
 * Created: 22.04.2012 17:06:44
 *  Author: Julian
 */ 

//Digital input via shift register 74HC165

#ifndef DIN_H_
#define DIN_H_
#include <avr/io.h>

//number of connected inputs
#define NUM_INPUTS 39

//outputs
#define DIN_LOAD_PIN	((uint8_t)(1<<PC5))	//LOW level enables parallel load, disables clock
#define DIN_CLK_PIN		((uint8_t)(1<<PC6))	//shift on transition to HIGH, LOAD pin must be HIGH to enable clocking

//Input
#define DIN_INPUT_PIN	((uint8_t)(1<<PC7))	//the output of the shift register, this is where the data comes into the uC

//port
#define DIN_PORT		PORTC
#define DIN_DDR			DDRC
#define DIN_INPUT		PINC

extern uint8_t din_inputData[NUM_INPUTS/8+1];
//---------------------------------------------------------------------------------
//init the digital input interface, configure pins...
void din_init();
//---------------------------------------------------------------------------------
// shift the next value to the input and store it in the inputData array.
// if all inputs are read, start a new paralell load
void din_readNextInput();
//---------------------------------------------------------------------------------
void din_shift();
//---------------------------------------------------------------------------------
void din_load();
//---------------------------------------------------------------------------------

#endif /* DIN_H_ */
