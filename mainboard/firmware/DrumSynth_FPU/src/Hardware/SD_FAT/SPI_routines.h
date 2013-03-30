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
#include "config.h"
#if USE_SD_CARD
#include "stm32f4xx.h"

/*
#define SPI_SD             SPCR = 0x52 = 1010010 = SPR1 | MSTR | SPE // SPI enabled, Master mode, OSC / 64 (8mHz/64=125kHz)
#define SPI_HIGH_SPEED     SPCR = 0x50 = 1010000 =  MSTR | SPE ; SPSR |= (1<<SPI2X) // SPI enabled, Master mode, OSC / 4 (8mHz/64=2mHz), double spi speed
*/
static SPI_InitTypeDef SPI_InitStructure;

#define SPI_SD			SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;SPI_Init(SPI1, &SPI_InitStructure);
#define SPI_HIGH_SPEED 	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;SPI_Init(SPI1, &SPI_InitStructure);


#define SPI_MISO_MOSI_PORT	GPIOA
#define SPI_CS_SCK_PORT		GPIOB
#define SPI_MISO_PIN		GPIO_Pin_6	//a
#define SPI_MOSI_PIN		GPIO_Pin_7	//a
#define SPI_SCK_PIN			GPIO_Pin_3  //b
#define SPI_CS_PIN			GPIO_Pin_0  //b

#define SPI_MISO_PIN_SRC	GPIO_PinSource6	//a
#define SPI_MOSI_PIN_SRC	GPIO_PinSource7	//a
#define SPI_SCK_PIN_SRC		GPIO_PinSource3  //b
#define SPI_CS_PIN_SRC		GPIO_PinSource0  //b


void spi_init(void);
unsigned char SPI_transmit(unsigned char);
unsigned char SPI_receive(void);

#endif
#endif
