/*
 * FileParser.c
 *
 * Created: 07.07.2012 17:57:36
 *  Author: Julian
 */ 
#include "FileParser.h"
#include "sd_routines.h"
#include "avr/interrupt.h"
#include "avr/boot.h"
#include "userInterface.h"
#include "config.h"
#include <stdlib.h>
#include <util/delay.h>
#include "Bootloader/BootloaderCommands.h"
//TODO: contributors für crc !!!
#include "util/crc16.h"

struct InfoHeader infoHeader;
volatile uint32_t fileParser_bytesRead=0;
volatile uint16_t fileParser_pagesWritten=0;

enum StateEnum
{
	INFO_HEADER,
	AVR_DATA,
	CORTEX_DATA
};

volatile uint8_t fileParser_parseState=INFO_HEADER;

volatile uint32_t addressCounter=0;



uint16_t calcCrc(uint8_t cmd, uint8_t* data)
{
	uint16_t crc=0;
	
	crc = _crc_xmodem_update(crc,cmd);
	crc = _crc_xmodem_update(crc,data[3]);
	crc = _crc_xmodem_update(crc,data[2]);
	crc = _crc_xmodem_update(crc,data[1]);
	crc = _crc_xmodem_update(crc,data[0]);
	
	return crc;
}
//------------------------------------------------------------------------
void boot_program_page (uint32_t page, uint8_t *buf)
{
    uint16_t i;
    uint8_t sreg;
 
    /* Disable interrupts.*/
    sreg = SREG;
    cli();
 
    eeprom_busy_wait ();
 
    boot_page_erase (page);
    boot_spm_busy_wait ();      /* Wait until the memory is erased. */
 
    for (i=0; i<SPM_PAGESIZE; i+=2)
    {
        /* Set up little-endian word. */
        uint16_t w = *buf++;
        w += (*buf++) << 8;
    
        boot_page_fill (page + i, w);
		//boot_page_fill (page + i, page&0xffff);
    }
    boot_page_write (page);     /* Store buffer in flash page.		*/
    boot_spm_busy_wait();       /* Wait until the memory is written.*/
 
    /* Reenable RWW-section again. We need this if we want to jump back */
    /* to the application after bootloading. */
    boot_rww_enable ();
 
    /* Re-enable interrupts (if they were ever enabled). */
    SREG = sreg;
}
//------------------------------------------------------------------------
void fileParser_resetCortex()
{
	//set port low

	//pin is output
	CORTEX_RESET_DDR |= (1<<CORTEX_RESET_PIN);
	//signal low
	CORTEX_RESET_PORT &= ~(1<<CORTEX_RESET_PIN);
	_delay_ms(500);
	
	//set port high (input)
	//as input
	CORTEX_RESET_DDR &= ~(1<<CORTEX_RESET_PIN);
	//no pull up
	CORTEX_RESET_PORT &= ~(1<<CORTEX_RESET_PIN);
	_delay_ms(50);
	
};

//------------------------------------------------------------------------

uint8_t fileParser_parseNextBlock(unsigned long filesize)
{

	switch(fileParser_parseState)
	{
		case INFO_HEADER:
		{
			//--- read info header ---
			struct InfoHeader *tmpHeader = (struct InfoHeader *) &sd_buffer[0];
			infoHeader = *tmpHeader;
	
			if(	(infoHeader.headerId[0] != 'S') ||
				(infoHeader.headerId[1] != 'P') ||
				(infoHeader.headerId[2] != 'F') ||
				(infoHeader.headerId[3] != 'I')
			)
			{
				//header not right -> abort
				
				lcd_clear();
				lcd_home();
				lcd_string("hd err");
				
				while(1);
				return 0;
			}
			/*
			lcd_clear();
			lcd_home();
			
			lcd_string("fnd hd");
			lcd_setcursor(0,2);
			
			lcd_string("avr ");
			*/
		//	char text[10];
		//	uint8_t hi,lo;
		//	hi = infoHeader.avrCodeSize>>8;
		//	lo = infoHeader.avrCodeSize&0xff;
		//	infoHeader.avrCodeSize = hi | (lo<<8);
		//	uint16_t test = infoHeader.avrCodeSize;
		//	lcd_string(utoa(test,text,10));
		//	_delay_ms(1500);
			
		
			fileParser_bytesRead += 512;
			
			if ((fileParser_bytesRead) >= filesize )
			{
				
				lcd_setcursor(0,2);
				lcd_string("end hd");
				while(1);
				
				 return 0;
			}		
			
			fileParser_parseState = AVR_DATA;
		}			
		break;
	
		case AVR_DATA:
		{
			//--- read avr code ---
			
	
				
			//program data into flash (512 bytes data to 4 pages a 128 bytes -> mega32 SPM_PAGESIZE = 128
			//program data into flash (512 bytes data to 2 pages a 256 bytes -> mega644 SPM_PAGESIZE = 256
			
			
			for(int i=0; (i<(512/SPM_PAGESIZE)) && (fileParser_bytesRead < (infoHeader.avrCodeSize +  512)); i++ )
			{
				boot_program_page(fileParser_pagesWritten,(uint8_t *)&sd_buffer[0+i*SPM_PAGESIZE]);
				fileParser_bytesRead += SPM_PAGESIZE;
				fileParser_pagesWritten+=SPM_PAGESIZE;
				
				//toggle the start stop led
			//	ui_ledToggle();	
				/*
							lcd_clear();
			lcd_home();
			lcd_string("page");
			
			char text[10];
			lcd_string(utoa(fileParser_pagesWritten,text,10));
			*/
			//lcd_setcursor(0,2);
			//uint16_t tmp = sd_buffer[0+i*SPM_PAGESIZE];
			//lcd_string(utoa(tmp,text,10));
			//_delay_ms(500)
			;/*
lcd_setcursor(0,2);
			
			lcd_string("wrt ");
			//char text[10];
			lcd_string(utoa(fileParser_bytesRead,text,10));
		//	_delay_ms(1500);
		*/
			
			
			}
			

			/*
			char text[10];
			lcd_string(" fs");
			lcd_string(utoa(filesize,text,10));
			*/
			/*				
			boot_program_page(fileParser_pagesWritten++,&sd_buffer[128]);
			boot_program_page(fileParser_pagesWritten++,&sd_buffer[256]);
			boot_program_page(fileParser_pagesWritten++,&sd_buffer[384]);
			*/
				
			//increment the byte counter
			//fileParser_bytesRead += 512
			if ((fileParser_bytesRead) >= filesize )
			{
			//	lcd_setcursor(0,2);
			//	lcd_string("end avr");
			//	while(1);
				 return 0;
			}				 
			
			//check if AVR code end is reached
			if(fileParser_bytesRead >= (infoHeader.avrCodeSize +  512) )
			{
				
				
				//reset cortex chip
				//lcd_clear();
				lcd_home();
				lcd_string("updating...(1/2)");
				
				fileParser_resetCortex();
				
				
				//initialize the cortex bootloader
				//try 10 times to give the cortex some time to boot and answer
				int i=0;
				for(;;)
				{
					uart_tx(INIT_BOOTLOADER);
					_delay_ms(1000);
					//if received data is available
#ifdef MEGA32					
					if( (UCSRA & (1<<RXC)) )
#else
					if( (UCSR0A & (1<<RXC0)) )
#endif
					{
			//			lcd_setcursor(0,2);
			//	lcd_string("wait ack");
						uint8_t data = uart_rxWait();	
						
						
						if(data == ACK)
						{
							//bootloader successfully initialized
							//it is now waiting for commands

							break;
						}
					}
					
				}
				
				//check if init is ok (ACK received)
				if(i>=10)
				{
					//an error occured
					//could not initialize bootloader
					lcd_home();
					lcd_string("maiboard error");
					while(1);
					return 0;
				}
				
				//now we can send the cortex bootloader commands and data
				fileParser_parseState = CORTEX_DATA;
				
				
				
				//give the cortex time to erase the flash
			//	_delay_ms(2000); //ack is send after flash delete
				lcd_home();
				lcd_string("updating...(2/2)");
				
				return 1;
			}
			}			
		break;
		
		case CORTEX_DATA:
			//--- read cortex code ---
			
			//we have 512 bytes of data
			//data is 32 bit unsigned int
			//we increment with 4 because in each run we send out 4 bytes => 1 32 bit int message
			for(int i=0;i<512;i+=4)
			{
				
				uint16_t crc;
			
				// send the address
				
				crc = calcCrc(WRITE_ADDRESS,(uint8_t*)&addressCounter);
				//send next address packet until ACK received
				do 
				{
					uart_tx(WRITE_ADDRESS);
					//lcd_home();
					//lcd_string("cmd");
					uart_tx(addressCounter>>24);
			//		lcd_home();
		//	lcd_string("1");
					uart_tx(addressCounter>>16);
		//			lcd_home();
		//	lcd_string("2");
					uart_tx(addressCounter>>8);
		//			lcd_home();
		//	lcd_string("3");
					uart_tx(addressCounter);
		//			lcd_home();
		//	lcd_string("4");
					//send calculated CRC
					uart_tx(crc>>8);
		//			lcd_home();
		//	lcd_string("c1");
					uart_tx(crc&0xff);
		//			lcd_home();
		//	lcd_string("c2");
				} 
				while (uart_checkAck()!=ACK);	
				
			
				//calc crc for data block
				crc = calcCrc(WRITE_DATA,&sd_buffer[i]);
				//send next data packet until ACK received
				do 
				{
					//send command
					uart_tx(WRITE_DATA);
					//after the command, send the 4 data bytes
					uart_tx(sd_buffer[i+3]);
					uart_tx(sd_buffer[i+2]);
					uart_tx(sd_buffer[i+1]);
					uart_tx(sd_buffer[i]);
					//send calculated CRC
					uart_tx(crc>>8);
					uart_tx(crc&0xff);
				} 
				while (uart_checkAck()!=ACK);
				
				//transfer succeeded
				addressCounter++;
											
			}				
			
			
		//ui_ledToggle();	
		/*
			lcd_clear();
			lcd_home();
			lcd_string("add:");
			char text[10];
			lcd_string(utoa(addressCounter,text,10));
			*/
			
				
			fileParser_bytesRead+=512;
			//send cortex bootloader data
				
			if ((fileParser_bytesRead) >= filesize )
			{
				lcd_clear();
				lcd_home();
				lcd_string("success!");
				lcd_setcursor(0,2);
				lcd_string("please reboot...");
				uart_tx(END_BOOTLOADER);
				while(1);
				 return 0;
			}		
		break;
	}

	return 1;
};