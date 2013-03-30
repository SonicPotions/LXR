/*
 * automationNode.h
 *
 *  Created on: 12.02.2013
 *      Author: Julian
 *      In contrast to the modulationNode the automation Node sets an absolute value.
 *      used for the automation tracks
 */

#ifndef AUTOMATIONNODE_H_
#define AUTOMATIONNODE_H_


#include "stm32f4xx.h"
#include "MidiParser.h"



typedef struct AutomationStruct
{

	uint16_t	destination;	/**< dest param nr */
	//uint8_t		originalValue;	/**< stores the original value of the parameter*/
	//uint8_t		lastDestination /**< remember the last modified dest nr. needed for reset*/
} AutomationNode;



void autoNode_init(AutomationNode* node);

/**called whenever a value is changed. update the original value and calc a new mod value*/
//void modNode_originalValueChanged(uint16_t idx);


//void autoNode_resets();


/** if multiple nodes address the same target we need to update the other modNodes if one of them changes the destionation*/
//void autoNode_originalValueModulated(uint16_t idx, ModulationNode* modSource);
//void autoNode_originalValueChanged(uint16_t idx);

void autoNode_setDestination(AutomationNode* node, uint16_t dest);

void autoNode_updateValue(AutomationNode* node, uint8_t val);


#endif /* AUTOMATIONNODE_H_ */
