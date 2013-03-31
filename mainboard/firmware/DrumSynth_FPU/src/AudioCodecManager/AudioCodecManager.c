/*
 *  ------------------------------------------------------------------------------
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
 */

#include "AudioCodecManager.h"

#include <string.h>
#include "datatypes.h"
//------------------------------------------------------------------
#if DMA_MODE_ACTIVE
volatile int16_t dma_buffer[OUTPUT_DMA_SIZE*4];
volatile uint8_t dmaBufferPtr=0;
 volatile uint8_t doubleBufferPos=0;

 volatile int16_t dma_buffer2[OUTPUT_DMA_SIZE*4];
#endif

uint8_t bCurrentSampleValid = 0;
//__IO uint8_t codec_volume = 95;
int16_t audioOutBuffer[2];
//------------------------------------------------------------------
int CodecInit(uint32_t AudioFreq)
{
    dma_buffer[0] = 32756;
    codec_initCsCodec((uint32_t)dma_buffer, OUTPUT_DMA_SIZE*2,(uint32_t)dma_buffer2, OUTPUT_DMA_SIZE*2);
    return 0;
}
//------------------------------------------------------------------
