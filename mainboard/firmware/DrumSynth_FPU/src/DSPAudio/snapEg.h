/*
 * snapEg.h
 *
 *  Created on: 16.06.2013
 *      Author: Julian
 */

#ifndef SNAPEG_H_
#define SNAPEG_H_

#include "stm32f4xx.h"


typedef struct Snap_EG_Struct
{

	float value;
} SnapEg;


void SnapEg_init(SnapEg* eg);
void SnapEg_trigger(SnapEg* eg);
float SnapEg_calc(SnapEg* eg, float time);
void SnapEg_calcBlock(SnapEg* eg,int16_t* buf, const uint8_t size);


#endif /* SNAPEG_H_ */
