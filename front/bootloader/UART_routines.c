

#include "UART_routines.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "Bootloader/BootloaderCommands.h"
#include <util/delay.h>

//**************************************************
//UART0 initialize
//baud rate: 19200  (for controller clock = 8MHz)
//char size: 8 bit
//parity: Disabled
//**************************************************
void uart_init(void)
{
#ifdef MEGA32 		
	UBRRH = UBRR_VAL >> 8;
	UBRRL = UBRR_VAL & 0xFF;
	
	UCSRB |= (1<<RXEN)|(1<<TXEN);
	UCSRC = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);	// Asynchron 8N1 
#else
   UBRR0H = UBRRH_VALUE;
   UBRR0L = UBRRL_VALUE;
	
	#if USE_2X
	   /* U2X-Modus erforderlich */
	   UCSR0A |= (1 << U2X0);
	#else
	   /* U2X-Modus nicht erforderlich */
	   UCSR0A &= ~(1 << U2X0);
	#endif
	
	UCSR0B |= (1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (1<<UCSZ00)|(1<<UCSZ01);	// Asynchron 8N1 
	
	//UCSR0B |= (1<<RXEN0)|(1<<TXEN0);
	//UCSR0C = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);	// Asynchron 8N1 
#endif	
}

//**************************************************
//Function to receive a single byte
//*************************************************
unsigned char uart_rxWait( void )
{
#ifdef MEGA32 	
	while(!(UCSRA & (1<<RXC))); 	// Wait for incomming data
	//return received data
	return UDR;
#else
	while(!(UCSR0A & (1<<RXC0))); 	// Wait for incomming data
	//return received data
	return UDR0;
#endif
	

}

//***************************************************
//Function to transmit a single byte
//***************************************************
void uart_tx( unsigned char data )
{
#ifdef MEGA32 		
	while ( !(UCSRA & (1<<UDRE)) )
	{
		// Wait for empty tx buffer 
	}
	// send byte
	UDR = data; 	
#else
	while ( !(UCSR0A & (1<<UDRE0)) )
	{
		// Wait for empty tx buffer 
	}
	// send byte
	UDR0 = data; 	
#endif	
        
}
//----------------------------------------------------------
uint8_t uart_checkAck()
{

	uint8_t data = uart_rxWait();	
						
	if(data == ACK)
	{
		return ACK;
	}
	else if(data == NACK)
	{
		while(1);
		return NACK;
	}

	lcd_setcursor(0,2);
				lcd_string("no ack");
				while(1);
	return 0;
}
