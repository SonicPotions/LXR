

#ifndef _UART_ROUTINES_H_
#define _UART_ROUTINES_H_

#include "config.h"





/* 
  UART-Init: 
Berechnung des Wertes für das Baudratenregister 
aus Taktrate und gewünschter Baudrate
*/
 
#ifndef F_CPU
/* In neueren Version der WinAVR/Mfile Makefile-Vorlage kann
   F_CPU im Makefile definiert werden, eine nochmalige Definition
   hier wuerde zu einer Compilerwarnung fuehren. Daher "Schutz" durch
   #ifndef/#endif 
 
   Dieser "Schutz" kann zu Debugsessions führen, wenn AVRStudio 
   verwendet wird und dort eine andere, nicht zur Hardware passende 
   Taktrate eingestellt ist: Dann wird die folgende Definition 
   nicht verwendet, sondern stattdessen der Defaultwert (8 MHz?) 
   von AVRStudio - daher Ausgabe einer Warnung falls F_CPU
   noch nicht definiert: */
#warning "F_CPU war noch nicht definiert, wird nun nachgeholt mit 8000000"
#define F_CPU 20000000UL  // Systemtakt in Hz - Definition als unsigned long beachten 
                         // Ohne ergeben sich unten Fehler in der Berechnung
#endif
 
#define BAUD 500000UL//76800UL      // Baudrate 1M

#include <util/setbaud.h>

/*
 
// Berechnungen
#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)   // clever runden
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))     // Reale Baudrate
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUD) // Fehler in Promille, 1000 = kein Fehler.
 
#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))
  #error Systematischer Fehler der Baudrate grösser 1% und damit zu hoch! 
#endif 
*/

void uart_init(void);
unsigned char uart_rxWait(void);
void uart_tx(unsigned char);

//check if an ACK message is received.
//returns 1 if ack is received
//returns 0 if waiting times out
uint8_t uart_checkAck();


#endif