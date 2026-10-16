/*
 * avrCommsReceivingProtocol.c
 *
 * AVR receive parser for STM responses, SysEx pattern data, restore messages,
 * and long-operation receive state. STM-bound packet construction lives in
 * avrCommsSendingProtocol.c.
 *
 * Created: 27.04.2012 12:04:00
 *  Author: Julian
 */ 
#include "avrCommsReceivingProtocol.h"
#include "avrCommsSendingProtocol.h"
#include "Menu/menu.h"
#include "Menu/copyClearTools.h"
#include <stdio.h>

#include "ledHandler.h"
#include "buttonHandler.h"
#include "front.h"
//debug
#include <stdlib.h>
#include "Hardware/lcd.h"
#include "Preset/PresetManager.h"
#include <util/delay.h>
//--

static uint8_t avrCommsParser_rxCnt=0;
volatile MidiMsg avrCommsParser_command;
static uint16_t avrCommsParser_nrpnNr = 0;

volatile uint8_t avrCommsParser_restoreActive = 0; // RESTORE: True during a canonical parameter dump from STM
/* Sample import completion latch. The load menu clears this before sending
   SAMPLE_START_UPLOAD; the UART parser sets it only when the STM sends the
   explicit RESULT packet. This avoids the old first-load race with stale bytes
   and raw ACK waiting. */
volatile uint8_t avrComms_sampleUploadDone = 0;
volatile uint8_t avrComms_sampleUploadStatus = 0;
static uint16_t avrCommsParser_restoreCount = 0;
static uint16_t avrCommsParser_restoreMorphCount = 0;
static uint8_t avrCommsParser_reportGlobalMorphLsb = 0;
static uint8_t avrCommsParser_reportVoiceMorphLsb[6] = {0};
// DEBUG

uint8_t avrCommsParser_isRestoreActive(void)
{
   return avrCommsParser_restoreActive;
}

static void avrCommsParser_syncVoiceMorphDisplayValues(uint8_t amount)
{
   uint8_t voice;

   for(voice=0;voice<6;voice++)
      parameter_values[PAR_MORPH_DRUM1 + voice] = amount;
}

static uint8_t avrCommsParser_isVoiceMorphDisplayParam(uint16_t paramNr)
{
   return paramNr >= PAR_MORPH_DRUM1 && paramNr <= PAR_MORPH_HIHAT;
}

static void avrCommsParser_handleVoiceMorphReport(uint8_t slot, uint8_t payload)
{
   uint8_t voice;

   if(slot < 6)
   {
      avrCommsParser_reportVoiceMorphLsb[slot] = (uint8_t)(payload & 0x7f);
      return;
   }

   if(slot < 12)
   {
      uint8_t amount;

      voice = (uint8_t)(slot - 6);
      amount = (uint8_t)(avrCommsParser_reportVoiceMorphLsb[voice]
         | ((payload & 0x01) << 7));
      parameter_values[PAR_MORPH_DRUM1 + voice] = amount;
      menu_repaint();
   }
}

uint8_t avrComms_sysexMode = 0;
uint8_t avrCommsParser_sysexCallback = 0;
uint8_t avrCommsParser_rxDisable=0;

volatile uint8_t avrCommsParser_newSeqDataAvailable = 0;
volatile StepData avrCommsParser_stepData;
//sysex buffer used to store 7 byte sysex chunks
volatile uint8_t avrCommsParser_sysexBuffer[7];

uint8_t avrCommsParser_nameIndex = 0;
uint8_t avrComms_longOp;
uint8_t avrComms_morphArray;
uint8_t avrComms_morphAvail=0;

// case definitions for long ops that get dealt with once per main() loop
// bitwise definitions to decide what to do for multiple ops
#define BANK_1 0x01
#define BANK_2 0x02
#define BANK_3 0x04
#define BANK_4 0x08
#define BANK_5 0x10
#define BANK_6 0x20
#define BANK_7 0x40
#define BANK_GLOBAL 0x80
// banks 1-6 plus global stack to allow for multiple voices stacked on the same
// MIDI channel to respond to the same bank change command

// above BANK_GLOBAL it doesn't matter - we reset the command anyway
#define MORPH_OP 0x81
// there is space in here to add more long operations - pattern change
// must have the highest priority
#define PATTERN_CHANGE_OP 0xAF
#define NULL_OP 0x00
uint8_t avrComms_longData;

//------------------------------------------------------------
#define NRPN_MUTE_1 1000
#define NRPN_MUTE_7 1006
//------------------------------------------------------------
void avrCommsParser_parseNrpn(uint8_t value)
{
   uint16_t paramNr=avrCommsParser_nrpnNr+128;

   if(paramNr < NUM_PARAMS)
      parameter_values[paramNr] = value;
	
   if( (paramNr >= PAR_TARGET_LFO1) && (paramNr <= PAR_TARGET_LFO6) )
   {
   	//**LFO receive nrpn translate --AS TODO this needs to be checked. I'm not sure what needs to happen here.
   	// It seems like the assumption is that in this case, value represents an encoded menupage value (this code
   	// used to call the now defunct getModTargetValue)
   
   	//LFO
      uint8_t lfoNr = (uint8_t)(paramNr-PAR_TARGET_LFO1);
      if(lfoNr>5)lfoNr=5;
   
   	// value (might) represents an actual parameter number, we need to convert to index into modTargets
      parameter_values[paramNr]=paramToModTarget[value];
   	// this was the old code
   	//since the LFO target calculation from the index number needs to know about the menu structure (menuPages)
   	//we need to send back the right target param number to the cortex
   	//value = getModTargetValue(parameter_values[avrCommsParser_nrpnNr+128],
   	//		(uint8_t)(parameter_values[PAR_VOICE_LFO1+lfoNr]-1));
   
      uint8_t upper = (uint8_t)(((value&0x80)>>7) | (((lfoNr)&0x3f)<<1));
      uint8_t lower = value&0x7f;
      avrComms_sendData(CC_LFO_TARGET,upper,lower);
   }
   else if ( (paramNr >= PAR_VEL_DEST_1) && (paramNr <= PAR_VEL_DEST_6) )
   {
   	//**VELO receive nrpn translate to parameter. --AS TODO this needs to be checked as well
   
   	// value (might) represents an actual parameter number, we need to convert to index into modTargets
      parameter_values[paramNr]=paramToModTarget[value];
   
   	// old code
   	//uint8_t param = parameter_values[avrCommsParser_nrpnNr+128];
   	//if(param > (NUM_SUB_PAGES * 8 -1))
   	//param = (NUM_SUB_PAGES * 8 -1);
   	//uint8_t value = getModTargetValue(param, (uint8_t)(avrCommsParser_nrpnNr+128 - PAR_VEL_DEST_1));
   			
      uint8_t upper,lower;
      upper = (uint8_t)((uint16_t)((value&0x80)>>7) | (((paramNr-PAR_VEL_DEST_1)&0x3f)<<1));
      lower = value&0x7f;
      avrComms_sendData(CC_VELO_TARGET,upper,lower);
   
   } 
   else if ( (avrCommsParser_nrpnNr >= NRPN_MUTE_1) && (avrCommsParser_nrpnNr <= NRPN_MUTE_7) )
   {
      const uint8_t voice = (uint8_t)(avrCommsParser_nrpnNr - NRPN_MUTE_1);
      const uint8_t onOff = value;
      buttonHandler_muteVoice(voice,onOff);
   	
   }
}
//------------------------------------------------------------
void avrCommsPanelParser_ccHandler(void)
{
	//get the real parameter number from the cc number
   const uint8_t parNr =(uint8_t)( avrCommsParser_command.data1 - 1);
	
   if(parNr == NRPN_DATA_ENTRY_COARSE) {
      avrCommsParser_parseNrpn(avrCommsParser_command.data2);
   }
	
   if(parNr == NRPN_FINE) {
      avrCommsParser_nrpnNr &= 0x80; //clear lower 7 bit
      avrCommsParser_nrpnNr |= avrCommsParser_command.data2;
   }
		
   if(parNr == NRPN_COARSE) {
      avrCommsParser_nrpnNr &= 0x7f; //clear upper 7 bit
      avrCommsParser_nrpnNr |= (uint16_t)(avrCommsParser_command.data2<<7);
   }
	
	
	//set the parameter value
   parameter_values[parNr] = avrCommsParser_command.data2;
	
	//repaint the LCD
   menu_repaint();
	
}
//------------------------------------------------------------
void avrComms_parseData(uint8_t data)
{
	// if high byte set a new message starts
   if(data&0x80)
   {	
      if(data==PATCH_RESET)
      {
         /* PATCH_RESET now belongs to the AVR->STM reload path. If a legacy
            sender still emits it back toward AVR, do not restore the local
            file-load snapshots here; that behavior is retired. */
      }
      else if(data==CALLBACK_ACK)
      {
         avrCommsSending_handleCallbackAck();
      }
      else
      {
      	//reset the byte counter
         avrCommsParser_rxCnt = 0;
         avrCommsParser_command.status = data;
      }
   
   }
   else
   {
      if(avrCommsParser_command.status == SYSEX_START)
      {
       
         switch(avrComms_sysexMode)
         {
            case SYSEX_BEGIN_PATTERN_TRANSMIT:
               {
                  if(data==SYSEX_BEGIN_PATTERN_TRANSMIT)
                  {
                     avrCommsParser_sysexCallback=STEP_CALLBACK;
                  }
                  else if(data==SYSEX_STEP_ACK)
                  {
                     avrCommsParser_sysexCallback=STEP_ACK;
                  }
               }
               break;
            case SYSEX_SEND_MAIN_STEP_DATA:
               {
                  if(data==SYSEX_SEND_MAIN_STEP_DATA)
                  
                  {
                     avrCommsParser_sysexCallback=MAINSTEP_CALLBACK;
                  }
               }
               break;
            case SYSEX_SEND_PAT_CHAIN_DATA:
               {
                  if(data==SYSEX_SEND_PAT_CHAIN_DATA)
                  
                  {
                     avrCommsParser_sysexCallback=PATCHAIN_CALLBACK;
                  }
               }
               break;
         
            case SYSEX_SEND_PAT_LEN_DATA:
               {
                  if(data==SYSEX_SEND_PAT_LEN_DATA)
                  
                  {
                     avrCommsParser_sysexCallback=LENGTH_CALLBACK;
                  }
               }
               break;
            case SYSEX_SEND_PAT_SCALE_DATA:
               {
                  if(data==SYSEX_SEND_PAT_SCALE_DATA)
                  {
                     avrCommsParser_sysexCallback=SCALE_CALLBACK;
                  }
               }
               break;
            case SYSEX_REQUEST_STEP_DATA:
               {
               //we are expecting step packages send from the sequencer
               //1st 7 lower nibble 7 bit messages
               //then an upper nibble 7 bit message containing the missing 7 upper bits
               //char text[5];
                  if(avrCommsParser_rxCnt<7)
                  {
                     avrCommsParser_sysexBuffer[avrCommsParser_rxCnt++] = data;
                  //	lcd_setcursor(5,2);
                  //	itoa(avrCommsParser_rxCnt,text,10);
                  //	lcd_string(text);
                  
                  }
                  else
                  {
                  //now we have to distribute the MSBs to the sysex data
                     for(int i=0;i<7;i++)
                     {
                     
                        avrCommsParser_sysexBuffer[i] |= (uint8_t)((data&(1<<i))<<(7-i));
                     
                     
                     }
                     avrCommsParser_stepData.volume = avrCommsParser_sysexBuffer[0];
                     avrCommsParser_stepData.prob = avrCommsParser_sysexBuffer[1];
                     avrCommsParser_stepData.note = avrCommsParser_sysexBuffer[2];
                     avrCommsParser_stepData.param1Nr = avrCommsParser_sysexBuffer[3];
                     avrCommsParser_stepData.param1Val = avrCommsParser_sysexBuffer[4];
                     avrCommsParser_stepData.param2Nr = avrCommsParser_sysexBuffer[5];
                     avrCommsParser_stepData.param2Val = avrCommsParser_sysexBuffer[6];
                  
                  //signal that a ne data chunk is available
                     avrCommsParser_newSeqDataAvailable = 1;
                  //reset receive counter for next chunk
                     avrCommsParser_rxCnt = 0;
                  
                  //lcd_setcursor(5,2);
                  //lcd_string("complete");
                  }					
               
               }
               break;
            case SYSEX_REQUEST_PATTERN_DATA:
               {
                  if(avrCommsParser_rxCnt<1)
                  {
                  //1st byte
                     avrCommsParser_sysexBuffer[avrCommsParser_rxCnt++] = data;
                  } 
                  else {
                  //2nd byte
                     avrCommsParser_sysexBuffer[avrCommsParser_rxCnt++] = data;
                  
                     uint8_t next = avrCommsParser_sysexBuffer[0];
                     uint8_t repeat = avrCommsParser_sysexBuffer[1];
                  //we abuse the stepData struct to store the pattern data
                     avrCommsParser_stepData.volume = next;
                     avrCommsParser_stepData.prob = repeat;
                  
                  //signal that a new data chunk is available
                     avrCommsParser_newSeqDataAvailable = 1;
                  //reset receive counter for next chunk
                     avrCommsParser_rxCnt = 0;
                  }
               
               }
               break;
            case SYSEX_REQUEST_MAIN_STEP_DATA:
               {
                  if(avrCommsParser_rxCnt<4)
                  {
                  //1st 2 nibbles + last 2 bit
                     avrCommsParser_sysexBuffer[avrCommsParser_rxCnt++] = data;             
                  } 
                  else {
                  // scale information
                     avrCommsParser_sysexBuffer[avrCommsParser_rxCnt++] = data;               
                  
                  // package the main step data so we know what to do with
                  
                     uint16_t mainStepData = avrCommsParser_sysexBuffer[0] |
                     (uint16_t)(avrCommsParser_sysexBuffer[1]<<7) |
                     (uint16_t)(avrCommsParser_sysexBuffer[2]<<14);
                  //we abuse the stepData struct to store the main step data and the scale
                     avrCommsParser_stepData.volume = (uint8_t)(mainStepData>>8);
                     avrCommsParser_stepData.prob = (uint8_t)(mainStepData&0xff);
                     avrCommsParser_stepData.note = avrCommsParser_sysexBuffer[3]; // this is the length data from uart
                     avrCommsParser_stepData.param1Nr = avrCommsParser_sysexBuffer[4]; // this is the scale data from uart
                  
                  
                  //signal that a new data chunk is available
                     avrCommsParser_newSeqDataAvailable = 1;
                  //reset receive counter for next chunk
                     avrCommsParser_rxCnt = 0;
                  }
               }
               break;  
            default:
               break;
         }
      }
      else if(avrCommsParser_rxCnt==0)
      {
         if(!avrCommsParser_rxDisable
            || (avrCommsParser_command.status == SEQ_CC)
            //|| (avrCommsParser_command.status == PRF_CACHE_STATUS)
            || (avrCommsParser_command.status == PARAM_RESTORE_BEGIN)
            || (avrCommsParser_command.status == PARAM_RESTORE_DONE)
            || (avrCommsParser_command.status == PRF_RESTORE_PARAM_CC)
            || (avrCommsParser_command.status == PRF_RESTORE_PARAM_CC2)
            || (avrCommsParser_command.status == PRF_RESTORE_MORPH_CC)
            || (avrCommsParser_command.status == PRF_RESTORE_MORPH_CC2))
         {
      	   //parameter nr
            avrCommsParser_command.data1 = data;
            avrCommsParser_rxCnt++;
         }
      }
      else
      {
      	//parameter value
         avrCommsParser_command.data2 = data;
         avrCommsParser_rxCnt=0;

         if(avrCommsParser_rxDisable)
         {
            if((avrCommsParser_command.status == SEQ_CC) && avrCommsSending_isFlowCommand(avrCommsParser_command.data1))
               avrCommsSending_handleFlowMessage(avrCommsParser_command.data1, avrCommsParser_command.data2);
#if 0
//#if 0
            //else if(avrCommsParser_command.status == PRF_CACHE_STATUS)
            //{
            //   avrCommsSending_handlePrfCacheStatus(avrCommsParser_command.data1, avrCommsParser_command.data2);
            //}
//#endif
#endif
            else if((avrCommsParser_command.status == PARAM_RESTORE_BEGIN)
               || (avrCommsParser_command.status == PARAM_RESTORE_DONE)
               || (avrCommsParser_command.status == PRF_RESTORE_PARAM_CC)
               || (avrCommsParser_command.status == PRF_RESTORE_PARAM_CC2)
               || (avrCommsParser_command.status == PRF_RESTORE_MORPH_CC)
               || (avrCommsParser_command.status == PRF_RESTORE_MORPH_CC2))
            {
               /* RESTORE: Allow these messages to fall through to the processor even when rxDisable is true. */
            }
            else if((avrCommsParser_command.status == SEQ_CC)
               && (avrCommsParser_command.data1 == SEQ_BACKGROUND_SWAP_DONE))
            {
               /* Background-load acknowledge: allow processing while file-load rxDisable is true. */
            }
            else
            {
               return;
            }
         }
      	//process the received data
         if(avrCommsParser_command.status == MIDI_CC) //sound parameter command from STM 
         {
            avrCommsPanelParser_ccHandler();
         }
         else
         {
            if(avrCommsParser_command.status == PRESET_NAME)
            {
            	
               if(avrCommsParser_command.data2 & 0x40)
               {
                  avrCommsParser_nameIndex = 0;
               }
            	
               preset_currentName[avrCommsParser_nameIndex] =(char)(
                  	(avrCommsParser_command.data1&0x7f) |
                  	((avrCommsParser_command.data2&0x7f)<<7));
               avrCommsParser_nameIndex++;
               avrCommsParser_nameIndex &= 0x7; //wrap at 8
               if(avrCommsParser_nameIndex==0)
               {
                  menu_repaintAll();
               }						
            					
            } 
            else if(avrCommsParser_command.status == SET_P1_DEST)
            {
            	//**AUTOM - translate cortex value to mod target index
            	// a value of FF means no automation (on the back end)
               uint8_t dst=(uint8_t)((avrCommsParser_command.data1<<7) | avrCommsParser_command.data2);
               if(dst==0xFF)
                  dst=0;
               parameter_values[PAR_P1_DEST] = paramToModTarget[dst];
               menu_repaintAll();
            }
            else if(avrCommsParser_command.status == SET_P2_DEST)
            {
            	//**AUTOM - translate cortex value to mod target index
               uint8_t dst=(uint8_t)((avrCommsParser_command.data1<<7) | avrCommsParser_command.data2);
               if(dst==0xFF)
                  dst=0;
               parameter_values[PAR_P2_DEST] = paramToModTarget[dst];
               menu_repaintAll();
            }
            else if(avrCommsParser_command.status == SET_P1_VAL)
            {
               parameter_values[PAR_P1_VAL] = (uint8_t)((avrCommsParser_command.data1<<7) | avrCommsParser_command.data2);
               menu_repaintAll();
            }
            else if(avrCommsParser_command.status == SET_P2_VAL)
            {
               parameter_values[PAR_P2_VAL] = (uint8_t)((avrCommsParser_command.data1<<7) | avrCommsParser_command.data2);
               menu_repaintAll();
            }
            else if(avrCommsParser_command.status == SEQ_CC)
            {
               switch(avrCommsParser_command.data1)
               {
               	
                  case SEQ_FLOW_BEGIN:
                  case SEQ_FLOW_GRANT:
                  case SEQ_FLOW_END:
                  case SEQ_FLOW_ABORT:
                     avrCommsSending_handleFlowMessage(avrCommsParser_command.data1, avrCommsParser_command.data2);
                     break;

                  case SEQ_BACKGROUND_SWAP_DONE:
                     preset_backgroundSwapDoneFromStm(avrCommsParser_command.data2);
                     buttonHandler_refreshTempPlaybackLedHint();
                     break;

                  case SEQ_REPORT_GLOBAL_MORPH_LSB:
                     avrCommsParser_reportGlobalMorphLsb =
                        (uint8_t)(avrCommsParser_command.data2 & 0x7f);
                     break;

                  case SEQ_REPORT_GLOBAL_MORPH_MSB:
                  {
                     uint8_t amount =
                        (uint8_t)(avrCommsParser_reportGlobalMorphLsb
                           | ((avrCommsParser_command.data2 & 0x01) << 7));
                     parameter_values[PAR_MORPH] = amount;
                     morphValue = amount;
                     avrCommsParser_syncVoiceMorphDisplayValues(amount);
                     menu_repaint();
                     break;
                  }

                  /* PERF ROLL DISPLAY REPORTS, STM -> AVR  (Session 039)
                     WHAT: STM reports a roll value changed by external MIDI
                           (Global NRPN 93 rate 0..15, 94 note, 95 velocity);
                           store it in the PERF page slot and repaint.
                     WHY:  keeps PAR_ROLL/PAR_ROLL_NOTE/PAR_ROLL_VELOCITY
                           equal to the STM value, so a later encoder/pot move
                           starts from it instead of resending a stale value.
                           Display-only: no echo back to STM (STM already
                           holds the value), and these runtime-only params are
                           never saved, so no file/snapshot state is touched.
                     INPUT:  avrCommsParser_command.data2 - new value.
                     OUTPUT: parameter_values[PAR_ROLL*] updated; menu_repaint().
                     ACCESSORS: this parser only.
                     AFFILIATES: STM frontPanelSending_sendRollParamReport(),
                                 globalMidiParser_applySpecialNrpn(); menu.c
                                 PAR_ROLL/_NOTE/_VELOCITY send cases (AVR->STM). */
                  case SEQ_REPORT_ROLL_RATE:
                     parameter_values[PAR_ROLL] = avrCommsParser_command.data2;
                     menu_repaint();
                     break;

                  case SEQ_REPORT_ROLL_NOTE:
                     parameter_values[PAR_ROLL_NOTE] = avrCommsParser_command.data2;
                     menu_repaint();
                     break;

                  case SEQ_REPORT_ROLL_VELOCITY:
                     parameter_values[PAR_ROLL_VELOCITY] = avrCommsParser_command.data2;
                     menu_repaint();
                     break;

                  case SEQ_SET_PAT_BEAT:
                     parameter_values[PAR_PATTERN_BEAT] = avrCommsParser_command.data2;
                     menu_repaint();
                     break;	
                  case SEQ_SET_PAT_NEXT:
                     parameter_values[PAR_PATTERN_NEXT] = avrCommsParser_command.data2;
                     menu_repaint();
                     break;
               	
                  case SEQ_TRACK_LENGTH:
                     parameter_values[PAR_TRACK_LENGTH] = avrCommsParser_command.data2;
                     menu_repaint();
                     break;
                  case SEQ_TRACK_SCALE:
                     parameter_values[PAR_TRACK_SCALE] = avrCommsParser_command.data2;
                     menu_repaint();
                     break;
               
               	// **PATROT - receive rotation value from back for active track
                 
                  case SEQ_TRACK_ROTATION:
                     parameter_values[PAR_TRACK_ROTATION] = avrCommsParser_command.data2;
                     menu_repaint();
                     /* Rotation LED refresh should follow the effective
                        shifted view. In global shift-toggle mode the physical
                        button may be released while the shifted PERF page is
                        still active. */
                     if ((buttonHandler_getMode() == SELECT_MODE_PERF)&&buttonHandler_getShift())
                     {  // rotation amount updated while viewing rotation - update the display
                        led_clearAllBlinkLeds();
                        led_setBlinkLed((uint8_t) (LED_STEP1 + parameter_values[PAR_TRACK_ROTATION]), 1);
                        led_setBlinkLed((uint8_t) (LED_VOICE1 + menu_getActiveVoice()), 1);
                        uint8_t viewedPattern = menu_getViewedPattern();
                        led_setBlinkLed((uint8_t)((viewedPattern == SEQ_TMP_PATTERN) ? LED_STEP16 : (LED_PART_SELECT1 + viewedPattern)), 1);
                        
                     }
                     break;
                     
                  case SEQ_TRANSPOSE:
                     parameter_values[PAR_TRANSPOSE] = avrCommsParser_command.data2;
                     menu_repaint();
                     break;
               	
                  case SEQ_EUKLID_LENGTH:
                     parameter_values[PAR_EUKLID_LENGTH] = avrCommsParser_command.data2;
                     menu_repaint();
                     break;
               	
                  case SEQ_EUKLID_STEPS:
                     parameter_values[PAR_EUKLID_STEPS] = avrCommsParser_command.data2;
                     menu_repaint();
                     break;
               	
                  case SEQ_EUKLID_ROTATION:
                     parameter_values[PAR_EUKLID_ROTATION] = avrCommsParser_command.data2;
                     menu_repaint();
                     break;
                  
                  case SEQ_EUKLID_SUBSTEP_ROTATION:
                     parameter_values[PAR_EUKLID_SUBSTEP_ROTATION] = avrCommsParser_command.data2;
                     menu_repaint();
                     break;
               
                  case SEQ_VOLUME:
                     parameter_values[PAR_STEP_VOLUME] = avrCommsParser_command.data2;
                     menu_repaintAll();
                     break;
               	
                  case SEQ_PROB:
                     parameter_values[PAR_STEP_PROB] = avrCommsParser_command.data2;
                     menu_repaintAll();
                     break;
               	
                  case SEQ_NOTE:
                     parameter_values[PAR_STEP_NOTE] = avrCommsParser_command.data2;
                     menu_repaintAll();
                     break;
               	
               	/*
               	case SEQ_GET_ACTIVE_PAT:
               	//only in mute mode relevant
               	//led_clearSequencerLeds9_16();
               	led_clearSelectLeds();
               	// set led to show active pattern
               	led_setValue(1,LED_PART_SELECT1+avrCommsParser_command.data2);
               	break;
               	*/
               	
                  case SEQ_CHANGE_PAT:
                     if(avrCommsParser_command.data2 > 15) 
                        return;
                 	//ack message that the sequencer changed to the requested pattern
                     uint8_t patMsg = avrCommsParser_command.data2;
                     uint8_t hadHeldVoiceAck = preset_workingVoiceArray ? 1 : 0;
                     uint8_t oldPlayedPattern = menu_playedPattern;
                     if(patMsg != SEQ_TMP_PATTERN)
                        patMsg &= 0x07;
                     uint8_t tempBoundaryAck =
                        (uint8_t)((oldPlayedPattern == SEQ_TMP_PATTERN)
                                  != (patMsg == SEQ_TMP_PATTERN));
                 	// Post-morph-move, STM owns the loaded kit images. Do not
                 	// replay AVR-side held voice loads on the first pattern ack.
                     if(hadHeldVoiceAck)
                     {
                        preset_workingVoiceArray = 0;
                     }
                     led_setBlinkLed((uint8_t)((patMsg == SEQ_TMP_PATTERN) ? LED_STEP16 : (LED_PART_SELECT1+patMsg)),0);
                  	//clear last pattern led
                     menu_playedPattern = patMsg;
                     preset_notePlayedPatternChanged(patMsg);
                  	
                     if(parameter_values[PAR_FOLLOW] || tempBoundaryAck) {

                        if( menu_activePage != PATTERN_SETTINGS_PAGE)
                        {
                           menu_setShownPattern(patMsg);
                           led_clearSequencerLeds();
                        	//query current sequencer step states and light up the corresponding leds
                           avrComms_updatePatternLeds();
                           avrComms_sendData(SEQ_CC,SEQ_REQUEST_PATTERN_PARAMS,patMsg);
                        }
                        else {
                        /* Session 036 fix (PROBABILITY_INVESTIGATION.md Part 5/6/7).
                           WHY: this branch runs while the front panel is on
                           PATTERN_SETTINGS_PAGE -- the page entered by
                           holding SHIFT while already on the PERF page
                           (menu_shiftPerf(), menu.c), which repurposes the
                           step LEDs to show the active track's rotation
                           value. This is NOT the separate EUKLID_PAGE /
                           SELECT_MODE_PAT_GEN Euclid generator page (entered
                           by holding SHIFT *then* pressing PERF via
                           menu_enterPatgenMode()); do not conflate the two,
                           see MEMORY.md's existing note on this exact mix-up.
                           The old code updated only the LOCAL menu_shownPattern so this
                           page's repurposed step LEDs stay undisturbed, but it
                           never told the STM the shown/edit pattern changed.
                           The comment here used to say the update was "for
                           shift button release handler" to pick up later, but
                           no such flush ever existed (menu_shiftPerf(0) at
                           menu.c never called menu_setShownPattern()), so the
                           STM's frontParser_shownPattern (uARTFrontSYX/
                           frontPanelReceivingProtocol.c) could stay pointed at
                           a stale pattern -- including SEQ_TMP_PATTERN during
                           a background load -- with no bounded-time recovery.
                           Any step edit made after that (FRONT_SEQ_PROB /
                           VOLUME / NOTE / FRONT_SET_P1_VAL / P2_VAL) would
                           silently land on the wrong pattern, or in the
                           scratch pat_tmpPattern buffer that Save's
                           preset_writePatternData() loop never visits, and
                           would never reach any saved pattern.
                           WHAT: call menu_setShownPattern(patMsg) instead of
                           assigning menu_shownPattern directly. That helper
                           only sets menu_shownPattern and sends
                           SEQ_SET_SHOWN_PATTERN over UART -- it has no LED
                           side effects of its own, so this page's rotation-
                           indicator LEDs (set by menu_shiftPerf(1)) are still
                           left untouched, exactly as before. This also fixes
                           a small pre-existing inconsistency: the old code
                           stored the raw avrCommsParser_command.data2, while
                           this branch (and the sibling "if" branch above) both
                           now use the already temp-sentinel-normalized patMsg.
                           INPUT: patMsg (uint8_t) -- the normalized pattern
                           index (0-7) or SEQ_TMP_PATTERN that this ACK just
                           confirmed as the sequencer's active pattern.
                           OUTPUT: menu_shownPattern updated locally (same as
                           before) and SEQ_SET_SHOWN_PATTERN sent to the STM,
                           so frontParser_shownPattern tracks the real active
                           pattern even while this page is displayed.
                           AFFILIATES: STM receiver
                           uARTFrontSYX/frontPanelReceivingProtocol.c
                           (FRONT_SEQ_SET_SHOWN_PATTERN, PatternData.c
                           pat_getStepPtr()); see PROBABILITY_INVESTIGATION.md
                           for the full investigation and risk assessment. */
                           menu_setShownPattern(patMsg);
                        }
                     }
                     if(hadHeldVoiceAck && (parameter_values[PAR_FOLLOW] || tempBoundaryAck) && (menu_activePage != PATTERN_SETTINGS_PAGE))
                     {
                        led_clearSequencerLeds();
                        avrComms_updatePatternLeds();
                     }
                  	
                     if( (buttonHandler_getMode() == SELECT_MODE_PERF) || (buttonHandler_getMode() == SELECT_MODE_PAT_GEN) )
                     {
                     	//only show pattern changes when in performance mode
                     		
                     	//led_clearSequencerLeds9_16();
                        led_clearSelectLeds();
                        led_clearAllBlinkLeds();
                     	// re init the LEDs shwoing active/viewed pattern
                        led_initPerformanceLeds();
                     	//led_setValue(1,LED_PART_SELECT1+avrCommsParser_command.data2);
                     }

                     buttonHandler_refreshTempPlaybackLedHint();
                     
                     break;
                  case SEQ_RUN_STOP:
                 	// --AS This tells the front that the sequencer has started/stopped due to MTC msg
                  	// we simply use this to turn on/off the led and cause the next press of start
                 	// button to act properly
                     buttonHandler_setRunStopState(avrCommsParser_command.data2);
                     break;
               	
                  case LED_QUERY_SEQ_TRACK:
                  //this message is only send by the frontpanel, so it doesnt need to handle it
                     break;
               
               
               };						
            }
            else if(avrCommsParser_command.status == SAMPLE_CC)
            {
               switch(avrCommsParser_command.data1)
               {
                  case SAMPLE_COUNT:
                     menu_setNumSamples(avrCommsParser_command.data2);
                     break;

                  case SAMPLE_UPLOAD_RESULT:
                     /* Store flags before done so the foreground wait loop
                        cannot observe completion with stale status. */
                     avrComms_sampleUploadStatus = avrCommsParser_command.data2;
                     avrComms_sampleUploadDone = 1u;
                     break;

                  case SAMPLE_UPLOAD_SAMPLE_PROGRESS:
                     /* STM sends one-based user-visible counts; the menu does
                        not infer progress from local directory state. */
                     menu_showSampleUploadProgress(0u, avrCommsParser_command.data2);
                     break;

                  case SAMPLE_UPLOAD_LOOP_PROGRESS:
                     /* Loop progress is separate so the LCD can switch wording
                        from "Sample upload" to "Loop upload" during pass two. */
                     menu_showSampleUploadProgress(1u, avrCommsParser_command.data2);
                     break;
               
                  default:
                     break;
               }
            }
            
            /* -bc- additions to front interpreter start here*/
            //-------------------------------------------------
            /* RESTORE PUSH-UP PROCESS (AVR Side)
               This mechanism handles the incoming parameter dump from STM32.

               Handshake Logic:
               1. AVR receives PARAM_RESTORE_BEGIN.
                  - Sets avrCommsParser_restoreActive = 1.
                  - Sends PARAM_RESTORE_READY back to STM.
               2. While avrCommsParser_restoreActive is 1, avrComms_sendData() suppresses all 
                  outbound parameter traffic to prevent feedback loops (where restored display 
                  values are misinterpreted as user edits and sent back to STM).
               3. AVR receives PRF_RESTORE_PARAM_CC/CC2 messages and updates parameter_values[].
                  - Crucially, it does NOT update parameters2[] (morph parameter endpoint) to avoid
                    corrupting the morph state during a display-only synchronization.
               4. AVR receives PARAM_RESTORE_DONE.
                  - Calls menu_repaintAll() to update the display.
                  - Sends PARAM_RESTORE_ACK back to STM.
                  - Clears avrCommsParser_restoreActive = 0.
            */
            else if(avrCommsParser_command.status == PARAM_CC)
            {
               parameter_values[avrCommsParser_command.data1]=avrCommsParser_command.data2;
               parameters2[avrCommsParser_command.data1]=avrCommsParser_command.data2;
               menu_repaint();
            
            }
            
            else if(avrCommsParser_command.status == PARAM_CC2)
            {
               parameter_values[avrCommsParser_command.data1+128]=avrCommsParser_command.data2;
               parameters2[avrCommsParser_command.data1+128]=avrCommsParser_command.data2;
               menu_repaint();
            
            }
            else if(avrCommsParser_command.status == PARAM_RESTORE_BEGIN)
            {
               // RESTORE: Start of a canonical parameter dump from STM.
               // lcd_setcursor(0,0);
               // lcd_string_F(PSTR("RESTORE BEGIN   "));
               avrCommsParser_restoreActive = 1;
               avrCommsParser_restoreCount = 0;
               // Inform STM we are ready to receive and have suppressed outbound traffic.
               avrComms_sendData(PARAM_RESTORE_READY, 0, 0);
            }
            else if(avrCommsParser_command.status == PRF_RESTORE_PARAM_CC)
            {
               // RESTORE: Update main parameters only. Do not touch parameters2 (morph parameter endpoint)
               // to avoid corrupting morph state during a display-only synchronization.
               if(!avrCommsParser_rxDisable
                  || !avrCommsParser_isVoiceMorphDisplayParam(avrCommsParser_command.data1))
                  parameter_values[avrCommsParser_command.data1]=avrCommsParser_command.data2;
               avrCommsParser_restoreCount++;
            }
            else if(avrCommsParser_command.status == PRF_RESTORE_PARAM_CC2)
            {
               uint16_t paramNr = (uint16_t)(avrCommsParser_command.data1+128);
               if(paramNr < NUM_PARAMS)
               {
                  if(!avrCommsParser_rxDisable
                     || !avrCommsParser_isVoiceMorphDisplayParam(paramNr))
                     parameter_values[paramNr]=avrCommsParser_command.data2;
                  avrCommsParser_restoreCount++;
                  // RESTORE: Parameters2 mirroring explicitly removed for restore dumps.
               }
            }
            else if(avrCommsParser_command.status == PRF_RESTORE_MORPH_CC)
            {
               // RESTORE: Morph-specific restore messages still update parameters2.
               if(avrCommsParser_command.data1 < END_OF_SOUND_PARAMETERS)
               {
                  parameters2[avrCommsParser_command.data1]=avrCommsParser_command.data2;
                  avrCommsParser_restoreCount++;
                  avrCommsParser_restoreMorphCount++;
               }
            }
            else if(avrCommsParser_command.status == PRF_RESTORE_MORPH_CC2)
            {
               uint16_t paramNr = (uint16_t)(avrCommsParser_command.data1+128);
               if(paramNr < END_OF_SOUND_PARAMETERS)
               {
                  parameters2[paramNr]=avrCommsParser_command.data2;
                  avrCommsParser_restoreCount++;
                  avrCommsParser_restoreMorphCount++;
               }
            }
            else if(avrCommsParser_command.status == PARAM_RESTORE_DONE)
            {
               // RESTORE: End of dump. Repaint the full menu to reflect new values.
               // char text[17];
               // sprintf(text, "M%d D%d       ", avrCommsParser_restoreMorphCount, avrCommsParser_restoreCount);
               // lcd_setcursor(0,0);
               // lcd_string(text);

               avrCommsParser_restoreActive = 0;

               // don't repaint during sessions that hold the panel state like file loads
               if(!avrCommsParser_rxDisable)
                  menu_repaintAll();
               // Inform STM we have finished and re-enabled normal operation.
               avrComms_sendData(PARAM_RESTORE_ACK, 0, 0);
            }
            //else if(avrCommsParser_command.status == PRF_CACHE_STATUS)
            //{
            //   avrCommsSending_handlePrfCacheStatus(avrCommsParser_command.data1, avrCommsParser_command.data2);
            //}
            
            else if(avrCommsParser_command.status == BANK_CHANGE_CC)
            {
               if (avrCommsParser_command.data1&&(avrComms_longOp<PATTERN_CHANGE_OP) )
               {
                  // we have a valid message and we're not waiting for a pattern change to finish
                  // all bits in data1 are set - global bank change operation
                  if ( avrCommsParser_command.data1==0x7f ) 
                  // global bank change request or, all voice channels set the same
                  {
                     // this is a time-consuming operation, cache it and deal
                     // with only one per loop of main()
                     avrComms_longOp=BANK_GLOBAL;
                     avrComms_longData=avrCommsParser_command.data2;
                     
                  
                  }
                  // individual voice bank-change
                  
                  else if (avrComms_longOp!=BANK_GLOBAL)
                  // don't override global bank changes
                  {
                     // stack the operations so multiple voice bank changes can take place
                     
                     avrComms_longOp=avrCommsParser_command.data1;
                     avrComms_longData=avrCommsParser_command.data2;
                  
                  }
               }
            }
            
            
            // morph operation
            else if(avrCommsParser_command.status == VOICE_MORPH)
            {
               /* Display-only STM report for full-range per-voice morph
                  amounts. Do not echo this back into STM sound state. */
               avrCommsParser_handleVoiceMorphReport(avrCommsParser_command.data1,
                                                     avrCommsParser_command.data2);
            }
            else if(avrCommsParser_command.status == MORPH_CC)
            {  
               /* STM owns live morph computation. Keep these legacy opcodes
                  inert so they cannot trigger AVR-side preset_morph(). */
            }
            
            
            //-------------------------------------------------
            /* -bc- additions to front interpreter end here*/
            
            
            
            else if(avrCommsParser_command.status == LED_CC)
            {
               uint8_t offset=0;
               switch(avrCommsParser_command.data1)
               {
                  case LED_CURRENT_STEP_NR: 
                     {
                     
                        if(avrCommsParser_command.data2 >=128) 
                           return;
                     
                        uint8_t shownPattern = menu_getViewedPattern();
                        uint8_t playedPattern = menu_playedPattern;
                     
                        if(shownPattern == playedPattern) {
                        //only update chaselight LED when it step edit mode
                           if( (menu_activePage < MENU_MIDI_PAGE) || menu_activePage == PERFORMANCE_PAGE ||menu_activePage == SEQ_PAGE || menu_activePage == EUKLID_PAGE) {
                              led_setActive_step(avrCommsParser_command.data2);
                           }							
                        } 
                        else {
                           led_clearActive_step();
                        }								
                     								
                     }							
                     break;
               	
               
               	
                  case LED_PULSE_BEAT:
                     if(avrCommsParser_command.data2!=0)
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
                     	
                     	
                        uint8_t stepNr = avrCommsParser_command.data2 & 0x7f;
                        uint8_t subStepRange = buttonHandler_selectedStep;
                     	//check if received step is a valid sub step
                        if( (stepNr >= subStepRange) && (stepNr<(subStepRange+8)) )
                        {
                           stepNr = (uint8_t)(stepNr - subStepRange);
                           led_setValue(1,(uint8_t)(LED_PART_SELECT1+stepNr));
                        }
                     	
                     	
                     }
                     break;
                  
                  case LED_SEQ_MAIN_FOUR:
                     offset=(uint8_t)(offset+4);
                  case LED_SEQ_MAIN_THREE:
                     offset=(uint8_t)(offset+4);
                  case LED_SEQ_MAIN_TWO:
                     offset=(uint8_t)(offset+4);
                  case LED_SEQ_MAIN_ONE:
                     {
                     	//parse sub steps
                        uint8_t i;
                        uint8_t ledArray=(avrCommsParser_command.data2 & 0x0f);
                        for (i=0;i<4;i++)
                        {
                           led_setValue( (uint8_t)(ledArray&(0x01<<i)),(uint8_t)(LED_STEP1+i+offset));
                        }
                     }
                     
                  
                     break;                  
                  case LED_SEQ_SUB_STEP_LOWER:
                     /* Substep LED packets are accepted whenever the effective
                        shifted voice view is active, including latched SHIFT
                        toggle, or while copy/clear temporarily owns the LEDs. */
                     if ( ( (menu_activePage<=VOICE7_PAGE)&&(buttonHandler_getShift()||copyClear_getCopyMode()) ) || (menu_activePage==SEQ_PAGE) || (menu_activePage==EUKLID_PAGE))
                     {
                     	//parse sub steps
                        uint8_t i;
                        uint8_t ledArray=(avrCommsParser_command.data2 & 0x0f);
                        for (i=0;i<4;i++)
                        {
                           led_setValue( (uint8_t)(ledArray&(0x01<<i)),(uint8_t)(LED_PART_SELECT1+i));
                        }
                     }
                     break;
                  
                  case LED_SEQ_SUB_STEP_UPPER:
                     /* Keep upper substep LED gating identical to the lower
                        half: use effective SHIFT so toggle mode is a real
                        held-button override. */
                     if ( ( (menu_activePage<=VOICE7_PAGE)&&(buttonHandler_getShift()||copyClear_getCopyMode()) ) || (menu_activePage==SEQ_PAGE) || (menu_activePage==EUKLID_PAGE))
                     {
                     	//parse sub steps
                        uint8_t i;
                        uint8_t ledArray=(avrCommsParser_command.data2 & 0x0f);
                        for (i=0;i<4;i++)
                        {
                           led_setValue( (uint8_t)(ledArray&(0x01<<i)),(uint8_t)(LED_PART_SELECT1+4+i));
                        }
                     }
                     break;
                  
                  case LED_SEQ_BUTTON:
                     {
                        if(menu_activePage != PERFORMANCE_PAGE) //do not show active steps on perf. page
                        {
                        //limit to 16 steps
                           uint8_t stepNr = (uint8_t)((avrCommsParser_command.data2&0x7f)/8); //limit to 127
                        
                           led_setValue(1,(uint8_t)(LED_STEP1+stepNr));
                        }
                     }		
                  
                     break;
                  case LCD_PRINT_SCREEN:
                     {
                        // This was possibly added by some hallucinating LLM, probably Gemini. Should never happen.
                        // char text[8];
                        // lcd_clear();
                        // lcd_home();
                        // lcd_string_F(PSTR("cortex says"));
                        // lcd_setcursor(0,2);
                        // itoa((int)avrCommsParser_command.data2,text,10);
                        // lcd_string(text);
                        // _delay_ms(2000);
                     }
                     break;
               }						
            }								
            else if(avrCommsParser_command.status == NOTE_ON)
            {
               if(avrCommsParser_command.data1 > 6) 
                  return;
            	//only SELECT_MODE_VOICE and SELECT_MODE_MUTE
               led_pulseLed((uint8_t)(LED_VOICE1+avrCommsParser_command.data1));
            }
         
         }				
      }
   }
}
//------------------------------------------------------------
void avrComms_checkLongOps(void)
{
   if (avrComms_longOp){
   
      if (avrComms_longOp==BANK_GLOBAL)
      {
         if (parameter_values[PAR_LOAD_PERF_ON_BANK]){
            preset_loadPerf(avrComms_longData,0x7f); // preset, isAll, release kitlock, voiceArray
            if(avrComms_morphAvail)
            {
               avrComms_longOp=MORPH_OP;
               avrComms_morphAvail=0;
            }
            else
               avrComms_longOp=NULL_OP; 
		 
	    buttonHandler_handleModeButtons(SELECT_MODE_PERF);
	    menu_repaint();
         }
         else {
            preset_loadDrumset(avrComms_longData,0x7f,0);
            menu_repaint();
            if(avrComms_morphAvail)
            {
               avrComms_longOp=MORPH_OP;
               avrComms_morphAvail=0;
            }
            else
               avrComms_longOp=NULL_OP;
         }
         
         
      }
      else if (avrComms_longOp==PATTERN_CHANGE_OP)
      {
         // nothing to see here yet
         if(avrComms_morphAvail)
         {
            avrComms_longOp=MORPH_OP;
            avrComms_morphAvail=0;
         }
         else
            avrComms_longOp=NULL_OP;
      }
         
      else if (avrComms_longOp<BANK_GLOBAL)
      // we have (possibly multiple) voice bank changes
      {
        
         if (parameter_values[PAR_LOAD_PERF_ON_BANK]){
            preset_loadPerf(avrComms_longData,avrComms_longOp);
            menu_repaint();
            if(avrComms_morphAvail)
            {
               avrComms_longOp=MORPH_OP;
               avrComms_morphAvail=0;
            }
            else
               avrComms_longOp=NULL_OP;            
         }
         else {
            preset_loadDrumset(avrComms_longData,avrComms_longOp,0);
            menu_repaint();
            if(avrComms_morphAvail)
            {
               avrComms_longOp=MORPH_OP;
               avrComms_morphAvail=0;
            }
            else
               avrComms_longOp=NULL_OP; 
         }
         
      }
      
      
      else if (avrComms_longOp==MORPH_OP)
      {
         avrComms_morphAvail=0;
         avrComms_longOp=NULL_OP;
      }
      
      else
      {
         avrComms_longOp=NULL_OP;
      }
   }
   
}
