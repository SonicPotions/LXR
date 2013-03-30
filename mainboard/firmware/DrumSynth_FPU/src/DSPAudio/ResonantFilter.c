/*
 * ResonantFilter.c
 *
 *  Created on: 05.04.2012
 *      Author: Julian
 */


#include "ResonantFilter.h"
#if 0
#define FMAX ((int16_t)(31000*0.7f))//32767
#define FMIN ((int16_t)(-31000*0.7f))//-32768

__inline float clamp(float value)
{
    const float temp = value + FMAX - fabsf(value-FMAX);
    /*
#if (FMIN == 0)
    return (temp + fabsf(temp)) * 0.25f;
#else
*/
    return (temp + (2.0f*FMIN) + fabsf(temp-(2.0f*FMIN))) * 0.25f;
    /*
#endif
*/
}
/*
You can also use a branch free min function (thanks Laurent from ohm force):
float minf(float a, float b)
{
    return 0.5f*(a+b - fabsf(a-b));
}
*/

#ifndef max
	#define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef min
	#define min( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif
#endif

// to make the linker happy. __errno seems not defined
// so libm.a won't link without this int.
int __errno;

//ResonantFilter resoFilters[NUM_VOICES];
//------------------------------------------------------------------------------------

/** set the resonance*/
void SVF_setReso(ResonantFilter* filter, float feedback)
{
	filter->feedback = feedback;
	filter->q = 1-feedback;//2.0f - 2.0f*min(filter->f * (2.0f - filter->f), feedback);

	if(filter->q<0.1f)filter->q = 0.1f;
}
//------------------------------------------------------------------------------------
void SVF_init(ResonantFilter* filter)
{
		filter->s1 = 0;
		filter->s2 = 0;

		filter->f = 0.20f;
		filter->q = 0.9f;
		filter->feedback = 0.10;

		filter->drive = 0.5f;

		SVF_directSetFilterValue(filter,0.25f);
}
//------------------------------------------------------------------------------------
static float fastTanh(float var)
{

   if(var < -1.95f)     return -1.0f;
   else if(var > 1.95f) return  1.0f;

   else
		return  4.15f*var/(4.29f+var*var);

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
/*
void SVF_setFreq(ResonantFilter* filter, float f)
{
	filter->f = M_TWOPI * f*2/REAL_FS;
}
*/
//------------------------------------------------------------------------------------
#ifndef UInt32
#define UInt32 uint32_t
#endif
/*
inline void erase_All_NaNs_Infinities_And_Denormals(float *in, float *in2)
{
	const UInt32 sample = (UInt32) *in;
	const UInt32 sample2 = (UInt32) *in2;
	const UInt32 exponent = sample & 0x7F800000;

	// exponent < 0x7F800000 is 0 if NaN or Infinity, otherwise 1
	// exponent > 0 is 0 if denormalized, otherwise 1

	const int aNaN = exponent < 0x7F800000;
//	int aDen = exponent > 0;


	*in2 = sample2*aNaN;
}
*/

void SVF_recalcFreq(ResonantFilter* filter)
{
	filter->g  = fastTan(M_PI * filter->f );
}
//------------------------------------------------------------------------------------
void SVF_directSetFilterValue(ResonantFilter* filter, float val)
{
	filter->f = val*(0.5f*0.96f);
	filter->g  = fastTan(M_PI * filter->f );
}



//------------------------------------------------------------------------------------
__inline float clip(float x)
{
	return fastTanh(x);
}
//------------------------------------------------------------------------------------
float tanhXdX(float x)
{

	//return 1 - 0.25 * fabsf(x);

	float a = x*x;
    // IIRC I got this as Pade-approx for tanh(sqrt(x))/sqrt(x)
	x = ((a + 105)*a + 945) / ((15*a + 420)*a + 945);

	return x;
}
//------------------------------------------------------------------------------------
float variableShaper(float x, float depth)
{
	//remember output has to be shaper()x/x!!!
	return 1-depth*fabsf(x);

}
//------------------------------------------------------------------------------------
float softClipTwo(float in)
{
	//return fastTanh(in/2.f)*2.2f;


	if(in > 0.76159415595576488811945828260479f) return fastTanh(in-0.76159415595576488811945828260479f)+0.76159415595576488811945828260479f;
	if(in < -0.76159415595576488811945828260479f) return fastTanh(in+0.76159415595576488811945828260479f)-0.76159415595576488811945828260479f;

	return in;

}

void SVF_calcBlockZDF(ResonantFilter* filter, const uint8_t type, int16_t* buf, const uint8_t size)
{
	uint8_t i;
	const float R = filter->q;//(1.f-filter->feedback);
	const float f = filter->g;

	const float ff = f*f;

	for(i=0;i<size;i++)
	{
		const float x = (buf[i]/((float)0x7fff))*filter->drive;

		// input with half sample delay, for non-linearities
		float ih = 0.5f * (x + filter->zi);
		filter->zi = x;

		// evaluate the non-linear gains
		/*
		You can travially remove any saturator by setting the corresponding gain t0,...,t1 to 1. Also, you can simply scale any saturator (i.e. change clipping threshold) to 1/a*tanh(a*x) by writing
		double t1 = tanhXdX(a*s[0]);
		*/
		const float offset = 0;//0.002f;
		const float tanhScale = 2.f;
		const float t0 = 1;//tanhXdX( (ih - 2*R*filter->s1 - filter->s2 +offset)/tanhScale )*tanhScale;
		const float t1 = 1;//tanhXdX( (filter->s1 - offset)/tanhScale )*tanhScale;


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



		//const float h = xx - t0*2*R*filter->y0 ;
		//const float b = y0;
		//const float l = y1;

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

