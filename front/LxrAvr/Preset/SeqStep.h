/*
 * SeqStep.h
 *
 * Created: 26.06.2012 16:46:31
 *  Author: Julian
 */ 


#ifndef SEQSTEP_H_
#define SEQSTEP_H_
#include <avr/io.h>

/** a copy of the step struct from the sequencer.h file from the cortex src
used to load step data from sequencer and write it to the pattern preset*/
typedef struct StepStruct
{
	//uint8_t  	active;		// bit 0: on/off
	uint8_t 	volume;		// 0-127 volume -> 0x7f => lower 7 bit, upper bit => active
	uint8_t  	prob;		//step probability
	uint8_t		note;

	//parameter automation
	uint8_t 	param1Nr;
	uint8_t 	param1Val;

	uint8_t 	param2Nr;
	uint8_t 	param2Val;

}StepData;




#endif /* SEQSTEP_H_ */