/*
 * FIFO.h
 *
 * ------------------------------------------------------------------------------
 *
 *  This file is part of the "Cortex Hardware Audio Operating System (CHAOS)".
 *
 *	Copyright 2012 Julian Schmidt / Sonic Potions
 *	http://www.sonic-potions.com
 *
 *  ------------------------------------------------------------------------------
 *
 *	CHAOS is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	CHAOS is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with CHAOS.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	------------------------------------------------------------------------------
 *
 *
 *  Created on: 20.04.2012
 *      Author: Julian
 */

#ifndef FIFO_H_
#define FIFO_H_

#include "globals.h"

#define BUFFER_SIZE 64 // muss 2^n betragen (8, 16, 32, 64 ...)
#define BUFFER_MASK (BUFFER_SIZE-1) // Klammern auf keinen Fall vergessen

typedef struct FifoStruct
{
	  volatile uint8_t data[BUFFER_SIZE];
	  volatile uint8_t read; // zeigt auf das Feld mit dem ältesten Inhalt
	  volatile uint8_t write; // zeigt immer auf leeres Feld
}Fifo;


void fifo_init(Fifo* fifo);

uint8_t fifo_bufferIn(Fifo* fifo, uint8_t byte);

uint8_t fifo_bufferOut(Fifo* fifo, uint8_t *pByte);

void fifo_clear(Fifo* fifo);


//--
#define BUFFER_SIZE_BIG 64 // muss 2^n betragen (8, 16, 32, 64 ...)
#define BUFFER_MASK_BIG (BUFFER_SIZE_BIG-1) // Klammern auf keinen Fall vergessen

typedef struct FifoStructBig
{
	volatile  uint8_t data[BUFFER_SIZE_BIG];
	volatile uint8_t read; // zeigt auf das Feld mit dem ältesten Inhalt
	volatile uint8_t write; // zeigt immer auf leeres Feld
}FifoBig;


void fifoBig_init(FifoBig* fifo);

uint8_t fifoBig_bufferIn(FifoBig* fifo, uint8_t byte);

uint8_t fifoBig_bufferOut(FifoBig* fifo, uint8_t *pByte);

void fifoBig_clear(FifoBig* fifo);


#endif /* FIFO_H_ */
