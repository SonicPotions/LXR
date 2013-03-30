/*
 * VelocityModulation.h
 *
 *  Created on: 06.01.2013
 *      Author: Julian
 */

#ifndef VELOCITYMODULATION_H_
#define VELOCITYMODULATION_H_

#include "stm32f4xx.h"
#include "ParameterArray.h"
/*
 special cases:

 fine coarse?
 dist shape
 pan!

 */


typedef struct ModulatorStruct
{

	uint16_t	destination;	/**< dest param nr */
	uint8_t		type;			/**< pointer type */
	ptrValue	originalValue;	/**< stores the original value of the parameter*/
	float		amount;			/**< modulation amount*/
	float 		lastVal;

} ModulationNode;

//TODO move into corresponding voice
extern ModulationNode velocityModulators[6];

void modNode_init(ModulationNode* vm);

/**called whenever a value is changed. update the original value and calc a new mod value*/
//void modNode_originalValueChanged(uint16_t idx);


void modNode_resetTargets();

void modNode_reassignVeloMod();

/** if multiple nodes address the same target we need to update the other modNodes if one of them changes the destionation*/
//void modNode_originalValueModulated(uint16_t idx, ModulationNode* modSource);
void modNode_originalValueChanged(uint16_t idx);

void modNode_setDestination(ModulationNode* vm, uint16_t dest);

void modNode_updateValue(ModulationNode* vm, float val);
#endif /* VELOCITYMODULATION_H_ */
