/*
 * transientGenerator.h
 *
 *  Created on: 29.06.2012
 *      Author: Julian
 *
 *      the transient generator generates a kind of 'click' oneshot sound when triggered
 *      a variety of different, very short [50 ms] transient samples is used
 *
 */

#ifndef TRANSIENTGENERATOR_H_
#define TRANSIENTGENERATOR_H_


#include "stm32f4xx.h"
#include <stdint.h>
#include "transientTables.h"

typedef struct TransientGeneratorStruct
{
	int16_t 	output;
	uint32_t	phase;		// the current phase of the osc
	float	 	pitch;		// we need no accurate tuning for a transient, so we use a pitch factor [0.25f:1.75f]
	uint8_t		waveform;	// the selected transient table of the osc
	float		volume;


	//for midi
	uint16_t	midiFreq;  //upper 8 bit coarse, lower 8 bit fine
} TransientGenerator;


void transient_init(TransientGenerator* transient);

void transient_trigger(TransientGenerator* transient);

void transient_calc(TransientGenerator* transient);
void transient_calcBlock(TransientGenerator* transient, int16_t* buf, const uint8_t size);

#endif /* TRANSIENTGENERATOR_H_ */
