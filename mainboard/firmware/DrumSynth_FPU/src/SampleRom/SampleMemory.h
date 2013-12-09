/*
 * SampleMemory.h
 *
 *  Created on: 16.10.2013
 *      Author: Julian
 */

#ifndef SAMPLEMEMORY_H_
#define SAMPLEMEMORY_H_

#include "stm32f4xx.h"
#include "flash_if.h"
#include "config.h"
#if USE_SD_CARD
#include "SD_Manager.h"
#endif
//flash region = 0x08000000 to 0x080FA000 (1024k)
//bootloader 0x08000000 to 0x08004000
//the main programm starts at 0x08004000
//if we reserve 512k for the main programm we get
// 0x08004000 + 512k = 0x08004000 + 0x0007D000 = 0x08081000
//page size is 0x4000 so we have to set the limit at 0x08080000 = ~524kb
//this results in a sample rom size of
// 0x080FA000 - 0x08080000 = 0x0007A000 = ~499kb
#define SAMPLE_ROM_START_ADDRESS 	((uint32_t)0x08080000)
#define SAMPLE_PAGE_SIZE               	(uint32_t)0x20000  		/* Page size = 128KByte */

//size for sample info struct = 7byte
//50 samples = 350 byte = 0x15e
// flash size 0x080FA000 - 0x15e = 0x080F9EA2
//auf 400 byte aufgestockt = 0x190
//flash size 0x080FA000 - 0x190 = 0x080F9E70
#define SAMPLE_INFO_START_ADDRESS 	((uint32_t)0x080F9E70)
#define SAMPLE_INFO_SIZE 			0x190
#define SAMPLE_ROM_SIZE				((uint32_t)0x00078E70) //499.216 kByte

typedef struct SampleInfoStruct
{
	char name[3];
	uint16_t size;		//size in bytes
	uint32_t offset;	//start address in bytes
} SampleInfo;


//--------------------------------------
void sampleMemory_init();
void sampleMemory_loadSamples();
SampleInfo sampleMemory_getSampleInfo(uint8_t index);
uint8_t sampleMemory_getNumSamples();
void sampleMemory_setNumSamples(uint8_t num);


#endif /* SAMPLEMEMORY_H_ */
