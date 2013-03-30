/*
 * EuklidGenerator.c
 *
 *  Created on: 04.08.2012
 *      Author: Julian
 */


#include "EuklidGenerator.h"
#include "sequencer.h"

static uint8_t euklid_length[NUM_TRACKS];
static uint8_t euklid_steps[NUM_TRACKS];

static uint16_t euklid_patternBuffer;	/**< 16 bits for maximum 16 steps. 1 is a note 0 is a pause*/

static uint16_t euklid_nextCnt1=0;
static uint16_t euklid_originalLength=0;
//-----------------------------------------------------
void euklid_init()
{
	int i;
	for(i=0;i<NUM_TRACKS;i++)
	{
		euklid_length[i] = 16;
		euklid_steps[i] = 4;
	}

	euklid_patternBuffer = 0;
}
//-----------------------------------------------------
/** calculate how many elements are in one group.
 *  the leading zeroes up to the first 1 are counted using the _CLZ(uint32) function starting at MSB
 *  since a group has always at least 1 element we don't return 0
 */
inline uint8_t getObjSize(uint16_t group)
{
	uint8_t ret = 32 - __CLZ((uint32_t)group);
	if(ret==0) return 1;
	return ret;
}
//-----------------------------------------------------

void euklid_calcRecursive(uint8_t length, uint8_t steps, uint8_t iteration, uint16_t group1, uint16_t group2)
{
	int i,j;
	uint16_t remainder;

	//omit division by zero
	if(steps!=0)
		remainder 	= length%steps;
	else
		remainder = 0;

	//In the first iteration we have to set the next count value to the number of steps
	if(euklid_nextCnt1==0)euklid_nextCnt1=steps;

	//calculate the objects per group and their size
	int objectCnt1 = euklid_nextCnt1;
	int objSize1 = getObjSize(group1);
	int objSize2 = getObjSize(group2);
	int objCnt2 = (euklid_originalLength - (objectCnt1*objSize1))  / (objSize2);

	//the number of group 1 objects in the next iteration equals the count of objects in the smaller group
	euklid_nextCnt1 = objectCnt1>objCnt2?objCnt2:objectCnt1;

	//if the 2nd group has only 1 or less objects left
	//the algorhythm has finished
	//--> write the generated pattern to euklid_patternBuffer
	if(objCnt2<=1 )
	{
		uint8_t bufferPos=0;
		//group1 objects
		for(i=0;i<objectCnt1;i++)
		{
			for(j=objSize1-1;j>=0;j--)
			{
				if(group1&(1<<j))
				{
					//printf("x");
					euklid_patternBuffer |= (1<<bufferPos);
				}
				bufferPos++;
			}
		}
		//group 2 objects
		for( i=0;i<objCnt2;i++)
		{
			for( j=objSize2-1;j>=0;j--)
			{
				if(group2&(1<<j))
				{
					//printf("x");
					euklid_patternBuffer |= (1<<bufferPos);
				}
				bufferPos++;
			}
		}

		return;
	}

	//shift the group 1 value to make space for the new group2 part
	//and save the original group content
	uint16_t originalGroup1 = group1;
	group1 = group1 << objSize2;


	//paste the group 2 values to the free space at the end of group 1
	for(i=0;i<objSize2;i++)
	{
		//if group2 contains a 1, set in in group1
		if(group2 & (1<<i) )
		{
			group1 |= (1<<i);
		}
	}

	//if there are group 2 objects left group 2 stays the same
	//if no group 2 objects are left the rest of the old group1 will be the new group 2
	group2 = objCnt2-objectCnt1>0?group2:originalGroup1;

	//start the next iteration
	euklid_calcRecursive(steps,remainder,iteration+1,group1,group2);

}
//-----------------------------------------------------
void euklid_generate(uint8_t trackNr, uint8_t patternNr)
{
	uint8_t length,steps;
	length = euklid_length[trackNr];
	steps = euklid_steps[trackNr];
	//todo ist das hier nötig? zero steps sollte doch gehen...
	if(steps==0)steps++;

	//reset the global values
	euklid_nextCnt1 = 0;
	euklid_originalLength = length;

	//clear the buffer
	euklid_patternBuffer = 0;

	//let's calculate the new pattern
	euklid_calcRecursive(length,steps,1,1,0); //always start with 1st iterations
	//and store it in the active track
	euklid_transferPattern(trackNr, patternNr);
}
//-----------------------------------------------------
uint8_t euklid_getLength(uint8_t trackNr)
{
	return euklid_length[trackNr];
}
//-----------------------------------------------------
uint8_t euklid_getSteps(uint8_t trackNr)
{
	return euklid_steps[trackNr];
};

//-----------------------------------------------------
void euklid_setLength(uint8_t trackNr, uint8_t value, uint8_t patternNr)
{
	if(value<=0)value=1;
//	if(value<euklid_steps[trackNr] ) euklid_steps[trackNr] = value;
	euklid_length[trackNr] = value;
//	euklid_generate(trackNr, patternNr);
}
//-----------------------------------------------------
void euklid_setSteps(uint8_t trackNr, uint8_t value, uint8_t patternNr)
{
	if(value<=0)value=1;

	if(value>euklid_length[trackNr]) value= euklid_length[trackNr];

	euklid_steps[trackNr] = value;
	euklid_generate(trackNr, patternNr);
}
//-----------------------------------------------------
void euklid_transferPattern(uint8_t trackNr, uint8_t patternNr)
{
	int i;


	//TODO wäre schön wenn der originale noten wert erhalten bleibt...
	//dazu wäre aber ein flag auf bit 8 nötig das auch vom sequencer verstanden wird
	//copy the temp buffer to the current pattern track

	seq_patternSet.seq_mainSteps[patternNr][trackNr] = euklid_patternBuffer;

	//for(i=0;i<euklid_length[trackNr]*8;i++)
	for(i=0;i<128;i++)
	{
		//if(seq_patternSet.seq_subStepPattern[patternNr][trackNr][i].note >= PATTERN_END_MASK)
		if(seq_patternSet.seq_subStepPattern[patternNr][trackNr][i].note & PATTERN_END)
		{
			//TODO: Attention! this resets pitch information
			//seq_patternSet.seq_subStepPattern[patternNr][trackNr][i].note = SEQ_DEFAULT_NOTE;
			seq_patternSet.seq_subStepPattern[patternNr][trackNr][i].note &= ~PATTERN_END;
		}

		//if(((i%8)==0) && (euklid_patternBuffer&(  1<<(15-(i/8))  )  ))
		/*
		if((i%8==0) && (euklid_patternBuffer&(1<<(i/8)) )   )
		{
			seq_patternSet.seq_subStepPattern[patternNr][trackNr][i].volume |= STEP_ACTIVE_MASK;


		}
		else
		{
			seq_patternSet.seq_subStepPattern[patternNr][trackNr][i].volume &= ~STEP_ACTIVE_MASK;
		}
		*/
	}
	if(euklid_length[trackNr] <16)
	{
		seq_patternSet.seq_subStepPattern[patternNr][trackNr][euklid_length[trackNr]*8].note |= PATTERN_END;
	}
}
