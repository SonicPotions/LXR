/**
*****************************************************************************
**
**  File        : main.c
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

#include "stm32f4xx.h"
#include "core_cm4.h"
#include <stdio.h>
#include <stdint.h>

#include "globals.h"
#include "config.h"
#include "wavetable.h"
#include "Oscillator.h"
#include "ResonantFilter.h"
#include "random.h"
#include "AudioCodecManager.h"
#include "sequencer.h"

#include "mixer.h"
#include "Uart.h"
#include "FIFO.h"
#include "DrumVoice.h"
#include "CymbalVoice.h"
#include "HiHat.h"
#include "Snare.h"
#include "EuklidGenerator.h"
#include "ParameterArray.h"
#include "modulationNode.h"
#include "SomGenerator.h"

#include "usb_manager.h"
#include "MidiParser.h"

#include "TriggerOut.h"
#include <string.h>

//----------------------------------------------------------------
//stub function for newlib
void _exit(int status)
{
	_exit(status);
};

int _sbrk(int nbytes)
{
	UNUSED(nbytes);
	return 0;
};

#define __MYPID 1
int
_getpid()
{
  return __MYPID;
}

/*
 * kill -- go out via exit...
 */
int _kill(int pid, int sig)
{
  if(pid == __MYPID)
    _exit(sig);
  return 0;
}
//----------------------------------------------------------------
//----------------------------------------------------------------
// var to hold the clock infos
RCC_ClocksTypeDef RCC_Clocks;


float filFreq = .9f;
float sign = 1.f;
float tempo = 0.00005f;
float gain = 1.0f;
uint8_t fType = 0;
//----------------------------------------------------------------

#define SPI_MISO_MOSI_PORT	GPIOA
#define SPI_CS_SCK_PORT		GPIOB
#define SPI_MISO_PIN		GPIO_Pin_6	//a
#define SPI_MOSI_PIN		GPIO_Pin_7	//a
#define SPI_SCK_PIN			GPIO_Pin_3  //b
#define SPI_CS_PIN			GPIO_Pin_0  //b

#define SPI_MISO_PIN_SRC	GPIO_PinSource6	//a
#define SPI_MOSI_PIN_SRC	GPIO_PinSource7	//a
#define SPI_SCK_PIN_SRC		GPIO_PinSource3  //b
#define SPI_CS_PIN_SRC		GPIO_PinSource0  //b
//disable SPI pins since SD card is used by AVR
void initSpiPins()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	// init the pins
	//MISO and MOSI
	GPIO_InitStructure.GPIO_Pin = SPI_MISO_PIN | SPI_MOSI_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(SPI_MISO_MOSI_PORT, &GPIO_InitStructure);
	//SCK pins
	GPIO_InitStructure.GPIO_Pin = SPI_SCK_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(SPI_CS_SCK_PORT, &GPIO_InitStructure);
	//CS
	GPIO_InitStructure.GPIO_Pin = SPI_CS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(SPI_CS_SCK_PORT, &GPIO_InitStructure);
}
  //----------------------------------------------------------------

void initAudioJackDiscoverPins()
{
	//l1 = pa0
	//r2 = PA5
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_5;

	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//r1 = pc4
	//l2 = pc5
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;

	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

}

//---------------------------------------------------------
inline void calcNextSampleBlock()
{
#if USE_DAC2
	mixer_calcNextSampleBlock((int16_t*)&dma_buffer[bCurrentSampleValid*(OUTPUT_DMA_SIZE*2)],(int16_t*)&dma_buffer2[bCurrentSampleValid*(OUTPUT_DMA_SIZE*2)]);
#else
	mixer_calcNextSampleBlock(&dma_buffer[bCurrentSampleValid*(OUTPUT_DMA_SIZE*2)],&dma_buffer2[(1-bCurrentSampleValid)*(OUTPUT_DMA_SIZE*2)]);
#endif
	bCurrentSampleValid = SAMPLE_VALID;
}
//---------------------------------------------------------
int main(void)
{
	initSpiPins();

	/* get system clock info*/
	RCC_GetClocksFreq(&RCC_Clocks);
	/* set timebase systick to 1ms*/
	//SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);
	// looks like it's really being set to .25 ms
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 4000);

	initAudioJackDiscoverPins();

	mixer_init();

	//precalc the first dma buffer block
	calcNextSampleBlock();

	// start the audio codec
	CodecInit(SYNTH_FS);

	parameterArray_init();

	trigger_init();

	int i;
	for(i=0;i<6;i++)
	{
		modNode_init(&velocityModulators[i]);
	}

	initMidiUart();

	initFrontpanelUart();

	initRng();

	euklid_init();

	som_init();

	//init the seq speed
	seq_init();
	seq_setBpm(120);

	memset(midi_MidiChannels,0,8);
	memset(midi_NoteOverride,0,7);

	initDrumVoice();
	Snare_init();
	HiHat_init();
	Cymbal_init();

	usb_init();

	//--------------------------------------------------------------------
	//------------------------- Main Loop --------------------------------
	//--------------------------------------------------------------------

#if UART_DEBUG_ECHO_MODE
    while (1)
    {
    	//process incoming frontpanel data
    			uart_processFront();
    }
#else

    while (1)
    {

    	usb_tick();
    	//generate next sample if no valid sample is present
    	if(bCurrentSampleValid!= SAMPLE_VALID)
    	{
    		calcNextSampleBlock();
    	}

		//process midi on midi port
		uart_processMidi();

		//process incoming frontpanel data
		uart_processFront();

		//check if we have some usb midi messages and process them
		MidiMsg msg;
		if(usb_getMidi(&msg)) {
			midiParser_parseMidiMessage(msg);
		}

		//generate next sample if no valid sample is present
		if(bCurrentSampleValid!= SAMPLE_VALID)
		{
			calcNextSampleBlock();
		}
		//process the sequencer
		seq_tick();

		//handle trigger outs
		trigger_tick();
    }
#endif
}


