/*
 * Cc2Text.c
 *
 * Created: 16.02.2013 14:34:59
 *  Author: Julian
 */ 
#include "CcNr2Text.h"
#include "menu.h"
#include <avr/pgmspace.h>

Cc2Name menu_cc2name[END_OF_SOUND_PARAMETERS];

uint8_t page2voice(uint8_t page)
{
	if(page <= VOICE7_PAGE)
	{
		return page;
	}
	else
	{
		return VOICE7_PAGE+1; //no voice param
	}		
}
void cc2Name_init()
{
	int pages;
	for(pages=0;pages<VOICE7_PAGE;pages++)
	{
		uint8_t subPages;
		for(subPages=0;subPages<NUM_SUB_PAGES;subPages++)
		{
			uint8_t entries;
			for(entries=0;entries<8;entries++) 
			{
				
				uint8_t paramNr = pgm_read_byte(&menuPages[pages][subPages].bot1 + entries);
				
				if( (paramNr != PAR_NONE) && paramNr < END_OF_SOUND_PARAMETERS )
				{
					if(paramNr<127)paramNr++;
					menu_cc2name[paramNr].nameIdx = pgm_read_byte(&menuPages[pages][subPages].top1 + entries);
					menu_cc2name[paramNr].voiceNr = page2voice(pages);
				}
			}				
		}
	}
	
	
} 