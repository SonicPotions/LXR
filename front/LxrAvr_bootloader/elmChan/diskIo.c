/*
 * diskIo.c
 *
 * io interface for the tinyFs
 *
 * Created: 09.05.2012 07:57:49
 *  Author: Julian
 */ 

#include "diskio.h"
#include "ff.h"
#include "../SPI_routines.h"
#include "..\lcd/lcd.h"
#include "../sd_routines.h"
#include <util\delay.h>
#include <avr/pgmspace.h>

#include "../config.h"


//-------------------------------------------------------------------------
static volatile DSTATUS Stat = STA_NOINIT;	/* Disk status */

//-------------------------------------------------------------------------
DSTATUS disk_initialize (BYTE drive)/* Physical drive number */
{
	
	unsigned char error,i;
	

	
	
	if (drive) return STA_NOINIT;		/* Supports only single drive */
	Stat |= STA_NOINIT;
	
	//init spi port for SD card
	spi_init();
//	lcd_home();
//	lcd_string_F(PSTR("spi init"));
	
	//init SD card for (i=0; i<10; i++)
	for (i=0; i<10; i++)
	{
		error = SD_init();
		if(!error) break;
	}


	if(error)
	{
		
		lcd_home();
		lcd_clear();
		lcd_string_F(PSTR("SD card"));
		lcd_setcursor(0,2);
		if(error == 1) {
			
			lcd_string_F(PSTR("not detected!"));
		}		
		else if(error == 2) lcd_string_F(PSTR("init error"));
		
		else lcd_string_F(PSTR("unknown error"));
		_delay_ms(2000);
		
		return Stat;
		

		//while(1);  //wait here forever if error in SD init 
	}
	else
	{
		lcd_home();	
		lcd_string_F(PSTR("SD card OK"));
		
	
		SPI_HIGH_SPEED;
		_delay_ms(5);

	}
	
	Stat &= ~STA_NOINIT;
	
	return Stat;

}
//-------------------------------------------------------------------------
DSTATUS disk_status (BYTE drive)/* Physical drive number */
{
	if (drive) return STA_NOINIT;		/* Supports only single drive */
	return Stat;
}
//-------------------------------------------------------------------------
DRESULT disk_read ( BYTE drive,          /* Physical drive number */
					BYTE* Buffer,        /* Pointer to the read data buffer */
					DWORD SectorNumber,  /* Start sector number */
					BYTE SectorCount     /* Number of sectros to read */)
{
	if (drive || !SectorCount) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;
	
	int i;
	for(i=0;i<SectorCount;i++)
	{
		SD_readSingleBlockCustomBuffer(SectorNumber,Buffer);	
	}
		
	return RES_OK;
}
//-------------------------------------------------------------------------
#if	_READONLY == 0
DRESULT disk_write (	BYTE drive,          /* Physical drive number */
						const BYTE* Buffer,  /* Pointer to the write data (may be non aligned) */
						DWORD SectorNumber,  /* Sector number to write */
						BYTE SectorCount)    /* Number of sectors to write */
{

	if (drive || !SectorCount) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;
	
	int i;
	for(i=0;i<SectorCount;i++)
	{	
		SD_writeSingleBlockCustomBuffer(SectorNumber,(uint8_t*)Buffer);
	}		
	
	return RES_OK;	
}
#endif
//-------------------------------------------------------------------------
DRESULT disk_ioctl (	BYTE drive,      /* Drive number */
						BYTE command,    /* Control command code */
						void* buffer)     /* Parameter and data buffer */
{
	if (drive) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;							

	switch (command) {
		case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (DWORD) */
			//only used by f_mkfs
			//so nothing to do here
			break;

		case GET_BLOCK_SIZE :	/* Get erase block size in sectors (DWORD) */
			*(DWORD*)buffer = 1;
			return RES_OK;

		case CTRL_SYNC :	/* Nothing to do */
			return RES_OK;

/*
		case ATA_GET_REV :	// Get firmware revision (8 chars) 
			
			break;
		

		case ATA_GET_MODEL :	// Get model name (40 chars) 

			break;

		case ATA_GET_SN :	// Get serial number (20 chars) 

			break;
	*/
		default:
			return RES_PARERR;
	}


	return RES_OK;
}
//-------------------------------------------------------------------------
DWORD get_fattime (void)
{
	//we have no RTC
	return 0;
}
//-------------------------------------------------------------------------

