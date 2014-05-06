/*
 * dout.h
 *
 * Created: 25.04.2012 15:19:43
 *  Author: Julian
 */ 


#ifndef DOUT_H_
#define DOUT_H_

#include <avr/io.h>

//number of connected outputs
#define NUM_OUTS 40

//outputs
#define DOUT_LOAD_PIN	(1<<PD4)	//transition to high loads data to the output pins
#define DOUT_CLK_PIN	(1<<PD5)	//shift on transition to HIGH
#define DOUT_OUTPUT_PIN	(1<<PD6)	//the output port where data enters the shift register

//port
#define DOUT_PORT		PORTD
#define DOUT_DDR		DDRD


extern uint8_t dout_outputData[NUM_OUTS/8];
//---------------------------------------------------------------------------------
void dout_init();
//---------------------------------------------------------------------------------
void dout_updateOutputs();
//---------------------------------------------------------------------------------
void dout_shift();
//---------------------------------------------------------------------------------
void dout_load();

#endif /* DOUT_H_ */