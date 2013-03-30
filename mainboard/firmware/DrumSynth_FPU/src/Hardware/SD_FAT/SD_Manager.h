/*
 * SD_Manager.h
 *
 *  Created on: 24.10.2012
 *      Author: Julian
 *
 *      handles all high level SD functionality. load/save preset/pattern etc
 */

#ifndef SD_MANAGER_H_
#define SD_MANAGER_H_
#include "config.h"
#if USE_SD_CARD
#include "stm32f4xx.h"
#include "MidiMessages.h"
#include "MidiParser.h"
#include "stdio.h"
#include "ff.h"
#include "FIFO.h"

#define NUM_PARAMS 185
#define PAR_BEGINNING_OF_GLOBALS 128

#define SD_STATE_IDLE 			0
#define SD_STATE_LOAD_SOUND		1
#define SD_STATE_SAVE_SOUND		2
#define SD_STATE_LOAD_PATTERN	3

void sdManager_init();
uint8_t sdManager_loadDrumset(uint8_t presetNr, uint8_t isMorph);
void sdManager_loadPattern(uint8_t patternNr);
void sdManager_tick(); //call periodically to ensure front panel is updated (needed because of fifo overruns when sending bulk data)



#endif /* SD_MANAGER_H_ */
#endif
