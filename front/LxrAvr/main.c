#include "config.h"
#include <avr/io.h>
#include "Hardware/lcd.h"
#include "encoder.h"
#include "stdlib.h"
#include <util/delay.h>
#include "Menu/menu.h"
#include "IO/din.h"
#include "IO/dout.h"
#include <stdio.h>
#include "IO/uart.h"
#include "IO/adcPots.h"

#if USE_SD_CARD
#include "Hardware/SD/SPI_routines.h"
#include "Hardware/SD/sd_routines.h"
#include "Hardware/SD/ff.h"
#endif

#include <avr/pgmspace.h>
#include "Preset/PresetManager.h"
#include "Hardware/timebase.h"
#include "Menu/screensaver.h"
#include "ledHandler.h"
#include "buttonHandler.h"
#include "Menu/copyClearTools.h"
#include "frontPanelParser.h"
//-----------------------------------------------
//defines
//-----------------------------------------------
#define FIRMWARE_VERSION "0.34"
#define CORTEX_RESET_PIN	PB0
//-----------------------------------------------
//code
//-----------------------------------------------

//debug helper to catch unhandled interrupts
//-----------------------------------------------
ISR(__vector_default)
{
#if DEBUG_CRASH_MODE	
	lcd_home();
	lcd_string_F(PSTR("Int. Error!"));
	while(1);
#endif
	
} 
//-----------------------------------------------
int main(void) 
{
	_delay_ms(100);
		
	//set the CORTEX_RESET line to input
	DDRB = 0 ;
	DDRB &= (uint8_t)~(1<<CORTEX_RESET_PIN);
	PORTB &= (uint8_t)~(1<<CORTEX_RESET_PIN);

	//init the lcd display
	lcd_init();

	//init the uart
	uart_init();

#if UART_DEBUG_ECHO_MODE
	//print boot up message
	lcd_home();
	lcd_string_F(PSTR("Sonic Potions"));
	//goto 2nd line
	lcd_setcursor(0,2);
	lcd_string_F(PSTR("UART test"));

#else
	//print boot up message
	lcd_home();
	lcd_string_F(PSTR("Sonic Potions"));
	//goto 2nd line
	lcd_setcursor(0,2);
	lcd_string_F(PSTR("LXR Drums V"));
	lcd_string(FIRMWARE_VERSION);
#endif

	
#if UART_DEBUG_ECHO_MODE
_delay_ms(1000);
sei();
	uint8_t debucCounter = 0;
	while(1) 
	{	
		uart_checkAndParse();
		_delay_ms(1000);
		uart_putc(debucCounter++);
		
	}		
#endif
	
	adc_init();
	
	//init encoder
	encode_init();
		
	din_init();
	dout_init();	
	led_init();
	//init the system timer
	time_initTimer();
	
	//toggle the first step sequencer led, otherwise it will be lit all the time
	led_toggle(0);
	
	//show message for 1 sec
	_delay_ms(2000);
	lcd_clear();

#if DEBUG_CRASH_MODE	
	//debug helper crash
	lcd_home();
	lcd_string_F(PSTR("preparing sei()"));
	_delay_ms(2000);
#endif
	//enable interrupts
	sei();
	
#if DEBUG_CRASH_MODE		
	//debug helper crash
	lcd_home();
	lcd_string_F(PSTR("sei() enabled"));
	_delay_ms(2000);
#endif	
	
	//init menu
	menu_init();
	
	//init sd card, fat and preset manager
	//has to be called after sei() because it sends uart data via interrupts
#if USE_SD_CARD
	preset_init();
#endif	

	//we have to read out the pots once to have a defined state before loading the preset data
	adc_checkPots();
	
#if USE_SD_CARD	
	preset_loadGlobals();
#endif

#if USE_SD_CARD		
	//load init preset
	if(!preset_loadDrumset(0,0)) {
		lcd_home();
		lcd_string_F(PSTR("Kit read error"));
		_delay_ms(2000);
		preset_sendDrumsetParameters(); // send initial parameters (all 0's)
	}
#endif		

	//initialize empty pattern
	copyClear_clearCurrentPattern();
	
	//request number of samples
	frontPanel_sendData(SAMPLE_CC,SAMPLE_COUNT,0x00);



	//main loop
	for(;;) // this usually results in less instructions than while(1)
	{		
		//read next button
		din_readNextInput();
		//update LEDs
		dout_updateOutputs();
		
		uart_checkAndParse();
		//get the encoder values
		const int8_t encoderValue = encode_read4();  
		const uint8_t button = encode_readButton();
		
		//update the menu according to the encoder values
		menu_parseEncoder(encoderValue,button);
	
		//check if there is data received for the parameters (from mainboard)
		//4 times to speed up msg handling
		uart_checkAndParse();
		uart_checkAndParse();
		uart_checkAndParse();
		uart_checkAndParse();
		
		//check the poti values
		adc_checkPots();

		led_tickHandler();

		screensaver_check();
		
		buttonHandler_tick();
		
		//check if new sd card is available after remove
		SD_checkCardAvailable();
		
	}
return 0;
}
//-----------------------------------------------
