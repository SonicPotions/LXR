/*
 * SlopeEg.c
 *
 *  Created on: 16.06.2012
  * ------------------------------------------------------------------------------------------------------------------------
 *  Copyright 2013 Julian Schmidt
 *  Julian@sonic-potions.com
 * ------------------------------------------------------------------------------------------------------------------------
 *  This file is part of the Sonic Potions LXR drumsynth firmware.
 * ------------------------------------------------------------------------------------------------------------------------
 *  Redistribution and use of the LXR code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *       - The code may not be sold, nor may it be used in a commercial product or activity.
 *
 *       - Redistributions that are modified from the original source must include the complete
 *         source code, including the source code for all components used by a binary built
 *         from the modified sources. However, as a special exception, the source code distributed
 *         need not include anything that is normally distributed (in either source or binary form)
 *         with the major components (compiler, kernel, and so on) of the operating system on which
 *         the executable runs, unless that component itself accompanies the executable.
 *
 *       - Redistributions must reproduce the above copyright notice, this list of conditions and the
 *         following disclaimer in the documentation and/or other materials provided with the distribution.
 * ------------------------------------------------------------------------------------------------------------------------
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *   USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ------------------------------------------------------------------------------------------------------------------------
 */




#include "SlopeEg2.h"
#include "config.h"
#include <math.h>

//--------------------------------------------------
void slopeEg2_init(SlopeEg2* eg)
{
	eg->attack	= 0.01f;
	eg->decay 	= 0.01f;
	eg->value 	= 0;
	eg->state 	= EG_STOPPED;
	eg->repeat	= 0;

	slopeEg2_setSlope(eg,0.5f);
}
//--------------------------------------------------
void slopeEg2_trigger(SlopeEg2* eg)
{
	//if no repeat is selected beginn in attack state
	if(!eg->repeat)
	{
		eg->state = EG_A;
	}
	// if repeat is active we want to repeat the decay stage during the attack time
	else
	{
		eg->state = EG_REPEAT;
		//since we have no attack we start with  eg->value = max
		//we use the attack time as a repeat phase decay time
		 eg->value = 1.0f;
	}
	eg->repeatCnt = eg->repeat;
}
//--------------------------------------------------
float slopeEg2_calcSlopeValue(float val, float slope)
{
	return (1+slope)*val/(1+slope*fabsf(val));
}
//--------------------------------------------------
float slopeEg2_calc(SlopeEg2* eg)
{
	register float val = eg->value;
	switch(eg->state)
	{

	case EG_STOPPED:
		return 0.f;
		break;

	case EG_REPEAT:
		/*
		 *  if the repeat mode is active we use the attack time as a 2nd decay time for the loop part
		 *  the attack phase is looped repeatCnt times
		 */
		if(val>0)
		{
			val -= eg->attack;
		}
		else
		{
			eg->repeatCnt--;
			if(eg->repeatCnt > 0)
			{
				//retrigger the decay phase and decrease repeat counter
				val = 1.f;
			}
			else
			{
				//repeat counter reached zero -> go to real decay stage
				eg->state = EG_D;
				val = 1.f;
			}
		}
		eg->value = val;
		//to get a analogue stile exp eg we need to invert the shape
		return slopeEg2_calcSlopeValue(val,eg->slope);

		break;
	case EG_A:
		val += eg->attack;
		if(val >= 1.0f)
		{
			val = 1.0f;
			eg->state = EG_D;
		}

		eg->value = val;
		//to get a analogue stile exp eg we need to invert the shape
		return slopeEg2_calcSlopeValue(val,eg->invSlope);
		break;

	case EG_D:

		if(val>0)
		{
			val -= eg->decay;
			eg->value = val;
			return slopeEg2_calcSlopeValue(val,eg->slope);
		}
		else
		{
			eg->value = 0;
			return 0;
		}
		break;
	}
	return 0;
}
//--------------------------------------------------
/*
x = input in [-1..1]
y = output
k = 2*amount/(1-amount);

f(x) = (1+k)*x/(1+k*abs(x))
*/
#define TIME_AMOUNT_DECAY 0.999f
#define TIME_AMOUNT_ATTACK 0.99f
#define TIME_K_ATTACK (2*TIME_AMOUNT_ATTACK/(1.f-TIME_AMOUNT_ATTACK))
#define TIME_K_DECAY (2*TIME_AMOUNT_DECAY/(1.f-TIME_AMOUNT_DECAY))
float slopeEg2_calcTime(uint8_t data2, float amount)
{
	const float val = (data2)/127.f;
	return 1.f-((1.f+amount)*val/(1.f+amount*fabsf(val)));

}
//--------------------------------------------------
void slopeEg2_setAttack(SlopeEg2* eg, uint8_t data2, uint8_t isSync)
{
	eg->attack = slopeEg2_calcTime(data2,TIME_K_ATTACK);
	if(isSync)
		eg->attack /= 16;
};
//--------------------------------------------------
void slopeEg2_setDecay(SlopeEg2* eg, uint8_t data2, uint8_t isSync)
{
	eg->decay = slopeEg2_calcTime(data2,TIME_K_DECAY);
	if(isSync)
			eg->decay /= 16;
};
//--------------------------------------------------
float slopeEg2_calcDecay(uint8_t data2)
{
	return slopeEg2_calcTime(data2,TIME_K_DECAY);
};
//--------------------------------------------------
void slopeEg2_setSlope(SlopeEg2* eg, uint8_t data2)
{
	const float amount 	 	= ((data2/127.f)-0.5f)*1.999f;
	//negate the amount
	const float invAmount 	= -amount;

	eg->slope 		= 2*amount/(1-amount);
	eg->invSlope	= 2*invAmount/(1-invAmount);

};
