/*
 * ResonantFilter.h
 *
 *  Created on: 05.04.2012
 *      Author: Julian
 */

#ifndef RESONANTFILTER_H_
#define RESONANTFILTER_H_

/*
 * ResonantFilter.c
 *
 *  Created on: 05.04.2012
 *      Author: Julian
 */


#include "globals.h"
#include "config.h"
#include "stm32f4xx.h"
#include "datatypes.h"
#include "math.h"

#include "1Pole0df.h"

/*//basic resonant 2 pole
f = 1.0 - exp(-2*pi * hz/rate);
q = feedback + feedback / (1.0 - f);

a += f * ((in - a) + q * (a - b));
b += f * (a - b);
*/

/* SVF
f = 2*pi * hz/rate;
q = 2.0 - 2.0*max(f * (2.0 - f), feedback);

h = in - (l + q * b);
b += f * h;
b = sat(b, r); //clip to the range you want it to oscillate at
l += f * b;
*/

enum filterTypeEnum
{
	FILTER_LP=1,
	FILTER_HP,
	FILTER_BP,
	FILTER_UNITY_BP,
	FILTER_NOTCH,
	FILTER_PEAK
};

#if UNIT_GAIN_DRIVE
#define FILTER_GAIN 0x70ff
#else
#define FILTER_GAIN 0x70ff
#endif

typedef struct ResoFilterStruct
{
	float f;	/**< cutoff frequency as integer from 0 to 0xffff where 0xffff = SR*/
	float q;	/**< q value calculated from setReso()*/

//	float h;
//	float b;
//	float l;

	float s1;
	float s2;


	float feedback; /**< resonance value between 0:1.0*/
	float drive;


//	float wd;
//	float T ;
//	float wa; 	// prewarped radian frequency for analog filter (Eq. 3.7)
	float g;            			// embedded integrator gain (Fig 3.11), wc == wa

//	float scale;
//	float ug;	//unity gain
	float zi;	//input z^(-1)
//	float y0,y1;

//	OnePole0dfFilter pole1Filter;
//	OnePole0dfFilter pole2Filter;


} ResonantFilter;


//extern ResonantFilter resoFilters[NUM_VOICES];

//TODO calc in fixed point math!

//------------------------------------------------------------------------------------

/** set the resonance*/
void SVF_setReso(ResonantFilter* filter, float feedback);
//------------------------------------------------------------------------------------
void SVF_init();
//------------------------------------------------------------------------------------
//void SVF_setFreq(ResonantFilter* filter, float f);
//------------------------------------------------------------------------------------
void SVF_directSetFilterValue(ResonantFilter* filter, float val);
//------------------------------------------------------------------------------------
//void SVF_calcBlock(ResonantFilter* filter, int16_t* input, const uint8_t size);
//------------------------------------------------------------------------------------
void SVF_calcBlockZDF(ResonantFilter* filter, const uint8_t type, int16_t* buf, const uint8_t size);
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
void SVF_recalcFreq(ResonantFilter* filter);
/*
//------------------------------------------------------------------------------------
int16_t* SVF_getLpBlockInt(const ResonantFilter* filter);
//------------------------------------------------------------------------------------
int16_t* SVF_getHpBlockInt(const ResonantFilter* filter);
*/
#endif /* RESONANTFILTER_H_ */
