/*
 * transientGenerator.c
 *
 *  Created on: 29.06.2012
 *      Author: Julian
 */


#include "transientGenerator.h"

//---------------------------------------------------------------
void transient_init(TransientGenerator* transient)
{
	transient->pitch 	= 1.f;
	transient->output 	= 0;
	transient->phase	= 0;
	transient->waveform	= 0;
	transient->volume	= 1.f;


};
//---------------------------------------------------------------
void transient_trigger(TransientGenerator* transient)
{
	//reset the phase to the beginning when the generator is triggered
	transient->phase	= 0;
}
//---------------------------------------------------------------
void transient_calcBlock(TransientGenerator* transient, int16_t* buf, const uint8_t size)
{
	if(transient->waveform<=1)
	{
		memset(buf,0,size*sizeof(int16_t));
		return; //snapEg and offset
	}

	uint8_t i;
	for(i=0;i<size;i++)
	{
		uint32_t phase = transient->phase;
		phase = phase >> 20;

		buf[i] = transient->volume*(transientData[transient->waveform-2][phase]<<8) * (phase < TRANSIENT_SAMPLE_LENGTH);//* transientVolumeTable[phase>>5];

		//if phase is < then table size, we increment it
		transient->phase += (transient->phase<2311061504u) * (transient->pitch*(1<<20)); //2311061504 => 2204<<20

	}
}
//---------------------------------------------------------------
void transient_calc(TransientGenerator* transient)
{
	uint32_t phase = transient->phase;
	//if phase is < than table size we increment it
	phase += (phase<2311061504u) * (transient->pitch*(1<<20)); //2311061504 => 2204<<20
	transient->phase = phase;
	phase = phase >> 20;
	if(transient->waveform==0)
	{
		//click
		transient->output = transient->volume * 32512 * transientVolumeTable[phase>>5];

	}
	else
	{
		transient->output = transient->volume*(transientData[transient->waveform-1][phase]<<8) * transientVolumeTable[phase>>5];
	}




};
//---------------------------------------------------------------
