/*
 * LCD_driver.c
 *
 *  Created on: 20.04.2012
 *      Author: Julian
 */


#include "LCD_driver.h"
#include "globals.h"
#include "FIFO.h"
//#include "delay.h"
#if USE_LCD

void initPort()
{
	//configure the IO port to use lcd
	GPIO_InitTypeDef  GPIO_InitStructure;
	/* LCD port Periph clock enable */
	RCC_AHB1PeriphClockCmd(LCD_CLOCK, ENABLE);
	/* Configure pins in output push-pull mode */
	GPIO_InitStructure.GPIO_Pin = LCD_RS | LCD_E| LCD_DB4| LCD_DB5| LCD_DB6| LCD_DB7;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	  /* standard output pin */
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	  GPIO_Init(LCD_PORT, &GPIO_InitStructure);
	//  LCD_PORT->ODR = 0;
}


//turn off the optimisation for the wait function, otherwise GCC will do funky stuff and the programm gets stuck here
#pragma GCC push_options
#pragma GCC optimize ("O0")
/** wait for ms*/
void wait(uint32_t ms)
{
	const volatile uint32_t startTicks = systick_ticks;

	while(1)
	{
		if(systick_ticks-startTicks >= ms) break;
	}

}
//restore optimisation level
#pragma GCC pop_options

void enableData()
{
	//set enable line high
	LCD_PORT->ODR |= LCD_E;
	//min 500 ns warten - das wären 84 cycles bei 168 mHz :(

	wait(1);
	//set enable line low
	LCD_PORT->ODR &= ~LCD_E;

}

void sendLowNibble(uint8_t low)
{
	//clear data lines
	LCD_PORT->ODR &= 0xf0;
	//set data lines
	LCD_PORT->ODR |= low&0x0f;
	enableData();
	//wait(46);
}

void sendByte(uint8_t data)
{
	//first send hi nibble
	//clear data lines
	LCD_PORT->ODR &= 0xf0;
	//set data lines
	LCD_PORT->ODR |= (data&0xf0)>>4;
	enableData();
	//wait(46);

	//send low nibble
	//clear data lines
	LCD_PORT->ODR &= 0xf0;
	//set data lines
	LCD_PORT->ODR |= data&0x0f;
	enableData();
	//wait(46);
}

// Sendet den Befehl zur Löschung des Displays
void lcd_clear( void )
{
	//clear RS pin => send command
	LCD_PORT->ODR &= ~LCD_RS;

	sendByte( LCD_CLEAR_DISPLAY );
    wait( 2 );
}

void lcd_setcursor( uint8_t x, uint8_t y )
{
    uint8_t data;

    switch (y)
    {
        case 1:    // 1. Zeile
            data = LCD_SET_DDADR + LCD_DDADR_LINE1 + x;
            break;

        case 2:    // 2. Zeile
            data = LCD_SET_DDADR + LCD_DDADR_LINE2 + x;
            break;

        case 3:    // 3. Zeile
            data = LCD_SET_DDADR + LCD_DDADR_LINE3 + x;
            break;

        case 4:    // 4. Zeile
            data = LCD_SET_DDADR + LCD_DDADR_LINE4 + x;
            break;

        default:
            return;                                   // für den Fall einer falschen Zeile
    }

	//clear RS pin => send command
	LCD_PORT->ODR &= ~LCD_RS;

    sendByte( data );

    //daten senden
    	LCD_PORT->ODR |= LCD_RS;
}

void lcd_string( const char *data )
{
    while( *data != '\0' )
    	lcdFifo_bufferIn( *data++ );
    	//sendByte( *data++ );
}

void lcd_init()
{

	initPort();

	//wait for min. 15ms untill lcd controller is ready
	wait(15);

	//clear RS pin => send command
	LCD_PORT->ODR &= ~LCD_RS;

	//activate 4-bit mode
	sendLowNibble(0x03);
	//mind 4.1ms warten
	wait(5);

	sendLowNibble(0x03);
	//mind 100us warten
	wait(1);

	sendLowNibble(0x03);
	sendLowNibble(0x02);

	//we are now in 4-bit mode

	//configure the display

	// 4-bit Modus / 2 Zeilen / 5x7
	sendByte(LCD_SET_FUNCTION |
			 LCD_FUNCTION_4BIT |
			 LCD_FUNCTION_2LINE |
			 LCD_FUNCTION_5X7);

	 // Display ein / Cursor aus / Blinken aus
	sendByte( LCD_SET_DISPLAY |
			 LCD_DISPLAY_ON |
			 LCD_CURSOR_OFF |
			 LCD_BLINKING_OFF);

	// Cursor inkrement / kein Scrollen
	sendByte( LCD_SET_ENTRY |
			 LCD_ENTRY_INCREASE |
			 LCD_ENTRY_NOSHIFT );

	lcd_clear();

	sendByte(LCD_CURSOR_HOME);
	wait( 2 );

	//daten senden
	LCD_PORT->ODR |= LCD_RS;

	//sendByte('0'); //0
	lcd_string("hallo welt");


}

#endif

