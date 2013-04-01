/*
 * ResonantFilter.c
 *
 *  Created on: 05.04.2012
 *      Author: Julian Schmidt
 */
#include "ResonantFilter.h"


// to make the linker happy. __errno seems not defined
// so libm.a won't link without this int.
int __errno;
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
void SVF_setReso(ResonantFilter* filter, float feedback)
{
	filter->q = 1-feedback;
	if(filter->q<0.1f)filter->q = 0.1f;
}
//------------------------------------------------------------------------------------
void SVF_init(ResonantFilter* filter)
{
		filter->s1 = 0;
		filter->s2 = 0;

		filter->f = 0.20f;
		filter->q = 0.9f;

		filter->drive = 0.5f;

		SVF_directSetFilterValue(filter,0.25f);
}
//------------------------------------------------------------------------------------
static float fastTanh(float var)
{
   if(var < -1.95f)     return -1.0f;
   else if(var > 1.95f) return  1.0f;
   else					return  4.15f*var/(4.29f+var*var);
}
//------------------------------------------------------------------------------------
float fastTan(float x)
{
#if 1
	float A = -15*x+x*x*x;
	float B = 3*(-5+2*x*x);
	return A/B;
#else
	const float A = 5*(-21*x+2*x*x*x);
	const float B = 105-45*x*x+x*x*x*x;
	return A/B;
#endif
}
//------------------------------------------------------------------------------------
void SVF_recalcFreq(ResonantFilter* filter)
{
	filter->g  = fastTan(M_PI * filter->f );
}
//------------------------------------------------------------------------------------
void SVF_directSetFilterValue(ResonantFilter* filter, float val)
{
	filter->f = val*(0.5f*0.93f);
	filter->g  = fastTan(M_PI * filter->f );
}
//------------------------------------------------------------------------------------
#if ENABLE_NONLINEAR_INTEGRATORS
float tanhXdX(float x)
{
	float a = x*x;
    // IIRC I got this as Pade-approx for tanh(sqrt(x))/sqrt(x)
	x = ((a + 105)*a + 945) / ((15*a + 420)*a + 945);
	return x;
}
#endif
//------------------------------------------------------------------------------------
float softClipTwo(float in)
{

	if(in > 0.76159415595576488811945828260479f) return fastTanh(in-0.76159415595576488811945828260479f)+0.76159415595576488811945828260479f;
	if(in < -0.76159415595576488811945828260479f) return fastTanh(in+0.76159415595576488811945828260479f)-0.76159415595576488811945828260479f;

	return in;
}
//------------------------------------------------------------------------------------
void SVF_calcBlockZDF(ResonantFilter* filter, const uint8_t type, int16_t* buf, const uint8_t size)
{
	uint8_t i;
	const float R 	= filter->q;
	const float f 	= filter->g;
	const float ff 	= f*f;

	for(i=0;i<size;i++)
	{
		const float x = ((buf[i]/((float)0x7fff))*filter->drive);

#if ENABLE_NONLINEAR_INTEGRATORS
		// input with half sample delay, for non-linearities
		float ih = 0.5f * (x + filter->zi);
		filter->zi = x;
#endif

		// evaluate the non-linear gains
		/*
		You can travially remove any saturator by setting the corresponding gain t0,...,t1 to 1. Also, you can simply scale any saturator (i.e. change clipping threshold) to 1/a*tanh(a*x) by writing
		double t1 = tanhXdX(a*s[0]);
		*/
#if ENABLE_NONLINEAR_INTEGRATORS
		const float offset = 0;//0.002f;
		const float tanhScale = 2.f;
		const float t0 = tanhXdX( (ih - 2*R*filter->s1 - filter->s2 +offset)/tanhScale )*tanhScale;
		const float t1 = tanhXdX( (filter->s1 - offset)/tanhScale )*tanhScale;
#else
		const float t0 = 1;
		const float t1 = 1;
#endif

		// g# the denominators for solutions of individual stages
		const float g0 = 1.f / (1.f + f*t0*2*R);

		const float s1 = filter->s1;
		const float s2 = filter->s2;

		// solve feedback
		const float f1 = ff*g0*t0*t1;
		float y1=(f1*x+s2+f*g0*t1*s1)/(f1+1);


		// solve the remaining stages with nonlinear gain
		 const float xx = t0*(x - y1);
		 const float y0 = (softClipTwo(s1) + f*xx)*g0;

		filter->s1   = softClipTwo(filter->s1) + 2*f*(xx - t0*2*R*y0);
		filter->s2   = (filter->s2)    + 2*f* t1*y0;

		int32_t tmp;
		switch(type)
		{
		default:
		case FILTER_LP:
			tmp = y1 * FILTER_GAIN;
			buf[i] = __SSAT(tmp,16);
			break;

		case FILTER_HP:
		{
			const float ugb = 2*R*y0;
			const float h = x - ugb - y1;
			tmp = h * FILTER_GAIN;
			buf[i] = __SSAT(tmp,16);
		}
			break;

		case FILTER_BP:
			tmp = y0 * FILTER_GAIN;
			buf[i] = __SSAT(tmp,16);
			break;

		case FILTER_UNITY_BP:
		{
			const float ugb = 2*R*y0;
			tmp = ugb * FILTER_GAIN;
			buf[i] = __SSAT(tmp,16);
		}
			break;

		case FILTER_NOTCH:
		{
			const float ugb = 2*R*y0;
			tmp = (x-ugb) * FILTER_GAIN;
			buf[i] = __SSAT(tmp,16);
		}
			break;

		case FILTER_PEAK:
		{
			const float ugb = 2*R*y0;
			const float h = x - ugb - y1;
			tmp = (y1-h) * FILTER_GAIN;
			buf[i] = __SSAT(tmp,16);
		}
			break;

		}
	}
}
//------------------------------------------------------------------------------------

