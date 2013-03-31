/*
 * valueShaper.h
 *
 *  Created on: 25.01.2013
 *      Author: Julian
 */

#ifndef VALUESHAPER_H_
#define VALUESHAPER_H_
//-------------------------------------------------------------
#include "stm32f4xx.h"
#include <math.h>
//-------------------------------------------------------------
//shape is a value between -1 and 1
static inline float valueShaperI2F(uint8_t data2, float shape)
{

	const float k = 2*shape/(1.0001f-shape);
	const float val = (data2)/127.f;
	return  ((1+k)*val/(1+k*fabsf(val)));
}
//-------------------------------------------------------------
static inline float valueShaperF2F(float val, float shape)
{
	const float k = 2*shape/(1.0001f-shape);
	return  ((1+k)*val/(1+k*fabsf(val)));
}
//-------------------------------------------------------------
#endif /* VALUESHAPER_H_ */
