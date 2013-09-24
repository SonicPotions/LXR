/*
 * MidiParser.c
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


#include "MidiParser.h"
#include "MidiNoteNumbers.h"
#include "DrumVoice.h"
#include "Snare.h"
#include "config.h"
#include "HiHat.h"
#include "CymbalVoice.h"
#include "Uart.h"
#include "sequencer.h"
#include "clockSync.h"
#include "mixer.h"
#include "valueShaper.h"
#include "modulationNode.h"
#include "frontPanelParser.h"

static uint16_t midiParser_activeNrpnNumber = 0;

uint8_t midiParser_originalCcValues[0xff];

enum State
{
	MIDI_STATUS,  		// waiting for status byte
	MIDI_DATA1,  		// waiting for data byte1
	MIDI_DATA2,  		// waiting for data byte2
	SYSEX_DATA,  		// read sysex data byte
	IGNORE				// set when unknown status byte received, stays in ignore mode until next known status byte
  };

// 2^(1/12) factor for 1 semitone
#define SEMITONE_UP 1.0594630943592952645618252949463f
#define SEMITONE_DOWN 0.94387431268169349664191315666753f

#define NUM_LFO 6
uint8_t midiParser_selectedLfoVoice[NUM_LFO] = {0,0,0,0,0,0};

//----------------------------------------------------------
inline uint16_t calcSlopeEgTime(uint8_t data2)
{
	float val = (data2+1)/128.f;
	return data2>0?val*val*data2*128:1;
}
//-----------------------------------------------------------
inline float calcPitchModAmount(uint8_t data2)
{
	const float val = data2/127.f;
	return val*val*PITCH_AMOUNT_FACTOR;
}

//-----------------------------------------------------------
// vars
//-----------------------------------------------------------
uint8_t midi_MidiChannels[7];	// the currently selected midi channel
uint8_t midi_mode;
MidiMsg midiMsg_tmp;				// buffer message where the incoming data is stored
uint8_t msgLength;					// number of following data bytes for current status
uint8_t parserState = MIDI_STATUS;	// state of the parser state machine
//-----------------------------------------------------------
//macros
//-----------------------------------------------------------
/**check if the lower nibble of the status byte fits our midi channel (track 1)*/
#define midiParser_isValidMidiChannel(x) ((x&0x0f) == midi_MidiChannels[0])
//-----------------------------------------------------------
//takes a midi value from 0 to 127 and return +/- 50 cent detune factor
float midiParser_calcDetune(uint8_t value)
{
	//linear interpolation between 1(no change) and semitone up/down)
	float frac = (value/127.f -0.5f);
	float cent = 1;
	if(cent>=0)
	{
		cent += frac*(SEMITONE_UP - 1);
	}
	else
	{
		cent += frac*(SEMITONE_UP - 1);
	}
	return cent;
}
//-----------------------------------------------------------
void midiParser_nrpnHandler(uint16_t value)
{
	MidiMsg msg2;
	msg2.status = MIDI_CC2;
	msg2.data1 = midiParser_activeNrpnNumber;
	msg2.data2 = value;
	midiParser_ccHandler(msg2,true);
}
//-----------------------------------------------------------
/** handle all incoming CCs and invoke action*/
void midiParser_ccHandler(MidiMsg msg, uint8_t updateOriginalValue)
{
	if(msg.status == MIDI_CC)
	{

		const uint16_t paramNr = msg.data1-1;
		if(updateOriginalValue) {
			midiParser_originalCcValues[paramNr+1] = msg.data2;
		}

		switch(msg.data1)
		{

		case CC_MOD_WHEEL:

			break;

		case NRPN_DATA_ENTRY_COARSE:
			midiParser_nrpnHandler(msg.data2);
			return;
			break;

		case NRPN_FINE:
			midiParser_activeNrpnNumber &= ~(0x7f);	//clear lower 7 bit
			midiParser_activeNrpnNumber |= (msg.data2&0x7f);
			break;

		case NRPN_COARSE:
			midiParser_activeNrpnNumber &= 0x7f;	//clear upper 7 bit
			midiParser_activeNrpnNumber |= (msg.data2<<7);
			break;

		case VOL_SLOPE1:
			slopeEg2_setSlope(&voiceArray[0].oscVolEg,msg.data2);

			break;
		case PITCH_SLOPE1:
			DecayEg_setSlope(&voiceArray[0].oscPitchEg,msg.data2);
			break;

		case VOL_SLOPE2:
			slopeEg2_setSlope(&voiceArray[1].oscVolEg,msg.data2);
			break;
		case PITCH_SLOPE2:
			DecayEg_setSlope(&voiceArray[1].oscPitchEg,msg.data2);
			break;

		case VOL_SLOPE3:
			slopeEg2_setSlope(&voiceArray[2].oscVolEg,msg.data2);
			break;
		case PITCH_SLOPE3:
			DecayEg_setSlope(&voiceArray[2].oscPitchEg,msg.data2);
			break;

		case PITCH_SLOPE4:
			DecayEg_setSlope(&snareVoice.oscPitchEg,msg.data2);
			break;
		case VOL_SLOPE6:
			slopeEg2_setSlope(&hatVoice.oscVolEg,msg.data2);
			break;

		case FILTER_FREQ_DRUM1:
		case FILTER_FREQ_DRUM2:
		case FILTER_FREQ_DRUM3:
		{
			const float f = msg.data2/127.f;
			//exponential full range freq
			SVF_directSetFilterValue(&voiceArray[msg.data1-FILTER_FREQ_DRUM1].filter,valueShaperF2F(f,FILTER_SHAPER) );
		}
			break;
		case RESO_DRUM1:
		case RESO_DRUM2:
		case RESO_DRUM3:
			SVF_setReso(&voiceArray[msg.data1-RESO_DRUM1].filter, msg.data2/127.f);
			break;

		case F_OSC1_COARSE:
		{
			//clear upper nibble
			voiceArray[0].osc.midiFreq &= 0x00ff;
			//set upper nibble
			voiceArray[0].osc.midiFreq |= msg.data2 << 8;
			osc_recalcFreq(&voiceArray[0].osc);
		}
			break;
		case F_OSC2_COARSE:
		{
			//clear upper nibble
			voiceArray[1].osc.midiFreq &= 0x00ff;
			//set upper nibble
			voiceArray[1].osc.midiFreq |= msg.data2 << 8;
			osc_recalcFreq(&voiceArray[1].osc);
		}
				break;
		case F_OSC3_COARSE:
		{
			//clear upper nibble
			voiceArray[2].osc.midiFreq &= 0x00ff;
			//set upper nibble
			voiceArray[2].osc.midiFreq |= msg.data2 << 8;
			osc_recalcFreq(&voiceArray[2].osc);
		}
				break;
		case F_OSC4_COARSE:
		{
			//clear upper nibble
			snareVoice.osc.midiFreq &= 0x00ff;
			//set upper nibble
			snareVoice.osc.midiFreq |= msg.data2 << 8;
			osc_recalcFreq(&snareVoice.osc);
		}
				break;
		case F_OSC5_COARSE:
		{
			//clear upper nibble
			cymbalVoice.osc.midiFreq &= 0x00ff;
			//set upper nibble
			cymbalVoice.osc.midiFreq |= msg.data2 << 8;
			osc_recalcFreq(&cymbalVoice.osc);
		}
				break;

		case F_OSC1_FINE:
		{
			//clear lower nibble
			voiceArray[0].osc.midiFreq &= 0xff00;
			//set lower nibble
			voiceArray[0].osc.midiFreq |= msg.data2;
			osc_recalcFreq(&voiceArray[0].osc);
		}
			break;
		case F_OSC2_FINE:
		{
			//clear lower nibble
			voiceArray[1].osc.midiFreq &= 0xff00;
			//set lower nibble
			voiceArray[1].osc.midiFreq |= msg.data2;
			osc_recalcFreq(&voiceArray[1].osc);
		}
				break;
		case F_OSC3_FINE:
		{
			//clear lower nibble
			voiceArray[2].osc.midiFreq &= 0xff00;
			//set lower nibble
			voiceArray[2].osc.midiFreq |= msg.data2;
			osc_recalcFreq(&voiceArray[2].osc);
		}
				break;
		case F_OSC4_FINE:
		{
			//TODO hier sollte was hin
		}
				break;

		case OSC_WAVE_DRUM1:
			voiceArray[0].osc.waveform = msg.data2;
			break;

		case MOD_WAVE_DRUM1:
			voiceArray[0].modOsc.waveform = msg.data2;
			break;

		case OSC_WAVE_DRUM2:
			voiceArray[1].osc.waveform = msg.data2;
			break;

		case MOD_WAVE_DRUM2:
			voiceArray[1].modOsc.waveform = msg.data2;
			break;

		case OSC_WAVE_DRUM3:
			voiceArray[2].osc.waveform = msg.data2;
			break;

		case MOD_WAVE_DRUM3:
			voiceArray[2].modOsc.waveform = msg.data2;
			break;

		case OSC_WAVE_SNARE:
			snareVoice.osc.waveform = msg.data2;
			break;


		case OSC1_DIST:
#if USE_FILTER_DRIVE
			voiceArray[0].filter.drive = 0.5f + (msg.data2/127.f) *6;
#else
			setDistortionShape(&voiceArray[0].distortion,msg.data2);
#endif
			break;
		case OSC2_DIST:
#if USE_FILTER_DRIVE
			voiceArray[1].filter.drive = 0.5f + (msg.data2/127.f)*6;
#else
			setDistortionShape(&voiceArray[1].distortion,msg.data2);
#endif
			break;
		case OSC3_DIST:
#if USE_FILTER_DRIVE
			voiceArray[3].filter.drive = 0.5f + (msg.data2/127.f)*6;
#else
			setDistortionShape(&voiceArray[2].distortion,msg.data2);
#endif
			break;

		case VELOA1:
			slopeEg2_setAttack(&voiceArray[0].oscVolEg,msg.data2,AMP_EG_SYNC);
			break;

		case VELOD1:
		{
			slopeEg2_setDecay(&voiceArray[0].oscVolEg,msg.data2,AMP_EG_SYNC);
		}
			break;

		case PITCHD1:
		{
			DecayEg_setDecay(&voiceArray[0].oscPitchEg,msg.data2);
		}
			break;

		case MODAMNT1:
			voiceArray[0].egPitchModAmount = calcPitchModAmount(msg.data2);
			break;

		case FMAMNT1:
			voiceArray[0].fmModAmount = msg.data2/127.f;
			break;

		case FMDTN1:
			//clear upper nibble
			voiceArray[0].modOsc.midiFreq &= 0x00ff;
			//set upper nibble
			voiceArray[0].modOsc.midiFreq |= msg.data2 << 8;
			osc_recalcFreq(&voiceArray[0].modOsc);
			break;

		case VOL1:
			voiceArray[0].vol = msg.data2/127.f;
			break;

		case PAN1:
			setPan(0,msg.data2);
			break;

		case VELOA2:
		{
			slopeEg2_setAttack(&voiceArray[1].oscVolEg,msg.data2,AMP_EG_SYNC);
		}
			break;

			case VELOD2:
			{
				slopeEg2_setDecay(&voiceArray[1].oscVolEg,msg.data2,AMP_EG_SYNC);
			}
				break;

			case PITCHD2:
			{
				DecayEg_setDecay(&voiceArray[1].oscPitchEg,msg.data2);
			}
				break;

			case MODAMNT2:
				voiceArray[1].egPitchModAmount = calcPitchModAmount(msg.data2);
				break;

			case FMAMNT2:
				voiceArray[1].fmModAmount = msg.data2/127.f;
				break;

			case FMDTN2:
				//clear upper nibble
				voiceArray[1].modOsc.midiFreq &= 0x00ff;
				//set upper nibble
				voiceArray[1].modOsc.midiFreq |= msg.data2 << 8;
				osc_recalcFreq(&voiceArray[1].modOsc);
				break;

			case VOL2:
				voiceArray[1].vol = msg.data2/127.f;
				break;

			case PAN2:
				setPan(1,msg.data2);
				break;


			case VELOA3:
			{
				slopeEg2_setAttack(&voiceArray[2].oscVolEg,msg.data2,AMP_EG_SYNC);
			}
				break;

				case VELOD3:
				{
					slopeEg2_setDecay(&voiceArray[2].oscVolEg,msg.data2,AMP_EG_SYNC);
				}
					break;

				case PITCHD3:
				{
					DecayEg_setDecay(&voiceArray[2].oscPitchEg,msg.data2);
				}
					break;

				case MODAMNT3:
					voiceArray[2].egPitchModAmount = calcPitchModAmount(msg.data2);
					break;

				case FMAMNT3:
					voiceArray[2].fmModAmount = msg.data2/127.f;
					break;

				case FMDTN3:
					//clear upper nibble
					voiceArray[2].modOsc.midiFreq &= 0x00ff;
					//set upper nibble
					voiceArray[2].modOsc.midiFreq |= msg.data2 << 8;
					osc_recalcFreq(&voiceArray[2].modOsc);
					break;

				case VOL3:
					voiceArray[2].vol = msg.data2/127.f;
					break;

				case PAN3:
					setPan(2,msg.data2);
					break;

				//snare
				case VOL4:
					snareVoice.vol = msg.data2/127.f;
					break;

				case PAN4:
					Snare_setPan(msg.data2);

					break;

				case SNARE_NOISE_F:
					snareVoice.noiseOsc.freq = msg.data2/127.f*22000;
					//TODO respond to midi note
					break;
				case VELOA4:
				{
					slopeEg2_setAttack(&snareVoice.oscVolEg,msg.data2,false);
				}
					break;
				case VELOD4:
				{
					slopeEg2_setDecay(&snareVoice.oscVolEg,msg.data2,false);
				}

					break;
				case PITCHD4:
				{
					DecayEg_setDecay(&snareVoice.oscPitchEg,msg.data2);
				}

					break;
				case MODAMNT4:
					snareVoice.egPitchModAmount = calcPitchModAmount(msg.data2);
					break;
				case SNARE_FILTER_F:
						{
			#if USE_PEAK
							peak_setFreq(&snareVoice.filter, msg.data2/127.f*20000.f);
			#else
							const float f = msg.data2/127.f;
							//exponential full range freq
							SVF_directSetFilterValue(&snareVoice.filter,valueShaperF2F(f,FILTER_SHAPER) );
			#endif
						}
						break;

				case SNARE_RESO:
	#if USE_PEAK
					peak_setGain(&snareVoice.filter, msg.data2/127.f);
	#else
					SVF_setReso(&snareVoice.filter, msg.data2/127.f);
	#endif
					break;

				case SNARE_MIX:
					snareVoice.mix = msg.data2/127.f;
					break;

				//snare 2
				case VOL5:
					cymbalVoice.vol = msg.data2/127.f;
				break;

				case PAN5:
					Cymbal_setPan(msg.data2);

					break;

				case VELOA5:
				{
					slopeEg2_setAttack(&cymbalVoice.oscVolEg,msg.data2,false);
				}

					break;
				case VELOD5:
				{
					slopeEg2_setDecay(&cymbalVoice.oscVolEg,msg.data2,false);
				}
					break;


				case CYM_WAVE1:
					cymbalVoice.osc.waveform = msg.data2;
					break;

				case CYM_WAVE2:
					cymbalVoice.modOsc.waveform = msg.data2;
					break;

				case CYM_WAVE3:
					cymbalVoice.modOsc2.waveform = msg.data2;
					break;

				case CYM_MOD_OSC_F1:
					//cymbalVoice.modOsc.freq = MidiNoteFrequencies[msg.data2];
					//clear upper nibble
					cymbalVoice.modOsc.midiFreq &= 0x00ff;
					//set upper nibble
					cymbalVoice.modOsc.midiFreq |= msg.data2 << 8;
					osc_recalcFreq(&cymbalVoice.modOsc);

					break;
				case CYM_MOD_OSC_F2:
					//clear upper nibble
					cymbalVoice.modOsc2.midiFreq &= 0x00ff;
					//set upper nibble
					cymbalVoice.modOsc2.midiFreq |= msg.data2 << 8;
					osc_recalcFreq(&cymbalVoice.modOsc2);
					break;
				case CYM_MOD_OSC_GAIN1:
					cymbalVoice.fmModAmount1 = msg.data2/127.f;
					break;
				case CYM_MOD_OSC_GAIN2:
					cymbalVoice.fmModAmount2 = msg.data2/127.f;
					break;

				case CYM_FIL_FREQ:
				{
					const float f = msg.data2/127.f;
					//exponential full range freq
					SVF_directSetFilterValue(&cymbalVoice.filter,valueShaperF2F(f,FILTER_SHAPER) );
				}
					break;

				case CYM_RESO:
					SVF_setReso(&cymbalVoice.filter, msg.data2/127.f);
					break;

				case CYM_REPEAT:
					cymbalVoice.oscVolEg.repeat = msg.data2;
					break;
				case CYM_SLOPE:
					slopeEg2_setSlope(&cymbalVoice.oscVolEg,msg.data2);
					break;

				// hat
				case WAVE1_HH:
					hatVoice.osc.waveform = msg.data2;
					break;
				case WAVE2_HH:
					hatVoice.modOsc.waveform = msg.data2;
					break;
				case WAVE3_HH:
					hatVoice.modOsc2.waveform = msg.data2;
					break;

				case VELOD6:
					hatVoice.decayClosed = slopeEg2_calcDecay(msg.data2);
					break;

				case VELOD6_OPEN:
					hatVoice.decayOpen = slopeEg2_calcDecay(msg.data2);
					break;

				case HAT_FILTER_F:
				{
					const float f = msg.data2/127.f;
					//exponential full range freq
					SVF_directSetFilterValue(&hatVoice.filter,valueShaperF2F(f,FILTER_SHAPER) );
				}
					break;

				case HAT_RESO:
					SVF_setReso(&hatVoice.filter, msg.data2/127.f);
					break;

				case VOL6:
					hatVoice.vol = msg.data2/127.f;
					break;

				case PAN6:
					HiHat_setPan(msg.data2);
					break;

				case F_OSC6_COARSE:
				{
					//clear upper nibble
					hatVoice.osc.midiFreq &= 0x00ff;
					//set upper nibble
					hatVoice.osc.midiFreq |= msg.data2 << 8;
					osc_recalcFreq(&hatVoice.osc);
				}
						break;

				case MOD_OSC_F1:
					//clear upper nibble
					hatVoice.modOsc.midiFreq &= 0x00ff;
					//set upper nibble
					hatVoice.modOsc.midiFreq |= msg.data2 << 8;
					osc_recalcFreq(&hatVoice.modOsc);
					break;

				case MOD_OSC_F2:
					//clear upper nibble
					hatVoice.modOsc2.midiFreq &= 0x00ff;
					//set upper nibble
					hatVoice.modOsc2.midiFreq |= msg.data2 << 8;
					osc_recalcFreq(&hatVoice.modOsc2);
					break;

				case MOD_OSC_GAIN1:
					hatVoice.fmModAmount1 = msg.data2/127.f;
					break;

				case MOD_OSC_GAIN2:
					hatVoice.fmModAmount2 = msg.data2/127.f;
					break;

				case VELOA6:
					slopeEg2_setAttack(&hatVoice.oscVolEg,msg.data2,false);
					break;

				case REPEAT1:
					snareVoice.oscVolEg.repeat = msg.data2;
					break;


				case EG_SNARE1_SLOPE:
					slopeEg2_setSlope(&snareVoice.oscVolEg,msg.data2);
					break;

				case SNARE_DISTORTION:
					setDistortionShape(&snareVoice.distortion,msg.data2);
					break;

				case CYMBAL_DISTORTION:
					setDistortionShape(&cymbalVoice.distortion,msg.data2);
					break;

				case HAT_DISTORTION:
					setDistortionShape(&hatVoice.distortion,msg.data2);
					break;

				case FREQ_LFO1:
				case FREQ_LFO2:
				case FREQ_LFO3:
					lfo_setFreq(&voiceArray[msg.data1-FREQ_LFO1].lfo,msg.data2);
									break;
				case FREQ_LFO4:
					lfo_setFreq(&snareVoice.lfo,msg.data2);
									break;
				case FREQ_LFO5:
					lfo_setFreq(&cymbalVoice.lfo,msg.data2);
									break;
				case FREQ_LFO6:
					lfo_setFreq(&hatVoice.lfo,msg.data2);
					break;

				case AMOUNT_LFO1:
				case AMOUNT_LFO2:
				case AMOUNT_LFO3:
					voiceArray[msg.data1-AMOUNT_LFO1].lfo.modTarget.amount = msg.data2/127.f;
					break;
				case AMOUNT_LFO4:
					snareVoice.lfo.modTarget.amount = msg.data2/127.f;
					break;
				case AMOUNT_LFO5:
					cymbalVoice.lfo.modTarget.amount = msg.data2/127.f;
					break;
				case AMOUNT_LFO6:
					hatVoice.lfo.modTarget.amount = msg.data2/127.f;
					break;

				case VOICE_DECIMATION1:
				case VOICE_DECIMATION2:
				case VOICE_DECIMATION3:
				case VOICE_DECIMATION4:
				case VOICE_DECIMATION5:
				case VOICE_DECIMATION6:
				case VOICE_DECIMATION_ALL:
					mixer_decimation_rate[msg.data1-VOICE_DECIMATION1] = valueShaperI2F(msg.data2,-0.7f);
					break;

				default:
					break;
		}
		modNode_originalValueChanged(paramNr);
	}

	else //MIDI_CC2
	{
		const uint16_t paramNr = msg.data1+1 + 127;

		if(updateOriginalValue) {
			midiParser_originalCcValues[paramNr] = msg.data2;
		}
		switch(msg.data1)
		{

			case CC2_TRANS1_WAVE:
					voiceArray[0].transGen.waveform = msg.data2;
				break;

			case CC2_TRANS1_VOL:
				voiceArray[0].transGen.volume = msg.data2/127.f;
				break;

			case CC2_TRANS1_FREQ:
				voiceArray[0].transGen.pitch = 1.f + ((msg.data2/33.9f)-0.75f) ;// range about  0.25 to 4 => 1/4 to 1*4
				break;

			case CC2_TRANS2_WAVE:
				voiceArray[1].transGen.waveform = msg.data2;
				break;

			case CC2_TRANS2_VOL:
				voiceArray[1].transGen.volume = msg.data2/127.f;
				break;

			case CC2_TRANS2_FREQ:
				voiceArray[1].transGen.pitch = 1.f + ((msg.data2/33.9f)-0.75f) ;
				break;

			case CC2_TRANS3_WAVE:
				voiceArray[2].transGen.waveform = msg.data2;
				break;

			case CC2_TRANS3_VOL:
				voiceArray[2].transGen.volume = msg.data2/127.f;
				break;

			case CC2_TRANS3_FREQ:
				voiceArray[2].transGen.pitch = 1.f + ((msg.data2/33.9f)-0.75f) ;
				break;

			case CC2_TRANS4_WAVE:
				snareVoice.transGen.waveform = msg.data2;
				break;

			case CC2_TRANS4_VOL:
				snareVoice.transGen.volume = msg.data2/127.f;
				break;
			case CC2_TRANS4_FREQ:
				snareVoice.transGen.pitch = 1.f + ((msg.data2/33.9f)-0.75f) ;
				break;

			case CC2_TRANS5_WAVE:
				cymbalVoice.transGen.waveform = msg.data2;
				break;

			case CC2_TRANS5_VOL:
				cymbalVoice.transGen.volume = msg.data2/127.f;
				break;
			case CC2_TRANS5_FREQ:
				cymbalVoice.transGen.pitch = 1.f + ((msg.data2/33.9f)-0.75f) ;
				break;

			case CC2_TRANS6_WAVE:
				hatVoice.transGen.waveform = msg.data2;
				break;

			case CC2_TRANS6_VOL:
				hatVoice.transGen.volume = msg.data2/127.f;
				break;
			case CC2_TRANS6_FREQ:
				hatVoice.transGen.pitch = 1.f + ((msg.data2/33.9f)-0.75f) ;
				break;

			case CC2_FILTER_TYPE_1:
			case CC2_FILTER_TYPE_2:
			case CC2_FILTER_TYPE_3:
				voiceArray[msg.data1-CC2_FILTER_TYPE_1].filterType = msg.data2+1;
				break;
			case CC2_FILTER_TYPE_4:
				snareVoice.filterType = msg.data2 + 1; // +1 because 0 is filter off which results in silence
				break;
			case CC2_FILTER_TYPE_5:
				//cymbal filter
				cymbalVoice.filterType = msg.data2+1;
				break;
			case CC2_FILTER_TYPE_6:
				//Hihat filter
				hatVoice.filterType = msg.data2+1;
				break;

			case CC2_FILTER_DRIVE_1:
			case CC2_FILTER_DRIVE_2:
			case CC2_FILTER_DRIVE_3:
#if UNIT_GAIN_DRIVE
				voiceArray[msg.data1-CC2_FILTER_DRIVE_1].filter.drive = (msg.data2/127.f);
#else
				SVF_setDrive(&voiceArray[msg.data1-CC2_FILTER_DRIVE_1].filter,msg.data2);
#endif
				break;
			case CC2_FILTER_DRIVE_4:
#if UNIT_GAIN_DRIVE
				snareVoice.filter.drive = (msg.data2/127.f);
#else
				SVF_setDrive(&snareVoice.filter, msg.data2);
#endif

				break;
			case CC2_FILTER_DRIVE_5:
#if UNIT_GAIN_DRIVE
				cymbalVoice.filter.drive = (msg.data2/127.f);
#else
				SVF_setDrive(&cymbalVoice.filter, msg.data2);
#endif

				break;
			case CC2_FILTER_DRIVE_6:
#if UNIT_GAIN_DRIVE
				hatVoice.filter.drive = (msg.data2/127.f);
#else
				SVF_setDrive(&hatVoice.filter, msg.data2);
#endif

				break;

			case CC2_MIX_MOD_1:
			case CC2_MIX_MOD_2:
			case CC2_MIX_MOD_3:
				voiceArray[msg.data1-CC2_MIX_MOD_1].mixOscs = msg.data2;
				break;

			case CC2_VOLUME_MOD_ON_OFF1:
			case CC2_VOLUME_MOD_ON_OFF2:
			case CC2_VOLUME_MOD_ON_OFF3:
				voiceArray[msg.data1-CC2_VOLUME_MOD_ON_OFF1].volumeMod = msg.data2;
				break;
			case CC2_VOLUME_MOD_ON_OFF4:
				snareVoice.volumeMod = msg.data2;
				break;
			case CC2_VOLUME_MOD_ON_OFF5:
				cymbalVoice.volumeMod = msg.data2;
				break;
			case CC2_VOLUME_MOD_ON_OFF6:
				hatVoice.volumeMod = msg.data2;
				break;

			case CC2_VELO_MOD_AMT_1:
			case CC2_VELO_MOD_AMT_2:
			case CC2_VELO_MOD_AMT_3:
			case CC2_VELO_MOD_AMT_4:
			case CC2_VELO_MOD_AMT_5:
			case CC2_VELO_MOD_AMT_6:
				velocityModulators[msg.data1-CC2_VELO_MOD_AMT_1].amount = msg.data2/127.f;
				break;

			case CC2_WAVE_LFO1:
			case CC2_WAVE_LFO2:
			case CC2_WAVE_LFO3:
				voiceArray[msg.data1-CC2_WAVE_LFO1].lfo.waveform = msg.data2;
				break;
			case CC2_WAVE_LFO4:
				snareVoice.lfo.waveform = msg.data2;
				break;
			case CC2_WAVE_LFO5:
				cymbalVoice.lfo.waveform = msg.data2;
				break;
			case CC2_WAVE_LFO6:
				hatVoice.lfo.waveform = msg.data2;
				break;

			case CC2_RETRIGGER_LFO1:
			case CC2_RETRIGGER_LFO2:
			case CC2_RETRIGGER_LFO3:
				voiceArray[msg.data1-CC2_RETRIGGER_LFO1].lfo.retrigger = msg.data2;
				break;
			case CC2_RETRIGGER_LFO4:
				snareVoice.lfo.retrigger = msg.data2;
				break;
			case CC2_RETRIGGER_LFO5:
				cymbalVoice.lfo.retrigger = msg.data2;
				break;
			case CC2_RETRIGGER_LFO6:
				hatVoice.lfo.retrigger = msg.data2;
				break;

			case CC2_SYNC_LFO1:
			case CC2_SYNC_LFO2:
			case CC2_SYNC_LFO3:
				lfo_setSync(&voiceArray[msg.data1-CC2_SYNC_LFO1].lfo, msg.data2);
				break;
			case CC2_SYNC_LFO4:
				lfo_setSync(&snareVoice.lfo, msg.data2);
				break;
			case CC2_SYNC_LFO5:
				lfo_setSync(&cymbalVoice.lfo, msg.data2);
				break;
			case CC2_SYNC_LFO6:
				lfo_setSync(&hatVoice.lfo, msg.data2);
				break;

			case CC2_VOICE_LFO1:
			case CC2_VOICE_LFO2:
			case CC2_VOICE_LFO3:
			case CC2_VOICE_LFO4:
			case CC2_VOICE_LFO5:
			case CC2_VOICE_LFO6:
				break;

			case CC2_TARGET_LFO1:
			case CC2_TARGET_LFO2:
			case CC2_TARGET_LFO3:
			case CC2_TARGET_LFO4:
			case CC2_TARGET_LFO5:
			case CC2_TARGET_LFO6:
				break;



			case CC2_OFFSET_LFO1:
			case CC2_OFFSET_LFO2:
			case CC2_OFFSET_LFO3:
				voiceArray[msg.data1-CC2_OFFSET_LFO1].lfo.phaseOffset = msg.data2/127.f * 0xffffffff;
				break;
			case CC2_OFFSET_LFO4:
				snareVoice.lfo.phaseOffset = msg.data2/127.f * 0xffffffff;
				break;
			case CC2_OFFSET_LFO5:
				cymbalVoice.lfo.phaseOffset = msg.data2/127.f * 0xffffffff;
				break;
			case CC2_OFFSET_LFO6:
				hatVoice.lfo.phaseOffset = msg.data2/127.f * 0xffffffff;
				break;

			case CC2_AUDIO_OUT1:
			case CC2_AUDIO_OUT2:
			case CC2_AUDIO_OUT3:
			case CC2_AUDIO_OUT4:
			case CC2_AUDIO_OUT5:
			case CC2_AUDIO_OUT6:
				mixer_audioRouting[msg.data1-CC2_AUDIO_OUT1] = msg.data2;
				break;

			case CC2_MUTE_1:
			case CC2_MUTE_2:
			case CC2_MUTE_3:
			case CC2_MUTE_4:
			case CC2_MUTE_5:
			case CC2_MUTE_6:
			case CC2_MUTE_7:
			{
				const uint8_t voiceNr = msg.data1 - CC2_MUTE_1;
				if(msg.data2 == 0)
				{
					seq_setMute(voiceNr,0);
				}
				else
				{
					seq_setMute(voiceNr,1);
				}

			}
				break;

			default:
				break;
		}
		modNode_originalValueChanged(paramNr);
	}
}
//-----------------------------------------------------------
/** Parse incoming midi messages and invoke corresponding action
 */
void midiParser_parseMidiMessage(MidiMsg msg)
{
	//only do something if the midi channel is right
	if(midiParser_isValidMidiChannel(msg.status))
	{
		switch(msg.status)
		{
		case NOTE_OFF:
			voiceControl_noteOff(msg.data1, msg.data2);
			break;

		case NOTE_ON:
			if(msg.data2 == 0)
			{
				//zero velocity note off
				voiceControl_noteOff(msg.data1, msg.data2);
			}
			else
			{
				voiceControl_noteOn(msg.data1, msg.data2);
				//Also used in sequencer trigger note function
				//to retrigger the LFOs the Frontpanel AVR needs to know about note ons
				uart_sendFrontpanelByte(msg.status);
				uart_sendFrontpanelByte(msg.data1-NOTE_VOICE1);
				uart_sendFrontpanelByte(msg.data2);
			}
			break;

		case MIDI_CC:
			//record automation if record is turned on
			seq_recordAutomation(frontParser_activeTrack, msg.data1, msg.data2);

			//handle midi data
			midiParser_ccHandler(msg,1);
			//we received a midi cc message from the physical midi in port
			//forward it to the front panel
			uart_sendFrontpanelByte(msg.status);
			uart_sendFrontpanelByte(msg.data1);
			uart_sendFrontpanelByte(msg.data2);
			break;

		case PROG_CHANGE:

			break;

		case MIDI_PITCH_WHEEL:

			break;

		default:
			//unknown
			parserState = IGNORE;
			break;
		}
	}
};
//-----------------------------------------------------------
void midiParser_handleSystemByte(unsigned char data)
{
	switch(data)
	{
	default:
		break;

	case MIDI_CLOCK:
		if(seq_getExtSync())
		{
			seq_sync();
		}
		break;

	case MIDI_START:
		if(seq_getExtSync())
		{
			sync_midiStartStop(1);
		}
		break;

	case MIDI_STOP:
		if(seq_getExtSync())
		{
			sync_midiStartStop(0);
		}
		break;
	}
}
//-----------------------------------------------------------
void midiParser_handleStatusByte(unsigned char data)
{
	// we received another status byte. this means the previous message should be completed
	// so we send it to the message handler
	if(midiParser_isValidMidiChannel(data))
	{
		switch(data)
		{
		// 2 databyte messages
		case NOTE_OFF:
		case NOTE_ON:
		case MIDI_CC:
		case MIDI_PITCH_WHEEL:
		case MIDI_AT:
		case CHANNEL_PRESSURE:

			// store the new status byte
			midiMsg_tmp.status = data;
			//status received, next should be data byte 1
			parserState = MIDI_DATA1;

			// status is followed by 2 data bytes
			msgLength = 2;

			break;
		// 1 databyte messages
		case PROG_CHANGE:
			// store the new status byte
			midiMsg_tmp.status = data;
			//status received, next should be data byte 1
			parserState = MIDI_DATA1;

			// status is followed by 1 data bytes
			msgLength = 1;
			break;

		default:
			//unkown status byte - ignore
			parserState = IGNORE;
			break;
		}
	}
	else
	{
		//not our channel
		parserState = IGNORE;
	}
}
//-----------------------------------------------------------
void midiParser_handleDataByte(unsigned char data)
{
	switch(parserState)
	{
	case MIDI_DATA1:
		midiMsg_tmp.data1 = data;
		parserState = MIDI_DATA2;
		break;

	case MIDI_DATA2:
		midiMsg_tmp.data2 = data;
		//both databytes received. next should be status again
		parserState = MIDI_STATUS;
		midiParser_parseMidiMessage(midiMsg_tmp);
		break;

	case MIDI_STATUS:
		// running status
		midiMsg_tmp.data1 = data;
		parserState = MIDI_DATA2;

		break;

	default:
		//we are expecting no data byte.
		break;
	}
}
//-----------------------------------------------------------
void midiParser_parseUartData(unsigned char data)
{
	// if high byte set its either a status or a system message
	if(data&0x80)
	{
		if( (data&0xf0) == 0xf0)
		{
			//system message
			midiParser_handleSystemByte(data);
		}
		else
		{
			//status byte
			midiParser_handleStatusByte(data);
		}
	}
	else
	{
		//data byte
		midiParser_handleDataByte(data);
	}

}
//-----------------------------------------------------------

