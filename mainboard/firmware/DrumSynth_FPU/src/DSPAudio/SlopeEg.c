/*
 * SlopeEg.c
 *
 *  Created on: 11.06.2012
 *      Author: Julian
 */


#if 0
#include "SlopeEg.h"
#include "config.h"

//--------------------------------------------------
void slopeEg_init(SlopeEg* eg)
{
	eg->attackTimeSamples	= 11025;
	eg->decayTimeSamples	= 11025;
	eg->value 	= 0;
	eg->state 	= EG_STOPPED;

	eg->repeat	= 0;

	eg->targetValue = 1;

	eg->slope = 1;

//	initOnePole(&eg->filter);

}
//--------------------------------------------------
void slopeEg_trigger(SlopeEg* eg)
{
	//beginn in attack state
	eg->state = EG_A;
//	setOnePoleCoef(&eg->filter,eg->a);
	eg->sampleToProcess = eg->attackTimeSamples;
	eg->targetValue = 1;
	eg->repeatCnt = eg->repeat;
}
//--------------------------------------------------
float slopeEg_calc(SlopeEg* eg)
{

	register float val = eg->value;
	switch(eg->state)
	{

	case EG_STOPPED:
		return 0.f;
		break;


	case EG_A:
		//val = calcOnePole(&eg->filter,1.1f);
		val +=eg->slope * (eg->targetValue-val)/eg->sampleToProcess;
		eg->sampleToProcess--;


		//if(val >= 1.0f)
		if(!eg->sampleToProcess)
		{

			val = 1.0f;
			if(eg->repeatCnt > 0)
			{
				eg->repeatCnt--;
				//setOnePoleValue(&eg->filter,0);
				val = 0;
				eg->sampleToProcess = eg->attackTimeSamples;

			}
			else
			{
				eg->state = EG_D;
				eg->sampleToProcess = eg->decayTimeSamples;
				//setOnePoleCoef(&eg->filter,eg->d);
				eg->targetValue = 0;
			}
		}

		eg->value = val;
		return val;
		break;

	case EG_D:

		if(eg->sampleToProcess)
		{
			//val = calcOnePole(&eg->filter,0.f);
			val +=eg->slope * (eg->targetValue-val)/eg->sampleToProcess;
			eg->sampleToProcess--;

			eg->value = val;
		}
		return val;

		break;
	}
	return 0;
}
//--------------------------------------------------
#endif
