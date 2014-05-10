/*
 * SD_Manager.c
 *
 *  Created on: 24.10.2012
 * ------------------------------------------------------------------------------------------------------------------------
 *  Copyright 2013 Julian Schmidt
 *  Julian@sonic-potions.com
 * ------------------------------------------------------------------------------------------------------------------------
 *  This file is part of the Sonic Potions LXR drumsynth firmware.
 * ------------------------------------------------------------------------------------------------------------------------
 *  Redistribution and use of the LXR code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *       - The code may not be sold, nor may it be used in a commercial product or activity.
 *
 *       - Redistributions that are modified from the original source must include the complete
 *         source code, including the source code for all components used by a binary built
 *         from the modified sources. However, as a special exception, the source code distributed
 *         need not include anything that is normally distributed (in either source or binary form)
 *         with the major components (compiler, kernel, and so on) of the operating system on which
 *         the executable runs, unless that component itself accompanies the executable.
 *
 *       - Redistributions must reproduce the above copyright notice, this list of conditions and the
 *         following disclaimer in the documentation and/or other materials provided with the distribution.
 * ------------------------------------------------------------------------------------------------------------------------
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *   USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ------------------------------------------------------------------------------------------------------------------------
 */

#include "config.h"
#if USE_SD_CARD
#include "SD_Manager.h"
#include "Uart.h"
#include "MidiMessages.h"
#include "sequencer.h"
#include <string.h>

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
FATFS sd_Fatfs;		/* File system object for the logical drive */
FIL sd_File;			/* place to hold 1 file*/
DIR sd_Directory;
uint8_t sd_initOkFlag = 0;
uint8_t sd_foundSampleFiles = 0;
uint32_t sd_currentSampleLength = 0;
char sd_currentSampleName[12];
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
void sdManager_init()
{
	FRESULT res;
	char *fn;   /* This function is assuming non-Unicode cfg. */

	sd_foundSampleFiles = 0;

	//init the Filesystem card
	f_mount(0,(FATFS*)&sd_Fatfs);

	//goto sample directory
	res = f_opendir(&sd_Directory, "/samples");                       /* Open the directory */
    if (res == FR_OK)
    {
		//count .wav files in sample dir
		FILINFO fno;

		while(1)
		{
			res = f_readdir(&sd_Directory, &fno);
			if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
			if (fno.fname[0] == '.') continue;             /* Ignore dot entry */

			fn = fno.fname; //8.3 format

			if (fno.fattrib & AM_DIR) {                    /* It is a directory */
				continue;									// skip directories
			}else {                                       /* It is a file. */
              //check if .wav file

				//get ext

				int i;
				for(i=0;i<12-3;i++)
				{
					if(fn[i]=='.')
					{
						if( (fn[i+1]=='W') && (fn[i+2]=='A') && (fn[i+3]=='V') )
						{
							sd_foundSampleFiles++;
						}
					}
				}

            }
		}

		sd_initOkFlag = 1;
    }

}

//---------------------------------------------------------------------------------------
uint8_t sd_getNumSamples()
{
	return sd_foundSampleFiles;
}
//---------------------------------------------------------------------------------------
/*
 * set the file read pointer to the beginning of the sample data in the data chunk
 * returns length of the data block in byte
 */
uint32_t findDataChunk()
{
	unsigned int bytesRead = 1;
	uint8_t data[4];
	memset(data,0,4);
	uint8_t pos = 0;

	//search substring 'data'
	while(bytesRead == 1) //while !EOF
	{
		f_read((FIL*)&sd_File,(void*)&data[pos],1,&bytesRead);

		//check
		if( (data[pos] == 'a') && (data[(pos+1)%4] == 'd') && (data[(pos+2)%4] == 'a') && (data[(pos+3)%4] == 't'))
		{
			//found 'data' header
			//-> read
			uint32_t length;
			f_read((FIL*)&sd_File,(void*)&length,4,&bytesRead);
			return length;
		}

		pos++;
		pos = pos%4;
	}
	return 0;
}
//---------------------------------------------------------------------------------------
//selects the active sample from the folder
void sd_setActiveSample(uint8_t sampleNr)
{
	FRESULT res;
	uint8_t currentSample=0;
	//goto sample directory
	res = f_opendir(&sd_Directory, "/samples");                       /* Open the directory */
    if (res == FR_OK)
    {
		//count .wav files in sample dir
		FILINFO fno;

		while(1)
		{
			res = f_readdir(&sd_Directory, &fno);
			if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
			if (fno.fname[0] == '.') continue;             /* Ignore dot entry */

			char *fn = fno.fname; //8.3 format

			if (fno.fattrib & AM_DIR) {                    /* It is a directory */
				continue;									// skip directories
			}else {                                       /* It is a file. */
              //check if .wav file
				//get ext

				int i;
				for(i=0;i<12-3;i++)
				{
					if(fn[i]=='.')
					{
						if( (fn[i+1]=='W') && (fn[i+2]=='A') && (fn[i+3]=='V') )
						{

							if(sampleNr == currentSample)
							{
								//found sample
								char filename[22] = "/samples/";
								memcpy(&filename[9],fn,13);
								filename[21] = 0;

								FRESULT res = f_open((FIL*)&sd_File,filename,FA_OPEN_EXISTING | FA_READ);

								if(res!=FR_OK)
								{
									//file open error... maybe the file does not exist?
									return;
								}
								//copy name
								memcpy(sd_currentSampleName,fn,11);
								sd_currentSampleName[11] = 0;
								//set read pointer to sample data
								sd_currentSampleLength = findDataChunk();

								return;
							}
							currentSample++;
						}
					}
				}

            }
		}

		sd_initOkFlag = 1;
    }
}
//---------------------------------------------------------------------------------------
uint32_t sd_getActiveSampleLength()
{
	return sd_currentSampleLength;
}
//---------------------------------------------------------------------------------------
//read sample data from the active file.
//returns the bytes read.
//if 0 is returned the EOF is reached
uint16_t sd_readSampleData(int16_t* data, uint16_t size)
{
	//read the file content
	unsigned int bytesRead;
	f_read((FIL*)&sd_File,(void*)data,size*2,&bytesRead);

	return bytesRead;
}
//---------------------------------------------------------------------------------------
char* sd_getActiveSampleName()
{
	return sd_currentSampleName;
}
//---------------------------------------------------------------------------------------
#endif
