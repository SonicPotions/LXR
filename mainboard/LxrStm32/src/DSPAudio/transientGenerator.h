/*
 * transientGenerator.h
 *
 *  Created on: 29.06.2012
 *
 *      the transient generator generates a kind of 'click' oneshot sound when triggered
 *      a variety of different, very short [50 ms] transient samples is used
 *
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
