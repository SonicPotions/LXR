/*
 * midiLfo.c
 *
 * Created: 28.04.2012 15:28:10
 *  Author: Julian
 */ 


#include "midiLfo.h"
#include "Menu/menu.h"
#include <avr/pgmspace.h>
#include "IO/uart.h"
#include "frontPanelParser.h"
#if USE_LFO


/*
enum NoteType
{
    Regular,
    Triplet,
    Dotted,
	Double,
	Quadruple
	
};

enum NoteDivision
{
    Whole,
    Half,
	Third,
    Quarter,
	Sixth,
    Eighth,
	Twelfth,
    Sixteenth,
	Twentyfourth,
    Thirtysecond,
    Sixtyfourth,
}; 

const float NoteTypeScalers[] = { 1.0f, 2.0f / 3.0f, 1.5f, 2, 4 };
const float NoteDivisionScalers[] = {  1.0f,
                                       2.0f,
									   3.0f ,
                                       4.0f ,
									   6.0f ,
                                       8.0f ,
									   12.0f,
                                       16.0f,
									   24.0f,
                                       32.0f,
                                       64.0f }; 
									   */


static uint16_t lfo_rng =  0;	//*<temporary var for rng generation*/
Lfo lfoArray[NUM_LFO];

volatile uint8_t lfo_diasbled=0;

//sample rate is for the LFOs is comming from the timer 1ms update interval => 1ms/1sec = 1000 Hz
#define LFO_SR (1001.641025641025641025641025641f)

const uint8_t  sinewave[256] PROGMEM= //256 values
{
0x80,0x83,0x86,0x89,0x8c,0x8f,0x92,0x95,0x98,0x9c,0x9f,0xa2,0xa5,0xa8,0xab,0xae,
0xb0,0xb3,0xb6,0xb9,0xbc,0xbf,0xc1,0xc4,0xc7,0xc9,0xcc,0xce,0xd1,0xd3,0xd5,0xd8,
0xda,0xdc,0xde,0xe0,0xe2,0xe4,0xe6,0xe8,0xea,0xec,0xed,0xef,0xf0,0xf2,0xf3,0xf5,
0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfc,0xfd,0xfe,0xfe,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0xfe,0xfd,0xfc,0xfc,0xfb,0xfa,0xf9,0xf8,0xf7,
0xf6,0xf5,0xf3,0xf2,0xf0,0xef,0xed,0xec,0xea,0xe8,0xe6,0xe4,0xe2,0xe0,0xde,0xdc,
0xda,0xd8,0xd5,0xd3,0xd1,0xce,0xcc,0xc9,0xc7,0xc4,0xc1,0xbf,0xbc,0xb9,0xb6,0xb3,
0xb0,0xae,0xab,0xa8,0xa5,0xa2,0x9f,0x9c,0x98,0x95,0x92,0x8f,0x8c,0x89,0x86,0x83,
0x80,0x7c,0x79,0x76,0x73,0x70,0x6d,0x6a,0x67,0x63,0x60,0x5d,0x5a,0x57,0x54,0x51,
0x4f,0x4c,0x49,0x46,0x43,0x40,0x3e,0x3b,0x38,0x36,0x33,0x31,0x2e,0x2c,0x2a,0x27,
0x25,0x23,0x21,0x1f,0x1d,0x1b,0x19,0x17,0x15,0x13,0x12,0x10,0x0f,0x0d,0x0c,0x0a,
0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x03,0x02,0x01,0x01,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x02,0x03,0x03,0x04,0x05,0x06,0x07,0x08,
0x09,0x0a,0x0c,0x0d,0x0f,0x10,0x12,0x13,0x15,0x17,0x19,0x1b,0x1d,0x1f,0x21,0x23,
0x25,0x27,0x2a,0x2c,0x2e,0x31,0x33,0x36,0x38,0x3b,0x3e,0x40,0x43,0x46,0x49,0x4c,
0x4f,0x51,0x54,0x57,0x5a,0x5d,0x60,0x63,0x67,0x6a,0x6d,0x70,0x73,0x76,0x79,0x7c
};

//------------------------------------------------------------------------------------
void lfo_init()
{
	
	for(int i=0;i<NUM_LFO;i++)
	{

		lfoArray[i].phase = 0;
		lfoArray[i].phaseInc = 5;
		lfoArray[i].waveform = 0;
		lfoArray[i].retrigger = 0;
		lfoArray[i].phaseOffset = 0;
	}		
};
//------------------------------------------------------------------------------------
//returns a pseudo random number
//TODO a random seed
uint8_t lfo_pseudoRng()
{
	lfo_rng++;
	lfo_rng *= 3;
	return (lfo_rng >> 2) & 0xFF;
		
}
//------------------------------------------------------------------------------------
//returns the next LFO value
uint8_t lfo_calcNextValue(Lfo* lfo)
{
	const uint32_t lastPhase = lfo->phase;
	lfo->phase += lfo->phaseInc;//<<3;
	switch(lfo->waveform)
	{
		case SINE:
		return pgm_read_byte(&sinewave[(lfo->phase>>24)]);
		break;
		
		case SAW_UP:
		return lfo->phase>>24;
		break;
		
		case SAW_DOWN:
		return 255-(lfo->phase>>24);
		break;
		
		case SQUARE:
		if(lfo->phase > 0x7fffffff)
		{
			return 255;
		}			
		else 
		{
			return 0;
		}			
		break;
		
		case RND:
		if(lastPhase > lfo->phase)
			lfo->rngValue =  lfo_pseudoRng()>>1; //0-127
			
			return lfo->rngValue;
		break;
		
		default:
		return 0;
		break;
	}
};
//----------------------------------------------------------------
void lfo_resetCurrentDestination(uint8_t lfoNr)
{
	//reset the current destination value
	//get the LFO destination
	const uint8_t lfoDest = PAR_TARGET_LFO1+lfoNr;
	const uint8_t lfoVoice = parameters[PAR_VOICE_LFO1+lfoNr].value;
	
	//--------------
	//get the page for the menuPage array
	const uint8_t page = (parameters[lfoDest].value&MASK_PAGE)>>PAGE_SHIFT;
	//get the active parameter on the page
	const uint8_t activeParameter	= parameters[lfoDest].value&MASK_PARAMETER;

	//print the corresponding parameter short name
	//&menuPages[voiceNr][page].top1 + activeParameter
					
	uint8_t parNr = pgm_read_byte(&menuPages[lfoVoice][page].bot1 + activeParameter);
					
	//--------------
	uint16_t parVal = parameters[parNr].value;
	
	if(parNr<128)
	{
		
		//send parameter change to uart tx
		uart_putc(MIDI_CC); //staus byte - midi cc
		uart_putc(parNr);//data 1 - parameter number
		uart_putc(parVal);//data 2 - value	
			
	}
	else
	{
		
		uart_putc(CC_2); //staus byte - midi cc
		uart_putc(parNr-128);//data 1 - parameter number
		uart_putc(parVal);//data 2 - value	
	}
	

};
//----------------------------------------------------------------
void lfo_setBPM(uint8_t bpm)
{
	for(int i=0;i<NUM_LFO;i++)
	{
		lfo_setFrequency(i,lfoArray[i].phaseInc);
	}
};
//----------------------------------------------------------------
uint32_t lfo_calcPhaseInc(uint8_t freq, uint8_t sync)
{
	/*
	0 - no sync
	1	4/1
	2	2/1
	3 - 1/1
	4 - 1/2
	5 - 1/3
	6 - 1/4
	7 - 1/6
	8 - 1/8
	9 - 1/12
	10 - 1/16
	*/
	
	if(sync==0)
	{
		return (freq/(float)0x7f)*0xffffff;	
	}
	
	//sync is on

	float bpm =  parameters[PAR_BPM].value;
	
	float tempoAsFrequency = bpm > 0.0 ? bpm / 60.f /4: 0.0f; //bpm/60 = beat duration /4 = bar duration
	
	float scaler;
	
	
	switch(sync)
	{
		default: //no sync
		
		return freq;
		break;
		
		case 1: // 4/1
		scaler = 0.25f;//NoteTypeScalers[Quadruple] * NoteDivisionScalers[Whole]; 
		break;
		
		case 2: // 2/1
		scaler = 0.5f;//NoteTypeScalers[Double] * NoteDivisionScalers[Whole]; 
		break;
		
		case 3: // 1/1
		scaler = 1;//NoteTypeScalers[Regular] * NoteDivisionScalers[Whole]; 
		break;
		
		case 4: // 1/2
		scaler = 2;// NoteTypeScalers[Regular] * NoteDivisionScalers[Half]; 
		break;
		
		case 5: //1/3
		scaler = 3;// NoteTypeScalers[Regular] * NoteDivisionScalers[Third]; 
		break;
		
		case 6: // 1/4
		scaler = 4;// NoteTypeScalers[Regular] * NoteDivisionScalers[Quarter]; 
		break;
		
		case 7: // 1/6
		scaler = 6;// NoteTypeScalers[Regular] * NoteDivisionScalers[Sixth]; 
		break;
		
		case 8: // 1/8
		scaler = 8;// NoteTypeScalers[Regular] * NoteDivisionScalers[Eighth];
		break;
		
		case 9: // 1/12
		scaler = 12;//NoteTypeScalers[Regular] * NoteDivisionScalers[Twelfth];
		break;
		
		case 10: // 1/16
		scaler = 16;//NoteTypeScalers[Regular] * NoteDivisionScalers[Sixteenth];
		break;
		
		
	}
	
	//TODO um sync zu garantieren muss man wohl das ganze zum beginn eines bars resetten
	//sonst läuft es wohl auseinander
	
	float lfoSyncfreq = tempoAsFrequency * scaler; 
//	lcd_home();
//	char text[16];
//	sprintf(text,"%d %d %d",(uint8_t)((lfoSyncfreq / LFO_SR) * 0xffff),(uint8_t)tempoAsFrequency*10,(uint8_t)scaler*10);
//	lcd_string(text);
	return (lfoSyncfreq / LFO_SR) * 0xffffffff; 
}
//----------------------------------------------------------------
void lfo_setFrequency(uint8_t lfoNumber, uint8_t freq)
{
	
	lfoArray[lfoNumber].phaseInc = lfo_calcPhaseInc(freq,parameters[PAR_SYNC_LFO1+lfoNumber].value);
	
}
//----------------------------------------------------------------
void lfo_calc()
{
	for(int i=0;i<NUM_LFO;i++)
	{
		if(!lfo_diasbled)
			{
			const uint8_t amount = parameters[PAR_AMOUNT_LFO1+i].value;
		
			//const uint8_t freq = parameters[PAR_FREQ_LFO1+i].value;
		//	lfoArray[i].phaseInc = freq;
		
			const uint8_t wav = parameters[PAR_WAVE_LFO1+i].value;
			lfoArray[i].waveform = wav;
		
			//get the LFO destination
			const uint8_t lfoDest = PAR_TARGET_LFO1+i;
			const uint8_t lfoVoice = parameters[PAR_VOICE_LFO1+i].value;
		
			//get offset and retrigger
			lfoArray[i].phaseOffset = parameters[PAR_OFFSET_LFO1+i].value * 516; //127*516 -> 65532 (nearly full 16 bit)
			lfoArray[i].retrigger = parameters[PAR_RETRIGGER_LFO1+i].value;
		
			//--------------
			//get the page for the menuPage array
			const uint8_t page = (parameters[lfoDest].value&MASK_PAGE)>>PAGE_SHIFT;
			//get the active parameter on the page
			const uint8_t activeParameter	= parameters[lfoDest].value&MASK_PARAMETER;

			//print the corresponding parameter short name
			//&menuPages[voiceNr][page].top1 + activeParameter
					
			uint8_t parNr = pgm_read_byte(&menuPages[lfoVoice][page].bot1 + activeParameter);
					
			//--------------
		
			uint16_t parVal = parameters[parNr].value + ((lfo_calcNextValue(&lfoArray[i]) * (amount))>>8) ;
			//parVal = (parVal >> 8);
			if(amount>0 )
			{
	
				if(parNr<128)
				{
		
					//send parameter change to uart tx
					uart_putc(MIDI_CC); //staus byte - midi cc
					uart_putc(parNr);//data 1 - parameter number
					uart_putc(parVal);//data 2 - value	
			
				}
				else
				{
		
					uart_putc(CC_2); //staus byte - midi cc
					uart_putc(parNr-128);//data 1 - parameter number
					uart_putc(parVal);//data 2 - value	
				}

			}	
		}				
	}		
};
//----------------------------------------------------------------
void lfo_retrigger(uint8_t voiceNr)
{
	for(int i=0;i<NUM_LFO;i++)
	{
		//if the LFO is retriggered by the current voice
		if(lfoArray[i].retrigger == voiceNr+1)
		{
			lfoArray[i].phase = lfoArray[i].phaseOffset;
		}

	}
	
};
//----------------------------------------------------------------
void lfo_resync(uint8_t voiceNr)
{
	//TODO voiceNr ungenutzt
	for(int i=0;i<NUM_LFO;i++)
	{
		//look for lfo that is modulating the requested voice
	//	if(parameters[PAR_VOICE_LFO1+i].value == voiceNr)
		//{
			//check if sync is active
			if(parameters[PAR_SYNC_LFO1+i].value != 0)
			{
				lfoArray[i].phase = lfoArray[i].phaseOffset;		
			}
	//	}
	}
};
//----------------------------------------------------------------
void lfo_disable(uint8_t disable)
{
	lfo_diasbled = disable;
};
//----------------------------------------------------------------
#endif