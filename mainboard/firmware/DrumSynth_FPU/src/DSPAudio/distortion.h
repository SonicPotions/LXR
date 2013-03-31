/*
 * distortion.h
 *
 *  Created on: 11.04.2012
 *      Author: Julian
 */

#ifndef DISTORTION_H_
#define DISTORTION_H_
//--------------------------------------------------
#include "stm32f4xx.h"
//--------------------------------------------------
typedef struct DistStruct
{
	float shape;
	float inv_shape;
}Distortion;
//--------------------------------------------------
void setDistortionShape(Distortion *dist, uint8_t shape);
//--------------------------------------------------
void calcDistBlock(const Distortion *dist, int16_t* buf, const uint8_t size);
//--------------------------------------------------
#endif /* DISTORTION_H_ */
