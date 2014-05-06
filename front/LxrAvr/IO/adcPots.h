/*
 * adcPots.h
 *
 * Created: 27.04.2012 15:51:29
 *  Author: Julian
 */ 


#ifndef ADCPOTS_H_
#define ADCPOTS_H_

#include <avr/io.h>

void adc_init(void);
 
uint16_t adc_read( uint8_t channel );
uint16_t adc_readAvg( uint8_t channel, uint8_t nsamples );
//check the connected potentiometers and parse their values
void adc_checkPots();
 


#endif /* ADCPOTS_H_ */