/*
 * SD_Manager.c
 *
 *  Created on: 24.10.2012
 *      Author: Julian
 */
#include "config.h"
#if USE_SD_CARD
#include "SD_Manager.h"
#include "Uart.h"
#include "MidiMessages.h"
#include "sequencer.h"

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
volatile FATFS preset_Fatfs;		/* File system object for the logical drive */
volatile FIL preset_File;			/* place to hold 1 file*/
char drumset_currentName[8];		/* buffer for the drumset name*/

//state machine
/** a state machine is used to read data from the sd card in time slices.
 * when a new sample needs to be calculated, the sd card load is interrupted
 * and continues on the next call to sdManager_tick()
 */
 uint8_t sdManager_state = SD_STATE_IDLE;
 uint16_t sdManager_stateIndex=0;

uint8_t drumset_parameterValues[NUM_PARAMS+1]; //the last value indicates the index of the sent data
#define PARAMETER_INDEX NUM_PARAMS // the position of the index of the sent data
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
void sdManager_init()
{
	//init the Filesystem card
	f_mount(0,(FATFS*)&preset_Fatfs);

	//TODO muss von front initialisiert werden, sonst bootet avr noch wenn cortwex sendet
	sdManager_loadDrumset(4,0);


	drumset_parameterValues[PARAMETER_INDEX] = 0;


}
//---------------------------------------------------------------------------------------
void sdManager_sendName2Front(char* name)
{
	uart_sendFrontpanelByte(FRONT_PRESET_NAME);
	uart_sendFrontpanelByte((name[0]&0x7f));
	uart_sendFrontpanelByte( ((0x40) | ((name[0]>>7)&0x7f)));//set 7th bit to indicate beginning of new name

	int i;
	for(i=1; i<8 ; i++)
	{
		uart_sendFrontpanelByte(FRONT_PRESET_NAME);
		uart_sendFrontpanelByte((name[i]&0x7f));
		uart_sendFrontpanelByte((name[i]>>7)&0x7f);
	}
}
//---------------------------------------------------------------------------------------
void sdManager_sendParameters2Front(uint8_t index, uint8_t value)
{
	/*
	uart_sendFrontpanelByte(MIDI_CC);
	uart_sendFrontpanelByte(index+1);
	uart_sendFrontpanelByte(value);
	*/
	drumset_parameterValues[index] = value;
	//drumset_parameterValues[PARAMETER_INDEX] = index;

}
//---------------------------------------------------------------------------------------
static volatile uint32_t counter_stateMachineSlice = 0;
#define STATE_MACHINE_TIMEOUT 10 //parameters

void sdManager_loadSoundState()
{

	//get current index
	counter_stateMachineSlice = sdManager_stateIndex;



	for(;sdManager_stateIndex<NUM_PARAMS;)
	{

		uint8_t data;
		unsigned int bytesRead;
		f_read((FIL*)&preset_File,&data,1,&bytesRead);

		MidiMsg msg;
		if(sdManager_stateIndex<PAR_BEGINNING_OF_GLOBALS)
		{
			//sound parameters
			msg.status = MIDI_CC;
			msg.data1 = sdManager_stateIndex+1 ;	//the parameter number
			msg.data2 = data;	//the value

			//osc waveform special case bring 0-4 value to 0-127
			if((msg.data1  >=OSC_WAVE_DRUM1) && (msg.data1  <=WAVE3_HH))
			{
				msg.data2 /= (127/5);
			}


			midiParser_ccHandler(msg);
		}
		else
		{
			/*
			//global parameters
			msg.status = MIDI_CC2;
			msg.data1 = i+1 ;	//the parameter number
			msg.data2 = data;	//the value
			midiParser_ccHandler(msg);
			*/
		}
		//send parametger to front
		sdManager_sendParameters2Front(sdManager_stateIndex,data);
		//wait(10);
		sdManager_stateIndex++;

		//if a new sampel calculation is needed, stop sd card loading
		//if(bCurrentSampleValid!= SAMPLE_VALID) return;
		if(sdManager_stateIndex-counter_stateMachineSlice >= STATE_MACHINE_TIMEOUT)
		{
			//int debug =1;
			//debug += systick_ticks;
			return;
		}
	}

	//update all
	drumset_parameterValues[PARAMETER_INDEX] = 0;

	//close the file handle
	f_close((FIL*)&preset_File);

	//send name to front panel
	sdManager_sendName2Front(drumset_currentName);

	sdManager_state = SD_STATE_IDLE;
}
//---------------------------------------------------------------------------------------
void sdManager_loadPatternState()
{

	//get current index
	//counter_stateMachineSlice = sdManager_stateIndex;

	unsigned int bytesRead;

	if(sdManager_stateIndex<(NUM_STEPS*NUM_PATTERN*NUM_TRACKS))
	{
		//seq_track[NUM_PATTERN][NUM_TRACKS][128];
		const uint8_t absPat 	= sdManager_stateIndex/128;
		const uint8_t currentTrack 		= absPat / 8;
		const uint8_t currentPattern 	= absPat - currentTrack*8;
		const uint8_t currentStep		= sdManager_stateIndex - absPat*128;


		f_read((FIL*)&preset_File,(void*)&seq_track[currentPattern][currentTrack][currentStep],sizeof(Step),&bytesRead);
		sdManager_stateIndex++;

		//if(sdManager_stateIndex-counter_stateMachineSlice >= STATE_MACHINE_TIMEOUT)
		//{
			return;
		//}

	}

	//close the file handle
	f_close((FIL*)&preset_File);

	//send name to front panel
	sdManager_sendName2Front(drumset_currentName);

	//back to idle state - job finished
	sdManager_state = SD_STATE_IDLE;

}
//---------------------------------------------------------------------------------------
static volatile uint32_t startTicks_frontLimit = 0;

void sdManager_tick()
{

	//parameter transfer to front
	if(drumset_parameterValues[PARAMETER_INDEX] < 128 )
	{
		if(systick_ticks-startTicks_frontLimit >= 10)
		{
			//get current time
			startTicks_frontLimit = systick_ticks;


			//we have data to send
			uart_sendFrontpanelByte(MIDI_CC);
			uart_sendFrontpanelByte(drumset_parameterValues[PARAMETER_INDEX]+1);
			uart_sendFrontpanelByte(drumset_parameterValues[drumset_parameterValues[PARAMETER_INDEX]]);

			//increase the index
			drumset_parameterValues[PARAMETER_INDEX]++;

		}
	}

	//SD card state machine
	switch(sdManager_state)
	{
	case SD_STATE_IDLE:

		break;

	case SD_STATE_LOAD_SOUND:
		sdManager_loadSoundState();
		break;

	case SD_STATE_LOAD_PATTERN:
		sdManager_loadPatternState();
		break;
	}

}

//---------------------------------------------------------------------------------------
uint8_t sdManager_loadDrumset(uint8_t presetNr, uint8_t isMorph)
{
	//filename in 8.3  format
	char filename[13];
	sprintf(filename,"p%03d.snd",presetNr);
	//open the file
	FRESULT res = f_open((FIL*)&preset_File,filename,FA_OPEN_EXISTING | FA_READ);

	if(res!=FR_OK)
	{
		//file open error... maybe the file does not exist?
		return 0;
	}
	else
	{
		//now read the file content
		unsigned int bytesRead;
		//first the preset name
		f_read((FIL*)&preset_File,(void*)drumset_currentName,8,&bytesRead);

		//then the data

		/*
		if(isMorph)
		{
			for(i=0;i<PAR_BEGINNING_OF_GLOBALS;i++)
			{
				f_read((FIL*)&preset_File,&parameters2[i].value,1,&bytesRead);
			}
		}
		else

		{*/

			sdManager_stateIndex=0;
			sdManager_state = SD_STATE_LOAD_SOUND;

#if 0
			for(;sdManager_stateIndex<NUM_PARAMS;sdManager_stateIndex++)
			{
				uint8_t data;
				f_read((FIL*)&preset_File,&data,1,&bytesRead);

				MidiMsg msg;
				if(i<PAR_BEGINNING_OF_GLOBALS)
				{
					//sound parameters
					msg.status = MIDI_CC;
					msg.data1 = i+1 ;	//the parameter number
					msg.data2 = data;	//the value

					//osc waveform special case bring 0-4 value to 0-127
					if((msg.data1  >=OSC_WAVE_DRUM1) && (msg.data1  <=WAVE3_HH))
					{
						msg.data2 /= (127/5);
					}


					midiParser_ccHandler(msg);
				}
				else
				{
					/*
					//global parameters
					msg.status = MIDI_CC2;
					msg.data1 = i+1 ;	//the parameter number
					msg.data2 = data;	//the value
					midiParser_ccHandler(msg);
					*/
				}
				//send parametger to front
				sdManager_sendParameters2Front(i,data);
				//wait(1);

				if(bCurrentSampleValid!= SAMPLE_VALID) break;
			}
		}

		//update all
		drumset_parameterValues[PARAMETER_INDEX] = 0;

		//close the file handle
		f_close((FIL*)&preset_File);

		//send name to front panel
		sdManager_sendName2Front(drumset_currentName);


		//now we need to send the new param values to the cortex and repaint the menu
		//menu_sendAllParameters();
		//preset_morph(parameters[PAR_MORPH].value + (parameters[PAR_MORPH].value==127)*1);

		//TODO send program change message
#endif
	}

	return 1;
};
//-------------------------------------------------------------------------------------------
void sdManager_loadPattern(uint8_t patternNr)
{
	//filename in 8.3  format
	char filename[13];
	sprintf(filename,"p%03d.pat",patternNr);
	//open the file
	FRESULT res = f_open((FIL*)&preset_File,filename,FA_OPEN_EXISTING | FA_READ);

	//preset_File.

	if(res!=FR_OK)
	{
		//file open error... maybe the file does not exist?
		return;
	}
	else
	{

		//now read the file content
		unsigned int bytesRead;
		//first the preset name
		f_read((FIL*)&preset_File,(void*)drumset_currentName,8,&bytesRead);

		sdManager_stateIndex=0;
		sdManager_state = SD_STATE_LOAD_PATTERN;
	}


}
#endif
