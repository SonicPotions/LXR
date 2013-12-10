/*
 * screensaver.c
 *
 * Created: 25.01.2013 17:08:02
 *  Author: Julian
 */ 
//to expand the lifetime of OLED displays, we need a timeout var to go to
//screensaver mode if the controls are not touched for a while
#include "../Hardware/timebase.h"
#include "../Hardware/lcd.h"

#include "menu.h"

#define SCREENSAVER_ACTIVE 1

//stuff for activation logic
volatile uint16_t screensaver_timer = 0;
#define SCREENSAVER_TIMEOUT 2//[minutes]
#define SCREENSAVER_SYSTICK (uint16_t)(((SCREENSAVER_TIMEOUT*60) / (1.f / ((F_CPU/1024.f)/256.f))))
volatile uint8_t screensaver_Active=0;
volatile uint16_t screensaver_Timeout=SCREENSAVER_SYSTICK;


//stuff for fancy drawing
static uint16_t screensaver_processTime = 0;
#define SCREENSAVER_PROCESS_SPEED ((0.5) / (1.f / ((F_CPU/1024.f)/256.f)))
enum
{
	CELL_OFF,
	CELL_1,
	CELL_2,
	CELL_3
};

uint8_t screensaver_activeCell = 0;
uint8_t screensaver_cellState = CELL_OFF;

uint16_t screensaver_rng =  0;	//*<temporary var for rng generation*/
//------------------------------------------------------------------
uint8_t screensaver_pseudoRng()
{
	screensaver_rng++;
	screensaver_rng *= 3;
	return (screensaver_rng >> 2) & 0xFF;
}
//------------------------------------------------------------------
void screensaver_touch()
{
	#if SCREENSAVER_ACTIVE
	screensaver_timer = 0;
	screensaver_Timeout = SCREENSAVER_SYSTICK;

	if(screensaver_Active)
	{
		//lcd_turnOn(1,0);
		screensaver_Active = 0;
		menu_repaintAll();
	}	
	#endif		
}
//------------------------------------------------------------------
//draw some fancy stuff on the LCD
void screensaver_process()
{
#if SCREENSAVER_ACTIVE
	if(time_sysTick >= screensaver_processTime )
	{
		screensaver_processTime = (uint16_t)( time_sysTick + SCREENSAVER_PROCESS_SPEED);
		
		if(screensaver_cellState == CELL_OFF)
		{
			screensaver_activeCell = screensaver_pseudoRng()&0x1f; //value between 0 and 31
		}
		
		char text[2];
		text[1] = 0;
		switch(screensaver_cellState)
		{
			default:
			case CELL_OFF:
				lcd_clear();
				screensaver_cellState = CELL_1;
				break;
				
				case CELL_1:
				if(screensaver_activeCell>15)
				{
					lcd_setcursor((uint8_t)(screensaver_activeCell-16),2);
				}
				else
				{
					lcd_setcursor(screensaver_activeCell,1);
				}				
				text[0] = 0xDF; //.
				lcd_string(text);
				screensaver_cellState = CELL_2;
				break;
				
				case CELL_2:
				if(screensaver_activeCell>15)
				{
					lcd_setcursor((uint8_t)(screensaver_activeCell-16),2);
				}
				else
				{
					lcd_setcursor(screensaver_activeCell,1);
				}				
				text[0] = 0xEF; //o
				lcd_string(text);
				screensaver_cellState = CELL_3;
				break;
				
				case CELL_3:
				if(screensaver_activeCell>15)
				{
					lcd_setcursor((uint8_t)(screensaver_activeCell-16),2);
				}
				else
				{
					lcd_setcursor(screensaver_activeCell,1);
				}				
				text[0] = 0xEE; //pop
				lcd_string(text);
				screensaver_cellState = CELL_OFF;
				break;
		}
	}
#endif	
}
//------------------------------------------------------------------
void screensaver_check()
{
#if SCREENSAVER_ACTIVE
	if(parameter_values[PAR_SCREENSAVER_ON_OFF])
	{
		if( (!screensaver_Active) )
		{
			if(screensaver_timer >= screensaver_Timeout)
			{
				//lcd_turnOn(0,0);
				screensaver_Active = 1;	
			}
		}
		else
		{
			screensaver_process();
		}	
	}
#endif
};
//------------------------------------------------------------------
