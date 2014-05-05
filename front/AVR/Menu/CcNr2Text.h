/*
 * CcNr2Text.h
 *
 * Created: 16.02.2013 13:57:05
 *  Author: Julian
 */ 


#ifndef CCNR2TEXT_H_
#define CCNR2TEXT_H_

#include "menu.h"

//typedef struct Cc2NameStruct
//{
//	uint8_t voiceNr; // 1-6 => 0x6 = 0b110
//	uint8_t nameIdx;//
//} Cc2Name;

// initialize the above array
void paramToModTargetInit();

// return the total number of mod targets that exist (all voices)
uint8_t getNumModTargets();

// returns 1 based index or 0 for invalid
uint8_t voiceFromModTargValue(uint8_t val);

// returns an index into modTargets based on a 0 based voice # and an index into modtarget gaps array
uint8_t getModTargetIdxFromGapIdx(uint8_t v, uint8_t gapidx);

// return an index into mod target gap array given an index into modTargets
uint8_t getModTargetGapIndex(uint8_t modTargetIdx);

#endif /* CCNR2TEXT_H_ */
