/*
 * 1PoleLp.h
 *
 *  Created on: 08.04.2012
 *      Author: Julian
 */

#ifndef ONE_POLELP_H_
#define ONE_POLELP_H_
//-------------------------------------------------------------
#include "stm32f4xx.h"
//-------------------------------------------------------------
typedef struct OnePoleStruct
{
	float r;
	float b;
} OnePoleFilter;
//-------------------------------------------------------------
void initOnePole(OnePoleFilter* filter);
float calcOnePole(OnePoleFilter* filter, float input);
void calcOnePoleBlock(OnePoleFilter* filter, int16_t* buf, const uint8_t size);
void calcOnePoleBlockFixedInput(OnePoleFilter* filter, float input,float* output, const uint8_t size);
void setOnePoleCoef(OnePoleFilter* filter,float val);
void setOnePoleValue(OnePoleFilter* filter,float val);

#endif /* ONE_POLELP_H_ */
