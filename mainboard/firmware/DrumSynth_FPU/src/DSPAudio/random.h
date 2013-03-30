/*
 * random.h
 *
 *  Created on: 07.04.2012
 *      Author: Julian
 */

#ifndef RANDOM_H_
#define RANDOM_H_

#include "stm32f4xx.h"
void initRng();


__inline uint32_t GetRngValue();

#endif /* RANDOM_H_ */
