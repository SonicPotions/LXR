/*
 * adcPots.c
 *
 * Created: 27.04.2012 16:15:14
 *  Author: Julian
 */ 
#include "adcPots.h"
#include <string.h>
#include "../Menu/menu.h"

#define HYSTERSIS     3
#define MIN( a, b ) (a < b) ? a : b


uint16_t adc_potValues[4];
uint16_t adc_potAvg[4];
//------------------------------------------------------------------------
void adc_init(void) {
 
  memset((void*)adc_potValues,0,8);
  memset((void*)adc_potAvg,0,8);
  uint16_t result;
 
  // AVCC as ref voltage
  ADMUX =  (1<<REFS0);
  
  // single conversion
  ADCSRA = (1<<ADPS2) |(1<<ADPS1) /*| (1<<ADPS0)*/;     // adc prescaler div 64 => 8mhz clock = 125 kHz (must be between 50 and 200kHz)
  ADCSRA |= (1<<ADEN);                  // ADC enable
 
  // dummy readout
  ADCSRA |= (1<<ADSC);                  // single readout
  while (ADCSRA & (1<<ADSC) ) {}        // wait to finish
  // read result 
  result = ADCW;
  (void)result; //ommit unused but set warning
}
//------------------------------------------------------------------------
uint16_t adc_read( uint8_t channel )
{
  //select channel
  ADMUX = (uint8_t)((ADMUX & ~(0x1F)) | (channel & 0x1F));
  ADCSRA |= (1<<ADSC);            // single readout
  while (ADCSRA & (1<<ADSC) ) {}  // wait to finish
  return ADCW;                    // read and return result
}
 //------------------------------------------------------------------------
uint16_t adc_readAvg( uint8_t channel, uint8_t nsamples )
{
  uint32_t sum = 0;
 
  for (uint8_t i = 0; i < nsamples; ++i ) {
    sum += adc_read( channel );
  }
  
  return (uint16_t)( sum / nsamples );
}
//------------------------------------------------------------------------ 
uint8_t adcCnt = 0;
void adc_checkPots()
{
	//adc_potAvg
	
	//if((adcCnt++)&0x07 == 0x00)
	{
		for(uint8_t i=0;i<4;i++)
		{	
		
			uint16_t newValue = adc_readAvg(i,4);//adc_read(i);
		
		   if ((newValue > ( adc_potValues[i] + HYSTERSIS)) ||
			   ( adc_potValues[i] > (newValue + HYSTERSIS)))
		   {
			   adc_potValues[i] = newValue;
			   menu_parseKnobValue(i,(uint8_t)(newValue>>2));
			   menu_repaintAll();
		   } 
		}
	}		
};
//------------------------------------------------------------------------
