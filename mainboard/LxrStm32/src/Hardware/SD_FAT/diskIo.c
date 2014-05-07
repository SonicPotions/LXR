/*
 * diskIo.c
 *
 * io interface for the tinyFs
 *
/---------------------------------------------------------------------------/
/  IO interface for STM32F4								2012 Julian Schmidt
/----------------------------------------------------------------------------/
/ This is an IO interface for STM32F4 devices to be used with ChaNs FatFs module.
/ This is a free software that opened for education, research and commercial
/ developments under license policy of following trems.
/
/  Copyright (C) 2012, Julian Schmidt, all right reserved.
/
/ * The diskIo module is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial product UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/----------------------------------------------------------------------------
 *
 * Created: 09.05.2012 07:57:49
 *  Author: Julian Schmidt
 */ 
#include "config.h"
#if USE_SD_CARD
#include "diskio.h"
#include "../globals.h"
#include "ff.h"
#include "SPI_routines.h"
#include "sd_routines.h"

//#pragma GCC push_options
//#pragma GCC optimize ("O0")
/** wait for ms*/
__attribute__((optimize("O0"))) void wait(uint32_t ms)
{
	const volatile uint32_t startTicks = systick_ticks;

	while(1)
	{
		if(systick_ticks-startTicks >= ms) break;
	}

}
//restore optimisation level
//#pragma GCC pop_options

//-------------------------------------------------------------------------
static volatile DSTATUS Stat = STA_NOINIT;	/* Disk status */

//-------------------------------------------------------------------------
DSTATUS disk_initialize (BYTE drive)/* Physical drive number */
{
	
	unsigned char error,i;
	
	//init low speed?
	SPI_SD;

	
	if (drive) return STA_NOINIT;		/* Supports only single drive */
	Stat |= STA_NOINIT;
	
	//init spi port for SD card
	spi_init();
	
	//init SD card for (i=0; i<10; i++)

	for (i=0; i<10; i++)
	{
		error = SD_init();
		if(!error) break;
	}


	if(error)
	//if(0)
	{
		/*
		lcd_home();
		
		if(error == 1) lcd_string_F(PSTR("SD card not detected.."));
		else if(error == 2) lcd_string_F(PSTR("Card Initialization failed.."));
		
		else lcd_string_F(PSTR("unknown SD card error"));
		*/
		return Stat;
		

		//while(1);  //wait here forever if error in SD init 
	}
	else
	{
	//	lcd_home();
	//	lcd_string_F(PSTR("sd card ok"));
		
	
		//SPI_HIGH_SPEED;
		wait(1);

	}
	
	Stat &= ~STA_NOINIT;
	
	return Stat;

};
//-------------------------------------------------------------------------
DSTATUS disk_status (BYTE drive)/* Physical drive number */
{
	if (drive) return STA_NOINIT;		/* Supports only single drive */
	return Stat;
};
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
};
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
};
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
			//only used my f_mkfs
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
};
//-------------------------------------------------------------------------
DWORD get_fattime (void)
{
	//we have no RTC
	return 0;
};
//-------------------------------------------------------------------------
#endif
