/*
 * buttonHandler.h
 *
 * Created: 24.04.2012 16:02:07
 *  Author: Julian
 */ 


#ifndef BUTTONHANDLER_H_
#define BUTTONHANDLER_H_

#include <avr/io.h>

extern uint8_t buttonHandler_selectedStep;
extern uint16_t buttonHandler_originalParameter;	//saves parameter number for step automation reset (stgep assign)
extern uint8_t buttonHandler_originalValue; 
extern uint8_t buttonHandler_resetLock;

#define BUTTON_TIMEOUT (38) //~500[ms]
#define NO_STEP_SELECTED -1
/**indicates which of the 4 select button modes is active.
0 = voice select
select the active voice
sequencer buttons set steps

1 = mute
mute the different voices
sequencer buttons used for performance?
manual roll? pattern selection?

2 = sequencer mode
select buttons controll 8 substeps for each step
sequencer buttons display the 16 main steps and can be used to access step specific settings (probability, velocity etc)

3 = LFOs
*/
#define SELECT_MODE_VOICE		0x00
#define SELECT_MODE_PERF		0x01
#define SELECT_MODE_STEP		0x02
#define SELECT_MODE_LOAD_SAVE	0x03

//#define SELECT_MODE_VOICE_STEP	0x04
#define SELECT_MODE_PAT_GEN		0x05
#define SELECT_MODE_SOM_GEN		0x06
#define SELECT_MODE_MENU		0x07
		

enum ButtonNumbers
{
	BUT_SEQ1 =0,
	BUT_SEQ2,
	BUT_SEQ3,
	BUT_SEQ4,
	BUT_SEQ5,
	BUT_SEQ6,
	BUT_SEQ7,
	BUT_SEQ8,
	
	//the 16 sequencer buttons
	BUT_SEQ9 = 8,
	BUT_SEQ10,
	BUT_SEQ11,
	BUT_SEQ12,
	BUT_SEQ13,
	BUT_SEQ14,
	BUT_SEQ15,
	BUT_SEQ16,
	
	//the 8 selection buttons to the right
	BUT_SELECT1 = 16,
	BUT_SELECT2,
	BUT_SELECT3,
	BUT_SELECT4,
	BUT_SELECT5,
	BUT_SELECT6,
	BUT_SELECT7,
	BUT_SELECT8,
	
	BUT_VOICE_1 = 24,
	BUT_VOICE_2,
	BUT_VOICE_3,
	BUT_VOICE_4,
	BUT_VOICE_5,
	BUT_VOICE_6,
	BUT_VOICE_7,
	BUT_COPY,
	
	BUT_START_STOP = 32,
	BUT_REC,
	
	BUT_SHIFT, //shift
	
	BUT_MODE1 ,
	BUT_MODE2 ,
	BUT_MODE3 ,
	BUT_MODE4 ,

};
//--------------------------------------------------------
void buttonHandler_tick();
//--------------------------------------------------------
void buttonHandler_buttonPressed(uint8_t buttonNr);
//--------------------------------------------------------
//uint8_t buttonHandler_getMutedVoices();
//--------------------------------------------------------
void buttonHandler_buttonReleased(uint8_t buttonNr);
//--------------------------------------------------------
//void buttonHandler_toggleEuklidMode();
//--------------------------------------------------------
/** return the menu state selected by mode2 button*/
uint8_t buttonHandler_getMode();
//--------------------------------------------------------
uint8_t buttonHandler_getShift();
//--------------------------------------------------------
int8_t buttonHandler_getArmedAutomationStep();
//--------------------------------------------------------
void buttonHandler_muteVoice(uint8_t voice, uint8_t isMuted);
//--------------------------------------------------------
void buttonHandler_showMuteLEDs();
//--------------------------------------------------------
void buttonHandler_setRunStopState(uint8_t running);
#endif /* BUTTONHANDLER_H_ */
