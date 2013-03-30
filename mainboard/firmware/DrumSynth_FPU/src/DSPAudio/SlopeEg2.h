/*
 * SlopeEg2.h
 *
 *  Created on: 16.06.2012
 *      Author: Julian
 */

#ifndef SLOPEEG2_H_
#define SLOPEEG2_H_



#include "stm32f4xx.h"


#define EG_STOPPED 	0
#define EG_A		1
#define EG_D		2
#define EG_REPEAT	3

typedef struct SLOPE_EG2_Struct
{
	float attack;
	float decay;
	float slope;
	float invSlope;

	uint8_t repeat; 				/**<number of repetitions of the attack phase*/
	uint8_t repeatCnt;				/**<a counter for the already played repeats*/

	float value;

	uint8_t state;

	//float targetValue;
} SlopeEg2;


void slopeEg2_init(SlopeEg2* eg);

void slopeEg2_trigger(SlopeEg2* eg);

float slopeEg2_calc(SlopeEg2* eg);

void slopeEg2_setAttack(SlopeEg2* eg, uint8_t data2, uint8_t isSync);
void slopeEg2_setDecay(SlopeEg2* eg, uint8_t data2, uint8_t isSync);
void slopeEg2_setSlope(SlopeEg2* eg, uint8_t data2);
float slopeEg2_calcDecay(uint8_t data2);


#endif /* SLOPEEG2_H_ */
