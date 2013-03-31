/*
 * FIFO.c
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

#include "FIFO.h"
#include "string.h"

//-------------------------------------------------------------------------
void fifo_init(Fifo* fifo)
{
	memset(fifo->data,0,BUFFER_SIZE);
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
void fifoBig_init(FifoBig* fifo)
{
	memset(fifo->data,0,BUFFER_SIZE_BIG);
	fifo->read = 0;
	fifo->write = 0;
}
//-------------------------------------------------------------------------
uint8_t fifoBig_bufferIn(FifoBig* fifo, uint8_t byte)
{
  uint8_t next = ((fifo->write + 1) & BUFFER_MASK_BIG);
  if (fifo->read == next)
    return 0;
  fifo->data[fifo->write] = byte;
  fifo->write = next;
  return 1;
}
//-------------------------------------------------------------------------
uint8_t fifoBig_bufferOut(FifoBig* fifo, uint8_t *pByte)
{
  if (fifo->read == fifo->write)
    return 0;
  *pByte = fifo->data[fifo->read];
  fifo->read = (fifo->read+1) & BUFFER_MASK_BIG;
  return 1;
}
//-------------------------------------------------------------------------
void fifoBig_clear(FifoBig* fifo)
{
	fifo->read = fifo->write;
}
//----------------------------------------------------
