/*
 * mixer.c
 *
 *  Created on: 11.04.2012
 *      Author: Julian
 */



#include "mixer.h"
#include "config.h"
#include "AudioCodecManager.h"
#include "CymbalVoice.h"
#include "DrumVoice.h"
#include "Snare.h"
#include "HiHat.h"
#include "BufferTools.h"



uint8_t mixer_audioRouting[6];

#if USE_DECIMATOR
float mixer_decimation_rate[7];		/**<sets the sample rate decimation. 0..1 = full rate*/
float mixer_decimation_cnt[6];		/**<s'n'h counter for decimator*/
int16_t mixer_voice_samples[6];		/**< stores the last outputted sample of the 6 voices*/
#endif

//int16_t mixer_workAudioBuffer[OUTPUT_DMA_SIZE];

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

uint8_t mixer_checkOutJackAvailable(uint8_t dest)
{
	//read input pins
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

inline void mixer_moveDataToOutput(const uint8_t voiceNr,uint8_t dest, const float panL, const float panR, int16_t* data,int16_t* outL,int16_t* outR,int16_t* outL2, int16_t* outR2)
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

inline void mixer_addDataToOutput(const uint8_t voiceNr,uint8_t dest, const float panL, const float panR,  int16_t* data,int16_t* outL,int16_t* outR,int16_t* outL2, int16_t* outR2)
{
	//check if a cable is in the selected out
	dest = mixer_checkOutJackAvailable(dest);

	uint8_t i;
	switch(dest)
	{

	case MIXER_ROUTING_DAC1_STEREO:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL2 += data[i] * panL;
			outL2 += 2;

			*outR2 += data[i] * panR;
			outR2 += 2;
		}
		break;
	case MIXER_ROUTING_DAC2_STEREO:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL += data[i] * panL;
			outL += 2;

			*outR += data[i] * panR;
			outR += 2;
		}
		break;
	case MIXER_ROUTING_DAC1_L:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL2 += data[i];
			outL2 += 2;
		}
		break;
	case MIXER_ROUTING_DAC1_R:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outR2 += data[i];
			outR2 += 2;
		}
		break;
	case MIXER_ROUTING_DAC2_L:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL += data[i];
			outL += 2;
		}
		break;
	case MIXER_ROUTING_DAC2_R:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outR += data[i];
			outR += 2;
		}
		break;

	}
}

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


#if BLOCK_BASED_AUDIO


	//an array to store intermediate voice samples
	//befor output distribution
	int16_t sampleData[OUTPUT_DMA_SIZE];

	//get the current position in the DMA buffer (wraps at 31)
	const uint8_t pos = dmaBufferPtr&0x1f;
/*
	int16_t* outL = &output[pos];
	int16_t* outR = &output[pos+1];

	int16_t* outL2 = &output2[pos];
	int16_t* outR2 = &output2[pos+1];
	*/
	bufferTool_clearBuffer(output,0x1f);
	bufferTool_clearBuffer(output2,0x1f);

	//calc voice 1
	calcDrumVoiceSyncBlock(0, sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(0,sampleData);
	//copy to selected dma buffer
	mixer_moveDataToOutput(0,mixer_audioRouting[0],voiceArray[0].panL ,voiceArray[0].panR, sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1]);

	//calc voice 2
	calcDrumVoiceSyncBlock(1, sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(1,sampleData);
	//copy to selected dma buffer
	mixer_addDataToOutput(1,mixer_audioRouting[1],voiceArray[1].panL,voiceArray[1].panR, sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1]);

	//calc voice 3
	calcDrumVoiceSyncBlock(2, sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(2,sampleData);
	//copy to selected dma buffer
	mixer_addDataToOutput(2,mixer_audioRouting[2],voiceArray[2].panL,voiceArray[2].panR, sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1]);

	//calc snare
	Snare_calcSyncBlock(sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(3,sampleData);
	//copy to selected dma buffer
	mixer_addDataToOutput(3,mixer_audioRouting[3],snareVoice.panL,snareVoice.panR, sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1]);


	//calc cymbal
	Cymbal_calcSyncBlock(sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(4,sampleData);
	//copy to selected dma buffer
	mixer_addDataToOutput(4,mixer_audioRouting[4],cymbalVoice.panL,cymbalVoice.panR, sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1]);

	//calc HiHat
	HiHat_calcSyncBlock(sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(5,sampleData);
	//copy to selected dma buffer
	mixer_addDataToOutput(5,mixer_audioRouting[5],hatVoice.panL,hatVoice.panR, sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1]);


	//update dma buffer position
	dmaBufferPtr += 32;





#else

	//calc the sync samples (osc, filter etc)

	int32_t leftBuffer,rightBuffer;
	int32_t leftBuffer2,rightBuffer2;

	//TODO optimize for block based calculations instead of sample based

	for(i=0;i<OUTPUT_DMA_SIZE;i++)
	{
		//wrap pos at 31 to ensure dma interlaced stereo buffer
		const uint8_t pos = dmaBufferPtr&0x1f;
		const uint8_t pos2 = pos+1;

		int16_t* outL = &output[pos];
		int16_t* outR = &output[pos2];

		int16_t* outL2 = &output2[pos];
		int16_t* outR2 = &output2[pos2];

		leftBuffer = rightBuffer = 0;
		leftBuffer2 = rightBuffer2 = 0;


		int16_t sample;

#if USE_DAC2
		//__SMUAD() dual 16 bit signed multiplication + add produkts
#if USE_DECIMATOR
		mixer_decimation_cnt[0] += mixer_decimation_rate[0]*mixer_decimation_rate[6];
		sample = calcDrumVoiceSync(0);
		if(mixer_decimation_cnt[0] >= 1)
		{
			mixer_decimation_cnt[0] -=1;
			mixer_voice_samples[0] = sample;

		}
		sample = mixer_voice_samples[0];

#else
		sample = calcDrumVoiceSync(0);
#endif
#if USE_SWITCH_ROUTING
		switch(mixer_audioRouting[0])
		{
		case MIXER_ROUTING_DAC1_STEREO:
			leftBuffer2 += sample * voiceArray[0].panL;
			rightBuffer2 += sample * voiceArray[0].panR;
			break;
		case MIXER_ROUTING_DAC2_STEREO:
			leftBuffer += sample * voiceArray[0].panL;
			rightBuffer += sample * voiceArray[0].panR;
			break;
		case MIXER_ROUTING_DAC1_L:
			leftBuffer2 += sample;
			break;
		case MIXER_ROUTING_DAC1_R:
			rightBuffer2 += sample;
			break;
		case MIXER_ROUTING_DAC2_L:
			leftBuffer += sample;
			break;
		case MIXER_ROUTING_DAC2_R:
			rightBuffer += sample;
			break;

		}
#else
		leftBuffer = sample * voiceArray[0].panL;
		rightBuffer = sample * voiceArray[0].panR;
#endif
#if USE_DECIMATOR
		mixer_decimation_cnt[1] += mixer_decimation_rate[1]*mixer_decimation_rate[6];
		sample = calcDrumVoiceSync(1);
		if(mixer_decimation_cnt[1] >= 1)
		{
			mixer_decimation_cnt[1] -=1;
			mixer_voice_samples[1] = sample;
		}
		sample = mixer_voice_samples[1];
#else
		sample = calcDrumVoiceSync(1);
#endif

#if USE_SWITCH_ROUTING
		switch(mixer_audioRouting[1])
		{
		case MIXER_ROUTING_DAC1_STEREO:
			leftBuffer2 += sample * voiceArray[1].panL;
			rightBuffer2 += sample * voiceArray[1].panR;
			break;
		case MIXER_ROUTING_DAC2_STEREO:
			leftBuffer += sample * voiceArray[1].panL;
			rightBuffer += sample * voiceArray[1].panR;
			break;
		case MIXER_ROUTING_DAC1_L:
			leftBuffer2 += sample;
			break;
		case MIXER_ROUTING_DAC1_R:
			rightBuffer2 += sample;
			break;
		case MIXER_ROUTING_DAC2_L:
			leftBuffer += sample;
			break;
		case MIXER_ROUTING_DAC2_R:
			rightBuffer += sample;
			break;

		}
#else
		leftBuffer += sample * voiceArray[1].panL;
		rightBuffer += sample * voiceArray[1].panR;
#endif

#if USE_DECIMATOR
		mixer_decimation_cnt[2] += mixer_decimation_rate[2]*mixer_decimation_rate[6];
		sample = calcDrumVoiceSync(2);
		if(mixer_decimation_cnt[2] >= 1)
		{
			mixer_decimation_cnt[2] -=1;
			mixer_voice_samples[2] = sample;
		}
		sample = mixer_voice_samples[2];
#else
		sample = calcDrumVoiceSync(2);
#endif

#if USE_SWITCH_ROUTING
		switch(mixer_audioRouting[2])
		{
		case MIXER_ROUTING_DAC1_STEREO:
			leftBuffer2 += sample * voiceArray[2].panL;
			rightBuffer2 += sample * voiceArray[2].panR;
			break;
		case MIXER_ROUTING_DAC2_STEREO:
			leftBuffer += sample * voiceArray[2].panL;
			rightBuffer += sample * voiceArray[2].panR;
			break;
		case MIXER_ROUTING_DAC1_L:
			leftBuffer2 += sample;
			break;
		case MIXER_ROUTING_DAC1_R:
			rightBuffer2 += sample;
			break;
		case MIXER_ROUTING_DAC2_L:
			leftBuffer += sample;
			break;
		case MIXER_ROUTING_DAC2_R:
			rightBuffer += sample;
			break;

		}
#else
		leftBuffer += sample * voiceArray[2].panL;
		rightBuffer += sample * voiceArray[2].panR;
#endif


		//now add the snare
#if USE_DECIMATOR
		mixer_decimation_cnt[3] += mixer_decimation_rate[3]*mixer_decimation_rate[6];
		sample = Snare_calcSync(3);
		if(mixer_decimation_cnt[3] >= 1)
		{
			mixer_decimation_cnt[3] -=1;
			mixer_voice_samples[3] = sample;
		}
		sample = mixer_voice_samples[3];
#else
		sample = Snare_calcSync();
#endif

#if USE_SWITCH_ROUTING
		switch(mixer_audioRouting[3])
		{
		case MIXER_ROUTING_DAC1_STEREO:
			leftBuffer2 += sample * snareVoice.panL;
			rightBuffer2 += sample * snareVoice.panR;
			break;
		case MIXER_ROUTING_DAC2_STEREO:
			leftBuffer += sample * snareVoice.panL;
			rightBuffer += sample * snareVoice.panR;
			break;
		case MIXER_ROUTING_DAC1_L:
			leftBuffer2 += sample;
			break;
		case MIXER_ROUTING_DAC1_R:
			rightBuffer2 += sample;
			break;
		case MIXER_ROUTING_DAC2_L:
			leftBuffer += sample;
			break;
		case MIXER_ROUTING_DAC2_R:
			rightBuffer += sample;
			break;

		}
#else
		 leftBuffer += sample * snareVoice.panL;
		 rightBuffer += sample * snareVoice.panR;
#endif


		 /*
		 sample = Snare_calcSync(1);
		 *outL += sample * snareVoice[1].panL;
		 *outR += sample * snareVoice[1].panR; */
#if USE_DECIMATOR
		mixer_decimation_cnt[4] += mixer_decimation_rate[4]*mixer_decimation_rate[6];
		sample = Cymbal_calcSync(4);
		if(mixer_decimation_cnt[4] >= 1)
		{
			mixer_decimation_cnt[4] -=1;
			mixer_voice_samples[4] = sample;
		}
		sample = mixer_voice_samples[4];
#else
		sample = Cymbal_calcSync();
#endif

#if USE_SWITCH_ROUTING
		switch(mixer_audioRouting[4])
		{
		case MIXER_ROUTING_DAC1_STEREO:
			leftBuffer2 += sample * cymbalVoice.panL;
			rightBuffer2 += sample * cymbalVoice.panR;
			break;
		case MIXER_ROUTING_DAC2_STEREO:
			leftBuffer += sample * cymbalVoice.panL;
			rightBuffer += sample * cymbalVoice.panR;
			break;
		case MIXER_ROUTING_DAC1_L:
			leftBuffer2 += sample;
			break;
		case MIXER_ROUTING_DAC1_R:
			rightBuffer2 += sample;
			break;
		case MIXER_ROUTING_DAC2_L:
			leftBuffer += sample;
			break;
		case MIXER_ROUTING_DAC2_R:
			rightBuffer += sample;
			break;

		}
#else
		 leftBuffer += sample * cymbalVoice.panL;
		 rightBuffer += sample * cymbalVoice.panR;
#endif


		//HiHat
#if USE_DECIMATOR
		mixer_decimation_cnt[5] += mixer_decimation_rate[5]*mixer_decimation_rate[6];
		sample = HiHat_calcSync(5);
		if(mixer_decimation_cnt[5] >= 1)
		{
			mixer_decimation_cnt[5] -=1;
			mixer_voice_samples[5] = sample;
		}
		sample = mixer_voice_samples[5];
#else
		sample = HiHat_calcSync();
#endif

#if USE_SWITCH_ROUTING
		switch(mixer_audioRouting[5])
		{
		case MIXER_ROUTING_DAC1_STEREO:
			leftBuffer2 += sample * hatVoice.panL;
			rightBuffer2 += sample * hatVoice.panR;
			break;
		case MIXER_ROUTING_DAC2_STEREO:
			leftBuffer += sample * hatVoice.panL;
			rightBuffer += sample * hatVoice.panR;
			break;
		case MIXER_ROUTING_DAC1_L:
			leftBuffer2 += sample;
			break;
		case MIXER_ROUTING_DAC1_R:
			rightBuffer2 += sample;
			break;
		case MIXER_ROUTING_DAC2_L:
			leftBuffer += sample;
			break;
		case MIXER_ROUTING_DAC2_R:
			rightBuffer += sample;
			break;

		}
#else
		 leftBuffer += sample * hatVoice.panL;
		 rightBuffer += sample * hatVoice.panR;
#endif


		 //DAC1 clip calculated outputs to 16 Bit
		 *outL = __SSAT(leftBuffer,16);
		 *outR = __SSAT(rightBuffer,16);

		 //DAC2
		 *outL2 = __SSAT(leftBuffer2,16);
		 *outR2 = __SSAT(rightBuffer2,16);
		// *outL = *outR =  GetRngValue();
#else
		//this is an ADC input codec test...
		 //it just forwards the input to the output

#if USE_DECIMATOR
		mixer_decimation_cnt[0] += mixer_decimation_rate[0]*mixer_decimation_rate[6];
	//	sample = *outL2;
		if(mixer_decimation_cnt[0] >= 1)
		{
			mixer_decimation_cnt[0] -=1;
			mixer_voice_samples[0] = *outL2;
			mixer_voice_samples[1] = *outR2;

		}
		 *outL = mixer_voice_samples[0];
		 *outR = mixer_voice_samples[1];

#else
		 *outL = *outL2;
		 *outR = *outR2;
#endif



#endif

		dmaBufferPtr+=2;
	}

#endif
}
