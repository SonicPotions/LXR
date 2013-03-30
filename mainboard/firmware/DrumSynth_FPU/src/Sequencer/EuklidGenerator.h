/*
 * EuklidGenerator.h
 *
 *  Created on: 04.08.2012
 *      Author: Julian
 */

#ifndef EUKLIDGENERATOR_H_
#define EUKLIDGENERATOR_H_

#include "stm32f4xx.h"

void euklid_init();
uint8_t euklid_getLength(uint8_t trackNr);
uint8_t euklid_getSteps(uint8_t trackNr);
void euklid_setLength(uint8_t trackNr, uint8_t value, uint8_t patternNr);
void euklid_setSteps(uint8_t trackNr, uint8_t value, uint8_t patternNr);
void euklid_transferPattern(uint8_t trackNr, uint8_t patternNr);
#endif /* EUKLIDGENERATOR_H_ */
