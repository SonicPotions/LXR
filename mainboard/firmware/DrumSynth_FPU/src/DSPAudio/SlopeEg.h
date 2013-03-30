/*
 * SlopeEg.h
 *
 *  Created on: 11.06.2012
 *      Author: Julian
 */

#ifndef SLOPEEG_H_
#define SLOPEEG_H_

#if 0
#include "stm32f4xx.h"


#define EG_STOPPED 	0
#define EG_A		1
#define EG_D		2

typedef struct SLOPE_EG_Struct
{
	uint16_t attackTimeSamples;		/**<time for the attack phase in samples*/
	uint16_t decayTimeSamples;			/**<time for the decay phase in samples*/
	uint16_t sampleToProcess;			/**<a sample counter to determine remaining EG time*/
	float slope;					/**<1->linear; >1 exponential*/

	uint8_t repeat; 				/**<number of repetitions of the attack phase*/
	uint8_t repeatCnt;				/**<a counter for the already played repeats*/

	float value;

	uint8_t state;

	float targetValue;
} SlopeEg;


void slopeEg_init(SlopeEg* eg);

void slopeEg_trigger(SlopeEg* eg);

float slopeEg_calc(SlopeEg* eg);

#endif
#endif /* SLOPEEG_H_ */
