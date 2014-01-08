/************************************************************************/
/*                                                                      */
/*                      Reading rotary encoder                          */
/*                      one, two and four step encoders supported       */
/*                                                                      */
/*              Author: Peter Dannegger                                 */
/*				Modified: Julian Schmidt								*/
/*                                                                      */
/************************************************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
 
// target: ATmega32
//------------------------------------------------------------------------
#ifndef F_CPU
#define F_CPU 20000000UL
#endif
//#define XTAL        8e6         // 8MHz
 
#define PIN_A			(1<<PC0)
#define PIN_B			(1<<PC1)
#define PIN_BUTTON		(1<<PC2)
 
#define PHASE_A			(PINC & (1<<PC0))
#define PHASE_B			(PINC & (1<<PC1))
#define ENCODER_BUTTON	(PINC & (1<<PC2))

#define ECODER_PORT		PORTC
#define ENCODER_DDR		DDRC	//pin direction i/o
 

 
 
//------------------------------------------------------------------------
//functions
//------------------------------------------------------------------------
void encode_init( void );
 
// 1ms for manual movement
ISR( TIMER0_COMP_vect );
 
// read single step encoders
int8_t encode_read1( void );
 
// read two step encoders
int8_t encode_read2( void );
 
// read four step encoders
int8_t encode_read4( void );

//void encode_setInterrupt(uint8_t enabled);

//get the state of the encoder button
uint8_t encode_readButton();
