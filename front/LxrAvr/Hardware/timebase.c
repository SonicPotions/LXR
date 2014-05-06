#include "timebase.h"

/*
 * timebase.c
 *
 * Created: 15.06.2012 16:17:40
 *  Author: Julian
 */ 


volatile uint16_t time_sysTick=0;

//---------------------------------------------
/*
Timer Overflow Interrupt handler
*/
#ifndef TIMER2_OVF_vect
// for older WinAVR versions e.g. WinAVR-20071221 
#define TIMER2_OVF_vect TIMER2_OVF0_vect
#endif
 
ISR (TIMER2_OVF_vect)
{
  /* 8-bit timer = 256 werte 
  Interrupt Aktion alle 
  (20000000/1024)/256 Hz = 76,2939453125 Hz
  bzw.
  1/ 76,2939453125  s = 13,1072 ms  
  */
  
  /* since sysTick is 16 bit = 65535 values
  and we have an interrupt duration of 13,1072 ms
  the sysTick value wraps every
  13,1072 ms * 65535 values = 858,980352 sec = 14,3163392 min
  */
  time_sysTick++;
  screensaver_timer++;
}
//---------------------------------------------
void time_initTimer()
{
	// Config Timer0
	//TCCR2A = ; // Prescaler 1024
	TCCR2B = (1<<CS22) | (1<<CS20) | (1<<CS21); // Prescaler 1024
 
	// enable Overflow Interrupt 
	TIMSK2 |= (1<<TOIE2);
};
//---------------------------------------------