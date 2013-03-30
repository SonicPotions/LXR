/*
 * Decay.c
 *
 *  Created on: 16.04.2012
 *      Author: Julian
 */


#include "Decay.h"
#include <math.h>

//-------------------------------------------------
float DecayEg_calcSlopeValue(float val, float slope)
{
	return (1+slope)*val/(1+slope*fabsf(val));
}
//-------------------------------------------------
void DecayEg_init(DecayEg* eg)
{
	eg->decay 		= 0.01f;
	eg->value 	= 0;
	//initOnePole(&eg->filter);
};
//-------------------------------------------------
#define TIME_K (2*0.99f/(1.f-0.99f))
float DecayEg_calcTime(uint8_t data2)
{
	const float val = (data2)/127.f;
	return 1.f-((1.f+TIME_K)*val/(1.f+TIME_K*fabsf(val)));

}
//-------------------------------------------------
void DecayEg_trigger(DecayEg* eg)
{
	//setOnePoleCoef(&eg->filter,eg->d);
	//setOnePoleValue(&eg->filter,1.f);
	eg->value = 1.f;
};
//-------------------------------------------------
float DecayEg_calc(DecayEg* eg)
{
	float val = eg->value;
	//if(val>0)
	{
		//val = calcOnePole(&eg->filter,0.f);
		val -= (val>0)*eg->decay;
		eg->value = val;
		return DecayEg_calcSlopeValue(val,eg->slope);
	}
	/*
	else
	{
		eg->value = 0;
		return 0;
	}
	*/
};
//-------------------------------------------------
void DecayEg_setDecay(DecayEg* eg, uint8_t data2)
{
	eg->decay = DecayEg_calcTime(data2);
}
//--------------------------------------------------
void DecayEg_setSlope(DecayEg* eg, uint8_t data2)
{
	const float amount 	= ((data2/127.f)-0.5f)*2;
	eg->slope 			= 2*amount/(1-amount);
}
