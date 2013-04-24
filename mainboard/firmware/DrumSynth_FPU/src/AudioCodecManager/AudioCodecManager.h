/*
 * AudioCodecManager.h
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
 *  Created on: 01.04.2012
 *      Author: Julian
 */

#ifndef AUDIOCODECMANAGER_H_
#define AUDIOCODECMANAGER_H_

#include "stm32f4xx.h"
#include "config.h"
#include "globals.h"
#include "cs4344_cs5343.h"



#define DMA_MODE_ACTIVE 1

//-----------------------------------

#if DMA_MODE_ACTIVE

												// and want to do double buffering = *2
//extern volatile uint8_t dmaBufferPtr;
//extern volatile uint8_t doubleBufferPos;
#endif


//-----------------------------------
int CodecInit(uint32_t AudioFreq);

#endif /* AUDIOCODECMANAGER_H_ */
