/*
 * dither.c
 *
 *  Created on: 20.06.2013
 *      Author: Julian
 */

#include "dither.h"

int16_t dither_process(Dither* dither, float in)
{

	dither->r2 = dither->r1;                               						//can make HP-TRI dither by
	dither->r1 = GetRngValue();//rand();                          							 	//subtracting previous rand()

	in += DITHER_S * (dither->s1 + dither->s1 - dither->s2);            		//error feedback
	dither->tmp = in + DITHER_O + DITHER_D * (float)(dither->r1 - dither->r2); 	//dc offset and dither

	dither->out = (int)(DITHER_W * dither->tmp);                								//truncate downwards
	if(dither->tmp<0.0f) dither->out--;                  						//this is faster than floor()

	dither->s2 = dither->s1;
	dither->s1 = in - DITHER_WI * (float)dither->out;           				//error

	return dither->out;
}
