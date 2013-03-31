/*
 * buttonHandler.c
 *
 * Created: 24.04.2012 16:04:29
 *  Author: Julian
 */ 
#include "buttonHandler.h"
#include "Menu/menu.h"
#include "ledHandler.h"
#include "Hardware/lcd.h"
#include <stdio.h>
#include "frontPanelParser.h"
#include "IO\din.h"
#include "Menu\screensaver.h"
#include "Menu/copyClearTools.h"
#include "Hardware/timebase.h"




//volatile uint8_t buttonHandler_selectButtonMode; 


//used to remember the last active page when entering the sequencer mode
//needed to return to the previously shown page when seq mode button is released
uint8_t lastActivePage=0;
uint8_t lastActiveSubPage=0;
/** selected step for sequencer mode*/
uint8_t buttonHandler_selectedStep=0; //TODO ist das gleiche wie der parameter PAR_ACTIVE_STEP
uint8_t selectedStepLed=LED_STEP1;

static uint16_t buttonHandler_buttonTimer = 0;
#define NO_STEP_SELECTED -1
static int8_t buttonHandler_buttonTimerStepNr = NO_STEP_SELECTED;

volatile struct 
{
	unsigned selectButtonMode:3;
	unsigned seqRunning:1;
	unsigned seqRecording:1;
	
	
}buttonHandler_stateMemory;

uint8_t buttonHandler_mutedVoices=0;
static int8_t buttonHandler_armedAutomationStep = NO_STEP_SELECTED;

#define ARM_AUTOMATION		0x40
#define DISARM_AUTOMATION	0x00

//--------------------------------------------------------
void buttonHandler_armTimerActionStep(uint8_t stepNr)
{
	//check if sub or main step
	const uint8_t isMainStep = ( (stepNr%8)==0 );
	buttonHandler_armedAutomationStep = stepNr;
	
	if(isMainStep)
	{
		const uint8_t mainStepNr = stepNr / 8;
		led_setBlinkLed(LED_STEP1 + mainStepNr,1);
	} else {
		const uint8_t selectButtonNr = stepNr%8;
		led_setBlinkLed(LED_PART_SELECT1 + selectButtonNr,1);
	}
	
	frontPanel_sendData(ARM_AUTOMATION_STEP,stepNr,menu_getActiveVoice() | ARM_AUTOMATION);
	
	
	
}
//--------------------------------------------------------
void buttonHandler_disarmTimerActionStep()
{
	//TODO FALSCH das löscht danna uch blinke LEDs die anbleiben sollen!
	//led_clearAllBlinkLeds();
	
	if(buttonHandler_armedAutomationStep != NO_STEP_SELECTED)
	{
		const uint8_t isMainStep = ( (buttonHandler_armedAutomationStep%8)==0 );
		if(isMainStep)
		{
			const uint8_t mainStepNr = buttonHandler_armedAutomationStep / 8;
			led_setBlinkLed(LED_STEP1 + mainStepNr,0);
		} else {
			const uint8_t selectButtonNr = buttonHandler_armedAutomationStep%8;
			led_setBlinkLed(LED_PART_SELECT1 + selectButtonNr,0);
		}
	}
	
	buttonHandler_armedAutomationStep = NO_STEP_SELECTED;
	frontPanel_sendData(ARM_AUTOMATION_STEP,0,DISARM_AUTOMATION);
};
//--------------------------------------------------------
int8_t buttonHandler_getArmedAutomationStep()
{
	return buttonHandler_armedAutomationStep;
};
//--------------------------------------------------------
uint8_t buttonHandler_TimerActionOccured()
{
	buttonHandler_disarmTimerActionStep();
	if(buttonHandler_buttonTimerStepNr == NO_STEP_SELECTED)
	{
		//a timed action apeared -> do nothing
		return 1;
	}
	buttonHandler_buttonTimerStepNr = NO_STEP_SELECTED;
	return 0;
}
void buttonHandler_setTimeraction(uint8_t buttonNr)
{
	buttonHandler_buttonTimer = time_sysTick+BUTTON_TIMEOUT;
	buttonHandler_buttonTimerStepNr = buttonNr;
	
}
//--------------------------------------------------------
/**returns 1 is the mode 2 select button is pressed*/
/*
uint8_t buttonHandler_isModeButtonPressed()
{
	const uint8_t arrayPos	= BUT_MODE/8;
	const uint8_t bitPos	= BUT_MODE&7;
	
	if(din_inputData[arrayPos] & (1<<bitPos))
	{
		return 0;
	}
	return 1;
}
*/

//--------------------------------------------------------
//periodically called handler for timed actions
//-> hold a step button for a long time to select/rec automation
void buttonHandler_tick()
{
	if( (time_sysTick > buttonHandler_buttonTimer))
	{
		if(buttonHandler_buttonTimerStepNr !=NO_STEP_SELECTED)
		{
			//select step
			buttonHandler_armTimerActionStep(buttonHandler_buttonTimerStepNr);
			//reset
			buttonHandler_buttonTimerStepNr = NO_STEP_SELECTED;
		}
	}
};
//--------------------------------------------------------

//--------------------------------------------------------
/**returns 1 is the shift button is pressed*/
uint8_t buttonHandler_getShift()
{
	const uint8_t arrayPos	= BUT_SHIFT/8;
	const uint8_t bitPos	= BUT_SHIFT&7;
	
	if(din_inputData[arrayPos] & (1<<bitPos))
	{
		return 0;
	}
	return 1;
}
//--------------------------------------------------------
void buttonHandler_handleModeButtons(uint8_t mode)
{


	if(buttonHandler_getShift())
	{
		
		//set the new mode
		buttonHandler_stateMemory.selectButtonMode = mode+4;
	}
	else
	{
		
		//set the new mode
		buttonHandler_stateMemory.selectButtonMode = mode;
	}
	
	/*
	if(buttonHandler_stateMemory.selectButtonMode!=SELECT_MODE_LOAD_SAVE) {
		buttonHandler_leaveSeqMode(); // to prevent blink leds from hanging 
	}	
	*/
	
	led_clearAllBlinkLeds();	
		

		//update the status LED
		led_setMode2(buttonHandler_stateMemory.selectButtonMode);
			
		switch(buttonHandler_stateMemory.selectButtonMode)
		{
			case SELECT_MODE_PERF:
			{
				//show the muted voices on the select buttons
				//led_setMode2Leds(buttonHandler_mutedVoices);
				
				//show the active pattern on the sequencer buttons 9-16
				led_clearSequencerLeds();
				led_clearSelectLeds();
				//request active pattern
				//frontPanel_sendData(SEQ_CC,SEQ_GET_ACTIVE_PAT,0x00);
				
				//led_clearSequencerLeds9_16();
				led_clearSelectLeds();
				// set led to show active pattern
				led_setValue(1, LED_PART_SELECT1 + menu_playedPattern);
				//set menu to mute page mode
				
				lastActiveSubPage	= menu_getSubPage();
				menu_switchPage(PERFORMANCE_PAGE);
				menu_switchSubPage(0);
				
			}
			break;
			
			case SELECT_MODE_STEP:
				menu_switchPage(menu_getActiveVoice());
				led_setActiveSelectButton(menu_getSubPage()); 
				buttonHandler_enterSeqModeStepMode();
				break;
			case SELECT_MODE_VOICE:
			//set menu to voice page mode
				menu_switchPage(menu_getActiveVoice());
				led_setActiveSelectButton(menu_getSubPage()); 
			break;
		
			case SELECT_MODE_LOAD_SAVE:
					menu_switchPage(LOAD_PAGE);
			break;
			
			case SELECT_MODE_MENU:
				menu_switchPage(MENU_MIDI_PAGE);

			break;
			

			
			case SELECT_MODE_PAT_GEN:
				frontPanel_sendData(SEQ_CC,SEQ_REQUEST_EUKLID_PARAMS,menu_getActiveVoice());
				menu_switchPage(EUKLID_PAGE);
			
			break;
			
			default:
			break;
				
							
		}
		
	
	//menu_switchSubPage(0);
}
//--------------------------------------------------------
void buttonHandler_handleSelectButton(uint8_t buttonNr)
{

	if(buttonHandler_getShift()) {
		
		switch(buttonHandler_stateMemory.selectButtonMode)
		{
			case SELECT_MODE_STEP:
			/*
			//change sub page -> osc, filter, mod etc...
			menu_switchSubPage(buttonNr);
			//go to 1st parameter on sub page
			menu_resetActiveParameter();
			led_setActiveSelectButton(buttonNr);
			*/
			
			//set/remove sub steps
			//break;
			
			case SELECT_MODE_VOICE: {
				//select buttons represent sub steps
		
				uint8_t stepNr = buttonHandler_selectedStep + buttonNr;
				uint8_t ledNr = LED_PART_SELECT1 + buttonNr;
				//toggle the led
				led_toggle(ledNr);
				//TODO lastActivePage zeigt nur auf den currentTrack wenn man im stepMode ist... tut nicht in den anderen modes :-/
				//set sequencer step on soundchip
		
				uint8_t trackNr = menu_getActiveVoice(); //max 6 => 0x6 = 0b110
				uint8_t patternNr = menu_getShownPattern(); //max 7 => 0x07 = 0b111
				uint8_t value = (trackNr<<4) | (patternNr&0x7);
				frontPanel_sendData(STEP_CC,value,stepNr);
		
				//request step parameters from sequencer
				frontPanel_sendData(SEQ_CC,SEQ_REQUEST_STEP_PARAMS,stepNr);
		
				parameters[PAR_ACTIVE_STEP].value = stepNr;
			}				
				break;
			case SELECT_MODE_PAT_GEN:
			case SELECT_MODE_PERF:
				//select shownPattern
					menu_setShownPattern(buttonNr);
					led_clearSelectLeds();
					led_clearAllBlinkLeds();
					led_setBlinkLed(LED_PART_SELECT1+buttonNr,1);
					//led_clearSequencerLeds();
					//query current sequencer step states and light up the corresponding leds 
					uint8_t trackNr = menu_getActiveVoice(); //max 6 => 0x6 = 0b110
					uint8_t patternNr = menu_getShownPattern(); //max 7 => 0x07 = 0b111
					uint8_t value = (trackNr<<4) | (patternNr&0x7);
					frontPanel_sendData(LED_CC,LED_QUERY_SEQ_TRACK, value);
					frontPanel_sendData(SEQ_CC,SEQ_REQUEST_PATTERN_PARAMS, patternNr);
					frontPanel_sendData(SEQ_CC,SEQ_REQUEST_EUKLID_PARAMS, menu_activePage);
			break;
		}			
	} else {
		switch(buttonHandler_stateMemory.selectButtonMode)
		{
			
			case SELECT_MODE_STEP:
			{
				//select buttons represent sub steps
		
				uint8_t stepNr = buttonHandler_selectedStep + buttonNr;
				uint8_t ledNr = LED_PART_SELECT1 + buttonNr;
				//toggle the led
				//led_toggle(ledNr);

				//uint8_t trackNr = menu_getActiveVoice(); //max 6 => 0x6 = 0b110
				//uint8_t patternNr = menu_getShownPattern(); //max 7 => 0x07 = 0b111
				//uint8_t value = (trackNr<<4) | (patternNr&0x7);
				//frontPanel_sendData(STEP_CC,value,stepNr);
		
				//request step parameters from sequencer
				frontPanel_sendData(SEQ_CC,SEQ_REQUEST_STEP_PARAMS,stepNr);
		
				parameters[PAR_ACTIVE_STEP].value = stepNr;
				
				//buttonHandler_armTimerActionStep(stepNr);
				led_clearAllBlinkLeds();
				led_setBlinkLed(LED_STEP1 + buttonHandler_selectedStep,1);
				
				const uint8_t selectButtonNr = stepNr%8;
				led_setBlinkLed(LED_PART_SELECT1 + selectButtonNr,1);
			}	
			break;
			
			
			case SELECT_MODE_VOICE:
			//change sub page -> osc, filter, mod etc...
			menu_switchSubPage(buttonNr);
			//go to 1st parameter on sub page
			menu_resetActiveParameter();
			led_setActiveSelectButton(buttonNr);
			break;
		
			case SELECT_MODE_PERF:
			
			
				//check if euklid or perf mode is active
				if(menu_getActivePage() == PERFORMANCE_PAGE)
				{
					//todo
					//change pattern
					//if shift -> select pattern for edit
					
					if(buttonHandler_getShift())
					{
						
					} else {
						//pattern
						
						//tell sequencer to change pattern
						frontPanel_sendData(SEQ_CC,SEQ_CHANGE_PAT,buttonNr);
						//flash corresponding LED until ACK (SEQ_CHANGE_PAT) received
						uint8_t ledNr = LED_PART_SELECT1 + buttonNr;
						led_clearAllBlinkLeds();
						
						led_setBlinkLed(ledNr,1);
							
						//request the pattern info for the selected pattern (bar cnt, next...)
					//	frontPanel_sendData(SEQ_CC,SEQ_REQUEST_PATTERN_PARAMS,buttonNr);
						
						
					}
					
					
#if 0
					if(buttonNr == 7)
					{
						//unmute all tracks
						for(int i=0;i<8;i++)
						{
							if(buttonHandler_mutedVoices & (1<<i))
								frontPanel_sendData(SEQ_CC,SEQ_UNMUTE_TRACK,i);
						}
						buttonHandler_mutedVoices = 0;
					}
					else if(buttonHandler_mutedVoices & (1<<buttonNr))
					{
						//unmute tracks 0-7	
						buttonHandler_mutedVoices &= ~(1<<buttonNr);
						frontPanel_sendData(SEQ_CC,SEQ_UNMUTE_TRACK,buttonNr);
					}
					else
					{
						//mute tracks 0-7
						buttonHandler_mutedVoices |= (1<<buttonNr);
						frontPanel_sendData(SEQ_CC,SEQ_MUTE_TRACK,buttonNr);
					}
				
					//muted tracks are lit
					led_setMode2Leds(buttonHandler_mutedVoices);
#endif
				}
				//----- Euklid Mode -----
				else
				{
					/* //moved to voice button
					//tell the sequencer the new active track
					//TODO muss nicht für button 8 gesendet werden
					frontPanel_sendData(SEQ_CC,SEQ_SET_ACTIVE_TRACK,buttonNr);
					//request the parameters for the new track
					frontPanel_sendData(SEQ_CC,SEQ_REQUEST_EUKLID_PARAMS,buttonNr);
					//the currently active track button is lit
					led_setActivePage(buttonNr);
					*/
					
										
				}					
			break;
		
	
		
			case SELECT_MODE_LOAD_SAVE:
				//the currently active button is lit
				led_setActivePage(buttonNr);
				
			//	menu_switchPage(LFO1_PAGE+buttonNr);
			break;
		}
	}		
	
}
//--------------------------------------------------------
void buttonHandler_updateSubSteps(uint8_t track)
{
	led_clearSelectLeds();
	//query current sequencer step states and light up the corresponding leds 
	uint8_t trackNr = menu_getActiveVoice(); //max 6 => 0x6 = 0b110
	uint8_t patternNr = menu_getShownPattern(); //max 7 => 0x07 = 0b111
	uint8_t value = (trackNr<<4) | (patternNr&0x7);
	frontPanel_sendData(LED_CC,LED_QUERY_SEQ_TRACK,value);
}
//--------------------------------------------------------
/* 
-> change display to show selected step options (volume, probability, note etc...)
-> seq buttons select step
-> selected step is illuminated on the seq buttons
-> select buttons show 8 sub steps for the selected step
*/
void buttonHandler_enterSeqModeStepMode()
{
//	lastActivePage = menu_activePage;
//	lastActiveSubPage = menu_getSubPage();
	menu_switchSubPage(0);
	menu_switchPage(SEQ_PAGE);

	buttonHandler_updateSubSteps(lastActivePage);
	
	led_setBlinkLed(selectedStepLed,1);
}
//--------------------------------------------------------
void buttonHandler_leaveSeqModeStepMode()
{
	//stop blinking active step led
	led_setBlinkLed(selectedStepLed,0);
	led_setValue(0,selectedStepLed);
	
	//reset select leds
	//led_clearSelectLeds();
	
//	menu_switchSubPage(lastActiveSubPage);
//	menu_switchPage(lastActivePage);
}
//--------------------------------------------------------
void buttonHandler_enterSeqMode()
{
	lastActivePage = menu_activePage;
	lastActiveSubPage = menu_getSubPage();
	menu_switchSubPage(0);
	menu_switchPage(SEQ_PAGE);

	buttonHandler_updateSubSteps(lastActivePage);
	
	led_setBlinkLed(selectedStepLed,1);
}
//--------------------------------------------------------
void buttonHandler_leaveSeqMode()
{
	//stop blinking active step led
	led_setBlinkLed(selectedStepLed,0);
	led_setValue(0,selectedStepLed);
	
	//reset select leds
	led_clearSelectLeds();
	
	menu_switchSubPage(lastActiveSubPage);
	menu_switchPage(lastActivePage);
}
//--------------------------------------------------------
void buttonHandler_buttonReleased(uint8_t buttonNr)
{
	

	switch(buttonNr)
	{
		
		case BUT_COPY:
			if(buttonHandler_getShift())
			{
				
				
			}
			else
			{
				//copy mode abort/exit
				copyClear_Mode = MODE_NONE;
				led_setBlinkLed(LED_COPY,0);
			
			}

		
		break;
		
		case BUT_SHIFT: 
		/* when this button is released, revert back to normal operating mode
		*/
		
		//leave clear mode
		if(copyClear_Mode == MODE_CLEAR)
		{
			copyClear_armClearMenu(0);
			copyClear_Mode = MODE_NONE;
		}			
		
		led_setValue(0,LED_SHIFT);
		if(buttonHandler_getMode() == SELECT_MODE_VOICE) {
			buttonHandler_leaveSeqMode();			
		} else if( (buttonHandler_getMode() == SELECT_MODE_PERF)) {
			led_clearAllBlinkLeds();
			led_clearSelectLeds();
			led_setValue(1,menu_playedPattern + LED_PART_SELECT1);
			menu_switchPage(PERFORMANCE_PAGE);
		} else if( (buttonHandler_getMode() == SELECT_MODE_PAT_GEN)) {
			led_clearAllBlinkLeds();
			led_clearSelectLeds();
			led_setValue(1,menu_playedPattern + LED_PART_SELECT1);
			menu_switchPage(EUKLID_PAGE);
		} else if ( (buttonHandler_getMode() == SELECT_MODE_STEP)) {
			buttonHandler_enterSeqModeStepMode();
		}			
		
		//show active voice if released
		led_setActiveVoice(menu_getActiveVoice());
		
		break;
		
			//the sequencer buttons
		case BUT_SEQ1:
		case BUT_SEQ2:
		case BUT_SEQ3:
		case BUT_SEQ4:
		case BUT_SEQ5:
		case BUT_SEQ6:
		case BUT_SEQ7:
		case BUT_SEQ8:
		case BUT_SEQ9:
		case BUT_SEQ10:
		case BUT_SEQ11:
		case BUT_SEQ12:
		case BUT_SEQ13:
		case BUT_SEQ14:
		case BUT_SEQ15:
		case BUT_SEQ16:
		{
			
			
			
			//which button is pressed
			uint8_t seqButtonPressed;
			uint8_t ledNr;
			
			seqButtonPressed = buttonNr;
			ledNr = buttonNr+LED_STEP1;
			
			if(buttonHandler_getShift())
			{
				//do nothing if timer action occured
				if(buttonHandler_TimerActionOccured())return;
			
				if( (buttonHandler_getMode() == SELECT_MODE_VOICE) )
				{
					//sequencer mode -> buttons select active step
					buttonHandler_selectActiveStep(ledNr, seqButtonPressed);
				} else if ( (buttonHandler_getMode() == SELECT_MODE_STEP) )
				{
					buttonHandler_setRemoveStep(ledNr,seqButtonPressed);
				}
				
			}
			else
			{
			
				switch(buttonHandler_stateMemory.selectButtonMode)
				{
					case SELECT_MODE_STEP:
						//do nothing if timer action occured
						if(buttonHandler_TimerActionOccured())return;
						
						buttonHandler_selectActiveStep(ledNr, seqButtonPressed);
					break;
					//--- edit the pattern -> button sets and removes a step ---
					case SELECT_MODE_VOICE:
					
						//do nothing if timer action occured
						if(buttonHandler_TimerActionOccured())return;
											
						buttonHandler_setRemoveStep(ledNr,seqButtonPressed);
					
					break;
					case SELECT_MODE_PERF:
						if(seqButtonPressed < 8 )
						{
							//turn roll off
							frontPanel_sendData(SEQ_CC,SEQ_ROLL_ON_OFF,(seqButtonPressed&0xf) );
							//turn button led off
							led_setValue(0,ledNr);
							
							
						}

					break;
	
					//--- unused (maybe lfo clock sync? ---
					case SELECT_MODE_LOAD_SAVE:
				
					break;
				
					default:
					break;
			
				}	
			}						
			
			
			///--------------------------------
			/*
			//which button is pressed
			uint8_t seqButtonPressed;
			uint8_t ledNr;

			seqButtonPressed = buttonNr;
			ledNr = buttonNr+LED_STEP1;

			
			if(buttonHandler_getShift())
			{
			}
			else
			{
				switch(buttonHandler_stateMemory.selectButtonMode)
				{
					
					case SELECT_MODE_VOICE:
					
					break;
					//--- buttons 1-8 turn off roll
					case SELECT_MODE_PERF:
						if(seqButtonPressed < 8 )
						{
							//turn roll on
							frontPanel_sendData(SEQ_CC,SEQ_ROLL_ON_OFF,(seqButtonPressed&0xf) );
							//turn button led off
							led_setValue(0,ledNr);
							
							
						}

					break;
					//--- select a step to edit ---
					case SELECT_MODE_STEP:
				
					break;
					//--- unused (maybe lfo clock sync? ---
					case SELECT_MODE_LOAD_SAVE:
				
					break;
				
					default:
					break;
			
				}					
			}	
			*/	
		}
		
		break;	
		
		//Part select buttons
		case BUT_SELECT1:
		case BUT_SELECT2:
		case BUT_SELECT3:
		case BUT_SELECT4:
		case BUT_SELECT5:
		case BUT_SELECT6:
		case BUT_SELECT7:
		case BUT_SELECT8:
			
			
			if(copyClear_Mode >= MODE_COPY_PATTERN)
			{
			
			}
			else
			{
				//do nothing if timer action occured
				if(buttonHandler_TimerActionOccured())return;
				
				buttonHandler_buttonTimerStepNr = NO_STEP_SELECTED;
				buttonHandler_handleSelectButton(buttonNr-BUT_SELECT1);
			}			

		break;
	}		
}
//--------------------------------------------------------
void buttonHandler_selectActiveStep(uint8_t ledNr, uint8_t seqButtonPressed)
{
	led_setBlinkLed(selectedStepLed,0);
	led_setValue(0,selectedStepLed);
				
	//update active step
	buttonHandler_selectedStep = seqButtonPressed*8;
	selectedStepLed = ledNr;
								
	parameters[PAR_ACTIVE_STEP].value = buttonHandler_selectedStep;
				
	//blink new step
	led_setBlinkLed(ledNr,1);
				
	//update sub steps
	buttonHandler_updateSubSteps(menu_activeVoice);
	//update sub steps
	//request step parameters from sequencer 
	frontPanel_sendData(SEQ_CC,SEQ_REQUEST_STEP_PARAMS,seqButtonPressed*8);
}
//--------------------------------------------------------
void buttonHandler_setRemoveStep(uint8_t ledNr, uint8_t seqButtonPressed)
{
	//led_toggle(ledNr); //handled by cortex
	led_setValue(0,ledNr);
	//we have 128 steps, the main buttons are only for multiples of 8
	seqButtonPressed *=8;
	//which track is active
	//uint8_t currentTrack = menu_getActivePage();
						
	//update active step (so that seq mode always shows the last set step)
	buttonHandler_selectedStep = seqButtonPressed;
	parameters[PAR_ACTIVE_STEP].value = buttonHandler_selectedStep;
	selectedStepLed = ledNr;
				
	//request step parameters from sequencer
	frontPanel_sendData(SEQ_CC,SEQ_REQUEST_STEP_PARAMS,seqButtonPressed);
						
	//set sequencer step on soundchip
	uint8_t trackNr = menu_getActiveVoice(); //max 6 => 0x6 = 0b110
	uint8_t patternNr = menu_getShownPattern(); //max 7 => 0x07 = 0b111
	uint8_t value = (trackNr<<4) | (patternNr&0x7);
	//frontPanel_sendData(STEP_CC,value,seqButtonPressed);
	frontPanel_sendData(MAIN_STEP_CC,value,seqButtonPressed/8);		
}
//--------------------------------------------------------
void buttonHandler_buttonPressed(uint8_t buttonNr)
{

/*
char text[5];
lcd_setcursor(0,0);
itoa(buttonNr,text,10);
				
lcd_clear();
lcd_string(text);

led_toggle(buttonNr);
//led_setValue(0,buttonNr)
return;
*/
	screensaver_touch();

	switch(buttonNr)
	{
		//Part select buttons
		case BUT_SELECT1:
		case BUT_SELECT2:
		case BUT_SELECT3:
		case BUT_SELECT4:
		case BUT_SELECT5:
		case BUT_SELECT6:
		case BUT_SELECT7:
		case BUT_SELECT8:
			
			
			
			if(copyClear_Mode >= MODE_COPY_PATTERN)
			{
				//copy mode 
				if(copyClear_srcSet())
				{
					//select dest
					copyClear_setDst(buttonNr-BUT_SELECT1,MODE_COPY_PATTERN);
					copyClear_copyPattern();
					led_clearAllBlinkLeds();
					//query current sequencer step states and light up the corresponding leds 
					//query current sequencer step states and light up the corresponding leds 
					
					uint8_t trackNr = menu_getActiveVoice(); //max 6 => 0x6 = 0b110
					uint8_t patternNr = menu_getShownPattern(); //max 7 => 0x07 = 0b111
					uint8_t value = (trackNr<<4) | (patternNr&0x7);
					frontPanel_sendData(LED_CC,LED_QUERY_SEQ_TRACK,value);
				
				}
				else
				{
					//select src
					copyClear_setSrc(buttonNr-BUT_SELECT1,MODE_COPY_PATTERN);
					led_setBlinkLed(LED_PART_SELECT1+buttonNr-BUT_SELECT1,1);
				}
			}
			else
			{
				//moved to release button
				//buttonHandler_handleSelectButton(buttonNr-BUT_SELECT1);
				
				//select sub step when button is held
				//-> step mode
				//-> voice mode + shift
				
				if ( /*(buttonHandler_getMode() == SELECT_MODE_STEP) || */((buttonHandler_getMode() == SELECT_MODE_VOICE) && buttonHandler_getShift() ) )
				{
					//TODO hier muss selektiert werden!
					uint8_t selectButtonNr = buttonNr-BUT_SELECT1;
					uint8_t buttonNr = buttonHandler_selectedStep*8 + selectButtonNr;
					buttonHandler_setTimeraction(buttonNr);
				} else {
					
					buttonHandler_handleSelectButton(buttonNr-BUT_SELECT1);
				}				

			}				
		break;
		
		case BUT_MODE1:
		case BUT_MODE2:
		case BUT_MODE3:
		case BUT_MODE4:
			buttonHandler_handleModeButtons(buttonNr-BUT_MODE1);
		break;
	
		
		//Sequencer Start Stop button
		//because the output shift registers are full, this buttons LED is on a single uC pin
		case BUT_START_STOP:
			//toggle the led
			//send run/stop command to soundchip
			led_setValue(buttonHandler_stateMemory.seqRunning,LED_START_STOP);
			
			buttonHandler_stateMemory.seqRunning = 1-buttonHandler_stateMemory.seqRunning;
			frontPanel_sendData(SEQ_CC,SEQ_RUN_STOP,buttonHandler_stateMemory.seqRunning);

		break;
		
		case BUT_REC:
			if(buttonHandler_getShift()) {
				menu_switchPage(RECORDING_PAGE);
			} else {
				//toggle the led
				buttonHandler_stateMemory.seqRecording = 1-buttonHandler_stateMemory.seqRecording;
				led_setValue(buttonHandler_stateMemory.seqRecording,LED_REC);
				//send run/stop command sequencer
				frontPanel_sendData(SEQ_CC,SEQ_REC_ON_OFF,buttonHandler_stateMemory.seqRecording);
			}				
			break;
			
		case BUT_VOICE_1:
		case BUT_VOICE_2:
		case BUT_VOICE_3:
		case BUT_VOICE_4:
		case BUT_VOICE_5:
		case BUT_VOICE_6:
		case BUT_VOICE_7:
		
		if(copyClear_Mode >= MODE_COPY_PATTERN)
		{
			//copy mode -> voice buttons select track copy src/dst
			if(copyClear_srcSet())
			{
				//select dest
				copyClear_setDst(buttonNr-BUT_VOICE_1,MODE_COPY_TRACK);
				copyClear_copyTrack();
				led_clearAllBlinkLeds();
				//query current sequencer step states and light up the corresponding leds 
				
				uint8_t trackNr = menu_getActiveVoice(); //max 6 => 0x6 = 0b110
				uint8_t patternNr = menu_getShownPattern(); //max 7 => 0x07 = 0b111
				uint8_t value = (trackNr<<4) | (patternNr&0x7);
				frontPanel_sendData(LED_CC,LED_QUERY_SEQ_TRACK,value);
				
			}
			else
			{
				//select src
				copyClear_setSrc(buttonNr-BUT_VOICE_1,MODE_COPY_TRACK);
				led_setBlinkLed(LED_VOICE1+buttonNr-BUT_VOICE_1,1);
			}
		}
		else
		{
			
			uint8_t muteModeActive;
			muteModeActive = buttonHandler_getShift();
			if(buttonHandler_getMode() == SELECT_MODE_PERF)
			{
				//invert mute functionality
				muteModeActive = 1-muteModeActive;
			}
			
		
			if(muteModeActive)
			{
				//un/mute
				/*
					if(buttonNr == 7)
					{
						//unmute all tracks
						for(int i=0;i<8;i++)
						{
							if(buttonHandler_mutedVoices & (1<<i))
								frontPanel_sendData(SEQ_CC,SEQ_UNMUTE_TRACK,i);
						}
						buttonHandler_mutedVoices = 0;
					}
					else
					*/
					const uint8_t voice = buttonNr-BUT_VOICE_1;
					if(buttonHandler_mutedVoices & (1<<voice))
					{
						//unmute tracks 0-7	
						buttonHandler_mutedVoices &= ~(1<<voice);
						frontPanel_sendData(SEQ_CC,SEQ_UNMUTE_TRACK,voice);
					}
					else
					{
						//mute tracks 0-7
						buttonHandler_mutedVoices |= (1<<voice);
						frontPanel_sendData(SEQ_CC,SEQ_MUTE_TRACK,voice);
					}
				
					//muted tracks are lit
					//led_setMode2Leds(buttonHandler_mutedVoices);
					led_setActiveVoiceLeds(~buttonHandler_mutedVoices);
			} 
			else
			{
				//select active voice
				
				//the currently active button is lit
				led_setActiveVoice((buttonNr-BUT_VOICE_1));
			
				//change voice page on display if in voice mode
				if( (buttonHandler_getMode() == SELECT_MODE_VOICE) /*|| (buttonHandler_getMode() ==SELECT_MODE_STEP) */)
				{
					menu_switchPage(buttonNr-BUT_VOICE_1);			
				}				
				frontPanel_sendData(SEQ_CC,SEQ_SET_ACTIVE_TRACK,buttonNr-BUT_VOICE_1);
			
				menu_setActiveVoice(buttonNr-BUT_VOICE_1);
				
				
				frontPanel_sendData(SEQ_CC,SEQ_REQUEST_EUKLID_PARAMS,buttonNr-BUT_VOICE_1);
				//request the pattern info for the selected pattern (bar cnt, next...)
				//frontPanel_sendData(SEQ_CC,SEQ_REQUEST_PATTERN_PARAMS,buttonNr);
				
				if((buttonHandler_getMode() ==SELECT_MODE_STEP))
				{
					//reactivate sequencer mode
					led_clearAllBlinkLeds();
					buttonHandler_enterSeqModeStepMode();
					
				}
			}	
		}						
			
			
		break;
		
		//the sequencer buttons
		case BUT_SEQ1:
		case BUT_SEQ2:
		case BUT_SEQ3:
		case BUT_SEQ4:
		case BUT_SEQ5:
		case BUT_SEQ6:
		case BUT_SEQ7:
		case BUT_SEQ8:
		case BUT_SEQ9:
		case BUT_SEQ10:
		case BUT_SEQ11:
		case BUT_SEQ12:
		case BUT_SEQ13:
		case BUT_SEQ14:
		case BUT_SEQ15:
		case BUT_SEQ16:
		{
			
			
			
			//which button is pressed
			uint8_t seqButtonPressed;
			uint8_t ledNr;
			
			seqButtonPressed = buttonNr;
			ledNr = buttonNr+LED_STEP1;
			
			if(buttonHandler_getShift())
			{
				
				if( (buttonHandler_getMode() == SELECT_MODE_VOICE) )
				{
					//sequencer mode -> buttons select active step
					buttonHandler_selectActiveStep(ledNr, seqButtonPressed);
				} /*else if ( (buttonHandler_getMode() == SELECT_MODE_STEP) )
				{
					buttonHandler_setRemoveStep(ledNr,seqButtonPressed);
				}
				*/
				if ( (buttonHandler_getMode() == SELECT_MODE_STEP) )
				{
					
					//buttonHandler_setTimeraction(seqButtonPressed*8);
					buttonHandler_setRemoveStep(ledNr,seqButtonPressed);
					
				}					
				
			}
			else
			{
			
				switch(buttonHandler_stateMemory.selectButtonMode)
				{
					
					case SELECT_MODE_STEP:
						buttonHandler_selectActiveStep(ledNr, seqButtonPressed);
					break;
					//--- edit the pattern -> button sets and removes a step ---
					/*
					case SELECT_MODE_STEP:
						buttonHandler_setTimeraction(seqButtonPressed*8);
					break;
					*/
					case SELECT_MODE_VOICE:
					
						buttonHandler_setTimeraction(seqButtonPressed*8);
						//buttonHandler_setRemoveStep(ledNr,seqButtonPressed);
					
					break;
					
					//--- buttons 1-8 initiate a manual roll, buttons 9-16 select the pattern ---
					case SELECT_MODE_PERF:
						if(seqButtonPressed < 8 )
						{
							//turn roll on
							frontPanel_sendData(SEQ_CC,SEQ_ROLL_ON_OFF,(seqButtonPressed&0xf) + 0x10 );
							//turn button led on
							led_setValue(1,ledNr);
							
						} 
						else
						{
							/*
							//moved pattern select to select buttons
							//pattern
						
							//tell sequencer to change pattern
							frontPanel_sendData(SEQ_CC,SEQ_CHANGE_PAT,seqButtonPressed-8);
							//flash corresponding LED until ACK (SEQ_CHANGE_PAT) received
							led_setBlinkLed(ledNr,1);
							
							//request the pattern info for the selected pattern (bar cnt, next...)
							frontPanel_sendData(SEQ_CC,SEQ_REQUEST_PATTERN_PARAMS,seqButtonPressed-8);
*/
						}
					break;
	
					//--- unused (maybe lfo clock sync? ---
					case SELECT_MODE_LOAD_SAVE:
				
					break;
				
					default:
					break;
			
				}	
			}						
			
			
			
			/*
			lcd_home();
			char text[16];
			sprintf(text,"cc:%d data:%d",currentTrack+SEQ_V1_TOGGLE_STEP,seqButtonPressed);
			lcd_string(text);
			*/
		}			
		break;

		case BUT_COPY:	
			if(buttonHandler_getShift())
			{
				//with shift -> clear mode
				if(copyClear_Mode == MODE_CLEAR)
				{
					//execute
					copyClear_executeClear();
					/*
					copyClear_clearCurrentTrack();
					copyClear_armClearMenu(0);
					copyClear_Mode = MODE_NONE;
					*/
				}
				else
				{
					copyClear_Mode = MODE_CLEAR;
					copyClear_armClearMenu(1);
				}					
				
			}
			else
			{
				//copy mode
				copyClear_Mode = MODE_COPY_TRACK;
				led_setBlinkLed(LED_COPY,1);
				led_clearSelectLeds();
				led_clearVoiceLeds();
			
			}
			return;
		
		break;
		
				
		//the mode selection for the 16 seq buttons
		case BUT_SHIFT: 
		/* while this button is pressed, the SEQUENCER mode is activated
		-> change display to show selected step options (volume, probability, note etc...)
		-> seq buttons select step
		-> selected step is illuminated on the seq buttons
		-> select buttons show 8 sub steps for the selected step
		
		->shows muted steps on voice leds while pressed
		*/
		
		led_setValue(1,LED_SHIFT);
		
		if(buttonHandler_getMode() == SELECT_MODE_VOICE)
		{
			buttonHandler_enterSeqMode();
		}
		else if( (buttonHandler_getMode() == SELECT_MODE_PERF) || (buttonHandler_getMode() == SELECT_MODE_PAT_GEN)) {
		
			//blink selected pattern LED
			menu_switchPage(PATTERN_SETTINGS_PAGE);
			led_clearSelectLeds();
			led_setBlinkLed(menu_getShownPattern()+LED_PART_SELECT1,1);
		}else if(buttonHandler_getMode() == SELECT_MODE_STEP)
		{
			buttonHandler_leaveSeqModeStepMode();
		}
		
		//show muted voices if pressed
		led_setActiveVoiceLeds(~buttonHandler_mutedVoices);
		
		break;
		//the mode selection for the 8 select buttons

		
		default:
		break;
	}
	
	/*
	char text[17];
	lcd_clear();
	led_clearAll();
	sprintf(text,"but%d",buttonNr);
	lcd_string(text);
	led_setValue(1,buttonNr);
	*/
	
	
};
//--------------------------------------------------------------
void buttonHandler_toggleEuklidMode()
{
	if(menu_getActivePage() == PERFORMANCE_PAGE)
	{
		//switch to Euklid
		menu_switchPage(EUKLID_PAGE);
	}
	else
	{
		//switch back to mute mode
		menu_switchPage(PERFORMANCE_PAGE);	
	}
	
}
//--------------------------------------------------------------
uint8_t buttonHandler_getMode()
{
	return buttonHandler_stateMemory.selectButtonMode;
};
//--------------------------------------------------------------