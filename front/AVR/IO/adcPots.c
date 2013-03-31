/*
 * adcPots.c
 *
 * Created: 27.04.2012 16:15:14
 *  Author: Julian
 */ 
#include "adcPots.h"
#include <string.h>
#include "..\Menu\menu.h"

#define HYSTERSIS     3

volatile uint16_t adc_potValues[4];
//volatile uint8_t adc_lastPotValues[4];
//------------------------------------------------------------------------
/* ADC initialisieren */
void adc_init(void) {
 
  memset((void*)adc_potValues,0,8);
 // memset(adc_lastPotValues,0,4);
  
  uint16_t result;
 
  // AVCC (5V) as reference
  ADMUX =  (1<<REFS0);
  
  // single conversion
  ADCSRA = (1<<ADPS2) |(1<<ADPS1) /*| (1<<ADPS0)*/;     // adc prescaler div 64 => 8mhz clock = 125 kHz (must be between 50 and 200kHz)
  ADCSRA |= (1<<ADEN);                  // ADC aktivieren
 
  /* nach Aktivieren des ADC wird ein "Dummy-Readout" empfohlen, man liest
     also einen Wert und verwirft diesen, um den ADC "warmlaufen zu lassen" */
 
  ADCSRA |= (1<<ADSC);                  // eine ADC-Wandlung 
  while (ADCSRA & (1<<ADSC) ) {}        // auf Abschluss der Konvertierung warten
  /* ADCW muss einmal gelesen werden, sonst wird Ergebnis der nächsten
     Wandlung nicht übernommen. */
  result = ADCW;
}
//------------------------------------------------------------------------
 /* ADC Einzelmessung */
uint16_t adc_read( uint8_t channel )
{
  // Kanal waehlen, ohne andere Bits zu beeinflußen
  ADMUX = (ADMUX & ~(0x1F)) | (channel & 0x1F);
  ADCSRA |= (1<<ADSC);            // eine Wandlung "single conversion"
  while (ADCSRA & (1<<ADSC) ) {}  // auf Abschluss der Konvertierung warten
  return ADCW;                    // ADC auslesen und zurückgeben
}
 //------------------------------------------------------------------------
/* ADC Mehrfachmessung mit Mittelwertbbildung */
/* beachte: Wertebereich der Summenvariablen */
uint16_t adc_readAvg( uint8_t channel, uint8_t nsamples )
{
  uint32_t sum = 0;
 
  for (uint8_t i = 0; i < nsamples; ++i ) {
    sum += adc_read( channel );
  }
 
  return (uint16_t)( sum / nsamples );
}
//------------------------------------------------------------------------ 
void adc_checkPots()
{
	for(int i=0;i<4;i++)
	{	
		
		uint16_t newValue = adc_readAvg(i,4);//adc_read(i);
		
		

	   if ((newValue > ( adc_potValues[i] + HYSTERSIS)) ||
		   ( adc_potValues[i] > (newValue + HYSTERSIS)))
	   {
		   adc_potValues[i] = newValue;
		  // menu_parsePotiValue(i,newValue>>3);
		   menu_parseKnobValue(i,newValue>>2);
		   
	   } 
	}
};
//------------------------------------------------------------------------