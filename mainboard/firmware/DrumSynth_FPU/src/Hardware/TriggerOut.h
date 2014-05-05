/*
 * TriggerOut.h
 *
 *  Created on: 06.05.2013
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


#ifndef TRIGGEROUT_H_
#define TRIGGEROUT_H_

#include "globals.h"

#define PIN_TRACK_1 GPIO_Pin_8	//Port D
#define PIN_TRACK_2 GPIO_Pin_9	//Port D
#define PIN_TRACK_3 GPIO_Pin_10	//Port D
#define PIN_TRACK_4 GPIO_Pin_11	//Port D
#define PIN_TRACK_5 GPIO_Pin_12	//Port D
#define PIN_TRACK_6 GPIO_Pin_13	//Port D
#define PIN_TRACK_7 GPIO_Pin_14	//Port D

#define PIN_CLOCK_1 GPIO_Pin_15	//Port D
#define PIN_CLOCK_2 GPIO_Pin_9	//Port A
#define PIN_RESET GPIO_Pin_10	//Port A

#define PIN_CLOCK_IN GPIO_Pin_9 //Port C
#define PIN_RESET_IN GPIO_Pin_8 //Port A

#define PULSE_LENGTH_MS 20 //[ms]
#define PULSE_LENGTH (PULSE_LENGTH_MS*4) //systick 0.25ms

enum
{
	TRIGGER_1 = 0,
	TRIGGER_2,
	TRIGGER_3,
	TRIGGER_4,
	TRIGGER_5,
	TRIGGER_6,
	TRIGGER_7,

	CLOCK_1,
	CLOCK_2,
	TRIGGER_RESET,
	TRIGGER_ALL,
	NUM_PINS,
};

// since we have 4 main steps per quarter and 8 sub steps per main step
// our native resolution is 32ppq
enum Prescaler
{
	PRE_1_PPQ	= 32/1,
	PRE_4_PPQ	= 32/4,
	PRE_8_PPQ	= 32/8,
	PRE_16_PPQ	= 32/16,
	PRE_32_PPQ	= 1,
};

typedef enum TriggerModes
{
	TRIGGER_ON,
	TRIGGER_OFF,
	TRIGGER_PULSE
} triggerMode;

extern uint8_t trigger_dividerClockOut1;
extern uint8_t trigger_dividerClockOut2;
extern uint8_t trigger_prescalerClockInput;

void trigger_init();
void trigger_tick();
void trigger_triggerVoice(uint8_t voice, triggerMode mode);
void trigger_clockTick(uint8_t pos);//the sequencer calls this function whenever a step is played to generate corresponding trigger out clocks
void trigger_reset(uint8_t value);
void trigger_tickPhaseCounter();
uint8_t trigger_isGateModeOn();
void trigger_setGatemode(uint8_t onOff);	//if gate mode is on, the trigger out will be high until a note off is send
void trigger_allOff();



#endif /* TRIGGEROUT_H_ */

