/*
 * SomGenerator.c
 *
 *  Created on: 30.04.2013
 *      Author: Julian
 */

#include "SomGenerator.h"
#include "SomData.h"
#include "Sequencer.h"
#include "math.h"

SomGenerator somGenerator;

//-----------------------------------------------
void som_init()
{

	somGenerator.x = 0;
	somGenerator.y = 0;

	somGenerator.frequency[0] = 0x7f;
	somGenerator.frequency[1] = 0x7f;
	somGenerator.frequency[2] = 0x7f;
	somGenerator.frequency[3] = 0x7f;
	somGenerator.frequency[4] = 0x7f;
	somGenerator.frequency[5] = 0x7f;
	somGenerator.frequency[6] = 0x7f;

	somGenerator.flux = 0.1;
}
//-----------------------------------------------
void som_interpolate(uint8_t stepNr, uint8_t* values, uint8_t node1, uint8_t node2, uint8_t node3, uint8_t node4)
{

	float x = fabsf(somGenerator.x );//* rnd);
	float y = fabsf(somGenerator.y );//* rnd);

	if(x<-1)x=-1;
	else if(x>1)x=1;

	if(y<-1)y=-1;
	else if(y>1)y=1;




	int i;
	for(i=0;i<7;i++)
	{
		const uint8_t a = x*(som_nodes[node1][i*128 + stepNr]-som_nodes[node2][i*128 + stepNr]) + som_nodes[node2][i*128 + stepNr];
		const uint8_t b = x*(som_nodes[node3][i*128 + stepNr]-som_nodes[node4][i*128 + stepNr]) + som_nodes[node4][i*128 + stepNr];
		values[i] = y*(a-b)+b;

		uint8_t rnd = GetRngValue()/(float)0xffffffff * somGenerator.flux * 254 ;
		if(values[i] <= 254-rnd)
		{
			values[i] += rnd;
		}
	}


}
//-----------------------------------------------
void som_tick(uint8_t stepNr, uint8_t mutedTracks)
{

	if(stepNr%8 != 0)return;

		uint8_t values[7];

		uint8_t area = 0;
		if (somGenerator.x < 0 && somGenerator.y < 0) area = 1;
		else if (somGenerator.x < 0 && somGenerator.y > 0) area = 2;
		else if (somGenerator.x >= 0 && somGenerator.y >= 0) area = 3;
		else if (somGenerator.x >= 0 && somGenerator.y <= 0) area = 4;

		switch(area)
		{
		default:

		case 1:
			som_interpolate(stepNr,values,1,0,4,3);
			break;

		case 2:
			som_interpolate(stepNr,values,4,3,7,6);
			break;

		case 3:
			som_interpolate(stepNr,values,4,5,7,8);
			break;

		case 4:
			som_interpolate(stepNr,values,1,2,4,5);
			break;

		}

		int i;
		for(i=0;i<7;i++)
		{
			if( somGenerator.frequency[i] < values[i])
			{
				if(!(mutedTracks & (1<<i) ) )
				{
					//trigger note
					uint8_t vol = 64+ (values[i] >> 2);
					seq_triggerVoice(i,vol,SEQ_DEFAULT_NOTE);
				}

			}
		}


}
//-----------------------------------------------
void som_setX(uint8_t x)
{
	somGenerator.x = (x/63.f)-1;
}
//-----------------------------------------------
void som_setY(uint8_t y)
{
	somGenerator.y = (y/63.f)-1;
}
//-----------------------------------------------
void som_setFlux(float flux)
{
	somGenerator.flux = flux;
}
//-----------------------------------------------
void som_setFreq(uint8_t freq, uint8_t voice)
{

	somGenerator.frequency[voice] = freq*2;

}
//-----------------------------------------------
