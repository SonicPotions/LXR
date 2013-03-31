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
//-------------------------------------------------------------
#include "stm32f4xx.h"
#include "MidiParser.h"
//-------------------------------------------------------------
typedef struct AutomationStruct
{
	uint16_t	destination;	/**< dest param nr */
} AutomationNode;
//-------------------------------------------------------------
void autoNode_init(AutomationNode* node);
void autoNode_setDestination(AutomationNode* node, uint16_t dest);
void autoNode_updateValue(AutomationNode* node, uint8_t val);

#endif /* AUTOMATIONNODE_H_ */
