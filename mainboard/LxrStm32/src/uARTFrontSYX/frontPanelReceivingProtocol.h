/*
 * frontPanelReceivingProtocol.h
 *
 * Front-panel receive/protocol boundary owned by the uARTFrontSYX layer.
 *
 * This header owns the STM front-panel opcode namespace and the receive-side
 * parser entry points. Uart.c/.h remains raw transport. Outbound packet
 * construction is owned by frontPanelSendingProtocol.c/.h.
 */

#ifndef UARTFRONTSYX_FRONTPANELRECEIVINGPROTOCOL_H_
#define UARTFRONTSYX_FRONTPANELRECEIVINGPROTOCOL_H_

#include <stdint.h>

// Cortex <-> Front messages

// control messages from cortex for leds
// status
#define FRONT_SEQ_VOICE_MORPH           0xab // automation morph amount for a drum voice
#define FRONT_CC_MACRO_TARGET          0xaa // macro destination/value change message
/* MACRO_CC message structure
byte1 - status byte 0xaa as above
byte2, data1 byte: xttaaa-b : tt= top level macro value sent (2 macros exist now, we can do 2 more if we want)
                              aaa= macro destination value sent (4 destinations exist now, can do 8)
                              b=macro mod target value top bit
                              I have left a blank bit above this to make it easier to make more than 255 kit parameters
                              if we ever want to take on that can of worms
                              
byte3, data2 byte: xbbbbbbb : b=macro mod target value lower 7 bits or top level value full
*/

#define FRONT_STEP_LED_STATUS_BYTE 		0xb1	// LED control/status message
#define FRONT_SEQ_CC					      0xb2	// sequencer command/control message
//#define FRONT_CODEC_CONTROL				0xb3	// codec/audio-engine control message, currently unused
#define VOICE_CC						      0xb4	// voice-specific control message
#define BANK_CHANGE_CC                  0xad	// bank selection / bank change
#define PARAM_CC                        0xae	// live parameter change, CC slot 1-127
#define PARAM_CC2                       0xaf	// live parameter change, CC slot 128+
#define FRONT_SET_BPM				   	0xb5	// set tempo using a 14-bit value
#define FRONT_CC_2					   	0xb6	// second CC bank for parameters above 127
#define FRONT_CC_LFO_TARGET				0xb7	// set the target parameter for an LFO lane
#define FRONT_CC_VELO_TARGET		   	0xb8	// set the target parameter for a velocity lane
#define FRONT_STEP_CC				   	0xb9	// toggle a step in the subStepPattern array
#define FRONT_SET_P1_DEST		   		0xba	// set automation destination for param lane 1
#define FRONT_SET_P2_DEST				   0xbb	// set automation destination for param lane 2
#define FRONT_SET_P1_VAL				   0xbc	// set automation value for param lane 1
#define FRONT_SET_P2_VAL				   0xbd	// set automation value for param lane 2
#define FRONT_MAIN_STEP_CC				   0xbe	// toggle main step
#define FRONT_ARM_AUTOMATION_STEP		0xbf	// arm/disarm automation for a step

#define SAMPLE_CC						      0xc0	// sample transfer/control message
#define PRF_RESTORE_PARAM_CC          0xc1	// restore a parameter value during PRF load
#define PRF_RESTORE_PARAM_CC2         0xc2	// restore a high-CC parameter value during PRF load
#define PRF_RESTORE_MORPH_CC          0xc3	// restore a morph value during PRF load
#define PRF_RESTORE_MORPH_CC2         0xc4	// restore a high-CC morph value during PRF load
//#define PRF_CACHE_STATUS              0xc5	// PRF cache handshake status response
#define PARAM_RESTORE_DONE            0xc6	// end of a parameter restore burst
#define PARAM_RESTORE_BEGIN           0xc7	// begin a parameter restore burst
#define PARAM_RESTORE_READY           0xc8	// ready/ack marker for parameter restore
#define PARAM_RESTORE_ACK             0xc9	// acknowledge a parameter restore request
/* MENU-ORIGINATED SOUND-PARAMETER EDITS, AVR -> STM  (Session 039 P3)
   WHAT: identical payload to MIDI_CC (raw id 0..127) / FRONT_CC_2 (raw id - 128)
         but sent only by AVR menu encoder/knob edits
         (menu_sendSoundParameter()).
   WHY:  the plain opcodes are also used by file/voice load and the
         armed-step revert, so the STM could not tell a menu edit apart.
         Only menu edits may be transmitted as MIDI CC/NRPN when txf C is on.
         Both are handled exactly like the plain opcodes, plus a TX queue call.
   ACCESSORS: frontParser_handleMidiMessage() MIDI_CC / FRONT_CC_2 cases.
   AFFILIATES: AVR MIDI_CC_MENU / CC_2_MENU (avrCommsReceivingProtocol.h,
               same values), outputControl_queueMenuParameterTx(). */
#define FRONT_CC_MENU                 0xca	// menu edit, parameter 0..127
#define FRONT_CC_2_MENU               0xcb	// menu edit, parameter 128+
//#define PRF_CACHE_REJECTED            0x00	// PRF cache request rejected
//#define PRF_CACHE_ACCEPTED            0x01	// PRF cache request accepted
/* SAMPLE_CC subcommands.
   AVR->STM: START_UPLOAD and COUNT.
   STM->AVR: COUNT, UPLOAD_RESULT, and progress messages.
   The result/progress packets replace the old raw ACK wait so the AVR parser
   can consume ordinary three-byte protocol traffic during the blocking import. */
#define FRONT_SAMPLE_START_UPLOAD       0x01 // begin sample upload
#define FRONT_SAMPLE_COUNT              0x02 // sample count request/response
#define FRONT_SAMPLE_UPLOAD_RESULT      0x03 // upload completed; data2 is SAMPLE_UPLOAD_STATUS_* flags
#define FRONT_SAMPLE_UPLOAD_SAMPLE_PROGRESS 0x04 // sample upload progress; data2 is 1-based sample number
#define FRONT_SAMPLE_UPLOAD_LOOP_PROGRESS   0x05 // loop upload progress; data2 is 1-based loop number

// message
#define FRONT_CURRENT_STEP_NUMBER_CC	0x01	/**< send the current active chase light step number to the frontplate*/
#define FRONT_LED_SEQ_BUTTON			   0x02	/**< turn on a step seq. led*/
#define FRONT_LED_QUERY_SEQ_TRACK		0x03	/**< the frontpanel wants to know whick seq. leds should be lit*/
#define FRONT_LED_PULSE_BEAT			   0x04	/**< pulse the beat indicator LED*/
#define FRONT_LED_SEQ_SUB_STEP	      0x05	// set one sub-step LED
#define FRONT_LED_ALL_SUBSTEP          0x3f  // send back all the substeps

#define FRONT_LED_SEQ_MAIN_ONE         0x40  // main-step LED block 1 of 4
#define FRONT_LED_SEQ_MAIN_TWO         0x41	// main-step LED block 2 of 4
#define FRONT_LED_SEQ_MAIN_THREE       0x42	// main-step LED block 3 of 4
#define FRONT_LED_SEQ_MAIN_FOUR        0x43	// main-step LED block 4 of 4

#define FRONT_LED_SEQ_SUB_STEP_LOWER	0x44  // lower half of the sub-step LEDs
#define FRONT_LED_SEQ_SUB_STEP_UPPER	0x45  // upper half of the sub-step LEDs
#define FRONT_LCD_PRINT_SCREEN	0x46	// LCD screen print command

// Sequencer commands
#define FRONT_SEQ_RUN_STOP				   0x01	// start/stop sequencer playback

#define FRONT_SEQ_MUTE_TRACK			   0x09	// mute a track
#define FRONT_SEQ_UNMUTE_TRACK			0x0a	// unmute a track
#define FRONT_SEQ_CHANGE_PAT			   0x0b	/**< the user requested a new pattern. send the same message back to the front as ack that the new pattern is loaded*/
#define FRONT_SEQ_CHANGE_TMP_PAT		   0x0e	/**< request pat_tmpPattern as a temporary ninth performance pattern */
#define FRONT_SEQ_ROLL_ON			   	0x0c	/**< start roll for voice data2*/
#define FRONT_SEQ_ROLL_OFF			   	0x0d	/**< stop roll for voice data2*/
#define FRONT_SEQ_REQUEST_STEP_PARAMS 	0x0f	// request the active step parameter set
#define FRONT_SEQ_ROLL_ON_OFF		   	0x10	/**< turn voice roll/flamm on/off. data 2 parameter is bit 0 to 3 = voice number, bit 4 is on/off flag*/
#define FRONT_SEQ_ROLL_RATE				0x11	// set roll speed
#define FRONT_SEQ_VOLUME			   	0x12	// set step/voice volume
#define FRONT_SEQ_NOTE				   	0x13	// set step note
#define FRONT_SEQ_PROB				   	0x14	// set step probability
#define FRONT_SEQ_SET_ACTIVE_TRACK 		0x15	/**< select the active track. all track specific messages (request step params etc) received will refer to the track selected with this command*/
//#define FRONT_SEQ_RESYNC_LFO			0x16	/**< LFO is no longer running on the front */
#define FRONT_SEQ_EUKLID_LENGTH 		   0x17	/** sets the length of the current track from 0 to 16 steps*/
#define FRONT_SEQ_EUKLID_STEPS			0x18	// set the Euclid step count
#define FRONT_SEQ_REQUEST_EUKLID_PARAMS 0x19	// request the current Euclid settings
#define FRONT_SEQ_SET_SHOWN_PATTERN		0x1A	// set the pattern shown on the front panel

#define FRONT_SEQ_REC_ON_OFF			   0x1B	/**< start(data2=1) or stop(data2=0) recording mode */
#define FRONT_SEQ_REQUEST_PATTERN_PARAMS 0x1C 	/**< the sequencer sends back the data of the active pattern */
#define FRONT_SEQ_SET_PAT_BEAT			0x1D	/**< on every Nth bar the pattern will change to the next pattern*/
#define FRONT_SEQ_SET_PAT_NEXT			0x1E	/**< the next pattern that will be played when the current finishes*/
#define FRONT_SEQ_CLEAR_TRACK			   0x1f	// clear a track
#define FRONT_SEQ_COPY_TRACK			   0x20	// copy a track
#define FRONT_SEQ_COPY_PATTERN			0x21	// copy a pattern
#define FRONT_SEQ_SET_QUANT				0x22	// set quantize amount
#define FRONT_SEQ_SET_AUTOM_TRACK		0x23 	// select the automation track
#define FRONT_SEQ_SELECT_ACTIVE_STEP 	0x24	// select the active step for automation destination edits
#define FRONT_SEQ_SHUFFLE				   0x25	// set shuffle amount
#define FRONT_SEQ_TRACK_LENGTH			0x26	// set track length
#define FRONT_SEQ_TRACK_SCALE          0x3c	// set track scale / step multiplier
#define FRONT_SEQ_CLEAR_PATTERN			0x27	// clear a pattern
#define FRONT_SEQ_CLEAR_AUTOM			   0x28 	// clear an automation lane

#define FRONT_SEQ_POSX					   0x29	// set X position
#define FRONT_SEQ_POSY					   0x2a	// set Y position
#define FRONT_SEQ_FLUX					   0x2b	// set flux amount
#define FRONT_SEQ_SOM_FREQ				   0x2c	// set SOM frequency
#define FRONT_SEQ_MIDI_CHAN				0x2d	// voiceNr (0xf0) + channel (0x0f). --AS voice 7=global channel
//#define FRONT_SEQ_MIDI_MODE				0x2e	// legacy MIDI mode selector, now unused
#define FRONT_SEQ_MIDI_ROUTING			0x2f	// MIDI routing selector
#define FRONT_SEQ_MIDI_TX_FILTER		   0x30    // MIDI transmit filter
#define FRONT_SEQ_MIDI_RX_FILTER		   0x31    // MIDI receive filter
#define FRONT_SEQ_BAR_RESET_MODE		   0x32	// reset bar on manual pattern change
#define FRONT_SEQ_ERASE_ON_OFF			0x33    // turn erase mode on/off
#define FRONT_SEQ_TRACK_ROTATION		   0x34	// rotate a track start position
#define FRONT_SEQ_EUKLID_ROTATION		0x35	// set Euclid rotation
#define FRONT_SEQ_EUKLID_SUBSTEP_ROTATION 0x46	// set Euclid sub-step rotation
#define FRONT_SEQ_EUKLID_RESET         0x47	// reset Euclid state / SHIFT+PERF temp-track snapshot control
/* `data2` values for FRONT_SEQ_EUKLID_RESET.
   0x01 is the legacy cache-clear used by file operations.
   The remaining values control one SHIFT+PERF visit: begin a snapshot window,
   end/commit it on page leave, or restore the tracks touched during it. */
#define FRONT_SEQ_EUKLID_RESET_CLEAR_ROTATION 0x01
#define FRONT_SEQ_EUKLID_RESET_BEGIN_VISIT    0x02
#define FRONT_SEQ_EUKLID_RESET_END_VISIT      0x03
#define FRONT_SEQ_EUKLID_RESET_RESTORE_VISIT  0x04

#define FRONT_SEQ_TRIGGER_IN_PPQ		   0x36	// input clock prescaler
#define FRONT_SEQ_TRIGGER_OUT1_PPQ 		0x37	// trigger output 1 prescaler
#define FRONT_SEQ_TRIGGER_OUT2_PPQ 		0x38	// trigger output 2 prescaler
#define FRONT_SEQ_TRIGGER_GATE_MODE 	0x39	// trigger gate mode

#define FRONT_SEQ_COPY_TRACK_PATTERN   0x3a // added message for single track pattern copy
#define FRONT_SEQ_PC_TIME_MODE         0x3b // setting for change pattern on bar or step
#define FRONT_SEQ_COPY_SRC            0x3d // added message for copy step
#define FRONT_SEQ_COPY_DST            0x3e // added message for copy step destination

#define FRONT_SEQ_ROLL_NOTE            0x40	// roll note value
#define FRONT_SEQ_ROLL_VELOCITY        0x41	// roll velocity value
#define FRONT_SEQ_ROLL_MODE            0x42	// roll behavior mode
#define FRONT_SEQ_TRANSPOSE            0x43	// transpose amount
#define FRONT_SEQ_TRANSPOSE_ON_OFF     0x44	// transpose enable/disable
#define FRONT_SEQ_SET_LOOP             0x45	// loop mode
#define FRONT_SEQ_LOAD_VOICE             0x48	// load a voice payload
#define FRONT_SEQ_UNHOLD_VOICE             0x49	// release a held voice after load
#define FRONT_SEQ_FILE_BEGIN             0x4f	// begin a file transfer
#define FRONT_SEQ_LOAD_FAST             0x50	// begin background file loading
#define FRONT_SEQ_FILE_DONE             0x51	// end a file transfer
#define FRONT_SEQ_TRACK_NOTE1		0x52	// track note payload slot 1
#define FRONT_SEQ_TRACK_NOTE2		0x53	// track note payload slot 2
#define FRONT_SEQ_TRACK_NOTE3		0x54	// track note payload slot 3
#define FRONT_SEQ_TRACK_NOTE4		0x55	// track note payload slot 4
#define FRONT_SEQ_TRACK_NOTE5		0x56	// track note payload slot 5
#define FRONT_SEQ_TRACK_NOTE6		0x57	// track note payload slot 6
#define FRONT_SEQ_TRACK_NOTE7		0x58	// track note payload slot 7
#define FRONT_SEQ_MIDI_CHAN_OFF         0x59	// disable MIDI output for a voice
#define FRONT_SEQ_FLOW_BEGIN            0x5a	// begin a credit-based flow-control channel
#define FRONT_SEQ_FLOW_GRANT            0x5b	// grant credits / ack a flow-control step
#define FRONT_SEQ_FLOW_END              0x5c	// end a flow-control channel
#define FRONT_SEQ_FLOW_ABORT            0x5d	// abort a flow-control channel
//#define FRONT_SEQ_PRF_CACHE_BEGIN       0x5e	// begin the legacy PRF cache/session handshake
//#define FRONT_SEQ_PRF_PENDING_BEGIN     0x5f	// mark pending PRF snapshot begin
//#define FRONT_SEQ_PRF_PENDING_DONE      0x60	// mark pending PRF snapshot complete
//#define FRONT_SEQ_PRF_CACHE_ABORT       0x61	// abort the legacy PRF cache/session handshake
//#define FRONT_SEQ_PRF_AVR_SNAPSHOT_BEGIN 0x62	// begin AVR-side snapshot capture
//#define FRONT_SEQ_PRF_AVR_SNAPSHOT_END   0x63	// end AVR-side snapshot capture
//#define FRONT_SEQ_PRF_RESTORE_AVR_LIVE   0x64	// restore AVR live state after snapshot
#define FRONT_SEQ_TMP_KIT_ENDPOINT_BEGIN 0x65	// begin a temp-kit endpoint copy
#define FRONT_SEQ_TMP_KIT_ENDPOINT_END   0x66	// end a temp-kit endpoint copy
#define FRONT_SEQ_TMP_KIT_AUTOMATION_PHASE 0x67	// mark the automation phase of a temp-kit copy
#define FRONT_SEQ_SET_GLOBAL_MORPH       0x68	// set global morph amount
#define FRONT_SEQ_SET_GLOBAL_MORPH_LSB   0x69	// set/report the low 7 bits of global morph
#define FRONT_SEQ_SET_GLOBAL_MORPH_MSB   0x6a	// set/report the high bit of global morph
#define FRONT_SEQ_REPORT_GLOBAL_MORPH_LSB 0x6b	// report the low 7 bits of global morph back to AVR
#define FRONT_SEQ_REPORT_GLOBAL_MORPH_MSB 0x6c	// report the high bit of global morph back to AVR
#define FRONT_SEQ_BACKGROUND_SWAP_BEGIN 0x6d	// request background-swap prep before file load
#define FRONT_SEQ_BACKGROUND_SWAP_DONE  0x6e	// background-swap prep complete
/* Front-panel command for the global MIDI roll-note offset. The value changes
   external MIDI note routing only, not performance roll controls. */
#define FRONT_SEQ_ROLL_NOTE_OFFSET     0x6f	// saved MIDI roll-note offset
#define FRONT_SEQ_OSC_WAVE_INTERPOLATION 0x70   // enable/disable waveform interpolation
/* PERF ROLL DISPLAY REPORTS, STM -> AVR  (Session 039)
   WHAT: FRONT_SEQ_CC sub-opcodes reporting a roll value changed by external
         MIDI (Global NRPN 93/94/95). data2 = new value: rate 0..15 (the same
         selector index as PAR_ROLL / FRONT_SEQ_ROLL_RATE), note 0..127,
         velocity 0..127.
   WHY:  display-only sync so the AVR PERF page matches STM. Distinct names
         from FRONT_SEQ_ROLL_RATE/_NOTE/_VELOCITY because protocol direction
         matters; those are AVR -> STM. PARAM_CC2 cannot be used: it cannot
         address PAR_ROLL_NOTE (270) and its AVR handler writes parameters2[],
         which is only END_OF_SOUND_PARAMETERS long.
   ACCESSORS: frontPanelSending_sendRollParamReport() (sender).
   AFFILIATES: AVR SEQ_REPORT_ROLL_* (avrCommsReceivingProtocol.h, same values). */
#define FRONT_SEQ_REPORT_ROLL_RATE       0x71	// report roll rate (0..15) back to AVR
#define FRONT_SEQ_REPORT_ROLL_NOTE       0x72	// report roll note back to AVR
#define FRONT_SEQ_REPORT_ROLL_VELOCITY   0x73	// report roll velocity back to AVR

#define FRONT_SEQ_TMP_KIT_ENDPOINT_BOTH 0x00	// copy both endpoints
#define FRONT_SEQ_TMP_KIT_ENDPOINT_FRONT_ONLY 0x01	// copy only the kit/front endpoint
#define FRONT_SEQ_TMP_KIT_ENDPOINT_MORPH_ONLY 0x02	// copy only the morph endpoint

#define FRONT_SEQ_TMP_KIT_AUTOMATION_NONE 0x00	// no automation sideband in the temp-kit copy
#define FRONT_SEQ_TMP_KIT_AUTOMATION_FRONT_ENDPOINT 0x01	// temp-kit front-endpoint automation sideband
#define FRONT_SEQ_TMP_KIT_AUTOMATION_MORPH_ENDPOINT 0x02	// temp-kit morph-endpoint automation sideband

#define FLOW_CH_LOAD_SESSION            0x00	// load-session flow channel
#define FLOW_CH_GLOBALS                 0x01	// globals file flow channel
#define FLOW_CH_VOICE_PARAM             0x02	// voice-parameter flow channel
#define FLOW_CH_DRUM_META               0x03	// drum-meta flow channel

#include "MidiMessages.h"

/* Shared receive-side front-panel state. These globals are owned by the
   receive protocol while legacy frontParser_* call sites remain. */
extern uint8_t frontParser_rxCnt;
/* Three-byte front-panel command assembly. The storage is MIDI-shaped because
   the legacy wire format is MIDI-shaped, but this is AVR/STM protocol state. */
extern MidiMsg frontParser_command;
extern uint8_t frontParser_activeTrack;
extern uint8_t frontParser_shownPattern;
extern uint8_t frontParser_sysexActive;
extern uint8_t frontParser_activeFrontTrack;
extern uint16_t frontParser_twoByteData;
extern uint8_t frontParser_sysexBuffer[16];
extern uint16_t frontParser_sysexSeqStepNr;
extern uint8_t frontParser_activeStep;
extern uint8_t frontParser_stepCopySource;
extern uint8_t frontParser_originalCcValues[0xff];
extern uint8_t midi_envPosition[6];

/* Receive-side entry points and parser state queries. */
void frontParser_parseUartData(unsigned char data);
void frontParser_handleMidiMessage(void);
void frontParser_applyParameterCommand(MidiMsg msg, uint8_t updateOriginalValue);
void frontParser_serviceBackgroundSwapAck(void);
uint8_t frontParser_isQuietUi();

#endif /* UARTFRONTSYX_FRONTPANELRECEIVINGPROTOCOL_H_ */
