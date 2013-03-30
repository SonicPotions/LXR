/*
 * 1PoleLp.c
 *
 *  Created on: 08.04.2012
 *      Author: Julian
 */
#include "globals.h"
#include "config.h"
#include "stm32f4xx.h"
#include "1PoleLp.h"

#if 1


void initOnePole(OnePoleFilter* filter)
{
	filter->b = 0.f;
	filter->r = 0.01f;
}

float calcOnePole(OnePoleFilter* filter, float input)
{
	filter->b += filter->r * (input - filter->b);
	return filter->b;
}

void calcOnePoleBlock(OnePoleFilter* filter, int16_t* buf, const uint8_t size)
{

	uint8_t i;
	for(i=0;i<size;i++)
	{
		filter->b += filter->r * (buf[i] - filter->b);
		buf[i] = filter->b;
	}
}

void calcOnePoleBlockFixedInput(OnePoleFilter* filter, float input,float* output, const uint8_t size)
{

	uint8_t i;
	for(i=0;i<size;i++)
	{
		filter->b += filter->r * (input - filter->b);
		output[i] = filter->b;
	}
}

void setOnePoleCoef(OnePoleFilter* filter,float val)
{
	filter->r = val;
}

void setOnePoleValue(OnePoleFilter* filter,float val)
{

	filter->b = val;
}
#endif
