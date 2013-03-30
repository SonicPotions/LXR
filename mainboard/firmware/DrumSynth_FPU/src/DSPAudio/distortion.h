/*
 * distortion.h
 *
 *  Created on: 11.04.2012
 *      Author: Julian
 */

#ifndef DISTORTION_H_
#define DISTORTION_H_

#include "stm32f4xx.h"

typedef struct DistStruct
{
	float shape;
	float inv_shape;


}Distortion;


/*
 amount should be in [-1..1[ Plot it and stand back in astonishment! ;)

Code :
x = input in [-1..1]
y = output
k = 2*amount/(1-amount);

f(x) = (1+k)*x/(1+k*abs(x))
 */
//--------------------------------------------------
inline float fastatan( float x );
//--------------------------------------------------
void setDistortionShape(Distortion *dist, uint8_t shape);
//--------------------------------------------------
void calcDistBlock(const Distortion *dist, int16_t* buf, const uint8_t size);
//--------------------------------------------------
/*
Even better, you can normalize the output using:

shape = 1..infinity

precalc:
  inv_atan_shape=1.0/atan(shape);
process:
  output = inv_atan_shape * atan(input*shape);

This gives a very soft transition from no distortion to hard clipping.
*/
int32_t calcDist(const Distortion *dist, const int32_t input);
//--------------------------------------------------
#endif /* DISTORTION_H_ */
