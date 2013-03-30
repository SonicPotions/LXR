/*
 * Decay.h
 *
 *  Created on: 16.04.2012
 *      Author: Julian
 */

#ifndef DECAY_H_
#define DECAY_H_


//#include "1PoleLp.h"
#include "stm32f4xx.h"


typedef struct Decay_EG_Struct
{
	float decay;

	//OnePoleFilter filter;
	float slope;

	float value;

} DecayEg;


void DecayEg_init(DecayEg* eg);

void DecayEg_trigger(DecayEg* eg);

void DecayEg_setDecay(DecayEg* eg, uint8_t data2);

float DecayEg_calc(DecayEg* eg);

void DecayEg_setSlope(DecayEg* eg, uint8_t data2);

#endif /* DECAY_H_ */
