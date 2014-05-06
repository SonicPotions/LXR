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

#ifndef _SPI_ROUTINES_H_
#define _SPI_ROUTINES_H_

#define SPI_SD              SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1);SPSR=0; //SPSR &= ~(1<<SPI2X)
#define SPI_HIGH_SPEED      SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0); SPSR = (1<<SPI2X);
#define SPI_LOW_SPEED		SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1); SPSR = 0x00;


void spi_init(void);
void spi_deInit();
unsigned char SPI_transmit(unsigned char);
unsigned char SPI_receive(void);


#endif
