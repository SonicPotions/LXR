//**************************************************************
// ****** FUNCTIONS FOR SD RAW DATA TRANSFER *******
//**************************************************************
//Controller: ATmega32 (Clock: 8 Mhz-internal)
//Compiler	: AVR-GCC (winAVR with AVRStudio)
//Project V.: Version - 2.4.1
//Author	: CC Dharmani, Chennai (India)
//			  www.dharmanitech.com
//Date		: 24 Apr 2011
//
//modified	: Julian Schmidt
//Date		: 9.5.2012
//**************************************************************

//Link to the Post: http://www.dharmanitech.com/2009/01/sd-card-interfacing-with-atmega8-fat32.html

#ifndef _SD_ROUTINES_H_
#define _SD_ROUTINES_H_
#include "config.h"
#if USE_SD_CARD
#include "stm32f4xx.h"

//Use following macro if you don't want to activate the multiple block access functions
//those functions are not required for FAT32

#define FAT_TESTING_ONLY

//use following macros if PB1 pin is used for Chip Select of SD
//#define SD_CS_ASSERT     PORTB &= ~0x02
//#define SD_CS_DEASSERT   PORTB |= 0x02

//pin used for Chip Select of SD
#define SD_CS_GPIO_PORT			GPIOB
#define SD_CS_GPIO_PIN			GPIO_Pin_0

#define SD_CS_ASSERT     GPIO_ResetBits(SD_CS_GPIO_PORT,SD_CS_GPIO_PIN) //SD_CS_GPIO_PORT->ODR &= ~(1<<SD_CS_GPIO_PIN)
#define SD_CS_DEASSERT   GPIO_SetBits(SD_CS_GPIO_PORT,SD_CS_GPIO_PIN)// (SD_CS_GPIO_PORT->ODR |= (1<<SD_CS_GPIO_PIN)



//SD commands, many of these are not used here
#define GO_IDLE_STATE            0
#define SEND_OP_COND             1
#define SEND_IF_COND			 8
#define SEND_CSD                 9
#define STOP_TRANSMISSION        12
#define SEND_STATUS              13
#define SET_BLOCK_LEN            16
#define READ_SINGLE_BLOCK        17
#define READ_MULTIPLE_BLOCKS     18
#define WRITE_SINGLE_BLOCK       24
#define WRITE_MULTIPLE_BLOCKS    25
#define ERASE_BLOCK_START_ADDR   32
#define ERASE_BLOCK_END_ADDR     33
#define ERASE_SELECTED_BLOCKS    38
#define SD_SEND_OP_COND			 41   //ACMD
#define APP_CMD					 55
#define READ_OCR				 58
#define CRC_ON_OFF               59


#define ON     1
#define OFF    0

volatile unsigned long startBlock, totalBlocks; 
volatile unsigned char SDHC_flag, cardType;
//volatile unsigned char buffer[512];

unsigned char SD_init(void);
unsigned char SD_sendCommand(unsigned char cmd, unsigned long arg);
//unsigned char SD_readSingleBlock(unsigned long startBlock);
unsigned char SD_readSingleBlockCustomBuffer(unsigned long startBlock, uint8_t *target);
//unsigned char SD_writeSingleBlock(unsigned long startBlock);
unsigned char SD_writeSingleBlockCustomBuffer(unsigned long startBlock, uint8_t *source);
//unsigned char SD_readMultipleBlock (unsigned long startBlock, unsigned long totalBlocks);
//unsigned char SD_writeMultipleBlock(unsigned long startBlock, unsigned long totalBlocks);
unsigned char SD_erase (unsigned long startBlock, unsigned long totalBlocks);


#endif
#endif
