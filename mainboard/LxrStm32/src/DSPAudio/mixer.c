/*
 * mixer.c
 *
 *  Created on: 11.04.2012
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

#include "mixer.h"
#include "config.h"
#include "AudioCodecManager.h"
#include "CymbalVoice.h"
#include "DrumVoice.h"
#include "Snare.h"
#include "HiHat.h"
#include "BufferTools.h"
#include "squareRootLut.h"
#include "../Hardware/TriggerOut.h"
//-----------------------------------------------------------------------
uint8_t mixer_audioRouting[6];
//-----------------------------------------------------------------------
#if USE_DECIMATOR
float mixer_decimation_rate[7];		/**<sets the sample rate decimation. 0..1 = full rate*/
float mixer_decimation_cnt[6];		/**<s'n'h counter for decimator*/
int16_t mixer_voice_samples[6];		/**< stores the last outputted sample of the 6 voices*/
#endif
//-----------------------------------------------------------------------
void mixer_init()
{
#if USE_DECIMATOR
	int i;
	for(i=0;i<6;i++)
	{
		mixer_decimation_rate[i] 	= 1;
		mixer_decimation_cnt[i] 	= 0;
		mixer_voice_samples[i] 		= 0;
		mixer_audioRouting[i]		= 0;
	}
	mixer_decimation_rate[6] 		= 1;
#endif
}
//-----------------------------------------------------------------------
void mixer_decimateBlock(const uint8_t voiceNr, int16_t* buffer)
{
	uint8_t i;
	for(i=0;i<OUTPUT_DMA_SIZE;i++)
	{
		mixer_decimation_cnt[voiceNr] += mixer_decimation_rate[voiceNr]*mixer_decimation_rate[6];
		if(mixer_decimation_cnt[voiceNr] >= 1.f)
		{
			mixer_decimation_cnt[voiceNr] -= 1.f;
			mixer_voice_samples[voiceNr] = buffer[i];

		}
		buffer[i] = mixer_voice_samples[voiceNr];
	}
}
//-----------------------------------------------------------------------
uint8_t mixer_checkOutJackAvailable(uint8_t dest)
{
	//read input pins
	//TODO move pin reading in low priority idle loop
	uint8_t r2_Available = (GPIOC->IDR & GPIO_Pin_4);
	uint8_t l1_Available = (GPIOC->IDR & GPIO_Pin_5);
	uint8_t l2_Available = (GPIOA->IDR & GPIO_Pin_0);
	uint8_t r1_Available = (GPIOA->IDR & GPIO_Pin_5);

	switch(dest)
		{

		case MIXER_ROUTING_DAC1_STEREO:
			if(r1_Available || l1_Available) {
				return dest;
			} else {
				return MIXER_ROUTING_DAC2_STEREO;
			}
			break;

		case MIXER_ROUTING_DAC2_STEREO:
			if(r2_Available || l2_Available) {
				return dest;
			} else {
				return MIXER_ROUTING_DAC1_STEREO;
			}
			break;

		case MIXER_ROUTING_DAC1_L:
			if(l1_Available) {
				return dest;
			} else if (r1_Available) {
				return MIXER_ROUTING_DAC1_R;
			} else if (l2_Available) {
				return MIXER_ROUTING_DAC2_L;
			} else if (r2_Available) {
				return MIXER_ROUTING_DAC2_R;
			}
			break;

		case MIXER_ROUTING_DAC1_R:
			if(r1_Available) {
				return dest;
			} else if (l1_Available) {
				return MIXER_ROUTING_DAC1_L;
			} else if (l2_Available) {
				return MIXER_ROUTING_DAC2_L;
			} else if (r2_Available) {
				return MIXER_ROUTING_DAC2_R;
			}
			break;

		case MIXER_ROUTING_DAC2_L:
			if(l2_Available) {
				return dest;
			} else if (r2_Available) {
				return MIXER_ROUTING_DAC2_R;
			} else if (l1_Available) {
				return MIXER_ROUTING_DAC1_L;
			} else if (r1_Available) {
				return MIXER_ROUTING_DAC1_R;
			}
			break;

		case MIXER_ROUTING_DAC2_R:
			if(r2_Available) {
				return dest;
			} else if (l2_Available) {
				return MIXER_ROUTING_DAC2_L;
			} else if (r1_Available) {
				return MIXER_ROUTING_DAC1_R;
			} else if (l1_Available) {
				return MIXER_ROUTING_DAC1_L;
			}
			break;
		}
	return dest;
}
//-----------------------------------------------------------------------
inline void mixer_moveDataToOutput(uint8_t dest, const float panL, const float panR, int16_t* data,int16_t* outL,int16_t* outR,int16_t* outL2, int16_t* outR2)
{
	//check if a cable is in the selected out
	dest = mixer_checkOutJackAvailable(dest);

	uint8_t i;
	switch(dest)
	{

	case MIXER_ROUTING_DAC1_STEREO:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL2 = data[i] * panL;
			outL2 += 2;

			*outR2 = data[i] * panR;
			outR2 += 2;
		}
		break;
	case MIXER_ROUTING_DAC2_STEREO:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL = data[i] * panL;
			outL += 2;

			*outR = data[i] * panR;
			outR += 2;
		}
		break;
	case MIXER_ROUTING_DAC1_L:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL2 = data[i];
			outL2 += 2;
		}
		break;
	case MIXER_ROUTING_DAC1_R:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outR2 = data[i];
			outR2 += 2;
		}
		break;
	case MIXER_ROUTING_DAC2_L:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL = data[i];
			outL += 2;
		}
		break;
	case MIXER_ROUTING_DAC2_R:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outR = data[i];
			outR += 2;
		}
		break;
	}
}
//-----------------------------------------------------------------------
inline void mixer_addDataToOutput(uint8_t dest, const float panL, const float panR,  int16_t* data,int16_t* outL,int16_t* outR,int16_t* outL2, int16_t* outR2)
{
	//check if a cable is in the selected out
	dest = mixer_checkOutJackAvailable(dest);

	//TODO may be possible tooptimize here using both halfwordsof qadd for stereo mixing
	uint8_t i;
	switch(dest)
	{

	case MIXER_ROUTING_DAC1_STEREO:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL2 = __QADD16(*outL2,(int16_t)(data[i] * panL)) & 0xFFFF;
			outL2 += 2;

			*outR2 = __QADD16(*outR2,(int16_t)(data[i] * panR)) & 0xFFFF;
			outR2 += 2;
		}
		break;
	case MIXER_ROUTING_DAC2_STEREO:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL = __QADD16(*outL,(int16_t)(data[i] * panL)) & 0xFFFF;
			outL += 2;

			*outR = __QADD16(*outR,(int16_t)(data[i] * panR)) & 0xFFFF;
			outR += 2;
		}
		break;
	case MIXER_ROUTING_DAC1_L:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL2 = __QADD16(*outL2,data[i]) & 0xFFFF;
			outL2 += 2;
		}
		break;
	case MIXER_ROUTING_DAC1_R:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outR2 = __QADD16(*outR2,data[i]) & 0xFFFF;
			outR2 += 2;
		}
		break;
	case MIXER_ROUTING_DAC2_L:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL = __QADD16(*outL,data[i]) & 0xFFFF;
			outL += 2;
		}
		break;
	case MIXER_ROUTING_DAC2_R:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outR = __QADD16(*outR,data[i]) & 0xFFFF;
			outR += 2;
		}
		break;

	}
}
//-----------------------------------------------------------------------
void mixer_calcNextSampleBlock(int16_t* output,int16_t* output2)
{
	modNode_resetTargets();
	//re assign velocity modulation
	modNode_reassignVeloMod();

	//calc and dispatch LFO
	lfo_dispatchNextValue(&voiceArray[0].lfo);
	lfo_dispatchNextValue(&voiceArray[1].lfo);
	lfo_dispatchNextValue(&voiceArray[2].lfo);
	lfo_dispatchNextValue(&snareVoice.lfo);
	lfo_dispatchNextValue(&cymbalVoice.lfo);
	lfo_dispatchNextValue(&hatVoice.lfo);

	//update filter frequencies
	SVF_recalcFreq(&voiceArray[0].filter);
	SVF_recalcFreq(&voiceArray[1].filter);
	SVF_recalcFreq(&voiceArray[2].filter);
	SVF_recalcFreq(&snareVoice.filter);
	SVF_recalcFreq(&cymbalVoice.filter);
	SVF_recalcFreq(&hatVoice.filter);

	//--- Calc async -----
	calcDrumVoiceAsync(0);
	calcDrumVoiceAsync(1);
	calcDrumVoiceAsync(2);

	Snare_calcAsync();
	Cymbal_calcAsync();
	HiHat_calcAsync();

	//calculate trigger io phase
	trigger_tickPhaseCounter();

	//an array to store intermediate voice samples
	//befor output distribution
	int16_t sampleData[OUTPUT_DMA_SIZE];

	const uint8_t pos = 0;

	bufferTool_clearBuffer(output,OUTPUT_DMA_SIZE*2);
	bufferTool_clearBuffer(output2,OUTPUT_DMA_SIZE*2);

	//calc voice 1
	calcDrumVoiceSyncBlock(0, sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(0,sampleData);
	//copy to selected dma buffer
	mixer_addDataToOutput(mixer_audioRouting[0],squareRootLut[127-voiceArray[0].pan] ,squareRootLut[voiceArray[0].pan], sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1]);


	//calc voice 2
	calcDrumVoiceSyncBlock(1, sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(1,sampleData);
	//copy to selected dma buffer
	mixer_addDataToOutput(mixer_audioRouting[1],squareRootLut[127-voiceArray[1].pan] ,squareRootLut[voiceArray[1].pan], sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1]);

	//calc voice 3
	calcDrumVoiceSyncBlock(2, sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(2,sampleData);
	//copy to selected dma buffer
	mixer_addDataToOutput(mixer_audioRouting[2],squareRootLut[127-voiceArray[2].pan] ,squareRootLut[voiceArray[2].pan], sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1]);

	//calc snare
	Snare_calcSyncBlock(sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(3,sampleData);
	//copy to selected dma buffer
	mixer_addDataToOutput(mixer_audioRouting[3],squareRootLut[127-snareVoice.pan],
			squareRootLut[snareVoice.pan], sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1]);
	//calc cymbal
	Cymbal_calcSyncBlock(sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(4,sampleData);
	//copy to selected dma buffer
	mixer_addDataToOutput(mixer_audioRouting[4],squareRootLut[127-cymbalVoice.pan],
			squareRootLut[cymbalVoice.pan], sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1]);



	//calc HiHat
	HiHat_calcSyncBlock(sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(5,sampleData);
	//copy to selected dma buffer
	mixer_addDataToOutput(mixer_audioRouting[5],squareRootLut[127-hatVoice.pan],
			squareRootLut[hatVoice.pan], sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1]);


}
