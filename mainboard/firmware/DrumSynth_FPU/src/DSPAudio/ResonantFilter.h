/*
 * ResonantFilter.h
 *
 *  Created on: 05.04.2012
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


#ifndef RESONANTFILTER_H_
#define RESONANTFILTER_H_

/*
 * ResonantFilter.c
 *
 *  Created on: 05.04.2012
 *      Author: Julian
 */
//----------------------------------------------------
#include "globals.h"
#include "config.h"
#include "stm32f4xx.h"
#include "datatypes.h"
#include "math.h"
#include "distortion.h"
//----------------------------------------------------
//removed to free some cpu cycles
#define ENABLE_NONLINEAR_INTEGRATORS 	1
#define FILTER_GAIN 					0x70ff

#define USE_SHAPER_NONLINEARITY 0


//----------------------------------------------------
enum filterTypeEnum
{
	FILTER_LP=1,
	FILTER_HP,
	FILTER_BP,
	FILTER_UNITY_BP,
	FILTER_NOTCH,
	FILTER_PEAK
};
//----------------------------------------------------
typedef struct ResoFilterStruct
{
	float f;	/**< cutoff frequency as integer from 0 to 0xffff where 0xffff = SR*/
	float g;    /**< embedded integrator gain (Fig 3.11), wc == wa*/
	float q;	/**< q value calculated from setReso()*/

	float s1;
	float s2;
#if ENABLE_NONLINEAR_INTEGRATORS
	float zi;	//input z^(-1)
#endif

	float drive;

#if USE_SHAPER_NONLINEARITY
	Distortion shaper;
#endif
} ResonantFilter;
//------------------------------------------------------------------------------------
void SVF_setReso(ResonantFilter* filter, float feedback);
//------------------------------------------------------------------------------------
void SVF_init();
//------------------------------------------------------------------------------------
void SVF_setDrive(ResonantFilter* filter, uint8_t drive);
//------------------------------------------------------------------------------------
void SVF_directSetFilterValue(ResonantFilter* filter, float val);
//------------------------------------------------------------------------------------
void SVF_calcBlockZDF(ResonantFilter* filter, const uint8_t type, int16_t* buf, const uint8_t size);
//------------------------------------------------------------------------------------
void SVF_recalcFreq(ResonantFilter* filter);

#endif /* RESONANTFILTER_H_ */
