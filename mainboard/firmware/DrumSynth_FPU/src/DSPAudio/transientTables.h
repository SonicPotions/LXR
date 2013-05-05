/*
 * transientTables.h
 *
 *  Created on: 29.06.2012
 *      Author: Julian
 */

#ifndef TRANSIENTTABLES_H_
#define TRANSIENTTABLES_H_

#include "stm32f4xx.h"
#include <stdint.h>

#define NUM_TRANSIENTS 12

/*
 * a transient sound is very short one shot sound to give the attack of a sound some character
 *
 */

/** we want the transient samples to be ~50ms long
 * so with a sample rate of 44100Hz we got
 * 1000 ms / 44100 samples  = 0,02267573696145124716553287981859 ms per sample
 * 50ms / (1000 ms / 44100 samples) =  2205 samples
 *
 * so we need about 2205 samples for a transient lasting 50ms
 */
#define TRANSIENT_SAMPLE_LENGTH  2205


/* Transient Volume Table
 * this table is used as a envelope for the transient sounds
 * the phase index (0-2204) is shifted right 5 bits
 * 2204>>5 = 68
 * so we get a value from 0 to 68
 *
 * that is used as an index for the volume table
 *
 * this way we get a constant decaying envelope
 * across the transient that scales with the pitch
 * the transient table is played at
 */

extern const float transientVolumeTable[69];



/* signed 8 bit pcm
 * 4102__patchen__atik-2-10-stereoatik
 */
extern const int8_t transientData[NUM_TRANSIENTS][TRANSIENT_SAMPLE_LENGTH];

#endif /* TRANSIENTTABLES_H_ */
