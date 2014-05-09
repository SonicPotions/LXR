/*
 * uart.c
 *
 * Created: 26.04.2012 14:28:31
 *  Author: Julian
 */ 

#include "uart.h"
#include <avr/io.h>
#include "../frontPanelParser.h"
#include "../config.h"
#if UART_DEBUG_ECHO_MODE
#include <stdlib.h>
#include "../Hardware/lcd.h"
#endif
#if 1
/* 
  UART-Init: 
Berechnung des Wertes f�r das Baudratenregister 
aus Taktrate und gew�nschter Baudrate
*/
 
#ifndef F_CPU
/* In neueren Version der WinAVR/Mfile Makefile-Vorlage kann
   F_CPU im Makefile definiert werden, eine nochmalige Definition
   hier wuerde zu einer Compilerwarnung fuehren. Daher "Schutz" durch
   #ifndef/#endif 
 
   Dieser "Schutz" kann zu Debugsessions f�hren, wenn AVRStudio 
   verwendet wird und dort eine andere, nicht zur Hardware passende 
   Taktrate eingestellt ist: Dann wird die folgende Definition 
   nicht verwendet, sondern stattdessen der Defaultwert (8 MHz?) 
   von AVRStudio - daher Ausgabe einer Warnung falls F_CPU
   noch nicht definiert: */
#warning "F_CPU war noch nicht definiert, wird nun nachgeholt mit 8000000"
#define F_CPU 20000000UL  // Systemtakt in Hz - Definition als unsigned long beachten 
                         // Ohne ergeben sich unten Fehler in der Berechnung
#endif
 
//#define BAUD 1000000UL//76800UL      // Baudrate 1M
#define BAUD    500000UL//76800UL      // Baudrate 500k

#include <util/setbaud.h>
//#define BAUD 38400UL      // Baudrate 1M
 
 /*
// Berechnungen
#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)   // clever runden
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))     // Reale Baudrate
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUD) // Fehler in Promille, 1000 = kein Fehler.
 
#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))
  #error Systematischer Fehler der Baudrate gr�sser 1% und damit zu hoch! 
#endif 

*/
//----------------------------------------------------
//working vars
//----------------------------------------------------
 FifoBuffer uart_txBuffer;
 FifoBuffer uart_rxBuffer;

//----------------------------------------------------
//interrupt service routines
//----------------------------------------------------

//RX complete - called when the UART has received some data
//ISR(SIG_UART_RECV) 

ISR(USART0_RX_vect) 
{
    // Interrupt Code 
	
	//put the received data in the rx fifo
	//fifo_bufferIn(&uart_rxBuffer,UDR);
	fifo_bufferIn(&uart_rxBuffer,UDR0);
	
}


//----------------------------------------------------
// USART Data Register Empty - called when the UART is ready to transmit data 
//(enabled by UDRIE bit in UCSRB register)
//ISR(SIG_UART_DATA) 
ISR(USART0_UDRE_vect ) 
{
    /* Interrupt Code */
	uint8_t data;
	if(fifo_BufferOut(&uart_txBuffer,&data))
	{
		//if there is still data in the tx buffer
		//UDR = data; // send the byte
		UDR0 = data; // send the byte
	}
	else
	{
		//no data left -> disable the data register empty interrupt
		//UCSRB &= ~(1<<UDRIE);	
		UCSR0B &= (uint8_t)(~(1<<UDRIE0));
	}
}
//----------------------------------------------------
void uart_init()
{
	fifo_init(&uart_txBuffer);
	fifo_init(&uart_rxBuffer);
	
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
   
#if USE_2X
   /* U2X-Modus erforderlich */
   UCSR0A |= (1 << U2X0);
#else
   /* U2X-Modus nicht erforderlich */
   UCSR0A &= ~(1 << U2X0);
#endif

//	UCSRB |= (1<<RXEN)|(1<<TXEN)|(1<<RXCIE);  // enable UART RX, TX and RX Interrupt 
	UCSR0B |= (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);  // enable UART RX, TX and RX Interrupt 
	//UCSRC = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);	// Asynchron 8N1 
	UCSR0C = (1<<UCSZ00)|(1<<UCSZ01);	// Asynchron 8N1 
};


//----------------------------------------------------
//output one byte on the uart
uint8_t uart_putc(unsigned char c)
{
	uint8_t ret = fifo_bufferIn(&uart_txBuffer,c);
    	
	//we've put new data into the tx fifo, so we need to enable the tx interrupt to send it out
	//UCSRB |= (1<<UDRIE);	
	UCSR0B |= (1<<UDRIE0);	
	
	return ret;
}

//----------------------------------------------------
//output a string (zero terminated!) on the uart
void uart_puts (char *s)
{
    while (*s)
    {   /* so lange *s != '\0' also ungleich dem "String-Endezeichen(Terminator)" */
        uart_putc(*s);
        s++;
    }
}

//----------------------------------------------------
//receive byte
//returns 1 if there is data to be read, else 0 if there is no data
uint8_t uart_getc(uint8_t *data)
{
	if(fifo_BufferOut(&uart_rxBuffer,data))
	{
		//there is valid data present
		return 1;
	}
	//no data to be read
	return 0;
}
//----------------------------------------------------
uint8_t uart_waitAck()
{
	//TODO: timeout
	//UCSR0B &= ~(1<<RXCIE0);  // disable RX Interrupt

	while(1)
	{
		uint8_t data;
		uint8_t ret = uart_getc(&data);
		if(ret)
		{
			//if(data == ACK || data == NACK)
			{
				//UCSR0B |= (1<<RXCIE0);  // enable RX Interrupt
				return data;
			}
		}
	}




}
//----------------------------------------------------
void uart_checkAndParse()
{
#if UART_DEBUG_ECHO_MODE
	uint8_t data;
	if(uart_getc(&data))
	{
		//print received data on LCD
		char text[5];
		itoa(data,text,10);
		lcd_clear();
		lcd_home();
		lcd_string(text);
		
	}

#else
	uint8_t data;
	if(uart_getc(&data))
	{
		//there is new data available
		frontPanel_parseData(data);
	}
#endif
};
//----------------------------------------------------

#endif
