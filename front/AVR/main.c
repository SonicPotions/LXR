#include "config.h"
#include <avr/io.h>
#include "Hardware\lcd.h"
#include "encoder.h"
#include "stdlib.h"
#include <util/delay.h>
#include "Menu\menu.h"
#include "IO/din.h"
#include "IO/dout.h"
#include <stdio.h>
#include "IO/uart.h"
#include "IO/adcPots.h"

#if USE_SD_CARD
#include "Hardware\SD/SPI_routines.h"
#include "Hardware\SD/sd_routines.h"
#include "Hardware\SD/ff.h"
#endif
#include <avr/pgmspace.h>
#include "Preset/PresetManager.h"
#include "Hardware\timebase.h"
#include "Menu\screensaver.h"
#include "ledHandler.h"
#include "buttonHandler.h"
//-----------------------------------------------
//defines
//-----------------------------------------------
#define FIRMWARE_VERSION "0.21"
#define CORTEX_RESET_PIN	PB0
//-----------------------------------------------
//code
//-----------------------------------------------




int main(void) 
{
	_delay_ms(100);
	//set PD2 as out pin (LED start stop)
	DDRD   |= (1<<PD2); //configure as output
	PORTD  &= ~(1<<PD2);//disable pull up	
	
	//set the CORTEX_RESET line to input
	DDRB = 0 ;
	DDRB &= ~(1<<CORTEX_RESET_PIN);
	PORTB &= ~(1<<CORTEX_RESET_PIN);

	//init the lcd display
	lcd_init();
	
	//init the uart
	uart_init();

	//print boot up message
	lcd_string_F(PSTR("Sonic Potions"));
	//goto 2nd line
	lcd_setcursor(0,2);
	lcd_string_F(PSTR("LXR Drums V"));
	lcd_string(FIRMWARE_VERSION);
	
	
	adc_init();
	
	//init encoder
	encode_init();
		
	din_init();
	dout_init();	
	led_init();
	//init the system timer
	time_initTimer();
	

	//we have to toggle the first step sequencer led, otherwise it will be lit all the time
	led_toggle(0);
	
	//show message for 1 sec
	_delay_ms(2000);
	lcd_clear();

	
	//enable interrupts
	sei();
	
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



	/*
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
		//check the poti values
		adc_checkPots();
	
		//check if there is data received for the parameters (from mainboard)
		//4 times to speed up msg handling
		uart_checkAndParse();
		uart_checkAndParse();
		uart_checkAndParse();
		uart_checkAndParse();
		
	

		led_tickHandler();

		screensaver_check();
		
		buttonHandler_tick();
		
		//check if new sd card is available after remove
		SD_checkCardAvailable();
		
	}
		*/
//	_delay_ms(500); //give the data some time to be transmitted
#if USE_SD_CARD		
	//load init preset
	preset_loadDrumset(0,0);
#endif		

	//initialize empty pattern
	copyClear_clearCurrentPattern();

	//main loop
	while(1) 
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
