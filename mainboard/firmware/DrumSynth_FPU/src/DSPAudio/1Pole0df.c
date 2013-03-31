/*
 * 1Pole0df.c
 *
 *  Created on: 11.01.2013
 *      Author: Julian
 */

#if 0
#include "globals.h"
#include "config.h"
#include "stm32f4xx.h"
#include "1Pole0df.h"
#include <math.h>



void initOnePole0df(OnePole0dfFilter* filter)
{
	filter->y = filter->s = 0.f;
	filter->f = 0.5f;
	filter->zi = 0;
}

float tanhXdX_(float x)
{

	//return 1 - 0.25 * fabsf(x);

	float a = x*x;
    // IIRC I got this as Pade-approx for tanh(sqrt(x))/sqrt(x)
	x = ((a + 105)*a + 945) / ((15*a + 420)*a + 945);

	return x;
}
float calcOnePole0df(OnePole0dfFilter* filter, float x)
{


		const float ih = 0.5f * (x + filter->zi);
		filter->zi = x;

		//calc nonlinear gain
		//float t = 1;
		const float offset = 0;//0.01f * ((M_PI*0.5f)-filter->f); //offset increases as filter freq is reduced
		const float t = tanhXdX_(ih - filter->s + offset);

	//	if((1 + filter->f*t) != 0)
			filter->y = (filter->s + filter->f*t*x) / (1 + filter->f*t);
	//	else
		//	filter->y = (filter->s + filter->f*t*x) / (0.000001f);
		 filter->s = filter->s + 2*filter->f*t*x;

		 return filter->y;

}

void setOnePoleFreq0df(OnePole0dfFilter* filter,float val)
{
	filter->f = val;
}

#endif

