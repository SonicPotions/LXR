/*
 * CcNr2Text.h
 *
 * Created: 16.02.2013 13:57:05
 *  Author: Julian
 */ 


#ifndef CCNR2TEXT_H_
#define CCNR2TEXT_H_

#include "menu.h"

typedef struct Cc2NameStruct
{
	uint8_t voiceNr; // 1-6 => 0x6 = 0b110
	uint8_t nameIdx;// 
} Cc2Name;

//todo: move to progmem once the list is fixed
extern Cc2Name menu_cc2name[END_OF_SOUND_PARAMETERS];

void cc2Name_init();


#endif /* CCNR2TEXT_H_ */