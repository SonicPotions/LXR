/*
 * tonecontrol.h
 *
 *  Created on: 30.06.2012
 *      Author: Julian
 *
 *      A tilt EQ.
 *      It uses a center frequency and boosts one of
 *       the frequency ranges above or below it, while doing the opposite to the other range
 */

#ifndef TONECONTROL_H_
#define TONECONTROL_H_

#include "stm32f4xx.h"
#include "../config.h"
#include <math.h>

#define AMP (6/logf(2))
//#define M_PI (3.1415927f)
#define TWO_PI (2*M_PI)
#define SR3 (REAL_FS*3.f)
#define QFACTOR (5.f)			//gfactor is the proportional gain

typedef struct TonecontrolStruct
{
	float output;
	float lpOutput;
//	float centerFreq;			// 20-20kHz
	float lgain;
	float hgain;
	float gainReduction;		// if we add 6dB with the EQ we will get clipping on a loud voice
								// so the gain reduction is used to scale the whole signal accordingly

	//coefficient
	float a0;
	float b1;


}ToneControl;


void toneControl_init(ToneControl* tone);

void toneControl_setCenterFreq(ToneControl* tone, float freq);

/** set the filter gain. valid inputs -6 <=> +6 [dB] */
void toneControl_setGain(ToneControl* tone, float gain);

void toneControl_calc(ToneControl* tone,float input);
void toneControl_calcBlock(ToneControl* tone,float* input, int16_t* output);

#endif /* TONECONTROL_H_ */
