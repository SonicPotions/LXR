/*
 * distortion.c
 *
 *  Created on: 14.04.2012
 *      Author: Julian
 */
#include "distortion.h"
#include "math.h"



/** \brief  Signed Saturate with shift

    This function saturates a signed value.

    \param [in]  value  Value to be saturated
    \param [in]    sat  Bit position to saturate to (1..32)
    \param [in]  shift  shift N bits LSL (left)
    \return             Saturated value
 */
#define __SSAT_LSL(ARG1,ARG2,ARG3) \
({                          \
  uint32_t __RES, __ARG1 = (ARG1); \
  __ASM ("ssat %0, %1, %2, LSL %3" : "=r" (__RES) :  "I" (ARG2), "r" (__ARG1),  "I" (ARG3) ); \
  __RES; \
 })
//--------------------------------------------------
/*
inline float fastatan( float x )
{
    return (x / (1.0f + 0.28f * (x * x)));
}
*/
//--------------------------------------------------
__inline void setDistortionShape(Distortion *dist, uint8_t shape)
{

	dist->shape = 2*(shape/128.f)/(1-(shape/128.f));
	//dist->shape = shape*shape/140.f;

#if 0
	dist->shape = 1+(shape/127.f)*10.f;
#else
	//dist->shape = 1+((shape/127.f)*10);
#endif
	/*
	const float nonZeroShape = shape*2 + (shape==0)*0.07f;
	dist->shape = nonZeroShape *(1.f/0x3fff);

	dist->inv_shape = (1.f/nonZeroShape) * 0x3fff;
	//dist->inv_atan_shape = 1.0f/fastatan(shape);
	 *
	 */
}


//--------------------------------------------------
void calcDistBlock(const Distortion *dist, int16_t* buf, const uint8_t size)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
			float x = buf[i]/32767.f;
			x = (1+dist->shape)*x/(1+dist->shape*fabsf(x));
			buf[i] = (x*32767);
	}

	/*
	uint8_t i;
	const float a = 1.f;
	const float b = -1.f;

	for(i=0;i<size;i++)
	{
		float x = buf[i]/32767.f*dist->shape;

	   const float x1 = fabsf (x-a);
	   const float x2 = fabsf (x-b);

		x = x1 + (a+b);
		x -= x2;
		x *= 0.5f;

		buf[i] = (x*32767);
	}
	*/
}

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
__inline int32_t calcDist(const Distortion *dist, const int32_t input)
{
	/*
	float fltInput = input * dist->shape;


	return (fltInput-(fltInput*fltInput*fltInput)*(1.f/11.f)) * dist->inv_shape;
	*/


	//TODO: badnlimit via frequency input -> higher freq => lower distortion level
//	return (1+dist->shape)*input/(32767+dist->shape*fabsf(input))*32756;


#if 0
	int32_t buf = input*dist->shape;
	buf = __SSAT(buf,16);
	return buf;
#else

	float x = input/32767.f*dist->shape;
	const float a = 1.f;
	const float b = -1.f;

	   const float x1 = fabsf (x-a);
	   const float x2 = fabsf (x-b);
	   x = x1 + (a+b);
	   x -= x2;
	   x *= 0.5f;
	   return (x*32767);

#endif

}
