/*
 * ResonantFilter.c
 *
 *  Created on: 05.04.2012
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

#include "ResonantFilter.h"


// to make the linker happy. __errno seems not defined
// so libm.a won't link without this int.
int __errno;
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
void SVF_setReso(ResonantFilter* filter, float feedback)
{
	filter->q = 1-feedback;
	if(filter->q<0.1f)filter->q = 0.02f;
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

#if USE_SHAPER_NONLINEARITY
		setDistortionShape(&filter->shaper, 0);
#endif
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
#if USE_SHAPER_NONLINEARITY
	setDistortionShape(&filter->shaper, filter->drive);
#endif
}
//------------------------------------------------------------------------------------
void SVF_setDrive(ResonantFilter* filter,uint8_t drive)
{
#if USE_SHAPER_NONLINEARITY
	filter->drive = drive;
	setDistortionShape(&filter->shaper, filter->drive);
#else
	filter->drive =  0.4f + (drive/127.f)*(drive/127.f)*6;
#endif

}
//------------------------------------------------------------------------------------
void SVF_directSetFilterValue(ResonantFilter* filter, float val)
{
	filter->f = val*(0.5f*0.90f);
	filter->g  = fastTan(M_PI * filter->f );

}
//------------------------------------------------------------------------------------
//#if ENABLE_NONLINEAR_INTEGRATORS
float tanhXdX(float x)
{
	float a = x*x;
    // IIRC I got this as Pade-approx for tanh(sqrt(x))/sqrt(x)
	x = ((a + 105)*a + 945) / ((15*a + 420)*a + 945);
	return x;
}
//#endif
//------------------------------------------------------------------------------------
float softClipTwo(float in)
{

	return in * tanhXdX(0.5*in);

	if(in > 0.76159415595576488811945828260479f) return fastTanh(in-0.76159415595576488811945828260479f)+0.76159415595576488811945828260479f;
	if(in < -0.76159415595576488811945828260479f) return fastTanh(in+0.76159415595576488811945828260479f)-0.76159415595576488811945828260479f;

	return in;
}
//------------------------------------------------------------------------------------
void SVF_calcBlockZDF(ResonantFilter* filter, const uint8_t type, int16_t* buf, const uint8_t size)
{
	uint8_t i;
	const float f 	= filter->g;
	//fix unstable filter for high f and r settings
	const float R 	= filter->f >= 0.4499f ? 1 : filter->q;
	const float ff 	= f*f;

	for(i=0;i<size;i++)
	{
#if USE_SHAPER_NONLINEARITY
		const float x = (buf[i]/((float)0x7fff));
#else
		const float x = softClipTwo((buf[i]/((float)0x7fff))*filter->drive);
#endif

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
		const float scale = 0.5f;
		const float t0 = tanhXdX(scale* (ih - 2*R*filter->s1 - filter->s2 ) );
		const float t1 = tanhXdX(scale* (filter->s1 ) );
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
			return;
			break;
		case FILTER_LP:
#if USE_SHAPER_NONLINEARITY

			buf[i] = FILTER_GAIN * fastTanh( distortion_calcSampleFloat(&filter->shaper, y1));
#else
			tmp = fastTanh(y1) * 0x7fff ;//FILTER_GAIN;
			buf[i] = __SSAT(tmp,16);
#endif
			break;

		case FILTER_HP:
		{
			const float ugb = 2*R*y0;
			const float h = x - ugb - y1;
#if USE_SHAPER_NONLINEARITY

			buf[i] = FILTER_GAIN * distortion_calcSampleFloat(&filter->shaper, h);
#else
			tmp = h * FILTER_GAIN;
			buf[i] = __SSAT(tmp,16);
#endif
		}
			break;

		case FILTER_BP:
#if USE_SHAPER_NONLINEARITY

			buf[i] = FILTER_GAIN * distortion_calcSampleFloat(&filter->shaper, y0);
#else
			tmp = y0 * FILTER_GAIN;
			buf[i] = __SSAT(tmp,16);
#endif
			break;

		case FILTER_UNITY_BP:
		{
			const float ugb = 2*R*y0;
#if USE_SHAPER_NONLINEARITY

			buf[i] = FILTER_GAIN * distortion_calcSampleFloat(&filter->shaper, ugb);
#else
			tmp = ugb * FILTER_GAIN;
			buf[i] = __SSAT(tmp,16);
#endif
		}
			break;

		case FILTER_NOTCH:
		{
			const float ugb = 2*R*y0;
#if USE_SHAPER_NONLINEARITY

			buf[i] = FILTER_GAIN * distortion_calcSampleFloat(&filter->shaper, (x-ugb));
#else
			tmp = (x-ugb) * FILTER_GAIN;
			buf[i] = __SSAT(tmp,16);
#endif
		}
			break;

		case FILTER_PEAK:
		{
			const float ugb = 2*R*y0;
			const float h = x - ugb - y1;
#if USE_SHAPER_NONLINEARITY

			buf[i] = FILTER_GAIN * distortion_calcSampleFloat(&filter->shaper, (y1-h));
#else
			tmp = (y1-h) * FILTER_GAIN;
			buf[i] = __SSAT(tmp,16);
#endif
		}
			break;

		}
	}
}
//------------------------------------------------------------------------------------

