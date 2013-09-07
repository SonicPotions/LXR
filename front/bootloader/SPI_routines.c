#include <avr/io.h>
#include "SPI_routines.h"

//SPI initialize for SD card
void spi_init(void)
{
	
	//init port
	DDRB  |= (1<<PB7) | (1<<PB5) | (1<<PB4) ;
	PORTB |= 0;
		
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1); //clock FCPU/64 = 20000000/64= 312.5kHz
	SPSR = 0x00;
}

unsigned char SPI_transmit(unsigned char data)
{
// Start transmission
SPDR = data;

// Wait for transmission complete
while(!(SPSR & (1<<SPIF)));
data = SPDR;

return(data);
}

unsigned char SPI_receive(void)
{
unsigned char data;
// Wait for reception complete

SPDR = 0xff;
while(!(SPSR & (1<<SPIF)));
data = SPDR;

// Return data register
return data;
}

