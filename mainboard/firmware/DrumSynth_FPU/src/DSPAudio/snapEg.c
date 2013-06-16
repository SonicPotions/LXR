/*
 * snapEg.c
 *
 *  Created on: 16.06.2013
 *      Author: Julian
 */
#include "snapEg.h"

#define SNAP_MAX_VALUE 12.f
#define SNAP_REDUCTION 0.2f

//float SNAP_MAX_VALUE = 12.f;
//float SNAP_REDUCTION = 0.2f;

void SnapEg_init(SnapEg* eg)
{
	eg->value = 0;
}

void SnapEg_trigger(SnapEg* eg)
{
	eg->value = 1;
}

float SnapEg_calc(SnapEg* eg, float time)
{
	if(eg->value>0)
	{
		eg->value -= SNAP_REDUCTION*time;
		return eg->value* eg->value * SNAP_MAX_VALUE;
	} else {
		return 0;
	}
}

void SnapEg_calcBlock(SnapEg* eg,int16_t* buf, const uint8_t size)
{

}

