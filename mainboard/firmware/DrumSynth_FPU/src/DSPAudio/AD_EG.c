/*
 * AD_EG.c
 *
 *  Created on: 08.04.2012
 *      Author: Julian
 */

#include "AD_EG.h"

#if 0
//--------------------------------------------------
void ADEG_init(AdEg* eg)
{
	eg->a 		= .9f;
	eg->d 		= 0.0001f;
	eg->value 	= 0;
	eg->state 	= EG_STOPPED;

	eg->repeat	= 0;

	initOnePole(&eg->filter);

}
//--------------------------------------------------
void ADEG_trigger(AdEg* eg)
{
	//beginn in attack state
	eg->state = EG_A;
	setOnePoleCoef(&eg->filter,eg->a);
	eg->repeatCnt = eg->repeat;
}
//--------------------------------------------------
float ADEG_calc(AdEg* eg)
{

	register float val;// = eg->value;
	switch(eg->state)
	{

	case EG_STOPPED:
		return 0.f;
		break;


	case EG_A:
		//eg->value = calcOnePole(&eg->filter,1.1f);
		val = calcOnePole(&eg->filter,1.1f);

		//if(eg->value >= 1.0f)
		if(val >= 1.0f)
		{
			//eg->value = 1.0f;
			val = 1.0f;
			if(eg->repeatCnt > 0)
			{
				eg->repeatCnt--;
				setOnePoleValue(&eg->filter,0);
				//setOnePoleCoef(&eg->filter,eg->a);
				//eg->state = EG_A;
			}
			else
			{
				eg->state = EG_D;
				setOnePoleCoef(&eg->filter,eg->d);
			}
			//attack phase finished
			//eg->state = EG_D;

		}
		//return eg->value;
		//eg->value = val;
		return val;
		break;

	case EG_D:
		//eg->value = calcOnePole(&eg->filter,-0.1f);
		val = calcOnePole(&eg->filter,0.f);

		//if(eg->value <= 0.0000f)
		//if(eg->value <= 0.0000f)
		{
		//	eg->value = 0.0f;
			//decay phase finished

			//eg->state = EG_STOPPED;

		}
		//return eg->value;
		return val;
		eg->value = val;
		break;
	}
	return 0;
}
//--------------------------------------------------
#endif
