/*
 * config.h
 *
 *  Created on: 01.04.2012
 *      Author: Julian Schmidt
 *
 *      Don't forget to make a rebuild all after you change settings in this file
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


#ifndef CONFIG_H_
#define CONFIG_H_

#include "stm32f4xx.h"
#include <stdio.h>

#define UNUSED(x) (void)(x)

#define true 1
#define false 0

#define UART_DEBUG_ECHO_MODE 0

#define OUTPUT_DMA_SIZE 32
#define DMA_MASK ((OUTPUT_DMA_SIZE*2)-1)
#define UNIT_GAIN_DRIVE 0
#define SET_PARAM_ARRAY_IN_PARSER 1
#define USE_FILTER_DRIVE 0
#define CALC_TONE_CONTROL 0

//if 1 the amp EGs will be calculated on a per sample basis
//takes too much calcuklation time
//if 0 they are calculated for each dma buffer once, only a smoothing LP will be calculated in the sync loop
#define AMP_EG_SYNC 0

//if 1 the 3 drum voices will have the option to mix the mod osc with the main osc, instead of modulating it
#define ENABLE_MIX_OSC 1
#define ENABLE_DRUM_SVF 1

#define EG_SPEED 	1;//0.04125f
#define PITCH_AMOUNT_FACTOR 32


#define USE_DAC2	1	/**< 1: use 2nd DAC 4344 - 0: use ADC 5343*/

#define USE_SD_CARD 1 //used by the user sample memory

//wether to interpolate the oscillator wavetables or not
#define INTERPOLATE_OSC 1
#define INTERPOLATE_FM_OSC 1

#define USE_BOOTLOADER 1 	// if 1 the image will be loaded to offset 0x4000 (you also have to change stm32_flash.ld manually!!!)


#define SYNTH_FS 			I2S_AudioFreq_44k		  	// the requested I2S samplerate => 44.1 kHz

#define REAL_FS				((float)44002.7573529412f) 	// since the prescaler wont allow for accurate 44.1kHz
#define REAL_FS2			(REAL_FS/2.f)
													  	// this is the real sample rate our system is running
#define LED_BLINK_TIME  	500 						// [ms]

//the number of drum voices (bd tom1 tom2)
#define NUM_VOICES 3

#if DMA_MODE_ACTIVE
#define OUTPUT_DMA_SIZE 16
#endif

//typedef  uint8_t bool;

#endif /* CONFIG_H_ */
