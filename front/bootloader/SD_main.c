//***********************************************************
// **** MAIN routine for Sonic Potions SD Card Bootloader ***
//***********************************************************
//Controller: ATmega32 (Clock: 16 Mhz-external)
//Compiler	: AVR-GCC (winAVR with AVRStudio)

//Author original FAT32 and SD routines	: CC Dharmani, Chennai (India)
//										  www.dharmanitech.com
//Link to the original src: http://www.dharmanitech.com/2009/01/sd-card-interfacing-with-atmega8-fat32.html

//Modified for bootloader:	Julian Schmidt
//							www.sonic-potions.de
//Date		: 7 Jul 2012
//***********************************************************



#define F_CPU 20000000UL		//freq 20 MHz
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "SPI_routines.h"
#include "SD_routines.h"
#include "UART_routines.h"
#include "FAT32.h"
#include "lcd/lcd.h"
#include "avr/wdt.h"
#include "userInterface.h"


void port_init(void)
{
PORTA = 0x00;
DDRA  = 0x00;
PORTB = 0xEF;
DDRB  = 0xBF; //MISO line i/p, rest o/p
PORTC = 0x00;
DDRC  = 0x00;
PORTD = 0x00;
//DDRD  = 0xFE;

//init cortex reset output pin
	//as input
	CORTEX_RESET_DDR &= ~(1<<CORTEX_RESET_PIN);
	//no pull up
	CORTEX_RESET_PORT &= ~(1<<CORTEX_RESET_PIN);


}

//---------------------------------------------------------------------
//jump to address 0x0000
void (*start)( void ) = 0x0000;
//---------------------------------------------------------------------
//call this routine to initialize all peripherals
void init_devices(void)
{
 cli();  //all interrupts disabled
 port_init();
 spi_init();

// twi_init();
 uart_init();

 MCUCR = 0x00;
#ifdef MEGA32 
 GICR  = 0x00;
 TIMSK = 0x00; //timer interrupt sources
#else
// MCUCR  = 0x00;
 TIMSK0 = 0x00; //timer interrupt sources
#endif
 //all peripherals are now initialized
 
  lcd_init();
 
}
//---------------------------------------------------------------------
//*************************** MAIN *******************************//
int __attribute__((OS_main)) __attribute__((noreturn))
  main(void)
{
	unsigned char error;
	unsigned int i;
	_delay_ms(100);  //delay for VCC stabilization
	

	
	 //init user ui
	ui_init();
	
	if(ui_isButtonPressed())
	{
		init_devices();
		lcd_string("bootloader");
		_delay_ms(500);
		cardType = 0;

		for (i=0; i<10; i++)
		{
		  error = SD_init();
		  if(!error) break;
		}

		if(error)
		{
			lcd_clear();
			lcd_home();
			lcd_string("SD error");
			while(1)  //wait here forever if error in SD init 
			{
			  
			  _delay_ms(100);
			  //ui_ledToggle();	  			  
			}
		}
		else
		{

			SPI_HIGH_SPEED;	//SCK - 4 MHz
			_delay_ms(1);   //some delay

			error = getBootSectorData (); //read boot sector and keep necessary data in global variables
			if(error) 	
			{
				lcd_clear();
				lcd_string("FS error");
				while(1)
				{	
		  		  _delay_ms(500);
				  //ui_ledToggle();
				}				  
			}
			else
			{
				//name without dot -> 11 byte in fat entry
				readFile((unsigned char*)"FIRMWAREBIN");	
				
				
				//software reset
				//wdt_enable(WDTO_15MS) ;
				//while(1);
				cli();
				start();
			}
		}	
	}		
	
	//Start normal program
	/* vor Rücksprung eventuell benutzte Hardware deaktivieren
       und Interrupts global deaktivieren, da kein "echter" Reset erfolgt */
 
    /* Interrupt Vektoren wieder gerade biegen */
    cli();
    /*
	temp = MCUCR;
    MCUCR = temp | (1<<IVCE);
    MCUCR = temp & ~(1<<IVSEL);
	*/
 
    /* Rücksprung zur Adresse 0x0000 */
//	SIGNAL_LED_PORT |= ((1<<SIGNAL_LED));

	//EIND = 0;
    start(); 

//	return 0;
}

