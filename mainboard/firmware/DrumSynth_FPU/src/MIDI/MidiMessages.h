/*
 * MidiMessages.h
 *
 *  Created on: 02.04.2012
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


#ifndef MIDIMESSAGES_H_
#define MIDIMESSAGES_H_

#include "stm32f4xx.h"

enum MidiSource {
	midiSourceMIDI,
	midiSourceUSB
};

struct MidiBits {
	enum MidiSource source:1; // 0 for midi, 1 for usb
	unsigned sysxbyte:1; // 1 if this message is a sysex payload only
	unsigned length:2; // how many data bytes have been filled
	unsigned :4;
};

//-----------------------------------------------------------
/** a struct defining a standard midi message*/
typedef struct MidiStruct {
	uint8_t status;
	uint8_t data1;
	uint8_t data2;
	struct MidiBits bits;
} MidiMsg;



//-----------------------------------------------------------
//Status bytes
#define NOTE_OFF 			0x80	// 2 data bytes
#define NOTE_ON 			0x90	// 2 data bytes
#define MIDI_CC				0xb0	// 2 data bytes
#define MIDI_CC2			0xF4	// 2 data bytes an unused midi status is used to indicate another cc message for params above 127
#define PROG_CHANGE			0xc0	// 1 data bytes
#define MIDI_PITCH_WHEEL	0xE0	// 2 data bytes
#define MIDI_AT				0xA0	// 2 data bytes
#define CHANNEL_PRESSURE	0xD0	// 2 data bytes


//-----------------------------------------------------------
// CCs (2nd byte controller number)
#define CC_BANK_CHANGE		0x00
#define CC_MOD_WHEEL		0x01
#define CC_ALL_SOUND_OFF	0x78
#define CC_ALL_NOTES_OFF	0x7B

//-----------------------------------------------------------
// system messages
#define MIDI_CLOCK			0xF8
#define SYSEX_START			0xF0
#define SYSEX_END			0xF7
#define MIDI_START			0xFA
#define MIDI_STOP			0xFC
#define MIDI_CONTINUE		0xFB
#define MIDI_MTC_QFRAME		0xF1	//--AS mtc timecodes
#define MIDI_SONG_SEL		0xF3	//--AS passthru only

//------------------------------------------------------------

#define NO_AUTOMATION 0xff	//used as a dummy message number for the automation tracks.
							// paramNr 0xff means no automation
// Custom CCs

enum
{
	I_DUNNO = 0, //was hiermit???	/*0*/
	CC_MODWHEEL,
	OSC_WAVE_DRUM1 = 2,
	OSC_WAVE_DRUM2,
	OSC_WAVE_DRUM3,
	OSC_WAVE_SNARE,
	NRPN_DATA_ENTRY_COARSE,
	CYM_WAVE1,
	WAVE1_HH,

	F_OSC1_COARSE,
	F_OSC1_FINE,					/*10*/
	F_OSC2_COARSE,
	F_OSC2_FINE,
	F_OSC3_COARSE,
	F_OSC3_FINE,
	F_OSC4_COARSE,
	F_OSC4_FINE,
	F_OSC5_COARSE,
	F_OSC5_FINE,
	F_OSC6_COARSE,
	F_OSC6_FINE,					/*20*/

	MOD_WAVE_DRUM1,
	MOD_WAVE_DRUM2,
	MOD_WAVE_DRUM3,
	CYM_WAVE2,
	CYM_WAVE3,
	WAVE2_HH,
	WAVE3_HH,

	SNARE_NOISE_F,
	SNARE_MIX,

	CYM_MOD_OSC_F1,					/*30*/
	CYM_MOD_OSC_F2,
	CYM_MOD_OSC_GAIN1,
	CYM_MOD_OSC_GAIN2,
	MOD_OSC_F1,
	MOD_OSC_F2,
	MOD_OSC_GAIN1,
	MOD_OSC_GAIN2,

	FILTER_FREQ_DRUM1,
	FILTER_FREQ_DRUM2,
	FILTER_FREQ_DRUM3,				/*40*/
	SNARE_FILTER_F,
	CYM_FIL_FREQ,
	HAT_FILTER_F,

	RESO_DRUM1,
	RESO_DRUM2,
	RESO_DRUM3,
	SNARE_RESO,
	CYM_RESO,
	HAT_RESO,

	VELOA1,							/*50*/
	VELOD1,
	VELOA2,
	VELOD2,
	VELOA3,
	VELOD3,
	VELOA4,
	VELOD4,
	VELOA5,
	VELOD5,
	VELOA6,							/*60*/
	VELOD6,
	VELOD6_OPEN,

	VOL_SLOPE1,
	VOL_SLOPE2,
	VOL_SLOPE3,
	EG_SNARE1_SLOPE,
	CYM_SLOPE,
	VOL_SLOPE6,

	REPEAT1,
	CYM_REPEAT,						/*70*/

	PITCHD1, //mod eg decay
	PITCHD2,
	PITCHD3,
	PITCHD4,

	MODAMNT1,
	MODAMNT2,
	MODAMNT3,
	MODAMNT4,

	PITCH_SLOPE1,
	PITCH_SLOPE2,					/*80*/
	PITCH_SLOPE3,
	PITCH_SLOPE4,

	FMAMNT1,	//TODO rename!
	FMDTN1,
	FMAMNT2,
	FMDTN2,
	FMAMNT3,
	FMDTN3,

	VOL1,
	VOL2,							/*90*/
	VOL3,
	VOL4,
	VOL5,
	VOL6,

	PAN1,
	PAN2,
	PAN3,
	NRPN_FINE,
	NRPN_COARSE,
	PAN4,							/*100*/
	PAN5,
	PAN6,

	OSC1_DIST,
	OSC2_DIST,
	OSC3_DIST,
	SNARE_DISTORTION,
	CYMBAL_DISTORTION,
	HAT_DISTORTION,

	VOICE_DECIMATION1,
	VOICE_DECIMATION2,				/*110*/
	VOICE_DECIMATION3,
	VOICE_DECIMATION4,
	VOICE_DECIMATION5,
	VOICE_DECIMATION6,
	VOICE_DECIMATION_ALL,

	FREQ_LFO1,	//todo rename cc and cc2 according to their new position
	FREQ_LFO2,
	FREQ_LFO3,
	FREQ_LFO4,
	FREQ_LFO5,						/*120*/
	FREQ_LFO6,

	AMOUNT_LFO1,
	AMOUNT_LFO2,
	AMOUNT_LFO3,
	AMOUNT_LFO4,
	AMOUNT_LFO5,
	AMOUNT_LFO6,

	RESERVED4,						/*128*/

}ParamEnums;

//for all parameters above 127
enum
{

	CC2_FILTER_DRIVE_1,
	CC2_FILTER_DRIVE_2,
	CC2_FILTER_DRIVE_3,
	CC2_FILTER_DRIVE_4,
	CC2_FILTER_DRIVE_5,
	CC2_FILTER_DRIVE_6,

	CC2_MIX_MOD_1,
	CC2_MIX_MOD_2,
	CC2_MIX_MOD_3,

	CC2_VOLUME_MOD_ON_OFF1,
	CC2_VOLUME_MOD_ON_OFF2,
	CC2_VOLUME_MOD_ON_OFF3,
	CC2_VOLUME_MOD_ON_OFF4,
	CC2_VOLUME_MOD_ON_OFF5,
	CC2_VOLUME_MOD_ON_OFF6,

	CC2_VELO_MOD_AMT_1,
	CC2_VELO_MOD_AMT_2,
	CC2_VELO_MOD_AMT_3,
	CC2_VELO_MOD_AMT_4,
	CC2_VELO_MOD_AMT_5,
	CC2_VELO_MOD_AMT_6,

	CC2_VEL_DEST_1,
	CC2_VEL_DEST_2,
	CC2_VEL_DEST_3,
	CC2_VEL_DEST_4,
	CC2_VEL_DEST_5,
	CC2_VEL_DEST_6,

	CC2_WAVE_LFO1,
	CC2_WAVE_LFO2,
	CC2_WAVE_LFO3,
	CC2_WAVE_LFO4,
	CC2_WAVE_LFO5,
	CC2_WAVE_LFO6,

	//the target and voice CC2ameters must be after one another!
	CC2_VOICE_LFO1,
	CC2_VOICE_LFO2,
	CC2_VOICE_LFO3,
	CC2_VOICE_LFO4,
	CC2_VOICE_LFO5,
	CC2_VOICE_LFO6,

	CC2_TARGET_LFO1,
	CC2_TARGET_LFO2,
	CC2_TARGET_LFO3,
	CC2_TARGET_LFO4,
	CC2_TARGET_LFO5,
	CC2_TARGET_LFO6,

	CC2_RETRIGGER_LFO1,
	CC2_RETRIGGER_LFO2,
	CC2_RETRIGGER_LFO3,
	CC2_RETRIGGER_LFO4,
	CC2_RETRIGGER_LFO5,
	CC2_RETRIGGER_LFO6,

	CC2_SYNC_LFO1,
	CC2_SYNC_LFO2,
	CC2_SYNC_LFO3,
	CC2_SYNC_LFO4,
	CC2_SYNC_LFO5,
	CC2_SYNC_LFO6,

	CC2_OFFSET_LFO1,
	CC2_OFFSET_LFO2,
	CC2_OFFSET_LFO3,
	CC2_OFFSET_LFO4,
	CC2_OFFSET_LFO5,
	CC2_OFFSET_LFO6,

	CC2_FILTER_TYPE_1,
	CC2_FILTER_TYPE_2,
	CC2_FILTER_TYPE_3,
	CC2_FILTER_TYPE_4,
	CC2_FILTER_TYPE_5,
	CC2_FILTER_TYPE_6,

	CC2_TRANS1_VOL,
	CC2_TRANS2_VOL,
	CC2_TRANS3_VOL,
	CC2_TRANS4_VOL,
	CC2_TRANS5_VOL,
	CC2_TRANS6_VOL,

	CC2_TRANS1_WAVE,
	CC2_TRANS2_WAVE,
	CC2_TRANS3_WAVE,
	CC2_TRANS4_WAVE,
	CC2_TRANS5_WAVE,
	CC2_TRANS6_WAVE,

	CC2_TRANS1_FREQ,
	CC2_TRANS2_FREQ,
	CC2_TRANS3_FREQ,
	CC2_TRANS4_FREQ,
	CC2_TRANS5_FREQ,
	CC2_TRANS6_FREQ,

	CC2_AUDIO_OUT1,
	CC2_AUDIO_OUT2,
	CC2_AUDIO_OUT3,
	CC2_AUDIO_OUT4,
	CC2_AUDIO_OUT5,
	CC2_AUDIO_OUT6,

	// --AS
	CC2_MIDI_NOTE1,
	CC2_MIDI_NOTE2,
	CC2_MIDI_NOTE3,
	CC2_MIDI_NOTE4,
	CC2_MIDI_NOTE5,
	CC2_MIDI_NOTE6,
	CC2_MIDI_NOTE7, // s/b 111 i think
	
	//<<insert new parameters here>>
	
	
	

	//Mute Button NRPN messages
	//these have to stay at the end of the CC2 list.
	//They are a special case to enable control of the channel muting via external NRPN messages
	CC2_MUTE_1 = 200,
	CC2_MUTE_2,
	CC2_MUTE_3,
	CC2_MUTE_4,
	CC2_MUTE_5,
	CC2_MUTE_6,
	CC2_MUTE_7,

}Param2Enums;


//Cortex <-> Front messages

//control messages from cortex for leds
//status
#define FRONT_STEP_LED_STATUS_BYTE 		0xb1
#define FRONT_SEQ_CC					0xb2
#define FRONT_CODEC_CONTROL				0xb3
#define VOICE_CC						0xb4
#define FRONT_SET_BPM					0xb5
#define FRONT_CC_2						0xb6	//for parameters above 127
#define FRONT_CC_LFO_TARGET				0xb7
#define FRONT_CC_VELO_TARGET			0xb8
#define FRONT_STEP_CC					0xb9	// toggle a step in the subStepPattern array
#define FRONT_SET_P1_DEST				0xba	// SET_P1_DEST, stepNr, destinationNr --> track(voice) via set active track cmd
#define FRONT_SET_P2_DEST				0xbb
#define FRONT_SET_P1_VAL				0xbc	// SET_P1_VAL, stepNr, value --> track(voice) via set active track cmd
#define FRONT_SET_P2_VAL				0xbd
#define FRONT_MAIN_STEP_CC				0xbe	// toggle main step
#define FRONT_ARM_AUTOMATION_STEP		0xbf	// status - stepNr - track | OnOff

#define SAMPLE_CC						0xc0
#define FRONT_SAMPLE_START_UPLOAD 		0x01
#define FRONT_SAMPLE_COUNT		 		0x02

//message
#define FRONT_CURRENT_STEP_NUMBER_CC	0x01	/**< send the current active chase light step number to the frontplate*/
#define FRONT_LED_SEQ_BUTTON			0x02	/**< turn on a step seq. led*/
#define FRONT_LED_QUERY_SEQ_TRACK		0x03	/**< the frontpanel wants to know whick seq. leds should be lit*/
#define FRONT_LED_PULSE_BEAT			0x04	/**< pulse the beat indicator LED*/
#define FRONT_LED_SEQ_SUB_STEP			0x05

//--AS appears unused
//#define VOICE_MIDI_CHAN					0x02

//Sequencer commands
#define FRONT_SEQ_RUN_STOP				0x01

#define FRONT_SEQ_MUTE_TRACK			0x09
#define FRONT_SEQ_UNMUTE_TRACK			0x0a
#define FRONT_SEQ_CHANGE_PAT			0x0b	/**< the user requested a new pattern. send the same message back to the front as ack that the new pattern is loaded*/
#define FRONT_SEQ_ROLL_ON				0x0c	/**< start roll for voice data2*/
#define FRONT_SEQ_ROLL_OFF				0x0d	/**< stop roll for voice data2*/
#define FRONT_SEQ_REQUEST_STEP_PARAMS 	0x0f
#define FRONT_SEQ_ROLL_ON_OFF			0x10	/**< turn voice roll/flamm on/off. data 2 parameter is bit 0 to 3 = voice number, bit 4 is on/off flag*/
#define FRONT_SEQ_ROLL_RATE				0x11
#define FRONT_SEQ_VOLUME				0x12
#define FRONT_SEQ_NOTE					0x13
#define FRONT_SEQ_PROB					0x14
#define FRONT_SEQ_SET_ACTIVE_TRACK 		0x15	/**< select the active track. all track specific messages (request step params etc) received will refer to the track selected with this command*/
//#define FRONT_SEQ_RESYNC_LFO			0x16	/**< LFO is no longer running on the front */
#define FRONT_SEQ_EUKLID_LENGTH 		0x17	/** sets the length of the current track from 0 to 16 steps*/
#define FRONT_SEQ_EUKLID_STEPS			0x18
#define FRONT_SEQ_REQUEST_EUKLID_PARAMS 0x19
#define FRONT_SEQ_SET_SHOWN_PATTERN		0x1A

#define FRONT_SEQ_REC_ON_OFF			0x1B	/**< start(data2=1) or stop(data2=0) recording mode */
#define FRONT_SEQ_REQUEST_PATTERN_PARAMS 0x1C 	/**< the sequencer sends back the data of the active pattern */
#define FRONT_SEQ_SET_PAT_BEAT			0x1D	/**< on every Nth bar the pattern will change to the next pattern*/
#define FRONT_SEQ_SET_PAT_NEXT			0x1E	/**< the next pattern that will be played when the current finishes*/
#define FRONT_SEQ_CLEAR_TRACK			0x1f
#define FRONT_SEQ_COPY_TRACK			0x20
#define FRONT_SEQ_COPY_PATTERN			0x21
#define FRONT_SEQ_SET_QUANT				0x22
#define FRONT_SEQ_SET_AUTOM_TRACK		0x23 	// SEQ_CC, SEQ_SET_AUTOM_TRACK, autoTrkNr
#define FRONT_SEQ_SELECT_ACTIVE_STEP 	0x24
#define FRONT_SEQ_SHUFFLE				0x25
#define FRONT_SEQ_TRACK_LENGTH			0x26
#define FRONT_SEQ_CLEAR_PATTERN			0x27
#define FRONT_SEQ_CLEAR_AUTOM			0x28 	//voice nr (0xf0) + autom track nr (0x0f)

#define FRONT_SEQ_POSX					0x29
#define FRONT_SEQ_POSY					0x2a
#define FRONT_SEQ_FLUX					0x2b
#define FRONT_SEQ_SOM_FREQ				0x2c
#define FRONT_SEQ_MIDI_CHAN				0x2d	//voiceNr (0xf0) + channel (0x0f). --AS voice 7=global channel
#define FRONT_SEQ_MIDI_MODE				0x2e //--AS not used anymore
#define FRONT_SEQ_MIDI_ROUTING			0x2f	// midi routing
#define FRONT_SEQ_MIDI_TX_FILTER		0x30    // tx filtering
#define FRONT_SEQ_MIDI_RX_FILTER		0x31    // rx filtering
#define FRONT_SEQ_BAR_RESET_MODE		0x32	// --AS reset bar on manual pattern change (0 is default - to not reset)
#define FRONT_SEQ_ERASE_ON_OFF			0x33    // --AS turn erase mode on/off
#define FRONT_SEQ_TRACK_ROTATION		0x34	// --AS rotate a track's start position 0 to 15
#define FRONT_SEQ_EUKLID_ROTATION		0x35

#define FRONT_SEQ_TRIGGER_IN_PPQ		0x36
#define FRONT_SEQ_TRIGGER_OUT1_PPQ 		0x37
#define FRONT_SEQ_TRIGGER_OUT2_PPQ 		0x38
#define FRONT_SEQ_TRIGGER_GATE_MODE 	0x39

//codec control messages
#define EQ_ON_OFF						0x01
#define EQ_BASS_F						0x02
#define EQ_TREB_F						0x03
#define EQ_TREB_GAIN					0x04
#define EQ_BASS_GAIN					0x05

#define LIMIT_ENABLE					0x06
#define LIMIT_ATT						0x07
#define LIMIT_REL						0x08
#define LIMIT_MAX						0x09
#define LIMIT_MIN						0x0A

//preset messages

//SysEx
#define SYSEX_INACTIVE					0x00	/**< SysEx mode is deactivated*/
#define SYSEX_REQUEST_STEP_DATA		 	0x01
#define SYSEX_RECEIVE_STEP_DATA			0x02
#define SYSEX_REQUEST_MAIN_STEP_DATA	0x03
#define SYSEX_RECEIVE_MAIN_STEP_DATA	0x04
#define SYSEX_REQUEST_PATTERN_DATA		0x05
#define SYSEX_RECEIVE_PAT_LEN_DATA		0x06
#define SYSEX_ACTIVE_MODE_NONE			0x7f	/**< a placeholder message indicating that sysex is active but no mode is selected yet*/
#endif /* MIDIMESSAGES_H_ */
