/*
 * config.h
 *
 *  Created on: 01.04.2012
 *      Author: Julian Schmidt
 *
 *      Don't forget to make a rebuild all after you change settings in this file
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "stm32f4xx.h"
#include <stdio.h>


#define true 1
#define false 0

#define UART_DEBUG_ECHO_MODE 0

#define OUTPUT_DMA_SIZE 32
#define DMA_MASK ((OUTPUT_DMA_SIZE*2)-1)
#define UNIT_GAIN_DRIVE 0
#define SET_PARAM_ARRAY_IN_PARSER 1
#define USE_FILTER_DRIVE 0
//#define BLOCK_BASED_AUDIO 1
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

#define USE_SD_CARD 0

//wether to interpolate the oscillator wavetables or not
#define INTERPOLATE_OSC 1
#define INTERPOLATE_FM_OSC 1

#define USE_BOOTLOADER 1 	// if 1 the image will be loaded to offset 0x4000 (you also have to change stm32_flash.ld manually!!!)

//#define USE_DMA 	0

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
