/*
 * presetManager.c
 *
 * Created: 09.05.2012 16:06:19
 *  Author: Julian
 */ 
#include "../config.h"
#include "PresetManager.h"
#include "../Hardware/SD/ff.h"
#include <stdio.h>
#include "../Menu/CcNr2Text.h"
#include "../Menu/menu.h"
#include <util/delay.h>
#include "../Hardware/lcd.h"
#include <avr/pgmspace.h>
#include "../frontPanelParser.h"
#include <stdlib.h>
#include <util/atomic.h> 
#include "../IO/uart.h"
#include "../IO/din.h"
#include "../IO/dout.h"

#include "../Hardware/timebase.h"

FATFS preset_Fatfs;		/* File system object for the logical drive */
FIL preset_File;		/* place to hold 1 file*/

char preset_currentName[8];


#define FILE_VERSION 2


#define NUM_TRACKS 7
#define NUM_PATTERN 8
#define STEPS_PER_PATTERN 128

#define FEXT_SOUND 	0
#define FEXT_PAT 	1
#define FEXT_ALL 	2
#define FEXT_PERF 	3
// fill buffer (length 9 total) with a filename. type is one of above
// eg p001.snd
static void preset_makeFileName(char *buf, uint8_t num, uint8_t type);


//----------------------------------------------------
void preset_init()
{

	//mount the filesystem	
	f_mount(0,(FATFS*)&preset_Fatfs);
	
	
}

//----------------------------------------------------
static void preset_writeDrumsetData(uint8_t isMorph)
{
	uint16_t i;
	unsigned int bytesWritten;
	
	if(isMorph)
	{
		for(i=0;i<END_OF_SOUND_PARAMETERS;i++)
		{
			uint8_t value;
			//Mod targets are not morphed!!!
			if( (i >= PAR_VEL_DEST_1) && (i <= PAR_VEL_DEST_6) )
			{
				value = parameter_values[i];
			} else if( (i >= PAR_TARGET_LFO1) && (i <= PAR_TARGET_LFO6) )
			{
				value = parameter_values[i];
			} else if( (i >= PAR_VOICE_LFO1) && (i <= PAR_VOICE_LFO6) )
			{
				value = parameter_values[i];
			} 
			else 
			{
				value = preset_getMorphValue(i,parameter_values[PAR_MORPH]);
			}					
			f_write((FIL*)&preset_File,&value,1,&bytesWritten);	
		}
	} else {
		for(i=0;i<END_OF_SOUND_PARAMETERS;i++)
		{
			f_write((FIL*)&preset_File,&parameter_values[i],1,&bytesWritten);
		}
	}
}

//----------------------------------------------------
void preset_saveDrumset(uint8_t presetNr, uint8_t isMorph)
{
#if USE_SD_CARD
	unsigned int bytesWritten;
	//filename in 8.3  format
	char filename[9];
	preset_makeFileName(filename,presetNr,FEXT_SOUND);
	//sprintf(filename,"p%03d.snd",presetNr);

	//open the file
	f_open((FIL*)&preset_File,filename,FA_CREATE_ALWAYS | FA_WRITE);
	
	//write preset name to file
	f_write((FIL*)&preset_File,(void*)preset_currentName,8,&bytesWritten);
	//write the preset data
	preset_writeDrumsetData(isMorph);
	
	//close the file
	f_close((FIL*)&preset_File);
#else
#endif
};

//----------------------------------------------------
static void preset_writeGlobalData()
{
	unsigned int bytesWritten;
	int i;
	for(i=PAR_BEGINNING_OF_GLOBALS;(i<NUM_PARAMS);i++)
	{
		f_write((FIL*)&preset_File,&parameter_values[i],1,&bytesWritten);
	}
}

//----------------------------------------------------
void preset_saveGlobals()
{
#if USE_SD_CARD	
	//open the file
	f_open((FIL*)&preset_File,"glo.cfg",FA_CREATE_ALWAYS | FA_WRITE);
	
	preset_writeGlobalData();

	//close the file
	f_close((FIL*)&preset_File);
#else
#endif	
}
//----------------------------------------------------
// returns 1 on success
static uint8_t preset_readGlobalData()
{
#if USE_SD_CARD
	int i;
	UINT bytesRead;
	for(i=PAR_BEGINNING_OF_GLOBALS;(i<NUM_PARAMS) &&( bytesRead!=0);i++) {
		f_read((FIL*)&preset_File,&parameter_values[i],1,&bytesRead);
		if(!bytesRead)
			return 0;
	}
	return 1;
#endif
}
//----------------------------------------------------
void preset_loadGlobals()
{
#if USE_SD_CARD
	//open the file
	FRESULT res = f_open((FIL*)&preset_File,"glo.cfg",FA_OPEN_EXISTING | FA_READ);
	
	if(res!=FR_OK)
		return; //file open error... maybe the file does not exist?

	preset_readGlobalData();
	f_close((FIL*)&preset_File);
	menu_sendAllGlobals();
	
#endif	
}
//----------------------------------------------------
// read the data into either the main parameters or the morph parameters.
// fix some values, and set values to 0 that were not read in
static FRESULT preset_readDrumsetData(uint8_t isMorph)
{
	
#if USE_SD_CARD		
	//read the file content
	unsigned int bytesRead=1;
	int16_t i;
	uint8_t *para;
	bytesRead = 0;
	
	if(isMorph)
		para=parameters2;
	else
		para=parameter_values;

	FRESULT res=f_read((FIL*)&preset_File, para,END_OF_SOUND_PARAMETERS, &bytesRead);
	if(res==FR_OK) {
		// set to 0 for any that were not read from the file
		if(END_OF_SOUND_PARAMETERS-bytesRead)
			memset(para+bytesRead,0,END_OF_SOUND_PARAMETERS-bytesRead);
		
		//special case mod targets. normalize.
		const uint8_t nmt=getNumModTargets();
		for(i=0;i<6;i++) {
			// --AS since I've changed the meaning of these, I'll ensure that it's valid for kits saved prior to the
			// change
			if(para[PAR_VEL_DEST_1+i] >= nmt )
				para[PAR_VEL_DEST_1+i] = 0;
			if(para[PAR_TARGET_LFO1+i] >= nmt )
				para[PAR_TARGET_LFO1+i] = 0;
		}
	}

	return res;

#endif
}

//----------------------------------------------------
uint8_t preset_loadDrumset(uint8_t presetNr, uint8_t isMorph)
{
#if USE_SD_CARD
	//filename in 8.3  format
	char filename[9];
	unsigned int bytesRead;

	//sprintf(filename,"p%03d.snd",presetNr);
	preset_makeFileName(filename,presetNr,FEXT_SOUND);

	//open the file
	FRESULT res = f_open((FIL*)&preset_File,filename,FA_OPEN_EXISTING | FA_READ);
	if(res!=FR_OK)
		goto error; //file open error... maybe the file does not exist?

	//first the preset name
	res=f_read((FIL*)&preset_File,(void*)preset_currentName,8,&bytesRead);
	if(bytesRead != 8) {
		res=FR_DISK_ERR;
		goto closeFile;
	}

	//then the data
	res=preset_readDrumsetData(isMorph);
	
closeFile:
	//close the file handle
	f_close((FIL*)&preset_File);

	// update the back with the new parameters
	if(res==FR_OK) {
		preset_sendDrumsetParameters();
		return 1;
	}
error:
	return 0;
#else
	frontPanel_sendData(PRESET,PRESET_LOAD,presetNr);
#endif
}

//----------------------------------------------------
// send loaded parameters to back
void preset_sendDrumsetParameters()
{
	uint8_t i;
	//special case mod targets
	for(i=0;i<6;i++)
	{
		//**VELO load drumkit. translate to param value before sending
		// parameter_values[PAR_VEL_DEST_1+i] is an index into modTargets, we need to send
		// a parameter number
		uint8_t value = (uint8_t)pgm_read_word(&modTargets[parameter_values[PAR_VEL_DEST_1+i]].param);
		uint8_t upper,lower;
		upper = (uint8_t)(((value&0x80)>>7) | (((i)&0x3f)<<1));
		lower = value&0x7f;
		frontPanel_sendData(CC_VELO_TARGET,upper,lower);

		// ensure target voice # is valid
		if(parameter_values[PAR_VOICE_LFO1+i] < 1 || parameter_values[PAR_VOICE_LFO1+i] > 6 )
			parameter_values[PAR_VOICE_LFO1+i]=1;

		// **LFO par_target_lfo will be an index into modTargets, but we need a parameter number to send
		value = (uint8_t)pgm_read_word(&modTargets[parameter_values[PAR_TARGET_LFO1+i]].param);

		upper = (uint8_t)(((value&0x80)>>7) | (((i)&0x3f)<<1));
		lower = value&0x7f;
		frontPanel_sendData(CC_LFO_TARGET,upper,lower);
	}

	// --AS todo will this morph (and fuck up) our modulation targets?
	// send parameters (possibly combined with morph parameters) to back
	preset_morph(parameter_values[PAR_MORPH]);



}
//----------------------------------------------------
char* preset_loadName(uint8_t presetNr, uint8_t what)
{
	uint8_t type;

	switch(what) {
	case SAVE_TYPE_PATTERN:
		type=FEXT_PAT;
		break;
	case SAVE_TYPE_KIT:
	case SAVE_TYPE_MORPH:
		type=FEXT_SOUND;
		break;
	case SAVE_TYPE_ALL:
		type=FEXT_ALL;
		break;
	case SAVE_TYPE_PERFORMANCE:
		type=FEXT_PERF;
		break;
	default:
		return NULL; // for glo and sample we don't load a name
	}
	

#if USE_SD_CARD		
	//filename in 8.3  format
	char filename[9];
	preset_makeFileName(filename,presetNr,type);

	//try to open the file
	FRESULT res = f_open((FIL*)&preset_File,filename,FA_OPEN_EXISTING | FA_READ);
	
	if(res!=FR_OK)
	{
		//error opening the file
		memcpy_P((void*)preset_currentName,PSTR("Empty   "),8);
		return NULL;	
	}
	
	//file opened correctly -> extract name (first 8 bytes)
	unsigned int bytesRead;
	f_read((FIL*)&preset_File,(void*)preset_currentName,8,&bytesRead);
	
	
	//close the file handle
	f_close((FIL*)&preset_File);
	
	return (char*)preset_currentName;
#else

return "ToDo";
#endif
}

//----------------------------------------------------
/** request step data from the cortex via uart and save it in the provided step struct*/
void preset_queryStepDataFromSeq(uint16_t stepNr)
{
	frontParser_newSeqDataAvailable = 0;

	//request step data
	//the max number for 14 bit data is 16383!!!
	//current max step nr is 128*7*8 = 7168
	frontPanel_sendByte((stepNr>>7)&0x7f);		//upper nibble 7 bit
	frontPanel_sendByte(stepNr&0x7f);			//lower nibble 7 bit

	//wait until data arrives
	uint8_t newSeqDataLocal = 0;
	uint16_t now = time_sysTick;
	while((newSeqDataLocal==0))
	{
		//we have to call the uart parser to handle incoming messages from the sequencer
		uart_checkAndParse();
		
		newSeqDataLocal = frontParser_newSeqDataAvailable;
		
		if(time_sysTick-now >= 31)
		{
			//timeout
			now = time_sysTick;
			//request step again
			frontPanel_sendByte((stepNr>>7)&0x7f);		//upper nibble 7 bit
			frontPanel_sendByte(stepNr&0x7f);
		}
	}
}
//----------------------------------------------------
static void preset_sendMainStepDataToSeq( uint16_t mainStepData)
{
	frontPanel_sendByte(mainStepData	& 0x7f);
	frontPanel_sendByte((mainStepData>>7)	& 0x7f);
	frontPanel_sendByte((uint8_t)((mainStepData>>14)	& 0x7f));
}
//----------------------------------------------------
/** send step data from SD card to the sequencer*/
static void preset_sendStepDataToSeq()
{
	frontPanel_sendByte(frontParser_stepData.volume	& 0x7f);
	frontPanel_sendByte(frontParser_stepData.prob	& 0x7f);
	frontPanel_sendByte(frontParser_stepData.note	& 0x7f);

	frontPanel_sendByte(frontParser_stepData.param1Nr	& 0x7f);
	frontPanel_sendByte(frontParser_stepData.param1Val	& 0x7f);

	frontPanel_sendByte(frontParser_stepData.param2Nr	& 0x7f);
	frontPanel_sendByte(frontParser_stepData.param2Val	& 0x7f);

	//now the MSBs from all 7 values
	frontPanel_sendByte((uint8_t) 	(((frontParser_stepData.volume 	& 0x80)>>7) |
							((frontParser_stepData.prob	 	& 0x80)>>6) |
							((frontParser_stepData.note	 	& 0x80)>>5) |
							((frontParser_stepData.param1Nr	& 0x80)>>4) |
							((frontParser_stepData.param1Val	& 0x80)>>3) |
							((frontParser_stepData.param2Nr	& 0x80)>>2) |
							((frontParser_stepData.param2Val	& 0x80)>>1))
							);
}	
//----------------------------------------------------
void preset_queryPatternInfoFromSeq(uint8_t patternNr, uint8_t* next, uint8_t* repeat)
{
	frontParser_newSeqDataAvailable = 0;
	//request pattern info
	frontPanel_sendByte(patternNr);	

	//wait until data arrives
	uint8_t newSeqDataLocal = 0;
	uint16_t now = time_sysTick;
	while((newSeqDataLocal==0))
	{
		//we have to call the uart parser to handle incoming messages from the sequencer
		uart_checkAndParse();
		
		newSeqDataLocal = frontParser_newSeqDataAvailable;
		
		if(time_sysTick-now >= 31)
		{
			//timeout
			now = time_sysTick;
			//request step again
			frontPanel_sendByte(patternNr);	
		}
	}
	
	//the stepdata struct is used as buffer for the data
	*next = frontParser_stepData.volume; 
	*repeat =  frontParser_stepData.prob;
}
//----------------------------------------------------
void preset_queryMainStepDataFromSeq(uint16_t stepNr, uint16_t *mainStepData, uint8_t *length)
{
	frontParser_newSeqDataAvailable = 0;

	//request step data
	//the max number for 14 bit data is 16383!!!
	//current max step nr is 7*8 = 56
	frontPanel_sendByte((stepNr>>7)&0x7f);		//upper nibble 7 bit
	frontPanel_sendByte(stepNr&0x7f);	//lower nibble 7 bit

	//wait until data arrives
	uint8_t newSeqDataLocal = 0;
	uint16_t now = time_sysTick;
	while((newSeqDataLocal==0))
	{
		//we have to call the uart parser to handle incoming messages from the sequencer
		uart_checkAndParse();
		
		newSeqDataLocal = frontParser_newSeqDataAvailable;
		
		if(time_sysTick-now >= 31)
		{
			//timeout
			now = time_sysTick;
			//request step again
			frontPanel_sendByte((stepNr>>7)&0x7f);		//upper nibble 7 bit
			frontPanel_sendByte(stepNr&0x7f);
		}
	}
	
	// we are reusing these members for purposes other than those that were originally intended
	*mainStepData =(uint16_t) ((frontParser_stepData.volume<<8) | frontParser_stepData.prob);
	*length=frontParser_stepData.note;
};

//----------------------------------------------------
static void preset_writePatternData()
{
	uint16_t bytesWritten;
	uint8_t length;


	//write the preset data
	//initiate the sysex mode
	
	while( (frontParser_midiMsg.status != SYSEX_START))
	{
		frontPanel_sendByte(SYSEX_START);
		uart_checkAndParse();
	}		
	_delay_ms(10);
	//enter step data mode
	frontPanel_sendByte(SYSEX_REQUEST_STEP_DATA);
	frontPanel_sysexMode = SYSEX_REQUEST_STEP_DATA;
	
	uint8_t percent=0;
	char text[5];
	uint16_t i;
	for(i=0;i<(STEPS_PER_PATTERN*NUM_PATTERN*NUM_TRACKS);i++)
	{
		if((i&0x80) == 0)
		{
			//set cursor to beginning of row 2
			lcd_setcursor(0,2);
			//print percent value
			percent = (uint8_t)(i * (100.f/(STEPS_PER_PATTERN*NUM_PATTERN*NUM_TRACKS)));
			itoa(percent,text,10);
			lcd_string(text);
		}			
		
		
		
		//get next data chunk and write it to file
		preset_queryStepDataFromSeq(i);
		f_write((FIL*)&preset_File,(const void*)&frontParser_stepData,sizeof(StepData),&bytesWritten);	
	}
	
	//end sysex mode
	frontPanel_sendByte(SYSEX_END);
	frontParser_midiMsg.status = 0;
	//now the main step data
	while( (frontParser_midiMsg.status != SYSEX_START))
	{
		frontPanel_sendByte(SYSEX_START);
		uart_checkAndParse();
	}	
	_delay_ms(50);	
	frontPanel_sendByte(SYSEX_REQUEST_MAIN_STEP_DATA);
	frontPanel_sysexMode = SYSEX_REQUEST_MAIN_STEP_DATA;
	
	uint16_t mainStepData;
	for(i=0;i<(NUM_PATTERN*NUM_TRACKS);i++)
	{
		//get next data chunk and write it to file (length here is ignored)
		preset_queryMainStepDataFromSeq(i, &mainStepData, &length);
		f_write((FIL*)&preset_File,(const void*)&mainStepData,sizeof(uint16_t),&bytesWritten);	
	}
		
	//end sysex mode
	frontPanel_sendByte(SYSEX_END);
	frontParser_midiMsg.status = 0;
	
	//----- pattern info (next/repeat) ------
	
	while( (frontParser_midiMsg.status != SYSEX_START))
	{
		frontPanel_sendByte(SYSEX_START);
		uart_checkAndParse();
	}	
	_delay_ms(50);	
	frontPanel_sendByte(SYSEX_REQUEST_PATTERN_DATA);
	frontPanel_sysexMode = SYSEX_REQUEST_PATTERN_DATA;
	
	uint8_t next;
	uint8_t repeat;
	for(i=0;i<(NUM_PATTERN);i++)
	{
		//get next data chunk and write it to file
		preset_queryPatternInfoFromSeq((uint8_t)i, &next, &repeat);
		f_write((FIL*)&preset_File,(const void*)&next,sizeof(uint8_t),&bytesWritten);	
		f_write((FIL*)&preset_File,(const void*)&repeat,sizeof(uint8_t),&bytesWritten);
	}
		
	//end sysex mode
	frontPanel_sendByte(SYSEX_END);
	frontParser_midiMsg.status = 0;
	
	
	//----- shuffle setting ------
	f_write((FIL*)&preset_File,(const void*)&parameter_values[PAR_SHUFFLE],sizeof(uint8_t),&bytesWritten);

	//----- pattern/track lengths ------
	// --AS we reuse the same call from above (when saving main step data)
	// but we only use the length info retrieved. We want to store it at the end
	// to avoid breaking compatibility with save file
	while( (frontParser_midiMsg.status != SYSEX_START))
	{
		frontPanel_sendByte(SYSEX_START);
		uart_checkAndParse();
	}
	_delay_ms(50);
	frontPanel_sendByte(SYSEX_REQUEST_MAIN_STEP_DATA);
	frontPanel_sysexMode = SYSEX_REQUEST_MAIN_STEP_DATA;

	for(i=0;i<(NUM_PATTERN*NUM_TRACKS);i++)
	{
		//get next data chunk and write it to file
		preset_queryMainStepDataFromSeq(i, &mainStepData, &length);
		f_write((FIL*)&preset_File,(const void*)&length,sizeof(uint8_t),&bytesWritten);
	}

	//end sysex mode
	frontPanel_sendByte(SYSEX_END);
	frontParser_midiMsg.status = 0;

}


//----------------------------------------------------
void preset_savePattern(uint8_t presetNr)
{
#if USE_SD_CARD
	uint16_t bytesWritten;
	lcd_clear();
	lcd_home();
	lcd_string_F(PSTR("Saving pattern"));
	
	
	//filename in 8.3  format
	char filename[9];
	//sprintf(filename,"p%03d.pat",presetNr);
	preset_makeFileName(filename,presetNr,FEXT_PAT);
	//open the file
	f_open((FIL*)&preset_File,filename,FA_CREATE_ALWAYS | FA_WRITE);

	//write preset name to file
	f_write((FIL*)&preset_File,(void*)preset_currentName,8,&bytesWritten);

	preset_writePatternData();

	//close the file
	f_close((FIL*)&preset_File);
	
	//reset the lcd
	menu_repaintAll();
	
#else
#endif
}

//----------------------------------------------------
// returns 1 on success
static uint8_t preset_readPatternData()
{
	//--AS note that the pattern length data is no longer stored in the same way.
	// This means that loading old patterns with non-standard length, the length will be
	// ignored and a standard length used. I think it's acceptable as long as users are aware of this.
#if USE_SD_CARD
	UINT bytesRead;
	uint8_t success=1; // start off succeeding
	uint16_t i;
	uint8_t repeat=0;
	uint8_t next=0;

	lcd_clear();
	lcd_home();
	lcd_string_F(PSTR("Loading pattern"));

	// ---------- Send step data first

	// Enter sysex mode
	while( (frontParser_midiMsg.status != SYSEX_START))
	{
		frontPanel_sendByte(SYSEX_START);
		uart_checkAndParse();
	}
	_delay_ms(10);

	// send step data
	frontPanel_sendByte(SYSEX_SEND_STEP_DATA);
	frontPanel_sysexMode = SYSEX_SEND_STEP_DATA;

	for(i=0;i<(STEPS_PER_PATTERN*NUM_PATTERN*NUM_TRACKS);i++)
	{
		f_read((FIL*)&preset_File,(void*)&frontParser_stepData,sizeof(StepData),&bytesRead);
		if(bytesRead==sizeof(StepData)) {
			preset_sendStepDataToSeq();
			//we have to give the cortex some time to cope with all the incoming data
			//since it is mainly calculating audio it takes a while to process all
			//incoming uart data also FIFO overflow
			//if((i&0x1f) == 0x1f) //every 32 steps
				_delay_us(100);
		} else {
			success=0;
			break; // file error, or we ran out of file (eof)
		}
	}

	//end sysex mode
	frontPanel_sendByte(SYSEX_END);


	// ---------- send the main step data next
	if(success) {
		frontParser_midiMsg.status = 0;
		while( (frontParser_midiMsg.status != SYSEX_START))
		{
			frontPanel_sendByte(SYSEX_START);
			uart_checkAndParse();
		}
		_delay_ms(50);
		frontPanel_sendByte(SYSEX_SEND_MAIN_STEP_DATA);
		frontPanel_sysexMode = SYSEX_SEND_MAIN_STEP_DATA;

		uint16_t mainStepData;
		for(i=0;i<(NUM_PATTERN*NUM_TRACKS);i++)
		{
			f_read((FIL*)&preset_File,(void*)&mainStepData,sizeof(uint16_t),&bytesRead);
			if( bytesRead==sizeof(uint16_t)) {
				preset_sendMainStepDataToSeq(mainStepData);
				//we have to give the cortex some time to cope with all the incoming data
				//since it is mainly calculating audio it takes a while to process all
				//incoming uart data
				//if((i&0x1f) == 0x1f) //every 32 steps
				_delay_us(200); //todo speed up using ACK possible?
			} else {
				success=0;
				break;
			}
		}

		//end sysex mode
		frontPanel_sendByte(SYSEX_END);
	}

	//---------- pattern info (next/repeat)

	if(success) {
		for(i=0;i<(NUM_PATTERN);i++)
		{
			f_read((FIL*)&preset_File,(void*)&next,sizeof(uint8_t),&bytesRead);
			if(!bytesRead) {
				success=0;
				break;
			}
			f_read((FIL*)&preset_File,(void*)&repeat,sizeof(uint8_t),&bytesRead);
			if(!bytesRead) {
				success=0;
				break;
			}

			frontPanel_sendData(SEQ_CC,SEQ_SET_SHOWN_PATTERN,(uint8_t)i);

			frontPanel_sendData(SEQ_CC,SEQ_SET_PAT_BEAT,repeat);
			frontPanel_sendData(SEQ_CC,SEQ_SET_PAT_NEXT,next);
			//we have to give the cortex some time to cope with all the incoming data
			//since it is mainly calculating audio it takes a while to process all
			//incoming uart data
			//if((i&0x1f) == 0x1f) //every 32 steps
			_delay_us(200); //todo speed up using ACK possible?
		}

		frontPanel_sendData(SEQ_CC,SEQ_SET_SHOWN_PATTERN,menu_shownPattern);
	}

	// ------------ shuffle settings
	if(success) {
		//load the shuffle settings
		f_read((FIL*)&preset_File,(void*)&parameter_values[PAR_SHUFFLE],sizeof(uint8_t),&bytesRead);
		//and update on cortex
		if(bytesRead)
			frontPanel_sendData(SEQ_CC,SEQ_SHUFFLE,parameter_values[PAR_SHUFFLE]);
		else {
			success=0;
		}
	}

	// ----------- Pattern/track lengths
	// -- AS this might not exist in the saved data file, we still want success
	if(success) {
		frontParser_midiMsg.status = 0;
		while( (frontParser_midiMsg.status != SYSEX_START))
		{
			frontPanel_sendByte(SYSEX_START);
			uart_checkAndParse();
		}
		_delay_ms(50);
		frontPanel_sendByte(SYSEX_SEND_PAT_LEN_DATA);
		frontPanel_sysexMode = SYSEX_SEND_PAT_LEN_DATA;

		for(i=0;i<(NUM_PATTERN*NUM_TRACKS);i++)
		{
			if(success)
				f_read((FIL*)&preset_File,(void*)&next,sizeof(uint8_t),&bytesRead);
			if( bytesRead==0) {
				next=0; // default to a length of 16 since we didn't read anything
				success=0;
			}
			frontPanel_sendByte(next);
			//we have to give the cortex some time to cope with all the incoming data
			//since it is mainly calculating audio it takes a while to process all
			//incoming uart data
			//if((i&0x1f) == 0x1f) //every 32 steps
			_delay_us(200); //todo speed up using ACK possible?
		}

		//end sysex mode
		frontPanel_sendByte(SYSEX_END);

		success=1; // just to document that we don't consider this a failure, we just didn't read the bytes for this
	}

	return success;

#else
frontPanel_sendData(PRESET,PATTERN_LOAD,presetNr);

#endif
}
//----------------------------------------------------
uint8_t preset_loadPattern(uint8_t presetNr)
{
#if USE_SD_CARD		
	//filename in 8.3  format
	char filename[9];
	UINT bytesRead;
	uint8_t success=0;
	//sprintf(filename,"p%03d.pat",presetNr);
	preset_makeFileName(filename,presetNr,FEXT_PAT);

	//open the file
	FRESULT res = f_open((FIL*)&preset_File,filename,FA_OPEN_EXISTING | FA_READ);
	
	if(res!=FR_OK)
		return 0; //file open error... maybe the file does not exist?
	
	//read the preset name
	f_read((FIL*)&preset_File,(void*)preset_currentName,8,&bytesRead);
	if(bytesRead != 8)
		goto closeFile;
	
	if(!preset_readPatternData())
		goto closeFile;
	
	success=1;
	//close the file handle
closeFile:
	f_close((FIL*)&preset_File);
	
	//force complete repaint
	menu_repaintAll();
	// return success or failure
	return success;
	
#else
frontPanel_sendData(PRESET,PATTERN_LOAD,presetNr);

#endif
}

//----------------------------------------------------
//val is interpolation value between 0 and 255 
//uses bankers rounding
static uint8_t interpolate(uint8_t a, uint8_t b, uint8_t x)
{
	uint16_t fixedPointValue = (uint16_t)(((a*256) + (b-a)*x));
	uint8_t result = (uint8_t)(fixedPointValue/256);
	
	return (uint8_t)((fixedPointValue&0xff) < 0x7f ? result : result+1);
	//return ((a*255) + (b-a)*x)/255;
}
//----------------------------------------------------
// This will determine an interpolated value between parameters
// and morph parameters and send the data to the back
// if the morph value is 0 it will just send the parameter values
// to the back
void preset_morph(uint8_t morph)
{
	int i;
	//TODO so far only sound parameters are interpolated, no LFOs etc

	for(i=0;i<END_OF_SOUND_PARAMETERS;i++)
	{
		uint8_t val;
		
		val = interpolate(parameter_values[i],parameters2[i],morph);
		if(i<128) {
			frontPanel_sendData(MIDI_CC,(uint8_t)i,val);
		} else {
			frontPanel_sendData(CC_2,(uint8_t)(i-128),val);
		}		
		
			
		//to omit front panel button/LED lag we have to process din dout and uart here
		//read next button
		din_readNextInput();
		//update LEDs
		dout_updateOutputs();
		//read uart messages from sequencer
		uart_checkAndParse();
	}		
}

//----------------------------------------------------
uint8_t preset_getMorphValue(uint16_t index, uint8_t morph)
{
	return interpolate(parameter_values[index],parameters2[index],morph);
};
//----------------------------------------------------


//----------------------------------------------------
//--AS **SAVEALL
// save all settings (global, pattern, kit)
// or save performance settings (pattern, kit)
#define GEN_BUF_LEN 9
void preset_saveAll(uint8_t presetNr, uint8_t isAll)
{
#if USE_SD_CARD
	uint16_t remain;
	uint8_t siz;
	//filename in 8.3  format
	char filename[GEN_BUF_LEN];
	if(isAll)
		preset_makeFileName(filename,presetNr,FEXT_ALL);
	else
		preset_makeFileName(filename,presetNr,FEXT_PERF);
	//open the file
	f_open((FIL*)&preset_File,filename,FA_CREATE_ALWAYS | FA_WRITE);


	unsigned int bytesWritten;
	//write preset name to file
	f_write((FIL*)&preset_File,(void*)preset_currentName,8,&bytesWritten);

	//write the version
	filename[0]=FILE_VERSION;
	f_write((FIL*)&preset_File, filename, 1, &bytesWritten);

	memset(filename,0xFF,GEN_BUF_LEN); // reuse this as our buffer for padding

	if(isAll) {
		// write global settings
		preset_writeGlobalData();

		// write some padding (we'll allocate 64 bytes for globals, right now we have +/- 18) so that
		// we don't need to change the file format later
		remain=	64- (NUM_PARAMS - PAR_BEGINNING_OF_GLOBALS);

	} else {
		// write the bpm
		f_write((FIL*)&preset_File, &parameter_values[PAR_BPM], 1, &bytesWritten);
		// bar reset mode
		f_write((FIL*)&preset_File, &parameter_values[PAR_BAR_RESET_MODE], 1, &bytesWritten);

		remain=	64 - (1+1);
	}

	// write padding after globals/perf data
	while(remain){
		if(remain < GEN_BUF_LEN)
			siz=(uint8_t)remain;
		else
			siz=GEN_BUF_LEN;
		f_write((FIL*)&preset_File, filename, siz, &bytesWritten);
		remain -=siz;
	}

	// save kit
	preset_writeDrumsetData(0);

	// write some more padding (we'll allocate 512 bytes for kit data. This should be more than
	// we'll ever need. Right now we use about 228 bytes).
	remain = 512 - (END_OF_SOUND_PARAMETERS);
	while(remain){
		if(remain < GEN_BUF_LEN)
			siz=(uint8_t)remain;
		else
			siz=GEN_BUF_LEN;
		f_write((FIL*)&preset_File, filename, siz, &bytesWritten);
		remain -=siz;
	}

	// write pattern data
	preset_writePatternData();

	//close the file
	f_close((FIL*)&preset_File);
#else
#endif
}

//----------------------------------------------------
void preset_loadAll(uint8_t presetNr, uint8_t isAll)
{

#if USE_SD_CARD
	//filename in 8.3  format
	char filename[GEN_BUF_LEN];
	UINT bytesRead;
	uint16_t remain;
	uint8_t siz;
	uint8_t version=0;
	//sprintf(filename,"p%03d.snd",presetNr);
	if(isAll)
		preset_makeFileName(filename,presetNr,FEXT_ALL);
	else
		preset_makeFileName(filename,presetNr,FEXT_PERF);

	//open the file
	FRESULT res = f_open((FIL*)&preset_File,filename,FA_OPEN_EXISTING | FA_READ);

	if(res!=FR_OK)
		return; //file open error... maybe the file does not exist?

	//first the preset name
	f_read((FIL*)&preset_File,(void*)preset_currentName,8,&bytesRead);
	if(!bytesRead)
		goto closeFile;

	// read version number and make sure its valid
	f_read((FIL*)&preset_File,&version,1,&bytesRead);
	if(!bytesRead || version > FILE_VERSION)
		goto closeFile;

	if(isAll){
		// read global data
		if(!preset_readGlobalData())
			goto closeFile;
		// size of padding after globals
		remain=	64- (NUM_PARAMS - PAR_BEGINNING_OF_GLOBALS);

	} else {
		// read bpm
		f_read((FIL*)&preset_File,&parameter_values[PAR_BPM],1,&bytesRead);
		if(!bytesRead)
			goto closeFile;

		// size of padding after these
		if(version>1) {
			// read pat reset bit
			f_read((FIL*)&preset_File,&parameter_values[PAR_BAR_RESET_MODE],1,&bytesRead);
			if(!bytesRead)
				goto closeFile;
			remain=	64- (1+1);
		} else
			remain=0; // version 1 didn't have padding for performances
	}

	while(remain){
		if(remain < GEN_BUF_LEN)
			siz=(uint8_t)remain;
		else
			siz=GEN_BUF_LEN;
		f_read((FIL*)&preset_File, filename, siz, &bytesRead);
		if(bytesRead !=siz)
			goto closeFile;
		remain -=siz;
	}



	// read kit data
	res=preset_readDrumsetData(0);
	if(res!=FR_OK)
		goto closeFile;

	// read padding
	remain = 512 - (END_OF_SOUND_PARAMETERS);
	while(remain){
		if(remain < GEN_BUF_LEN)
			siz=(uint8_t)remain;
		else
			siz=GEN_BUF_LEN;
		f_read((FIL*)&preset_File, filename, siz, &bytesRead);
		if(bytesRead !=siz)
			goto closeFile;
		remain -=siz;
	}


	// read pattern data
	if(!preset_readPatternData())
		goto closeFile;



closeFile:
	//close the file handle
	f_close((FIL*)&preset_File);

	// send drumset parameters to back
	preset_sendDrumsetParameters();

	// send global params
	menu_sendAllGlobals();

	//force complete repaint
	menu_repaintAll();

#else
	frontPanel_sendData(PRESET,PRESET_LOAD,presetNr);
#endif
}
//----------------------------------------------------
static void preset_makeFileName(char *buf, uint8_t num, uint8_t type)
{
	const char *ext;
	buf[0]='p';
	numtostrpu(&buf[1],num,'0');
	switch(type) {
	case 0:
		ext=PSTR(".snd");
		break;
	case 1:
		ext=PSTR(".pat");
		break;
	case 2:
		ext=PSTR(".all");
		break;
	default:
	case 3:
		ext=PSTR(".prf");
		break;
	}
	strcpy_P(&buf[4],ext);
}
