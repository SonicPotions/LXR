#ifndef _SPI_ROUTINES_H_
#define _SPI_ROUTINES_H_

#define SPI_SD              SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1);SPSR=0; //SPSR &= ~(1<<SPI2X)
#define SPI_HIGH_SPEED      SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0); SPSR = (1<<SPI2X);
#define SPI_LOW_SPEED		SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1); SPSR = 0x00;


void spi_init(void);
unsigned char SPI_transmit(unsigned char);
unsigned char SPI_receive(void);

#endif
