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
//----------------------------------------------------
#include "globals.h"
#include "config.h"
#include "stm32f4xx.h"
#include "datatypes.h"
#include "math.h"
//----------------------------------------------------
//removed to free some cpu cycles
#define ENABLE_NONLINEAR_INTEGRATORS 	1
#define FILTER_GAIN 					0x70ff
//----------------------------------------------------
enum filterTypeEnum
{
	FILTER_LP=1,
	FILTER_HP,
	FILTER_BP,
	FILTER_UNITY_BP,
	FILTER_NOTCH,
	FILTER_PEAK
};
//----------------------------------------------------
typedef struct ResoFilterStruct
{
	float f;	/**< cutoff frequency as integer from 0 to 0xffff where 0xffff = SR*/
	float g;    /**< embedded integrator gain (Fig 3.11), wc == wa*/
	float q;	/**< q value calculated from setReso()*/

	float s1;
	float s2;
#if ENABLE_NONLINEAR_INTEGRATORS
	float zi;	//input z^(-1)
#endif

	float drive;
} ResonantFilter;
//------------------------------------------------------------------------------------
void SVF_setReso(ResonantFilter* filter, float feedback);
//------------------------------------------------------------------------------------
void SVF_init();
//------------------------------------------------------------------------------------
void SVF_directSetFilterValue(ResonantFilter* filter, float val);
//------------------------------------------------------------------------------------
void SVF_calcBlockZDF(ResonantFilter* filter, const uint8_t type, int16_t* buf, const uint8_t size);
//------------------------------------------------------------------------------------
void SVF_recalcFreq(ResonantFilter* filter);

#endif /* RESONANTFILTER_H_ */
