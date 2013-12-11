/*
 * Decay.c
 *
 *  Created on: 16.04.2012
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



#include "Decay.h"
#include <math.h>

//-------------------------------------------------
float DecayEg_calcSlopeValue(float val, float slope)
{
	return (1+slope)*val/(1+slope*fabsf(val));
}
//-------------------------------------------------
void DecayEg_init(DecayEg* eg)
{
	eg->decay 	= 0.01f;
	eg->value 	= 0;
};
//-------------------------------------------------
#define TIME_K (2*0.99f/(1.f-0.99f))
float DecayEg_calcTime(uint8_t data2)
{
	const float val = (data2)/127.f;
	return 1.f-((1.f+TIME_K)*val/(1.f+TIME_K*fabsf(val)));
}
//-------------------------------------------------
void DecayEg_trigger(DecayEg* eg)
{
	eg->value = 1.f;
};
//-------------------------------------------------
float DecayEg_calc(DecayEg* eg)
{
	float val = eg->value;

	if(val>0)
	{
		val -= eg->decay;

	} else
	{
		val = 0;
	}
	eg->value = val;
	return DecayEg_calcSlopeValue(val,eg->slope);

};
//-------------------------------------------------
void DecayEg_setDecay(DecayEg* eg, uint8_t data2)
{
	eg->decay = DecayEg_calcTime(data2);
}
//--------------------------------------------------
void DecayEg_setSlope(DecayEg* eg, uint8_t data2)
{
	const float amount 	= ((data2/127.f)-0.5f)*2;
	eg->slope 			= 2*amount/(1-amount);
}
//--------------------------------------------------
