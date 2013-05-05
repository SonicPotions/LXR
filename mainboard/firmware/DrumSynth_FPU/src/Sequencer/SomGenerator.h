/*
 * SomGenerator.h
 *
 *  Created on: 30.04.2013
 *      Author: Julian
 */

#ifndef SOMGENERATOR_H_
#define SOMGENERATOR_H_

#include "stm32f4xx.h"
#include "random.h"


typedef struct SomGeneratorStruct
{
	//position
	float x;
	float y;

	//voice frequency
	uint8_t frequency[7];

	float flux;


}SomGenerator;

void som_init();
void som_tick(uint8_t stepNr, uint8_t mutedTracks);

void som_setX(uint8_t x);
void som_setY(uint8_t y);
void som_setFlux(float flux);
void som_setFreq(uint8_t freq, uint8_t voice);

extern SomGenerator somGenerator;


#endif /* SOMGENERATOR_H_ */
