/************************************************************************/
/*                                                                      */
/*                      Reading rotary encoder							*/
/*                      one, two and four step encoders supported		*/
/*                                                                      */
/*              Author: Peter Dannegger                                 */
/*                                                                      */
/************************************************************************/
#include "encoder.h"


 
                // target: ATmega644
//------------------------------------------------------------------------
 
volatile int8_t		enc_delta;          // -128 ... 127
static int8_t		last;
static int8_t		lastButton;
volatile uint8_t	buttonValue;

// --AS I don't think this is used anymore
//volatile  uint8_t encoder_calcLFOs = 1;

void encode_init( void )
{
	
	//port init
	// set used pins to input
    uint8_t pins = PIN_A|
                   PIN_B |
				   PIN_BUTTON;
				   
	ENCODER_DDR &= (uint8_t)~pins; //configure as input
	
	// enable internal pull ups
	ECODER_PORT |= pins;
	
  int8_t new;
 
  new = 0;
  if( PHASE_A )
    new = 3;
  if( PHASE_B )
    new ^= 1;                   // convert gray to binary
  last = new;                   // power on state
  enc_delta = 0;
  
  //timer 0
  //TCCR0 = 1<<WGM01^1<<CS01^1<<CS00;     // CTC, XTAL / 64
  TCCR0A = 1<<WGM01;     // CTC, XTAL / 64
  //TCCR0B = 1<<CS01 | 1<<CS00;
  TCCR0B = 1<<CS02; //prescaler 256 => 78125 Hz = 78,125 kHz
  //OCR0 = (uint8_t)(F_CPU / 64.0 * 1e-3 - 0.5);   // 1ms
  //OCR0A = (uint8_t)(F_CPU / 64.0 * 1e-3 - 0.5);   // 1ms -> not possible to reach 1ms with 20mHz clock and pre 64 => 312
  
  OCR0A = (uint8_t)(F_CPU / 256.0f * 0.001f); // => 78.125 => (int)78 ==> timerfrequenz: 1001,641025641025641025641025641 Hz = 0,9984 ms
  //TIMSK |= 1<<OCIE0;
  TIMSK0 |= 1<<OCIE0A;
  
  lastButton = 0;
  buttonValue = 1;
}

//void encode_setInterrupt(uint8_t enabled)
//{
//	encoder_calcLFOs = enabled;
//}
 
 
ISR( TIMER0_COMPA_vect )             // 1ms for manual movement
{
  int8_t new, diff;
 
  new = 0;
  if( PHASE_A )
    new = 3;
  if( PHASE_B )
    new ^= 1;						// convert gray to binary
  diff = (int8_t)(last - new);                // difference last - new
  if( diff & 1 ){					// bit 0 = value (1)
    last = new;						// store new as next last
    enc_delta = (int8_t)(enc_delta + (diff & 2) - 1);    // bit 1 = direction (+/-)
  }
  
  
  //button
  if(ENCODER_BUTTON == lastButton)
  {
	  buttonValue = (uint8_t)lastButton;
  }
  lastButton = ENCODER_BUTTON;
}
 
 
int8_t encode_read1( void )         // read single step encoders
{
  int8_t val;
 
  cli();
  val = enc_delta;
  enc_delta = 0;
  sei();
  return val;						// counts since last call
}
 
 
int8_t encode_read2( void )         // read two step encoders
{
  int8_t val;
 
  cli();
  val = enc_delta;
  enc_delta = val & 1;
  sei();
  return val >> 1;
}
 
 
int8_t encode_read4( void )         // read four step encoders
{
  int8_t val;
 
  cli();
  val = enc_delta;
  enc_delta = val & 3;
  sei();
  return val >> 2;
}

//get the button value
uint8_t encode_readButton()
{
	// return true if button is pressed
	return (buttonValue==0);
};
