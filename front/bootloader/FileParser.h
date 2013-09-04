/*
 * FileParser.h
 *
 * Created: 07.07.2012 17:57:25
 *  Author: Julian
 */ 


#ifndef FILEPARSER_H_
#define FILEPARSER_H_

#include <avr/io.h>
#include "UART_routines.h"
#include "config.h"

/************************************************************************/
/*  a firmware file consists of 3 elements
	1) a info header
	2) the AVR firmware
	3) the cortex firmware                                              
	
	the AVR and Cortex firmware needs to be zero padded for full sektor reads (512 bytes)*/
/************************************************************************/

struct InfoHeader
{
	char		headerId[4];	// should be SPFI (Sonic Potions Firmware Image)
	uint16_t	avrCodeSize;	// in byte
	uint32_t	cortexCodeSize;	//in byte
};

uint8_t fileParser_parseNextBlock(unsigned long filesize);
void fileParser_resetCortex();

#endif /* FILEPARSER_H_ */