/*
 * fifo.h
 *
 * Created: 25.04.2012 13:44:53
 *  Author: Julian
 */ 


#ifndef FIFO_H_
#define FIFO_H_

#include <avr/io.h>
#include <string.h>

#define BUFFER_SIZE 256 //must be power of 2!!! (8, 16, 32, 64 ...)
#define BUFFER_MASK (BUFFER_SIZE-1) 
//---------------------------------------------------------------------------- 
typedef struct FifoStruct {
  volatile uint8_t data[BUFFER_SIZE];
  volatile uint8_t read; 
  volatile uint8_t write;
  } FifoBuffer;
//----------------------------------------------------------------------------
void fifo_init(FifoBuffer* fifo);
//----------------------------------------------------------------------------
uint8_t fifo_bufferIn(FifoBuffer* fifo, uint8_t byte);
//----------------------------------------------------------------------------
uint8_t fifo_BufferOut(FifoBuffer* fifo, uint8_t *pByte);
//----------------------------------------------------------------------------

#endif /* FIFO_H_ */