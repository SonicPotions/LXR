/*
 * frontPanelParser.c
 *
 * Created: 27.04.2012 12:04:00
 *  Author: Julian
 */ 
#include "frontPanelParser.h"
#include "Menu/menu.h"
#include <stdio.h>

#include "ledHandler.h"
#include "IO\uart.h"
#include "buttonHandler.h"

//debug
#include <stdlib.h>
#include "Hardware/lcd.h"
#include <util\atomic.h>
#include "Preset\PresetManager.h"
//--

static uint8_t frontParser_rxCnt=0;
volatile MidiMsg frontParser_midiMsg;
static uint16_t frontParser_nrpnNr = 0;

uint8_t frontPanel_sysexMode = 0;

volatile uint8_t frontParser_newSeqDataAvailable = 0;
volatile StepData frontParser_stepData;
//sysex buffer used to store 7 byte sysex chunks
volatile uint8_t frontParser_sysexBuffer[7];

uint8_t frontParser_nameIndex = 0;


//------------------------------------------------------------
#define NRPN_MUTE_1 93
#define NRPN_MUTE_7 99
void frontParser_parseNrpn(uint8_t value)
{
	if(frontParser_nrpnNr+128 < NUM_PARAMS)
	{
		parameters[frontParser_nrpnNr+128].value = value;
	}		
	
	if( (frontParser_nrpnNr+128 >= PAR_TARGET_LFO1) && (frontParser_nrpnNr+128 <= PAR_TARGET_LFO6) )
	{
		//since the LFO target calculation from the index number needs to know about the menu structure (menuPages)
		//we need to send back the right target param number to the cortex
		//LFO
		uint8_t lfoNr = (frontParser_nrpnNr+128)-PAR_TARGET_LFO1;
		value = getModTargetValue(parameters[frontParser_nrpnNr+128].value,  parameters[PAR_VOICE_LFO1+lfoNr].value-1);
		uint8_t upper = ((value&0x80)>>7) | (((lfoNr)&0x3f)<<1);
		uint8_t lower = value&0x7f;
		frontPanel_sendData(CC_LFO_TARGET,upper,lower);
	}
	else if ( (frontParser_nrpnNr+128 >= PAR_VEL_DEST_1) && (frontParser_nrpnNr+128 <= PAR_VEL_DEST_6) )
	{
		uint8_t param = parameters[frontParser_nrpnNr+128].value;
		if(param > (NUM_SUB_PAGES * 8 -1))
		param = (NUM_SUB_PAGES * 8 -1); 
				
		uint8_t value = getModTargetValue(param, frontParser_nrpnNr+128 - PAR_VEL_DEST_1);
				
		uint8_t upper,lower;
		upper = ((value&0x80)>>7) | (((frontParser_nrpnNr+128 - PAR_VEL_DEST_1)&0x3f)<<1);
		lower = value&0x7f;
		frontPanel_sendData(CC_VELO_TARGET,upper,lower);
	} else if ( (frontParser_nrpnNr >= NRPN_MUTE_1) && (frontParser_nrpnNr <= NRPN_MUTE_7) )
	{
		const uint8_t voice = frontParser_nrpnNr - NRPN_MUTE_1;
		const uint8_t onOff = value;
		buttonHandler_muteVoice(voice,onOff);
		
	}
}
//------------------------------------------------------------
void frontPanel_ccHandler()
{
	//get the real parameter number from the cc number
	const uint8_t parNr = frontParser_midiMsg.data1 - 1;
	
	if(parNr == NRPN_DATA_ENTRY_COARSE) {
			frontParser_parseNrpn(frontParser_midiMsg.data2);
		}
		
	if(parNr == NRPN_FINE) {
			frontParser_nrpnNr &= ~0x7f;
			frontParser_nrpnNr |= frontParser_midiMsg.data2;
		}
		
	if(parNr == NRPN_COARSE) {
			frontParser_nrpnNr &= 0x7f;
			frontParser_nrpnNr |= frontParser_midiMsg.data2<<7;
		}
	
	//set the parameter value
	parameters[parNr].value = frontParser_midiMsg.data2;
	
	//repaint the LCD
	menu_repaint();
	
};
//------------------------------------------------------------
void frontPanel_parseData(uint8_t data)
{
	// if high byte set a new message starts
	if(data&0x80)
	{	
		//reset the byte counter
		frontParser_rxCnt = 0;
		frontParser_midiMsg.status = data;
		
		
		/*
		if(data==SYSEX_START  )
			{
				//we just entered the sysex mode - no data received yet
			//	lcd_setcursor(5,2);
			//	lcd_string("SYSSTART");
			
			}
			*/
			
		
	}
	else
	{
		if(frontParser_midiMsg.status == SYSEX_START)
		{
			/*
			//we are now in sysex mode
			if(data==SYSEX_START)
			{
				//we just entered the sysex mode - no data received yet
			}				
			else
			*/
			if(frontPanel_sysexMode == SYSEX_REQUEST_STEP_DATA)
			{
				//we are expecting step packages send from the sequencer
				//1st 7 lower nibble 7 bit messages
				//then an upper nibble 7 bit message containing the missing 7 upper bits
				//char text[5];
				if(frontParser_rxCnt<7)
				{
					frontParser_sysexBuffer[frontParser_rxCnt++] = data;
				//	lcd_setcursor(5,2);
				//	itoa(frontParser_rxCnt,text,10);
				//	lcd_string(text);
					
				}
				else
				{
					//now we have to distribute the MSBs to the sysex data
					for(int i=0;i<7;i++)
					{
						frontParser_sysexBuffer[i] |= ((data&(1<<i))<<(7-i));
						
					}
					frontParser_stepData.volume = frontParser_sysexBuffer[0];
					frontParser_stepData.prob = frontParser_sysexBuffer[1];
					frontParser_stepData.note = frontParser_sysexBuffer[2];
					frontParser_stepData.param1Nr = frontParser_sysexBuffer[3];
					frontParser_stepData.param1Val = frontParser_sysexBuffer[4];
					frontParser_stepData.param2Nr = frontParser_sysexBuffer[5];
					frontParser_stepData.param2Val = frontParser_sysexBuffer[6];
					
					//signal that a ne data chunk is available
					frontParser_newSeqDataAvailable = 1;
					//reset receive counter for next chunk
					frontParser_rxCnt = 0;
					
					//lcd_setcursor(5,2);
					//lcd_string("complete");
				}					
				
			}
			else if (frontPanel_sysexMode == SYSEX_REQUEST_PATTERN_DATA)
			{
				if(frontParser_rxCnt<1)
				{
					//1st byte
					frontParser_sysexBuffer[frontParser_rxCnt++] = data;
				} else {
					//2nd byte
					frontParser_sysexBuffer[frontParser_rxCnt++] = data;
					
					uint8_t next = frontParser_sysexBuffer[0];
					uint8_t repeat = frontParser_sysexBuffer[1];
					//we abuse the stepData struct to store the pattern data
					frontParser_stepData.volume = next;
					frontParser_stepData.prob = repeat;
					
					//signal that a ne data chunk is available
					frontParser_newSeqDataAvailable = 1;
					//reset receive counter for next chunk
					frontParser_rxCnt = 0;
				}
				
			}
			else if(frontPanel_sysexMode == SYSEX_REQUEST_MAIN_STEP_DATA)
			{
				if(frontParser_rxCnt<2)
				{
					//1st 2 nibbles
					frontParser_sysexBuffer[frontParser_rxCnt++] = data;
				} else {
					//last 2 bit
					frontParser_sysexBuffer[frontParser_rxCnt++] = data;
					
					uint16_t mainStepData = frontParser_sysexBuffer[0] | (frontParser_sysexBuffer[1]<<7) | (frontParser_sysexBuffer[2]<<14);
					//we abuse the stepData struct to store the main step data
					frontParser_stepData.volume = mainStepData>>8;
					frontParser_stepData.prob = mainStepData&0xff;
					
					//signal that a new data chunk is available
					frontParser_newSeqDataAvailable = 1;
					//reset receive counter for next chunk
					frontParser_rxCnt = 0;
				}
			}
		}
		else if(frontParser_rxCnt==0)
		{
			//parameter nr
			frontParser_midiMsg.data1 = data;
			frontParser_rxCnt++;
		}
		else
		{
			//parameter value
			frontParser_midiMsg.data2 = data;
			frontParser_rxCnt=0;
			//process the received data
			if(frontParser_midiMsg.status == MIDI_CC) //sound cc data from cortex 
			{
				frontPanel_ccHandler();
			}
			else
			{
				if(frontParser_midiMsg.status == PRESET_NAME)
				{
					
					if(frontParser_midiMsg.data2 & 0x40)
					{
						frontParser_nameIndex = 0;
					}
					
					preset_currentName[frontParser_nameIndex] = (frontParser_midiMsg.data1&0x7f) | ((frontParser_midiMsg.data2&0x7f)<<7);
					frontParser_nameIndex++;
					frontParser_nameIndex &= 0x7; //wrap at 8
					if(frontParser_nameIndex==0)
					{
						menu_repaintAll();
					}						
									
				} 
				else if(frontParser_midiMsg.status == SET_P1_DEST)
				{
					parameters[PAR_P1_DEST].value = (frontParser_midiMsg.data1<<7) | frontParser_midiMsg.data2;
					menu_repaintAll();
				}
				else if(frontParser_midiMsg.status == SET_P2_DEST)
				{
					parameters[PAR_P2_DEST].value = (frontParser_midiMsg.data1<<7) | frontParser_midiMsg.data2;
					menu_repaintAll();
				}
				else if(frontParser_midiMsg.status == SET_P1_VAL)
				{
					parameters[PAR_P1_VAL].value = (frontParser_midiMsg.data1<<7) | frontParser_midiMsg.data2;
					menu_repaintAll();
				}
				else if(frontParser_midiMsg.status == SET_P2_VAL)
				{
					parameters[PAR_P2_VAL].value = (frontParser_midiMsg.data1<<7) | frontParser_midiMsg.data2;
					menu_repaintAll();
				}
				
				else if(frontParser_midiMsg.status == SEQ_CC)
				{
					switch(frontParser_midiMsg.data1)
					{
						
						case SEQ_SET_PAT_BEAT:
						parameters[PAR_PATTERN_BEAT].value = frontParser_midiMsg.data2;
						menu_repaint();
						break;	
						case SEQ_SET_PAT_NEXT:
						parameters[PAR_PATTERN_NEXT].value = frontParser_midiMsg.data2;
						menu_repaint();
						break;
						
						case SEQ_TRACK_LENGTH:
						parameters[PAR_TRACK_LENGTH].value = frontParser_midiMsg.data2;
						menu_repaint();
						break;
						
						case SEQ_EUKLID_LENGTH:
						parameters[PAR_EUKLID_LENGTH].value = frontParser_midiMsg.data2;
						menu_repaint();
						break;
						
						case SEQ_EUKLID_STEPS:
						parameters[PAR_EUKLID_STEPS].value = frontParser_midiMsg.data2;
						menu_repaint();
						break;
						
						case SEQ_VOLUME:
						parameters[PAR_STEP_VOLUME].value = frontParser_midiMsg.data2;
						menu_repaintAll();
						break;
						
						case SEQ_PROB:
						parameters[PAR_STEP_PROB].value = frontParser_midiMsg.data2;
						menu_repaintAll();
						break;
						
						case SEQ_NOTE:
						parameters[PAR_STEP_NOTE].value = frontParser_midiMsg.data2;
						menu_repaintAll();
						break;
						
						/*
						case SEQ_GET_ACTIVE_PAT:
						//only in mute mode relevant
						//led_clearSequencerLeds9_16();
						led_clearSelectLeds();
						// set led to show active pattern
						led_setValue(1,LED_PART_SELECT1+frontParser_midiMsg.data2);
						break;
						*/
						
						case SEQ_CHANGE_PAT:
						
						if(frontParser_midiMsg.data2 > 7) return;
						//ack message that the sequencer changed to the requested pattern
						
						//stop blinking pattern led
						led_setBlinkLed(LED_PART_SELECT1+frontParser_midiMsg.data2,0);
						//clear last pattern led
						
						if( (parameters[PAR_FOLLOW].value) ) {
							
							if( menu_activePage != PATTERN_SETTINGS_PAGE)
							{
								menu_setShownPattern(frontParser_midiMsg.data2);
								led_clearSequencerLeds();
								//query current sequencer step states and light up the corresponding leds 
								uint8_t trackNr = menu_getActiveVoice(); //max 6 => 0x6 = 0b110
								uint8_t patternNr = menu_getViewedPattern(); //max 7 => 0x07 = 0b111
								uint8_t value = (trackNr<<4) | (patternNr&0x7);
								frontPanel_sendData(LED_CC,LED_QUERY_SEQ_TRACK,value);
								frontPanel_sendData(SEQ_CC,SEQ_REQUEST_PATTERN_PARAMS,frontParser_midiMsg.data2);
							} else {
								//store the pending pattern update for shift button release handler
								menu_shownPattern = frontParser_midiMsg.data2;
							}								
						}	
						menu_playedPattern = frontParser_midiMsg.data2;						
						
						if( (buttonHandler_getMode() == SELECT_MODE_PERF) || (buttonHandler_getMode() == SELECT_MODE_PAT_GEN) )
						{
							//only show pattern changes when in performance mode
								
							//led_clearSequencerLeds9_16();
							led_clearSelectLeds();
							led_clearAllBlinkLeds();
							// re init the LEDs shwoing active/viewed pattern
							led_initPerformanceLeds();
							//led_setValue(1,LED_PART_SELECT1+frontParser_midiMsg.data2);
						}			
						
						
						break;
						
						case LED_QUERY_SEQ_TRACK:
						//this message is only send by the frontpanel, so it doesnt need to handle it
						break;
						
						case SEQ_RESYNC_LFO:

						break;
					};						
				}
				else if(frontParser_midiMsg.status == LED_CC)
				{
					switch(frontParser_midiMsg.data1)
					{
						
						case LED_CURRENT_STEP_NR: {
							
							if(frontParser_midiMsg.data2 >=128) return;
							
							uint8_t shownPattern = menu_getViewedPattern();
							uint8_t playedPattern = menu_playedPattern;
							
							if(shownPattern == playedPattern) {
								//only update chaselight LED when it step edit mode
								if( (menu_activePage < MENU_MIDI_PAGE) || menu_activePage == PERFORMANCE_PAGE ||menu_activePage == SEQ_PAGE || menu_activePage == EUKLID_PAGE) {
									led_setActive_step(frontParser_midiMsg.data2);
								}							
							} else {
								led_clearActive_step();
							}								
															
						}							
						break;
						

						
						case LED_PULSE_BEAT:
						if(frontParser_midiMsg.data2!=0)
						{
							led_setValue(1,LED_START_STOP);
						}
						else
						{
							led_setValue(0,LED_START_STOP);
						}							
						break;
						
	
						case LED_SEQ_SUB_STEP:
						if(buttonHandler_getShift() || buttonHandler_getMode() == SELECT_MODE_STEP)
							{
								//parse sub steps
								
								
								uint8_t stepNr = frontParser_midiMsg.data2 & 0x7f;
								uint8_t subStepRange = buttonHandler_selectedStep;
								//check if received step is a valid sub step
								if( (stepNr >= subStepRange) && (stepNr<(subStepRange+8)) )
								{
									stepNr = stepNr - subStepRange;
									led_setValue(1,LED_PART_SELECT1+stepNr);
								}
								
								
							}
						break;
						case LED_SEQ_BUTTON:
						{
							if(menu_activePage != PERFORMANCE_PAGE) //do not show active steps on perf. page
							{
								//limit to 16 steps
								uint8_t stepNr = ((frontParser_midiMsg.data2&0x7f)/8); //limit to 127
								
								led_setValue(1,LED_STEP1+stepNr);
							}
						}		
						
						break;
					}						
				}								
				else if(frontParser_midiMsg.status == NOTE_ON)
				{
					if(frontParser_midiMsg.data1 > 6) return;
					//only SELECT_MODE_VOICE and SELECT_MODE_MUTE
					led_pulseLed(LED_VOICE1+frontParser_midiMsg.data1);
				}

			}				
		}
	}
};
//------------------------------------------------------------
void frontPanel_sendMidiMsg(MidiMsg msg)
{
	while(uart_putc(msg.status) == 0); 
	//data 1 - parameter number
	while(uart_putc(msg.data1) == 0);
	//data 2 - value	
	while(uart_putc(msg.data2) == 0);
};
//------------------------------------------------------------
void frontPanel_sendData(uint8_t status, uint8_t data1, uint8_t data2)
{
	//we need atomic acess, otherwise the LFO will interrupt message sending
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
	{
		while(uart_putc(status) == 0); 
		//data 1 - parameter number
		while(uart_putc(data1) == 0);
		//data 2 - value	
		while(uart_putc(data2) == 0);
	}		
};
//------------------------------------------------------------
void frontPanel_sendByte(uint8_t data)
{
	while(uart_putc(data) == 0); 
};
//------------------------------------------------------------