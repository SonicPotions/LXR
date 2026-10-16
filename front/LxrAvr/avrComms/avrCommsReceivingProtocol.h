/*
 * avrCommsReceivingProtocol.h
 *
 * Created: 27.04.2012 12:03:52
 *  Author: Julian
 */ 


#ifndef AVRCOMMSRECEIVINGPROTOCOL_H_
#define AVRCOMMSRECEIVINGPROTOCOL_H_
#include <avr/io.h>
#include "./Preset/SeqStep.h"



/**for pattern request from seq we need a flag to signal if new data arrived*/
extern volatile uint8_t avrCommsParser_newSeqDataAvailable;
//a step instance to buffer the data received from the sequencer
extern volatile StepData avrCommsParser_stepData;
extern uint8_t avrComms_sysexMode;
extern uint8_t avrComms_longOp;
extern uint8_t avrComms_longData;
extern uint8_t avrCommsParser_rxDisable;
/* Latched by SAMPLE_UPLOAD_RESULT while the menu is waiting for a blocking STM
   sample import to finish. Volatile because UART RX parsing can update it while
   menu_waitSampleUploadResult() polls from foreground code. */
extern volatile uint8_t avrComms_sampleUploadDone;
extern volatile uint8_t avrComms_sampleUploadStatus;
uint8_t avrCommsParser_isRestoreActive(void);


#define NOTE_ON 			0x90	// MIDI note-on status byte (2 data bytes)
#define MIDI_CC				0xb0	// MIDI CC status byte (2 data bytes)
//#define MIDI_CC2			0xF4	// 2 data bytes an unused midi status is used to indicate another cc message for params above 127


#define END_PATTERN_NOTE_ON 0x // legacy placeholder for the old pattern-note terminator

#define SYSEX_START			0xF0	// SysEx start byte
#define SYSEX_END			0xF7	// SysEx end byte
#define PATCH_RESET			0xFE	// SHIFT+PLAY reload request; AVR sends it to STM and does not restore local snapshots
#define CALLBACK_ACK		0xFD	// callback/priority-byte acknowledge
//control messages from cortex for leds
//status, param changes

#define VOICE_MORPH     0xab // automation morph amount for a single voice
#define MACRO_CC        0xaa // macro destination/value change message
/* MACRO_CC message structure
byte1 - status byte 0xaa as above
byte2, data1 byte: xttaaa-b : tt= top level macro value sent (2 macros exist now, we can do 2 more if we want)
                              aaa= macro destination value sent (4 destinations exist now, can do 8)
                              b=macro mod target value top bit
                              I have left a blank bit above this to make it easier to make more than 255 kit parameters
                              if we ever want to take on that can of worms
                              
byte3, data2 byte: xbbbbbbb : b=macro mod target value lower 7 bits or top level value full
*/

#define MORPH_CC        0xac	// global morph amount
#define BANK_CHANGE_CC  0xad	// bank selection / bank change
#define PARAM_CC        0xae	// live parameter change, CC slot 1-127
#define PARAM_CC2       0xaf	// live parameter change, CC slot 128+
#define LED_CC				0xb1	// LED control/status message
#define SEQ_CC				0xb2	// sequencer command/control message
//#define CODEC_CC			0xb3	// codec/audio-engine control message, currently unused
#define VOICE_CC			0xb4	// voice-specific control message
#define SET_BPM				0xb5	// set tempo using a 14-bit value
#define CC_2				0xb6		// second CC bank for parameters above 127
#define CC_LFO_TARGET		0xb7		// set the target parameter for an LFO lane
#define CC_VELO_TARGET		0xb8	// set the target parameter for a velocity lane
#define STEP_CC				0xb9	// toggle a sub-step in the current pattern
#define SET_P1_DEST			0xba	// set automation destination for param lane 1
#define SET_P2_DEST			0xbb	// set automation destination for param lane 2
#define SET_P1_VAL			0xbc	// set automation value for param lane 1
#define SET_P2_VAL			0xbd	// set automation value for param lane 2
#define MAIN_STEP_CC		0xbe	// toggle a main step
#define ARM_AUTOMATION_STEP	0xbf	// arm/disarm automation for a step

#define SAMPLE_CC			0xc0	// sample transfer/control message
#define PRF_RESTORE_PARAM_CC  0xc1	// restore a parameter value during PRF load
#define PRF_RESTORE_PARAM_CC2 0xc2	// restore a high-CC parameter value during PRF load
#define PRF_RESTORE_MORPH_CC  0xc3	// restore a morph value during PRF load
#define PRF_RESTORE_MORPH_CC2 0xc4	// restore a high-CC morph value during PRF load
//#define PRF_CACHE_STATUS      0xc5	// PRF cache handshake status response
#define PARAM_RESTORE_DONE    0xc6	// end of a parameter restore burst
#define PARAM_RESTORE_BEGIN   0xc7	// begin a parameter restore burst
#define PARAM_RESTORE_READY   0xc8	// ready/ack marker for parameter restore
#define PARAM_RESTORE_ACK     0xc9	// acknowledge a parameter restore request
/* MENU-ORIGINATED SOUND-PARAMETER EDITS, AVR -> STM  (Session 039 P3)
   WHAT: same payload as MIDI_CC (raw id 0..127) / CC_2 (raw id - 128); sent
         only by menu encoder/knob edits via menu_sendSoundParameter().
   WHY:  lets the STM transmit menu changes as MIDI CC/NRPN (txf C) while
         loads, the armed-step revert and restores keep MIDI_CC/CC_2 and are
         never transmitted.
   AFFILIATES: STM FRONT_CC_MENU / FRONT_CC_2_MENU (frontPanelReceivingProtocol.h,
               same values). */
#define MIDI_CC_MENU          0xca	// menu edit, parameter 0..127
#define CC_2_MENU             0xcb	// menu edit, parameter 128+
//#define PRF_CACHE_REJECTED    0x00	// PRF cache request rejected
//#define PRF_CACHE_ACCEPTED    0x01	// PRF cache request accepted
/* SAMPLE_CC subcommands mirror STM frontPanelReceivingProtocol.h.
   START_UPLOAD/COUNT are AVR requests; RESULT/progress are STM replies. */
#define SAMPLE_START_UPLOAD 0x01	// begin sample upload
#define SAMPLE_COUNT		0x02	// sample count request/response
#define SAMPLE_UPLOAD_RESULT 0x03	// upload completed; data2 is SAMPLE_UPLOAD_STATUS_* flags
#define SAMPLE_UPLOAD_SAMPLE_PROGRESS 0x04	// data2 is 1-based sample number
#define SAMPLE_UPLOAD_LOOP_PROGRESS   0x05	// data2 is 1-based loop number
/* Status bits shown by the load menu after RESULT. They must stay in sync with
   mainboard/LxrStm32/src/SampleRom/SampleMemory.h. */
#define SAMPLE_UPLOAD_STATUS_OK          0x00
#define SAMPLE_UPLOAD_STATUS_COUNT_LIMIT 0x01
#define SAMPLE_UPLOAD_STATUS_FLASH_LIMIT 0x02
#define SAMPLE_UPLOAD_STATUS_READ_ERROR  0x04

//preset status bytes
#define PRESET_NAME				0xb4	/**< this message consists of 4 messages with status FRONT_PRESET_NAME and 2 data bytes each with 2 charactzers of the name*/
#define PRESET					0xb5	// legacy preset data/status byte


//preset
#define PRESET_LOAD				0x01	// load a preset image
//#define PRESET_SAVE				0x02	// save a preset image
//#define PATTERN_LOAD			0x03	// load pattern data



// end of 1st byte 'STATUS' byte messages
//led_cc messages - if 1st byte is 'LED_CC'
#define LED_CURRENT_STEP_NR 0x01	// report the current step number
#define LED_SEQ_BUTTON		0x02	// light a sequencer button LED
#define LED_QUERY_SEQ_TRACK 0x03	// query which track LEDs should be lit
#define LED_PULSE_BEAT		0x04	/**< pulse the beat indicator LED*/
#define LED_SEQ_SUB_STEP	0x05	// set one sub-step LED
#define LED_ALL_SUBSTEP        0x3f	// mask for all sub-step LEDs

#define LED_SEQ_MAIN_ONE      0x40  // main-step LED block 1 of 4
#define LED_SEQ_MAIN_TWO      0x41	// main-step LED block 2 of 4
#define LED_SEQ_MAIN_THREE    0x42	// main-step LED block 3 of 4
#define LED_SEQ_MAIN_FOUR     0x43	// main-step LED block 4 of 4

#define LED_SEQ_SUB_STEP_LOWER 0x44	// lower half of the sub-step LEDs
#define LED_SEQ_SUB_STEP_UPPER 0x45	// upper half of the sub-step LEDs
#define LCD_PRINT_SCREEN	0x46	// LCD screen print command

//#define LED_TRIGGER_VOICE	0x05	/**< send by the sequencer whenever a voice is triggered*/


//VOICE_CC
//#define VOICE_AUDIO_OUT		0x01
// --AS appears unused
//#define VOICE_MIDI_CHAN		0x02
#define VOICE_DECIMATION	0x03	// voice decimation / bit-depth style control

//TODO in hex werten... nicht dezimal als hex
//seq_cc messages
#define SEQ_RUN_STOP		0x01	// start/stop sequencer playback

#define SEQ_MUTE_TRACK		0x09	// mute a track
#define SEQ_UNMUTE_TRACK	0x0a	// unmute a track
#define SEQ_CHANGE_PAT		0x0b	/**<requested a new pattern. the same message is send back to the front as ack that the new pattern is loaded (after the current patern finished)*/
#define SEQ_CHANGE_TMP_PAT	0x0e	/**< request STM seq_tmpPattern as a temporary ninth performance pattern */
//#define SEQ_ROLL_ON			0x0c	/**< start roll for voice data2*/
//#define SEQ_ROLL_OFF		0x0d	/**< stop roll for voice data2*/
//#define SEQ_GET_ACTIVE_PAT	0x0e	/**< get the active pattern number from the sequencer */
#define SEQ_REQUEST_STEP_PARAMS 0x0f	// request the active step parameter set
#define SEQ_ROLL_ON_OFF		0x10	/**< turn voice roll/flamm on/off. data 2 parameter is bit 0 to 3 = voice number, bit 4 is on/off flag*/
#define SEQ_ROLL_RATE		0x11	// set roll speed
#define SEQ_VOLUME			0x12	// set step/voice volume
#define SEQ_NOTE			0x13	// set step note
#define SEQ_PROB			0x14	// set step probability
#define SEQ_SET_ACTIVE_TRACK 0x15	// select the active track
//#define SEQ_RESYNC_LFO		0x16	/**< LFO is no longer running on the front */
#define SEQ_EUKLID_LENGTH   0x17	/** sets the length of the current track from 0 to 15 steps*/
#define SEQ_EUKLID_STEPS	0x18	// set the Euclid step count
#define SEQ_REQUEST_EUKLID_PARAMS 0x19	// request the current Euclid settings
#define SEQ_SET_SHOWN_PATTERN	0x1A	// set the pattern shown on the front panel

#define SEQ_TMP_PATTERN 8	// temp/performance pattern slot index

#define SEQ_REC_ON_OFF		0x1B		/**< start(data2=1) or stop(data2=0) recording mode */
#define SEQ_REQUEST_PATTERN_PARAMS 0x1C /**< the sequencer sends back the data of the active pattern */
#define SEQ_SET_PAT_BEAT	0x1D		/**< on every Nth bar the pattern will change to the next pattern*/
#define SEQ_SET_PAT_NEXT	0x1E		/**< the next pattern that will be played when the current finishes*/

#define SEQ_CLEAR_TRACK		0x1f	// clear a track
#define SEQ_COPY_TRACK		0x20	// copy a track
#define SEQ_COPY_PATTERN	0x21	// copy a pattern
#define SEQ_SET_QUANT		0x22	// set quantize amount
#define SEQ_SET_AUTOM_TRACK	0x23 // select the automation track
#define SEQ_SELECT_ACTIVE_STEP 0x24 // select the active step for automation destination edits
#define SEQ_SHUFFLE			0x25	// set shuffle amount
#define SEQ_TRACK_LENGTH	0x26	// set track length
#define SEQ_TRACK_SCALE    0x3c	// set track scale / step multiplier
#define SEQ_CLEAR_PATTERN	0x27	// clear a pattern
#define SEQ_CLEAR_AUTOM		0x28 // clear an automation lane

#define SEQ_POSX			0x29	// set X position
#define SEQ_POSY			0x2a	// set Y position
#define SEQ_FLUX			0x2b	// set flux amount
#define SEQ_SOM_FREQ		0x2c	// set SOM frequency
#define SEQ_MIDI_CHAN		0x2d	// voiceNr (0xf0) + channel (0x0f). --AS voice 7=global channel
//#define SEQ_MIDI_MODE		0x2e	// legacy MIDI mode selector, now unused
#define SEQ_MIDI_ROUTING    0x2f	// MIDI routing selector
#define SEQ_MIDI_FILT_TX	0x30	// MIDI transmit filter
#define SEQ_MIDI_FILT_RX	0x31	// MIDI receive filter

#define SEQ_BAR_RESET_MODE  0x32	// reset bar on manual pattern change
#define SEQ_ERASE_ON_OFF    0x33	// erase notes while recording
#define SEQ_TRACK_ROTATION	0x34 // rotate a track start position
#define SEQ_EUKLID_ROTATION	0x35	// set Euclid rotation
#define SEQ_EUKLID_SUBSTEP_ROTATION 0x46	// set Euclid sub-step rotation
#define SEQ_EUKLID_RESET   0x47	// reset Euclid state / SHIFT+PERF temp-track snapshot control
/* `data2` values for SEQ_EUKLID_RESET.
   0x01 keeps the legacy meaning used by file/preset operations: clear the STM
   Euclid working caches before loading or saving pattern payloads.
   The higher values are page-visit control for the SHIFT+PERF Euclid page and
   reuse the existing opcode instead of adding another transport command. */
#define SEQ_EUKLID_RESET_CLEAR_ROTATION 0x01
#define SEQ_EUKLID_RESET_BEGIN_VISIT    0x02
#define SEQ_EUKLID_RESET_END_VISIT      0x03
#define SEQ_EUKLID_RESET_RESTORE_VISIT  0x04

#define SEQ_TRIGGER_IN_PPQ	  0x36	// input clock prescaler
#define SEQ_TRIGGER_OUT1_PPQ  0x37	// trigger output 1 prescaler
#define SEQ_TRIGGER_OUT2_PPQ  0x38	// trigger output 2 prescaler
#define SEQ_TRIGGER_GATE_MODE 0x39	// trigger gate mode
#define SEQ_ROLL_NOTE       0x40	// roll note value
#define SEQ_ROLL_VELOCITY       0x41	// roll velocity value
#define SEQ_ROLL_MODE          0x42	// roll behavior mode

#define SEQ_TRANSPOSE            0x43	// transpose amount
#define SEQ_TRANSPOSE_ON_OFF     0x44	// transpose enable/disable
#define SEQ_SET_LOOP             0x45	// loop mode
#define SEQ_LOAD_VOICE		 0x48	// load a voice payload
#define SEQ_UNHOLD_VOICE             0x49	// release a held voice after load
#define SEQ_FILE_BEGIN             0x4f	// begin a file transfer
#define SEQ_LOAD_BACKGROUND       0x50	// begin background file loading
#define SEQ_FILE_DONE             0x51	// end a file transfer
#define SEQ_TRACK_NOTE1           0x52	// track note payload slot 1
#define SEQ_TRACK_NOTE2           0x53	// track note payload slot 2
#define SEQ_TRACK_NOTE3           0x54	// track note payload slot 3
#define SEQ_TRACK_NOTE4           0x55	// track note payload slot 4
#define SEQ_TRACK_NOTE5           0x56	// track note payload slot 5
#define SEQ_TRACK_NOTE6           0x57	// track note payload slot 6
#define SEQ_TRACK_NOTE7           0x58	// track note payload slot 7
#define SEQ_MIDI_CHAN_OFF         0x59	// disable MIDI output for a voice
#define SEQ_FLOW_BEGIN            0x5a	// begin a credit-based flow-control channel
#define SEQ_FLOW_GRANT            0x5b	// grant credits / ack a flow-control step
#define SEQ_FLOW_END              0x5c	// end a flow-control channel
#define SEQ_FLOW_ABORT            0x5d	// abort a flow-control channel
//#define SEQ_PRF_CACHE_BEGIN       0x5e	// begin the legacy PRF cache/session handshake
//#define SEQ_PRF_PENDING_BEGIN     0x5f	// mark pending PRF snapshot begin
//#define SEQ_PRF_PENDING_DONE      0x60	// mark pending PRF snapshot complete
//#define SEQ_PRF_CACHE_ABORT       0x61	// abort the legacy PRF cache/session handshake
//#define SEQ_PRF_AVR_SNAPSHOT_BEGIN 0x62	// begin AVR-side snapshot capture
//#define SEQ_PRF_AVR_SNAPSHOT_END   0x63	// end AVR-side snapshot capture
//#define SEQ_PRF_RESTORE_AVR_LIVE   0x64	// restore AVR live state after snapshot
#define SEQ_TMP_KIT_ENDPOINT_BEGIN 0x65	// begin a temp-kit endpoint copy
#define SEQ_TMP_KIT_ENDPOINT_END   0x66	// end a temp-kit endpoint copy
#define SEQ_TMP_KIT_AUTOMATION_PHASE 0x67	// mark the automation phase of a temp-kit copy
#define SEQ_SET_GLOBAL_MORPH       0x68	// set global morph amount
#define SEQ_SET_GLOBAL_MORPH_LSB   0x69	// set/report the low 7 bits of global morph
#define SEQ_SET_GLOBAL_MORPH_MSB   0x6a	// set/report the high bit of global morph
#define SEQ_REPORT_GLOBAL_MORPH_LSB 0x6b	// report the low 7 bits of global morph back to AVR
#define SEQ_REPORT_GLOBAL_MORPH_MSB 0x6c	// report the high bit of global morph back to AVR
#define SEQ_BACKGROUND_SWAP_BEGIN  0x6d	// request STM background-swap prep before file load
#define SEQ_BACKGROUND_SWAP_DONE   0x6e	// STM background-swap prep complete
/* Raw global MIDI roll-note offset: 0/off or a positive 1..127 semitone
   shift above the normal trigger note; STM owns held-roll release on change. */
#define SEQ_ROLL_NOTE_OFFSET       0x6f	// saved MIDI roll-note offset
#define SEQ_OSC_WAVE_INTERPOLATION 0x70 // enable/disable waveform interpolation
/* PERF ROLL DISPLAY REPORTS, STM -> AVR  (Session 039)
   WHAT: SEQ_CC sub-opcodes carrying a roll value changed by external MIDI
         (Global NRPN 93/94/95); data2 = rate 0..15 / note 0..127 /
         velocity 0..127.
   WHY:  display sync for the PERF page. Distinct from SEQ_ROLL_RATE (0x11),
         SEQ_ROLL_NOTE (0x40) and SEQ_ROLL_VELOCITY (0x41), which are AVR -> STM.
   ACCESSORS: avrCommsReceivingProtocol.c SEQ_CC switch.
   AFFILIATES: STM FRONT_SEQ_REPORT_ROLL_* (frontPanelReceivingProtocol.h,
               same values). */
#define SEQ_REPORT_ROLL_RATE       0x71	// STM reports roll rate (0..15)
#define SEQ_REPORT_ROLL_NOTE       0x72	// STM reports roll note
#define SEQ_REPORT_ROLL_VELOCITY   0x73	// STM reports roll velocity

#define SEQ_TMP_KIT_AUTOMATION_NONE 0x00	// no automation sideband in the temp-kit copy
#define SEQ_TMP_KIT_AUTOMATION_FRONT_ENDPOINT 0x01	// temp-kit front-endpoint automation sideband
#define SEQ_TMP_KIT_AUTOMATION_MORPH_ENDPOINT 0x02	// temp-kit morph-endpoint automation sideband

#define SEQ_TMP_KIT_ENDPOINT_BOTH 0x00	// copy both endpoints
#define SEQ_TMP_KIT_ENDPOINT_FRONT_ONLY 0x01	// copy only the kit/front endpoint
#define SEQ_TMP_KIT_ENDPOINT_MORPH_ONLY 0x02	// copy only the morph endpoint

#define FLOW_CH_LOAD_SESSION      0x00	// load-session flow channel
#define FLOW_CH_GLOBALS           0x01	// globals file flow channel
#define FLOW_CH_VOICE_PARAM       0x02	// voice-parameter flow channel
#define FLOW_CH_DRUM_META         0x03	// drum-meta flow channel
//bc adds

#define SEQ_COPY_TRACK_PATTERN         0x3a	// copy a single track pattern
#define SEQ_PC_TIME                    0x3b	// pattern change timing mode
#define SEQ_COPY_STEP_SET_SRC          0x3d // choose the step-copy source
#define SEQ_COPY_STEP_SET_DST          0x3e // choose the step-copy destination

//SysEx
#define SYSEX_REQUEST_STEP_DATA			0x01	// request a step-data SysEx block
#define SYSEX_SEND_STEP_DATA			0x02	// send a step-data SysEx block
#define SYSEX_REQUEST_MAIN_STEP_DATA		0x03	// request a main-step SysEx block
#define SYSEX_SEND_MAIN_STEP_DATA		0x04	// send a main-step SysEx block
#define SYSEX_REQUEST_PATTERN_DATA		0x05	// request a pattern-data SysEx block
#define SYSEX_SEND_PAT_LEN_DATA			0x06	// send pattern length data
#define SYSEX_SEND_PAT_SCALE_DATA		0x07	// send pattern scale data
#define SYSEX_SEND_PAT_CHAIN_DATA		0x0a	// send pattern chain data
// bc - added to deal with changing only some patterns. let the mainboard
// know the pattern transmit is done and which voices should change
#define SYSEX_BEGIN_PATTERN_TRANSMIT	0x08	// begin a pattern transmit burst
#define SYSEX_END_PATTERN_TRANSMIT		0x09	// end a pattern transmit burst
#define SYSEX_STEP_ACK                 0x10	// acknowledge a received step block
#define SYSEX_WAIT						0x11    // wait for main to catch up
#define SYSEX_ACTIVE_MODE_NONE			0x7f	/**< a placeholder message indicating that sysex is active but no mode is selected yet*/

/** Three-byte AVR comms command storage. The wire format is MIDI-shaped, but
    this object represents AVR/STM protocol traffic. */
typedef struct MidiStruct {
	uint8_t status;
	uint8_t data1;
	uint8_t data2;
} MidiMsg;

enum sysexCallBack
{
	NO_CALLBACK=0,
	SCALE_CALLBACK,
	LENGTH_CALLBACK,
	PATCHAIN_CALLBACK,
	MAINSTEP_CALLBACK,
	STEP_CALLBACK,
   STEP_ACK,

};

/** parse incoming data from the cortex*/
void avrComms_parseData(uint8_t data);
void avrCommsParser_parseNrpn(uint8_t value);
void avrCommsPanelParser_ccHandler(void);

extern volatile MidiMsg avrCommsParser_command;
extern uint8_t avrCommsParser_sysexCallback; 
void avrComms_checkLongOps(void);

#endif /* AVRCOMMSRECEIVINGPROTOCOL_H_ */
