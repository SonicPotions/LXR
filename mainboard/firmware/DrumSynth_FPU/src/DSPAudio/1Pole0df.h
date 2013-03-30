/*
 * 1Pole0df.h
 *
 *  Created on: 11.01.2013
 *      Author: Julian
 */

#ifndef POLE0DF_H_
#define POLE0DF_H_

#include "stm32f4xx.h"

/*
 t = T(0.5*(x[n] + x[n-1]) - s[n])
 y[n+1] = (s[n] + f*t*x[n]) / (1 + f*t)
 s[n+1] = s[n] + 2*f*t*x[n]
 */
typedef struct OnePole0dfStruct
{
	float y;
	float s;

	float f;
	float zi;
} OnePole0dfFilter;


void initOnePole0df(OnePole0dfFilter* filter);


float calcOnePole0df(OnePole0dfFilter* filter, float x);

void setOnePoleFreq0df(OnePole0dfFilter* filter,float val);

#endif /* POLE0DF_H_ */
