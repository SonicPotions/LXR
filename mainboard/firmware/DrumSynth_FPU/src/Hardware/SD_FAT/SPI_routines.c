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

//#include <avr/io.h>
#include "config.h"
#if USE_SD_CARD
#include "SPI_routines.h"

//-------------------------------------------------------------------------
void spi_initInterface()
{


	/* Enable the SPI peripheral clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE); //APB2 Prescaler 2 = 168000000/2 = 84000000 = 84mHz

	/* SPI1 peripheral configuration */
	SPI_I2S_DeInit(SPI1);

	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256; //84000000/256 = 328125 Hz = 328kHz
	SPI_InitStructure.SPI_CPHA				= SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_CPOL				= SPI_CPOL_High;
	SPI_InitStructure.SPI_CRCPolynomial		= 7;
	SPI_InitStructure.SPI_DataSize			= SPI_DataSize_8b;
	SPI_InitStructure.SPI_Direction			= SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_FirstBit			= SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_Mode				= SPI_Mode_Master;
	SPI_InitStructure.SPI_NSS				= SPI_NSS_Soft;

	/* Initialize the SPI1 peripheral with the structure above */
	SPI_Init(SPI1, &SPI_InitStructure);

	SPI_Cmd(SPI1,ENABLE);
}

//-------------------------------------------------------------------------
void spi_gpioInit()
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable spi port clock */
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB, ENABLE);

	// init the pins
	//MISO and MOSI
	GPIO_InitStructure.GPIO_Pin = SPI_MISO_PIN | SPI_MOSI_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(SPI_MISO_MOSI_PORT, &GPIO_InitStructure);
	//SCK pins
	GPIO_InitStructure.GPIO_Pin = SPI_SCK_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(SPI_CS_SCK_PORT, &GPIO_InitStructure);
	//CS
	GPIO_InitStructure.GPIO_Pin = SPI_CS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(SPI_CS_SCK_PORT, &GPIO_InitStructure);

	//configure pins to alternate function
	GPIO_PinAFConfig(SPI_MISO_MOSI_PORT, 	SPI_MISO_PIN_SRC, 	GPIO_AF_SPI1);
	GPIO_PinAFConfig(SPI_MISO_MOSI_PORT, 	SPI_MOSI_PIN_SRC, 	GPIO_AF_SPI1);
	GPIO_PinAFConfig(SPI_CS_SCK_PORT, 		SPI_SCK_PIN_SRC, 	GPIO_AF_SPI1);
}
//-------------------------------------------------------------------------
//SPI initialize for SD card
//clock rate: 125Khz
void spi_init(void)
{
	
	//init port
	//DDRB  |= (1<<PB7) | (1<<PB5) | (1<<PB4) ;//0xBF; //MISO line i/p, rest o/p
	//PORTB |= (1<<PB7) | (1<<PB6) | (1<<PB5);//0xEF
	spi_gpioInit();
	
	
	
	//SPCR = 0x52; //setup SPI: Master mode, MSB first, SCK phase low, SCK idle low
	//SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1); //clock FCPU/64 = 8000000/64= 125kHz
	//SPSR = 0x00;
	spi_initInterface();
}

unsigned char SPI_transmit(unsigned char data)
{

	/*
	// Start transmission
	SPDR = data;

	// Wait for transmission complete
	while(!(SPSR & (1<<SPIF)));
	data = SPDR;

	return(data);
	*/





	/*!< Loop while DR register in not empty */
		while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);

	/*!< Send byte through the SPI1 peripheral */
	SPI_I2S_SendData(SPI1, data);

	/*!< Wait to receive a byte */
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);

	/*!< Return the byte read from the SPI bus */
	return SPI_I2S_ReceiveData(SPI1);
}

unsigned char SPI_receive(void)
{

	/*
	unsigned char data;
	// Wait for reception complete

	SPDR = 0xff;
	while(!(SPSR & (1<<SPIF)));
	data = SPDR;

	// Return data register
	return data;
	*/
	return (SPI_transmit(0xff));
}
#endif
//******** END ****** www.dharmanitech.com *****
