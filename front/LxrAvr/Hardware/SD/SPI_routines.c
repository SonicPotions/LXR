//**************************************************************
// ****** FUNCTIONS FOR SPI COMMUNICATION *******
//**************************************************************
//Controller: ATmega32 (Clock: 8 Mhz-internal)
//Compiler	: AVR-GCC (winAVR with AVRStudio)
//Project V.: Version - 2.4.1
//Author	: CC Dharmani, Chennai (India)
//			  www.dharmanitech.com
//Date		: 24 Apr 2011
//**************************************************************

#include <avr/io.h>
#include "SPI_routines.h"



//SPI initialize for SD card
//clock rate: 125Khz
void spi_init(void)
{
	
	//init port
	DDRB  |= (1<<PB7) | (1<<PB5) | (1<<PB4) ;//0xBF; //MISO line i/p, rest o/p
	PORTB |= 0;//(1<<PB7) | (1<<PB6) | (1<<PB5);//0xEF
	

	
	
	//SPCR = 0x52; //setup SPI: Master mode, MSB first, SCK phase low, SCK idle low
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1); //clock FCPU/64 = 20000000/64= 312.5kHz
	SPSR = 0x00;
}

void spi_deInit()
{
	//disable SPI
	SPCR &= (uint8_t)~(1<<SPE);
	//all SPI ports as input
	DDRB &= (uint8_t)~((1<<PB5) | (1<<PB4) |(1<<PB7) );
	PORTB &= (uint8_t)~((1<<PB5) | (1<<PB4) |(1<<PB7) );

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

//******** END ****** www.dharmanitech.com *****
