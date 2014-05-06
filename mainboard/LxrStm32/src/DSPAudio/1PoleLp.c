/*
 * 1PoleLp.c
 *
 *  Created on: 08.04.2012
 * ------------------------------------------------------------------------------------------------------------------------
 *  Copyright 2013 Julian Schmidt
 *  Julian@sonic-potions.com
 * ------------------------------------------------------------------------------------------------------------------------
 *  This file is part of the Sonic Potions LXR drumsynth firmware.
 * ------------------------------------------------------------------------------------------------------------------------
 *  Redistribution and use of the LXR code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *       - The code may not be sold, nor may it be used in a commercial product or activity.
 *
 *       - Redistributions that are modified from the original source must include the complete
 *         source code, including the source code for all components used by a binary built
 *         from the modified sources. However, as a special exception, the source code distributed
 *         need not include anything that is normally distributed (in either source or binary form)
 *         with the major components (compiler, kernel, and so on) of the operating system on which
 *         the executable runs, unless that component itself accompanies the executable.
 *
 *       - Redistributions must reproduce the above copyright notice, this list of conditions and the
 *         following disclaimer in the documentation and/or other materials provided with the distribution.
 * ------------------------------------------------------------------------------------------------------------------------
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *   USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ------------------------------------------------------------------------------------------------------------------------
 */

//-------------------------------------------------------------
#include "globals.h"
#include "config.h"
#include "stm32f4xx.h"
#include "1PoleLp.h"
//-------------------------------------------------------------
void initOnePole(OnePoleFilter* filter)
{
	filter->b = 0.f;
	filter->r = 0.01f;
}
//-------------------------------------------------------------
float calcOnePole(OnePoleFilter* filter, float input)
{
	filter->b += filter->r * (input - filter->b);
	return filter->b;
}
//-------------------------------------------------------------
void calcOnePoleBlock(OnePoleFilter* filter, int16_t* buf, const uint8_t size)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		filter->b += filter->r * (buf[i] - filter->b);
		buf[i] = filter->b;
	}
}
//-------------------------------------------------------------
void calcOnePoleBlockFixedInput(OnePoleFilter* filter, float input,float* output, const uint8_t size)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		filter->b += filter->r * (input - filter->b);
		output[i] = filter->b;
	}
}
//-------------------------------------------------------------
void setOnePoleCoef(OnePoleFilter* filter,float val)
{
	filter->r = val;
}
//-------------------------------------------------------------
void setOnePoleValue(OnePoleFilter* filter,float val)
{
	filter->b = val;
}
//-------------------------------------------------------------

