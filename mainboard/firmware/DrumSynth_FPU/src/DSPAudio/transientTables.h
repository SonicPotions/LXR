/*
 * transientTables.h
 *
 *  Created on: 29.06.2012
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
