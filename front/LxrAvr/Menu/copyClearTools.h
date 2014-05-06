/*
 * copyClearTools.h
 *
 * Created: 26.01.2013 08:44:09
 *  Author: Julian
 */ 


#ifndef COPYCLEARTOOLS_H_
#define COPYCLEARTOOLS_H_

#include <avr/io.h>

//copy/clear mode helpers
enum
{
	MODE_NONE,
	MODE_CLEAR,
	MODE_COPY_PATTERN,
	MODE_COPY_TRACK,
};

//clear targets
enum
{
	CLEAR_TRACK,
	CLEAR_PATTERN,
	CLEAR_AUTOMATION1,
	CLEAR_AUTOMATION2,
};
extern uint8_t copyClear_Mode;

void copyClear_executeClear();
void copyClear_clearCurrentTrack();
void copyClear_clearCurrentPattern();
void copyClear_copyTrack();
void copyClear_copyPattern();
uint8_t copyClear_getClearTarget();
uint8_t copyClear_isClearModeActive();
void copyClear_setClearTarget(uint8_t mode);
void copyClear_armClearMenu(uint8_t isShown);

uint8_t copyClear_srcSet();
void copyClear_setSrc(int8_t src, uint8_t type);
void copyClear_setDst(int8_t dst, uint8_t type);

void copyClear_reset();


#endif /* COPYCLEARTOOLS_H_ */