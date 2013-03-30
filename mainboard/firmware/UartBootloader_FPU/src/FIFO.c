/*
 * FIFO.c
 *
 *  Created on: 20.04.2012
 *      Author: Julian
 */

#include "FIFO.h"
#include "string.h"

//-------------------------------------------------------------------------
void fifo_init(Fifo* fifo)
{
	memset((void*)fifo->data,0,BUFFER_SIZE);
	fifo->read = 0;
	fifo->write = 0;
}
//-------------------------------------------------------------------------
uint8_t fifo_bufferIn(Fifo* fifo, uint8_t byte)
{
  uint8_t next = ((fifo->write + 1) & BUFFER_MASK);
  if (fifo->read == next)
    return 0;
  fifo->data[fifo->write] = byte;
  // buffer.data[buffer.write & BUFFER_MASK] = byte; // absolut Sicher
  fifo->write = next;
  return 1;
}
//-------------------------------------------------------------------------
uint8_t fifo_bufferOut(Fifo* fifo, uint8_t *pByte)
{
  if (fifo->read == fifo->write)
    return 0;
  *pByte = fifo->data[fifo->read];
  fifo->read = (fifo->read+1) & BUFFER_MASK;
  return 1;
}
//-------------------------------------------------------------------------
void fifo_clear(Fifo* fifo)
{
	fifo->read = fifo->write;
}
//-------------------------------------------------------------------------
