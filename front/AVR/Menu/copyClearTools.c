/*
 * copyClearTools.c
 *
 * Created: 26.01.2013 08:45:55
 *  Author: Julian
 */ 
#include "copyClearTools.h"
#include "../frontPanelParser.h"
#include <avr/io.h>
#include "../Hardware/lcd.h"
#include "../ledHandler.h"
#include "menu.h"

uint8_t copyClear_Mode = MODE_NONE;
#define SRC_DST_NONE -1
int8_t buttonHandler_copySrc = SRC_DST_NONE;
int8_t buttonHandler_copyDst = SRC_DST_NONE;
static uint8_t copyClear_clearTarget = CLEAR_TRACK;


//-----------------------------------------------------------------------------
void copyClear_clearTrackAutom(uint8_t automTrack)
{
	uint8_t voice = menu_getActiveVoice();
	frontPanel_sendData(SEQ_CC,SEQ_CLEAR_AUTOM,(uint8_t)((voice<<4)|(automTrack&0x0f)));
};
//-----------------------------------------------------------------------------
void copyClear_clearCurrentPattern()
{
	uint8_t pattern = menu_getViewedPattern();
	led_clearSequencerLeds();
	frontPanel_sendData(SEQ_CC,SEQ_CLEAR_PATTERN,pattern);
};
//-----------------------------------------------------------------------------
void copyClear_executeClear()
{
	switch(copyClear_clearTarget)
	{
		default:
		case CLEAR_TRACK:
			copyClear_clearCurrentTrack();
		break;
		
		case CLEAR_PATTERN:
			copyClear_clearCurrentPattern();
		break;
		
		case CLEAR_AUTOMATION1:
			copyClear_clearTrackAutom(0);
		break;
		
		case CLEAR_AUTOMATION2:
			copyClear_clearTrackAutom(1);
		break;
	}
	
	copyClear_armClearMenu(0);
	copyClear_Mode = MODE_NONE;
};
//-----------------------------------------------------------------------------
void copyClear_reset()
{
	copyClear_Mode = MODE_NONE;
	led_clearAllBlinkLeds();
	buttonHandler_copySrc = buttonHandler_copyDst = SRC_DST_NONE;
	
};
//-----------------------------------------------------------------------------
void copyClear_setSrc(int8_t src, uint8_t type)
{
	buttonHandler_copySrc = src;
	copyClear_Mode = type;
};
//-----------------------------------------------------------------------------
void copyClear_setDst(int8_t dst, uint8_t type)
{
	buttonHandler_copyDst = dst;
	copyClear_Mode = type;
};
//-----------------------------------------------------------------------------
uint8_t copyClear_srcSet()
{
	return buttonHandler_copySrc != SRC_DST_NONE;
}
//-----------------------------------------------------------------------------
void copyClear_clearCurrentTrack()
{
	uint8_t voice = menu_getActiveVoice();
	led_clearSequencerLeds();
	frontPanel_sendData(SEQ_CC,SEQ_CLEAR_TRACK,voice);
	
};
//-----------------------------------------------------------------------------
void copyClear_copyTrack()
{
	if(copyClear_Mode != MODE_COPY_TRACK)
	{
		return;
	}
	uint8_t value = (uint8_t)(((buttonHandler_copySrc&0xf)<<4) | (buttonHandler_copyDst&0xf));
	led_clearSequencerLeds();
	frontPanel_sendData(SEQ_CC,SEQ_COPY_TRACK,value);
	
	buttonHandler_copySrc = buttonHandler_copyDst = SRC_DST_NONE;
};
//-----------------------------------------------------------------------------
void copyClear_copyPattern()
{
	if(copyClear_Mode != MODE_COPY_PATTERN)
	{
		return;
	}
	uint8_t value = (uint8_t)(((buttonHandler_copySrc&0xf)<<4) | (buttonHandler_copyDst&0xf));
	led_clearSequencerLeds();
	frontPanel_sendData(SEQ_CC,SEQ_COPY_PATTERN,value);
	
	buttonHandler_copySrc = buttonHandler_copyDst = SRC_DST_NONE;
};
//-----------------------------------------------------------------------------
uint8_t copyClear_isClearModeActive() 
{
	return (copyClear_Mode == MODE_CLEAR);
}
//-----------------------------------------------------------------------------
uint8_t copyClear_getClearTarget() 
{
	return copyClear_clearTarget;
}	
//-----------------------------------------------------------------------------
void copyClear_setClearTarget(uint8_t mode)
{
	copyClear_clearTarget = mode;
	if(copyClear_Mode == MODE_CLEAR)
	{
		//repaint
		copyClear_armClearMenu(1);
	}
}
//-----------------------------------------------------------------------------
void copyClear_armClearMenu(uint8_t isShown)
{
	if(isShown)
	{
		//TODO this wastes RAM!!!
		lcd_clear();
		lcd_setcursor(0,1);
		lcd_string("clear [");
		
		switch(copyClear_clearTarget)
		{
			default:
			case CLEAR_TRACK:
			lcd_string("track");
			break;
			
			case CLEAR_PATTERN:
			lcd_string("pattern");
			break;
			
			case CLEAR_AUTOMATION1:
			lcd_string("autom.1");
			break;
			
			case CLEAR_AUTOMATION2:
			lcd_string("autom.2");
			break;
		}
		lcd_string("]?");
		led_clearAllBlinkLeds();
		led_setValue(1,LED_COPY);

		
	}
	else
	{
		led_setValue(0,LED_COPY);
		menu_repaintAll();
	}
};
//-----------------------------------------------------------------------------
