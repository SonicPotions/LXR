/*
 * toneControl.c
 *
 *  Created on: 30.06.2012
 *      Author: Julian
 */


#include "toneControl.h"

//-------------------------------------------------------------------
void toneControl_init(ToneControl* tone)
{
	tone->lpOutput = 0;
	tone->output = 0;

	//400Hz
	toneControl_setCenterFreq(tone,400);
	//flat
	toneControl_setGain(tone,0);
}
//-------------------------------------------------------------------
void toneControl_setCenterFreq(ToneControl* tone, float freq)
{
//	tone->centerFreq = freq;

	const float omega = TWO_PI*freq;
	const float n = 1.f/(SR3 + omega);
	tone->a0 = 2*omega*n;
	tone->b1 = (SR3 - omega)*n;



}
//-------------------------------------------------------------------
/** set the filter gain. valid inputs -6 <=> +6 [dB] */
void toneControl_setGain(ToneControl* tone, float gain)
{
	float g1,g2;
	if (gain > 0)
	{
		g1 = -QFACTOR*gain;
		g2 = gain;
	}
	else
	{
		g1 = -gain;
		g2 = QFACTOR*gain;
	};

	//two separate gains
	tone->lgain = expf(g1/AMP)-1.f;
	tone->hgain = expf(g2/AMP)-1.f;

	if(tone->lgain > tone->hgain)
	{
		tone->gainReduction = 1.5f-(tone->lgain);
	}
	else
	{
		tone->gainReduction = 1.5f-(tone->hgain);
	}

}
//-------------------------------------------------------------------
void toneControl_calc(ToneControl* tone,float input)
{
	const float lpout =  tone->a0*input + tone->b1*tone->lpOutput;
	tone->output = (input*tone->gainReduction) + tone->lgain*lpout + tone->hgain*(input - lpout);
	//tone->output = (input) + tone->lgain*lpout + tone->hgain*(input - lpout);

	tone->lpOutput = lpout;
}
//-------------------------------------------------------------------

