/*
 * FIFO.h
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


#endif /* FIFO_H_ */
