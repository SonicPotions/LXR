/*
 * AD_EG.h
 *
 *  Created on: 11.04.2012
 *      Author: Julian
 *
 *      a simple attack/decay envelope based on a 1-pole filter
 */

#ifndef AD_EG_H_
#define AD_EG_H_

#if 0
#include "1PoleLp.h"


#define EG_STOPPED 	0
#define EG_A		1
#define EG_D		2

typedef struct AD_EG_Struct
{
	float a;
	float d;

	uint8_t repeat; /**<number of repetitiona of the a/d cycle*/
	uint8_t repeatCnt;

	OnePoleFilter filter;

	float value;

	uint8_t state;
} AdEg;


void ADEG_init(AdEg* eg);

void ADEG_trigger(AdEg* eg);

float ADEG_calc(AdEg* eg);

#endif
#endif /* AD_EG_H_ */
