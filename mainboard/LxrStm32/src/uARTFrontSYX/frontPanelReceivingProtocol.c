/*
 * frontPanelReceivingProtocol.c
 *
 * STM front-panel receive parser, SysEx ingress, and load-session receive
 * state. The exported function names keep the legacy frontParser_* prefix
 * until the remaining call sites are migrated.
 *
 *  Created on: 27.04.2012
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



#include "frontPanelReceivingProtocol.h"
#include "MidiMessages.h"
#include "frontPanelSendingProtocol.h"
#include "globals.h"
#include "Preset/EndpointRestore.h"
#include "Preset/MorphEngine.h"
#include "Preset/ParameterArray.h"
#include "sequencer.h"
#include "MidiParser.h"
#include "MidiOutputControl.h"	/* menu MIDI TX queue (Session 039 P3) */
#include "PatternData.h"
#include "Preset/ParameterIngress.h"
#include "Uart.h"
#include "SD_Manager.h"
#include "EuklidGenerator.h"
#include "config.h"
#include "mixer.h"
#include "valueShaper.h"
#include "modulationNode.h"

#include "DrumVoice.h"
#include "CymbalVoice.h"
#include "HiHat.h"
#include "Snare.h"
#include "SomGenerator.h"
#include "TriggerOut.h"
#include <string.h>

void frontParser_handleMidiMessage(void);
static void frontParser_handleSysexData(unsigned char data);
static void frontParser_handleSeqCC();
static void frontParser_handleVoiceMorph(uint8_t slot, uint8_t payload);

#define FLOW_INITIAL_GRANT 4
#define FLOW_ACK_CREDITS 1
#define FRONT_FILE_DONE_TYPE_PATTERN 7
#define FRONT_FILE_DONE_TYPE_PERFORMANCE 8
#define FRONT_FILE_DONE_TYPE_ALL 9
#define FRONT_BACKGROUND_SWAP_ACK_DELAY_TICKS 400U

static uint8_t comm_loadSessionActive = 0;
static uint8_t comm_quietUi = 0;
static uint8_t comm_flowActive = 0;
static uint8_t comm_flowChannel = 0;
static uint8_t comm_flowBudgetRemaining = 0;
static uint8_t frontParser_backgroundSwapPending = 0;
static uint8_t frontParser_backgroundSwapFileType = 0;
static uint8_t frontParser_backgroundSwapPrepared = 0;
static uint8_t frontParser_backgroundSwapPreparedType = 0;
static uint8_t frontParser_backgroundSwapAckDelayActive = 0;
static uint32_t frontParser_backgroundSwapStartTick = 0;
static uint8_t frontParser_fileLoadIngressActive = 0;
static uint8_t frontParser_fileLoadBracketActive = 0;
static uint8_t frontParser_fileLoadEndpointMode = PRESET_KIT_ENDPOINT_BOTH;
static uint8_t frontParser_fileLoadMirrorToTmp = 0;
static uint8_t frontParser_fileLoadPreserveTempPreset = 0;
/* SHIFT+PERF edits use the temporary pattern slot as a one-visit backup for
   each touched track. This state records whether the current Euclid page
   visit is active, which normal pattern it is editing, and which tracks have
   already been snapshotted so we only copy once per track until the user
   leaves the page or explicitly re-selects it to restore. */
static uint8_t frontParser_euklidSnapshotVisitActive = 0;
static uint8_t frontParser_euklidSnapshotTrackMask = 0;
static uint8_t frontParser_euklidSnapshotPattern = 0;
static uint8_t frontParser_euklidSnapshotSteps[NUM_TRACKS];
static uint8_t frontParser_euklidSnapshotRotation[NUM_TRACKS];
static uint8_t frontParser_euklidSnapshotSubStepRotation[NUM_TRACKS];

static void frontParser_beginEuklidSnapshotVisit(void)
{
   uint8_t pattern = pat_normalizePatternNumber(frontParser_shownPattern);

   frontParser_euklidSnapshotVisitActive = (pattern != SEQ_TMP_PATTERN);
   frontParser_euklidSnapshotTrackMask = 0;
   frontParser_euklidSnapshotPattern = pattern;
}

static void frontParser_endEuklidSnapshotVisit(void)
{
   uint8_t track;

   /* On a committed page-exit (navigating away to any mode page other than
      SHIFT+PERF) the pattern edits are already live in normal storage — that
      happened incrementally as the user adjusted rotation/length/steps.
      What we must clean up here is the Euclid engine's *rotation cache*:
      euklid_rotation[] / euklid_subStepRotation[] record the cumulative
      rotation delta applied so far.  If we leave them at their post-rotation
      values the next SHIFT+PERF visit will start from the wrong baseline:
        - euklid_setRotation() computes an incremental delta as
          (new_value - euklid_rotation[track]), so a stale cached value means
          the first encoder tick on next entry would mis-rotate the pattern.
        - The AVR display would show the old (non-zero) rotation value,
          confusing the user into thinking edits are still pending.
      The fix: for every track that was touched during this visit, zero its
      rotation caches via euklid_clearTrackRotation().  The pattern data stays
      as-is (the rotation is now baked into normal storage), and the cache is
      reset to the fresh-start state that euklid_setRotation() expects.
      The mask must be read before being zeroed, so the loop comes first. */
   for(track = 0; track < NUM_TRACKS; track++)
   {
      if(frontParser_euklidSnapshotTrackMask & (uint8_t)(0x01u << track))
      {
         /* Zeroes euklid_rotation[track] and euklid_subStepRotation[track]
            so the next visit computes incremental deltas from 0. */
         euklid_clearTrackRotation(track);
      }
   }

   /* After resetting rotation caches, push updated Euclid params to the AVR
      for the currently viewed track so its display immediately shows rotation
      = 0 rather than the stale post-rotation value.  Only send if at least
      one track was actually edited during this visit — avoids spurious UART
      traffic when the user enters and leaves the page without touching
      anything.  Other tracks (not currently viewed) will refresh automatically
      on the next SEQ_REQUEST_EUKLID_PARAMS when the user selects them. */
   if(frontParser_euklidSnapshotTrackMask)
      frontPanelSending_sendEuklidParamsReply(frontParser_activeTrack);

   frontParser_euklidSnapshotVisitActive = 0;
   frontParser_euklidSnapshotTrackMask = 0;
   frontParser_euklidSnapshotPattern = 0;
}

static void frontParser_snapshotEuklidTrackIfNeeded(uint8_t trackNr)
{
   uint8_t trackBit;

   if(trackNr >= NUM_TRACKS)
      return;
   if(!frontParser_euklidSnapshotVisitActive)
      return;
   if(frontParser_euklidSnapshotPattern == SEQ_TMP_PATTERN)
      return;

   trackBit = (uint8_t)(0x01u << trackNr);
   if(frontParser_euklidSnapshotTrackMask & trackBit)
      return;

   /* The first Euclid/rotation edit to a track during one SHIFT+PERF visit
      must preserve the pre-edit track image in the temp pattern slot. Later
      edits in the same visit must reuse that same backup instead of
      overwriting it, so the copy and the cached Euclid UI values are captured
      exactly once here before any mutation reaches the normal track. */
   pat_copyTrackPattern(trackNr, SEQ_TMP_PATTERN, frontParser_euklidSnapshotPattern);
   frontParser_euklidSnapshotSteps[trackNr] = euklid_getSteps(trackNr);
   frontParser_euklidSnapshotRotation[trackNr] = euklid_getRotation(trackNr);
   frontParser_euklidSnapshotSubStepRotation[trackNr] = euklid_getSubStepRotation(trackNr);
   frontParser_euklidSnapshotTrackMask |= trackBit;
}

static void frontParser_restoreEuklidSnapshotTracks(void)
{
   uint8_t track;

   if(!frontParser_euklidSnapshotVisitActive)
      return;
   if(frontParser_euklidSnapshotPattern == SEQ_TMP_PATTERN)
   {
      frontParser_euklidSnapshotTrackMask = 0;
      return;
   }

   /* Re-selecting SHIFT+PERF during the same visit is a rollback request, not
      a page-navigation request. Every track backed up during this visit is
      restored from temp to the normal pattern so the user gets the exact
      pre-edit track image and cached Euclid page values back in one action. */
   for(track = 0; track < NUM_TRACKS; track++)
   {
      if(frontParser_euklidSnapshotTrackMask & (uint8_t)(0x01u << track))
      {
         pat_copyTrackPattern(track, frontParser_euklidSnapshotPattern, SEQ_TMP_PATTERN);
         euklid_restoreTrackState(track,
                                  frontParser_euklidSnapshotPattern,
                                  frontParser_euklidSnapshotSteps[track],
                                  frontParser_euklidSnapshotRotation[track],
                                  frontParser_euklidSnapshotSubStepRotation[track]);
      }
   }

   frontParser_euklidSnapshotTrackMask = 0;

   frontParser_activeFrontTrack = frontParser_activeTrack;
   frontPanelSending_updateTrackLeds(frontParser_activeTrack,
                                     frontParser_euklidSnapshotPattern,
                                     frontParser_activeStep);
   frontPanelSending_sendEuklidParamsReply(frontParser_activeTrack);
}

static void frontParser_beginFileLoadIngress(uint8_t bracketed)
{
   frontParser_fileLoadIngressActive = 1;
   if(bracketed)
      frontParser_fileLoadBracketActive = 1;

   preset_setIngressTarget(SEQ_PARAM_INGRESS_NORMAL_KIT_ENDPOINT);
   preset_setAutomationIngressTarget(SEQ_AUTOMATION_INGRESS_NONE);
}

static void frontParser_endFileLoadIngress(void)
{
   frontParser_fileLoadIngressActive = 0;
   frontParser_fileLoadBracketActive = 0;
   frontParser_fileLoadMirrorToTmp = 0;
   frontParser_fileLoadPreserveTempPreset = 0;
   frontParser_fileLoadEndpointMode = PRESET_KIT_ENDPOINT_BOTH;
   preset_setIngressTarget(SEQ_PARAM_INGRESS_CURRENT_IMAGE);
   preset_setAutomationIngressTarget(SEQ_AUTOMATION_INGRESS_NONE);
}

static void frontParser_clearVoiceLoading(uint8_t voice)
{
   if(voice >= 5)
      seq_voicesLoading &= (uint8_t)~0x60;
   else
      seq_voicesLoading &= (uint8_t)~(0x01 << voice);
}

static uint16_t frontParser_activeNrpnNumber = 0;
uint8_t frontParser_originalCcValues[0xff];
uint8_t midi_envPosition[6];

static ModulationNode* frontParser_getLfoModNode(uint8_t voice)
{
   switch(voice)
   {
      case 0:
      case 1:
      case 2:
         return &voiceArray[voice].lfo.modTarget;
      case 3:
         return &snareVoice.lfo.modTarget;
      case 4:
         return &cymbalVoice.lfo.modTarget;
      case 5:
      default:
         return &hatVoice.lfo.modTarget;
   }
}

static void frontParser_setLfoModAmount(uint8_t voice, uint8_t value)
{
   ModulationNode *node;

   if(voice >= 6)
      return;

   node = frontParser_getLfoModNode(voice);
   node->amount = value / 127.f;
   modNode_updateValue(node, node->lastVal);
}

static void frontParser_setVelocityModAmount(uint8_t voice, uint8_t value)
{
   if(voice >= 6)
      return;

   velocityModulators[voice].amount = value / 127.f;
   modNode_updateValue(&velocityModulators[voice],
                       velocityModulators[voice].lastVal);
}

static inline float calcPitchModAmount(uint8_t data2)
{
   const float val = data2/127.f;
   return val*val*PITCH_AMOUNT_FACTOR;
}

//-----------------------------------------------------------
static void frontParser_nrpnHandler(uint16_t value)
{
   MidiMsg msg2;
   msg2.status = MIDI_CC2;
   msg2.data1 = frontParser_activeNrpnNumber;
   msg2.data2 = value;
   frontParser_applyParameterCommand(msg2,true);
}
//-----------------------------------------------------------
/* Apply an internal CC/CC2-shaped parameter command to live DSP state. */
void frontParser_applyParameterCommand(MidiMsg msg, uint8_t updateOriginalValue)
{
   if(msg.status == MIDI_CC)
   {
   
      const uint16_t paramNr = msg.data1-1;
      if(updateOriginalValue) {
         frontParser_originalCcValues[paramNr+1] = msg.data2;
         preset_storeParameterIngress(paramNr+1, msg.data2);
      }
   
      switch(msg.data1)
      {
      
         case CC_MOD_WHEEL:
         
            break;
      
         case NRPN_DATA_ENTRY_COARSE:
            frontParser_nrpnHandler(msg.data2);
            return;
            break;
      
         case NRPN_FINE:
            frontParser_activeNrpnNumber &= ~(0x7f);	//clear lower 7 bit
            frontParser_activeNrpnNumber |= (msg.data2&0x7f);
            break;
      
         case NRPN_COARSE:
            frontParser_activeNrpnNumber &= 0x7f;	//clear upper 7 bit
            frontParser_activeNrpnNumber |= (msg.data2<<7);
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
            //clear lower nibble
               snareVoice.osc.midiFreq &= 0xff00;
            //set lower nibble
               snareVoice.osc.midiFreq |= msg.data2;
               osc_recalcFreq(&snareVoice.osc);
            }
            break;
      
         case F_OSC5_FINE:
            {
            //clear lower nibble
               cymbalVoice.osc.midiFreq &= 0xff00;
            //set lower nibble
               cymbalVoice.osc.midiFreq |= msg.data2;
               osc_recalcFreq(&cymbalVoice.osc);
            }
            break;
      
         case F_OSC6_FINE:
            {
            //clear lower nibble
               hatVoice.osc.midiFreq &= 0xff00;
            //set lower nibble
               hatVoice.osc.midiFreq |= msg.data2;
               osc_recalcFreq(&hatVoice.osc);
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
            frontParser_setLfoModAmount((uint8_t)(msg.data1-AMOUNT_LFO1), msg.data2);
            break;
         case AMOUNT_LFO4:
            frontParser_setLfoModAmount(3, msg.data2);
            break;
         case AMOUNT_LFO5:
            frontParser_setLfoModAmount(4, msg.data2);
            break;
         case AMOUNT_LFO6:
            frontParser_setLfoModAmount(5, msg.data2);
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
   } //msg.status == MIDI_CC
   
   else //MIDI_CC2
   {
      const uint16_t paramNr = msg.data1+1 + 127;
   
      if(updateOriginalValue) {
         frontParser_originalCcValues[paramNr] = msg.data2;
         preset_storeParameterIngress(paramNr, msg.data2);
      }
      switch(msg.data1)
      {
      
         case CC2_TRANS1_WAVE:
            //voiceArray[0].transGen.waveform = msg.data2;
            transient_setWaveform(&voiceArray[0].transGen, msg.data2);
            break;
      
         case CC2_TRANS1_VOL:
            voiceArray[0].transGen.volume = msg.data2/127.f;
            break;
      
         case CC2_TRANS1_FREQ:
            voiceArray[0].transGen.pitch = 1.f + ((msg.data2/33.9f)-0.75f) ;// range about  0.25 to 4 => 1/4 to 1*4
            break;
      
         case CC2_TRANS2_WAVE:
            //voiceArray[1].transGen.waveform = msg.data2;
            transient_setWaveform(&voiceArray[1].transGen, msg.data2);
            break;
      
         case CC2_TRANS2_VOL:
            voiceArray[1].transGen.volume = msg.data2/127.f;
            break;
      
         case CC2_TRANS2_FREQ:
            voiceArray[1].transGen.pitch = 1.f + ((msg.data2/33.9f)-0.75f) ;
            break;
      
         case CC2_TRANS3_WAVE:
            //voiceArray[2].transGen.waveform = msg.data2;
            transient_setWaveform(&voiceArray[2].transGen, msg.data2);
            break;
      
         case CC2_TRANS3_VOL:
            voiceArray[2].transGen.volume = msg.data2/127.f;
            break;
      
         case CC2_TRANS3_FREQ:
            voiceArray[2].transGen.pitch = 1.f + ((msg.data2/33.9f)-0.75f) ;
            break;
      
         case CC2_TRANS4_WAVE:
            //snareVoice.transGen.waveform = msg.data2;
            transient_setWaveform(&snareVoice.transGen, msg.data2);
            break;
      
         case CC2_TRANS4_VOL:
            snareVoice.transGen.volume = msg.data2/127.f;
            break;
         case CC2_TRANS4_FREQ:
            snareVoice.transGen.pitch = 1.f + ((msg.data2/33.9f)-0.75f) ;
            break;
      
         case CC2_TRANS5_WAVE:
            //cymbalVoice.transGen.waveform = msg.data2;
            transient_setWaveform(&cymbalVoice.transGen, msg.data2);
            break;
      
         case CC2_TRANS5_VOL:
            cymbalVoice.transGen.volume = msg.data2/127.f;
            break;
         case CC2_TRANS5_FREQ:
            cymbalVoice.transGen.pitch = 1.f + ((msg.data2/33.9f)-0.75f) ;
            break;
      
         case CC2_TRANS6_WAVE:
            //hatVoice.transGen.waveform = msg.data2;
            transient_setWaveform(&hatVoice.transGen, msg.data2);
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
            //SVF_reset(&voiceArray[msg.data1-CC2_FILTER_TYPE_1].filter);
            break;
         case CC2_FILTER_TYPE_4:
            snareVoice.filterType = msg.data2 + 1; // +1 because 0 is filter off which results in silence
            //SVF_reset(&snareVoice.filter);
            break;
         case CC2_FILTER_TYPE_5:
         //cymbal filter
            cymbalVoice.filterType = msg.data2+1;
            //SVF_reset(&cymbalVoice.filter);
            break;
         case CC2_FILTER_TYPE_6:
         //Hihat filter
            hatVoice.filterType = msg.data2+1;
            //SVF_reset(&hatVoice.filter);
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
            frontParser_setVelocityModAmount((uint8_t)(msg.data1-CC2_VELO_MOD_AMT_1),
                                            msg.data2);
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
      
      //--AS
         case CC2_ENVELOPE_POSITION_1:
         case CC2_ENVELOPE_POSITION_2:
         case CC2_ENVELOPE_POSITION_3:
         case CC2_ENVELOPE_POSITION_4:
         case CC2_ENVELOPE_POSITION_5:
         case CC2_ENVELOPE_POSITION_6:
         //--AS set the note override for the voice. 0 means use the note value, anything else means
         // that the note will always play with that note
            midi_envPosition[msg.data1-CC2_ENVELOPE_POSITION_1] = msg.data2;
            drumVoice_setEnvelope(msg.data1-CC2_ENVELOPE_POSITION_1,msg.data2);
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
         case CC2_MAC1_DST1:       // bc: these need to be handled with a separate status message like LFO dest's
         case CC2_MAC1_DST2:       // this happens in frontPanelParser
         case CC2_MAC2_DST1:
         case CC2_MAC2_DST2:
            break;
         /* Session 025: legacy macro amount messages are ignored during the
            deprecation test so the old performance-macro path cannot apply. */
#if 0
         case CC2_MAC1_DST1_AMT:       // bc: change perf macro destination amounts
            if (msg.data2)
               macroModulators[0].amount = ((msg.data2+1)/64.f)-1;
            else
               macroModulators[0].amount = -1;
            modNode_updateValue(&macroModulators[0],macroModulators[0].lastVal);
            break;
         case CC2_MAC1_DST2_AMT:
            if (msg.data2)
               macroModulators[1].amount = ((msg.data2+1)/64.f)-1;
            else
               macroModulators[1].amount = -1;
            modNode_updateValue(&macroModulators[1],macroModulators[1].lastVal);
            break;
         case CC2_MAC2_DST1_AMT:
            if (msg.data2)
               macroModulators[2].amount = ((msg.data2+1)/64.f)-1;
            else
               macroModulators[2].amount = -1;
            modNode_updateValue(&macroModulators[2],macroModulators[2].lastVal);
            break;
         case CC2_MAC2_DST2_AMT:
            if (msg.data2)
               macroModulators[3].amount = ((msg.data2+1)/64.f)-1;
            else
               macroModulators[3].amount = -1;
            modNode_updateValue(&macroModulators[3],macroModulators[3].lastVal);
            break;            
#endif
         default:
            break;
      }
      modNode_originalValueChanged(paramNr);
   }
}


extern MidiMsg frontParser_command;
extern uint8_t frontParser_sysexActive;

/* Flow-control bookkeeping for the transitional receive-side parser. */
static uint8_t frontParser_isFlowMessage(uint8_t status, uint8_t data1)
{
   return (status == FRONT_SEQ_CC)
      && (data1 >= FRONT_SEQ_FLOW_BEGIN)
      && (data1 <= FRONT_SEQ_FLOW_ABORT);
}

static void frontParser_sendFlowGrant(uint8_t channel, uint8_t credits)
{
   if((frontParser_sysexActive != SYSEX_INACTIVE) && (channel != FLOW_CH_LOAD_SESSION))
   {
      comm_flowActive = 0;
      comm_flowBudgetRemaining = 0;
      return;
   }

   frontPanelSending_sendFlowGrant(channel, credits);
}

//#if 0
//static void frontParser_sendFlowGrantWait(uint8_t channel, uint8_t credits)
//{
//   frontPanelSending_sendFlowGrantWait(channel, credits);
//}
//#endif

//#if 0
//static void frontParser_sendPrfCacheStatus(uint8_t command, uint8_t status)
//{
//   frontPanelSending_sendPrfCacheStatus(command, status);
//}
//#endif

static void frontParser_suspendCreditFlowForSysex()
{
   comm_flowActive = 0;
   comm_flowBudgetRemaining = 0;
}

static void frontParser_flowMessageApplied()
{
   if(!comm_flowActive || frontParser_isFlowMessage(frontParser_command.status, frontParser_command.data1))
      return;

   if(comm_flowBudgetRemaining)
      comm_flowBudgetRemaining--;

   if(comm_flowBudgetRemaining == 0)
   {
      comm_flowBudgetRemaining = FLOW_INITIAL_GRANT;
      frontParser_sendFlowGrant(comm_flowChannel, FLOW_INITIAL_GRANT);
   }
}

uint8_t frontParser_isQuietUi()
{
   /* Query whether the parser is currently suppressing ordinary UI traffic. */
   return comm_quietUi;
}

static uint8_t frontParser_backgroundSwapTempPlaybackReady(void)
{
   uint8_t track;

   if(seq_activePattern != SEQ_TMP_PATTERN)
      return 0;

   for(track=0; track<NUM_TRACKS; track++)
   {
      if(seq_perTrackActivePattern[track] != SEQ_TMP_PATTERN)
         return 0;
   }

   if(preset_tempPlaybackSwitchState.patternOnlyTempPlayback)
      return preset_allVoiceSourcesUseNormal();

   return preset_allVoiceSourcesUseTmp();
}

static uint8_t frontParser_fileLoadHasPresetEndpoints(uint8_t fileType)
{
   return fileType != FRONT_FILE_DONE_TYPE_PATTERN;
}

static uint8_t frontParser_fileLoadShouldPreserveTempPreset(uint8_t fileType)
{
   return frontParser_backgroundSwapPrepared
       && (frontParser_backgroundSwapPreparedType == fileType)
       && ((fileType == FRONT_FILE_DONE_TYPE_PERFORMANCE)
        || (fileType == FRONT_FILE_DONE_TYPE_ALL));
}

static void frontParser_mirrorNormalPresetEndpointsToTmp(uint8_t endpointMode)
{
   /* Ordinary loads mirror the loaded endpoint subset straight into temp so
      temporary storage always stays the authoritative “last loaded preset”
      image. Protected .prf/.all background loads are the only exception. */
   preset_setEndpointIngressSuppressed(1);
   preset_copyKitEndpoints(&preset_tmpKitState,
                           &preset_normalKitState,
                           endpointMode);
   preset_setEndpointIngressSuppressed(0);
   preset_invalidateLiveMorphApplyCache(PRESET_MORPH_IMAGE_TMP);
   preset_tempPlaybackSwitchState.resnapshotTmpFromNormalPending = 0;
   preset_setTempPresetPlaybackActive(0);

   if(preset_isTmpKitActive() || !preset_allVoiceSourcesUseNormal())
      preset_reapplyCurrentPlaybackSources();
}

void frontParser_serviceBackgroundSwapAck(void)
{
   if(!frontParser_backgroundSwapPending)
      return;

   if(!frontParser_backgroundSwapAckDelayActive)
   {
      if(!frontParser_backgroundSwapTempPlaybackReady())
         return;

      frontParser_backgroundSwapAckDelayActive = 1;
      frontParser_backgroundSwapStartTick = systick_ticks;
      return;
   }

   if((uint32_t)(systick_ticks - frontParser_backgroundSwapStartTick) < FRONT_BACKGROUND_SWAP_ACK_DELAY_TICKS)
      return;

   frontParser_backgroundSwapPending = 0;
   frontParser_backgroundSwapAckDelayActive = 0;
   frontPanelSending_sendPriorityTriplet(FRONT_SEQ_CC,
                                         FRONT_SEQ_BACKGROUND_SWAP_DONE,
                                         frontParser_backgroundSwapFileType);
}
/* Front-panel receive state: byte counter, current message assembly, sysex
   mode, and the current display/track selection. */
uint8_t frontParser_rxCnt=0;

/* Currently selected parameter / current assembled front-panel command. */
MidiMsg frontParser_command;

/* Currently active front-panel track and the visible sequence display state. */
uint8_t frontParser_activeFrontTrack=0;

/* Sysex mode gates all non-sysex traffic while a front-panel transfer is open. */
uint8_t frontParser_sysexActive=0;
/* Two 7-bit bytes assembled into one 14-bit value. */
uint16_t frontParser_twoByteData=0;
static uint8_t frontParser_globalMorphLsb = 0;
static uint8_t frontParser_voiceMorphLsb[6] = {0};

uint8_t frontParser_sysexBuffer[16];

/* Counter for incoming sequencer step data packets. */
uint16_t frontParser_sysexSeqStepNr=0;

/* Current track/pattern display selection and step-copy source. */
uint8_t frontParser_activeTrack=0;	/** the active track on the Frontpanel. track specific messages refer to the track selected with this command*/
uint8_t frontParser_shownPattern = 0;
uint8_t frontParser_activeStep=0;

uint8_t frontParser_stepCopySource=0;

/* Main front-panel receive entry point. */
void frontParser_parseUartData(unsigned char data)
{

	//TODO der ganze sysex kram kann sicher noch optimiert werden
	//das das nicht andauernd abgefragt werden muss

	// if high bit is set, a new message starts
   if(data&0x80)
   {
   	//reset the byte counter
      frontParser_rxCnt = 0;
      frontParser_command.status = data;
      if(data==SYSEX_START)
      {
         frontParser_suspendCreditFlowForSysex();
         frontParser_sysexActive = SYSEX_ACTIVE_MODE_NONE;
         uart_clearFrontFifo();
         
      	//send SYSEX_START as ACK
         frontPanelSending_sendSysexStartAck();
      }
      else if(data==SYSEX_END)
      {
         frontPanelSending_sendSysexEndAck();
         frontParser_sysexActive = SYSEX_INACTIVE;
      }
      else if(data==PATCH_RESET)
      {
         /* AVR performs a UI-level disable for SHIFT+PLAY, but STM remains the
            authoritative guard. If temp preset playback is still the active
            sound source, ignore PATCH_RESET here so a stale front-panel state
            cannot trigger a forbidden temp->normal restore. PATCH_RESET stays
            on the wire, but its meaning is now an STM-owned storage restore. */
         (void)preset_reloadNormalFromTemporaryPreset();
      }
      else if(data==FRONT_CALLBACK_ACK)
      {
         frontPanelSending_sendCallbackAck();
      }
      else
      {
         frontParser_sysexActive = SYSEX_INACTIVE;
      }
   }
   else // data byte
   {
      if(frontParser_command.status == SYSEX_START)
      {
         frontParser_handleSysexData(data);
      }
      else if(frontParser_rxCnt==0) //normal operation
      {
      	//parameter nr
         frontParser_command.data1 = data;
         frontParser_rxCnt++;
      }
      else
      {
      	//parameter value
         frontParser_command.data2 = data;
      	//message received. process the front-panel command
         frontParser_handleMidiMessage();
      }
   }
};

//------------------------------------------------------
// This is called when we are in sysex mode and are receiving the sysex bytes
static void frontParser_handleSysexData(unsigned char data)
{
	//SYSEX
	// this is not a correct sysex implementation.
	// in the front panel communication it is used to send sequencer data for preset saving
	// while the preset saving is active, no other data has to be send over the uart!!!
	// so when the SYSEX_START is received, all other communication is stopped until theSYSEX_END message is received
	// no abort and no manufacturer id is supported

	//we have received sysex data
	//first we receive a mode message
	//SYSEX_REQUEST_STEP_DATA or SYSEX_SEND_STEP_DATA
	//then the corresponding data

   switch(frontParser_sysexActive) {
      case SYSEX_REQUEST_PATTERN_DATA:
      //1 byte = pattern nr
      //send back next and repeat
         frontPanelSending_sendPatternDataReply(data);
         break;
   
      case SYSEX_REQUEST_MAIN_STEP_DATA:
      //we expect a 2 byte message containing a step nr
      //which tells us which main step data to send back
         frontParser_rxCnt++;
         if(frontParser_rxCnt == 1)
         {
         //first nibble received -> upper nibble
            frontParser_twoByteData = data<<7;
         }
         else
         {
         //second nibble -> lower nibble
            frontParser_twoByteData |= data&0x7f;
         //reset rxCount for next 2 nibble message
            frontParser_rxCnt = 0;
         //we have received a complete 2 nibble step nr
         //send data back to front
            seq_sendMainStepInfoToFront(frontParser_twoByteData);
         }
         break;
   
      case SYSEX_REQUEST_STEP_DATA:
      //we expect a 2 byte message containing a step nr
      //which tells us which step data to send back
         frontParser_rxCnt++;
      
         if(frontParser_rxCnt == 1)
         {
         //first nibble received -> upper nibble
            frontParser_twoByteData = data<<7;
         }
         else
         {
         //second nibble -> lower nibble
            frontParser_twoByteData |= data&0x7f;
         //reset rxCount for next 2 nibble message
            frontParser_rxCnt = 0;
         //we have received a complete 2 nibble step nr
         //send data back to front
            seq_sendStepInfoToFront(frontParser_twoByteData);
         }
      
         break;
   
      
      case SYSEX_RECEIVE_MAIN_STEP_DATA:
         if(frontParser_rxCnt<3)
         {
            frontParser_sysexBuffer[frontParser_rxCnt++] = data;
         }
         else
         {
            frontParser_sysexBuffer[frontParser_rxCnt++] = data;
         
         //calculate the step pattern and track indices
            uint8_t currentTrack = (frontParser_sysexBuffer[0]>>3)&0x07;
            uint8_t currentPattern = (frontParser_sysexBuffer[0]&0x07);
         
            uint16_t mainStepData = frontParser_sysexBuffer[1] | frontParser_sysexBuffer[2]<<7 | frontParser_sysexBuffer[3]<<14;
         
         // File receive writes directly to normal pattern storage.
            PatternSet* patternSet = &pat_patternSet;
            patternSet->pat_mainSteps[currentPattern][currentTrack] = mainStepData;
         
            
            frontParser_rxCnt = 0;
            frontPanelSending_sendSysexReceiveAck(SYSEX_RECEIVE_MAIN_STEP_DATA);
         }
         break;
      case SYSEX_RECEIVE_PAT_CHAIN_DATA:
         if(frontParser_rxCnt<1)
         {
            frontParser_sysexBuffer[frontParser_rxCnt++] = data;
         }
         else
         {
            frontParser_sysexBuffer[frontParser_rxCnt++] = data;
            //calculate the pattern index. we expect 'next', then all 'repeat' for pat 0-7
            const uint8_t currentPattern	= frontParser_sysexSeqStepNr;
            uint8_t next = frontParser_sysexBuffer[0];
            uint8_t repeat = frontParser_sysexBuffer[1];
            // File receive writes directly to normal pattern storage.
            PatternSet* patternSet = &pat_patternSet;
            patternSet->pat_patternSettings[currentPattern].nextPattern = next;
            patternSet->pat_patternSettings[currentPattern].changeBar = repeat;
            //inc the step counter
            frontParser_sysexSeqStepNr++;
            frontParser_rxCnt = 0;
         
         }
         frontPanelSending_sendSysexReceiveAck(SYSEX_RECEIVE_PAT_CHAIN_DATA);
         break;
      case SYSEX_RECEIVE_PAT_LEN_DATA:
      // --AS same as above but we are receiving length data for each pattern
         if(frontParser_rxCnt<1)
         {
            frontParser_sysexBuffer[frontParser_rxCnt++] = data;
         }
         else
         {
            frontParser_sysexBuffer[frontParser_rxCnt++] = data;
         //calculate the step pattern and track indices
            uint8_t currentTrack = (frontParser_sysexBuffer[0]>>3)&0x07;
            uint8_t currentPattern = (frontParser_sysexBuffer[0]&0x07);
         // File receive writes directly to normal pattern storage.
            PatternSet* patternSet = &pat_patternSet;
         
            patternSet->pat_patternLengthRotate[currentPattern][currentTrack].length = data;
            
            frontParser_rxCnt = 0;
            frontPanelSending_sendSysexReceiveAck(SYSEX_RECEIVE_PAT_LEN_DATA);
         }
         break;
         
      case SYSEX_RECEIVE_PAT_SCALE_DATA:
       // --BC same as above but we are receiving scale data for each pattern
         if(frontParser_rxCnt<1)
         {
            frontParser_sysexBuffer[frontParser_rxCnt++] = data;
         }
         else
         {
         
            frontParser_sysexBuffer[frontParser_rxCnt++] = data;
         //calculate the step pattern and track indices
            uint8_t currentTrack = (frontParser_sysexBuffer[0]>>3)&0x07;
            uint8_t currentPattern = (frontParser_sysexBuffer[0]&0x07);
            /*
            const uint8_t currentPattern	= frontParser_sysexSeqStepNr / 7;
            const uint8_t currentTrack  	= frontParser_sysexSeqStepNr - currentPattern*7;
            */
         
         // File receive writes directly to normal pattern storage.
            PatternSet* patternSet = &pat_patternSet;
         
            patternSet->pat_patternLengthRotate[currentPattern][currentTrack].scale = data;
         
         
            //inc the step counter
            frontParser_sysexSeqStepNr++;
            frontParser_rxCnt = 0;
         
         // File receive no longer stages a background pattern switch here.
         /*
            if( seq_isRunning() && (frontParser_sysexSeqStepNr == NUM_TRACKS*NUM_PATTERN)) {
               preset_tempPlaybackSwitchState.newPatternAvailable = 1;
            }
            */
            frontPanelSending_sendSysexReceiveAck(SYSEX_RECEIVE_PAT_SCALE_DATA);
         }
         
         break;
      
   
      case SYSEX_RECEIVE_STEP_DATA:
      // we expect a bunch of 8 byte sysex message containing new step data for the sequencer
      // beginning with step 0 up to NUMBER_STEPS*NUM_TRACKS*NUM_PATTERN = 128*7*8 = 7168 steps
         if(frontParser_rxCnt<7)
         {
            frontParser_sysexBuffer[frontParser_rxCnt++] = data;
         }
         else
         {
         //now we have to distribute the MSBs to the sysex data
            uint8_t i;
            for(i=0;i<7;i++)
            {
               frontParser_sysexBuffer[i] |= ((data&(1<<i))<<(7-i));
            
            }
         
         //calculate the step pattern and track indices
            const uint8_t absPat 	= frontParser_sysexSeqStepNr/128;
            const uint8_t currentTrack 		= absPat / 8;
            const uint8_t currentPattern 	= absPat - currentTrack*8;
            const uint8_t currentStep		= frontParser_sysexSeqStepNr - absPat*128;
         
            PatternSet* patternSet = &pat_patternSet;
         /*
            if((seq_newPatternVoiceArray&(0x01<<currentTrack))==0)
            { // track is not in the array, use current step data instead
               frontParser_sysexBuffer[0] = patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].volume;
               frontParser_sysexBuffer[1] = patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].prob;
               frontParser_sysexBuffer[2] = patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].note;
               frontParser_sysexBuffer[3] = patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].param1Nr;
               frontParser_sysexBuffer[4] = patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].param1Val;
               frontParser_sysexBuffer[5] = patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].param2Nr;
               frontParser_sysexBuffer[6] = patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].param2Val;
            }
         */
         // File receive writes directly to normal pattern storage.
            patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].volume 	= frontParser_sysexBuffer[0];
            patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].prob 	= frontParser_sysexBuffer[1];
            patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].note 	= frontParser_sysexBuffer[2];
            patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].param1Nr = frontParser_sysexBuffer[3];
            patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].param1Val 	= frontParser_sysexBuffer[4];
            patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].param2Nr 	= frontParser_sysexBuffer[5];
            patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].param2Val 	= frontParser_sysexBuffer[6];
         //signal that a new data chunk is available
         //frontParser_newSeqDataAvailable = 1;
         //reset receive counter for next chunk
            frontParser_rxCnt = 0;
         
         //inc the step counter
            frontParser_sysexSeqStepNr++;
         }
         break;  
         
      case SYSEX_BEGIN_PATTERN_TRANSMIT:
        // we expect a bunch of 8 byte sysex message containing new step data for the sequencer
        // beginning with step 0 up to NUMBER_STEPS*NUM_TRACKS*NUM_PATTERN = 128*7*8 = 7168 steps
         if(frontParser_rxCnt<8)
         {
            frontParser_sysexBuffer[frontParser_rxCnt++] = data;
         }
         else
         {
            uint8_t currentTrack = (frontParser_sysexBuffer[0]>>3)&0x07;
            uint8_t currentPattern = (frontParser_sysexBuffer[0]&0x07);
            uint8_t currentStep = frontParser_sysexSeqStepNr;//frontParser_sysexBuffer[2];
            
            //now we have to distribute the MSBs to the sysex data
            uint8_t i;
            for(i=0;i<7;i++)
            {
               frontParser_sysexBuffer[i] = frontParser_sysexBuffer[i+1];
               frontParser_sysexBuffer[i] |= ((data&(1<<i))<<(7-i));
            
            }
         
            PatternSet* patternSet = &pat_patternSet;
            
         // File receive writes directly to normal pattern storage.
         
            patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].volume 	= frontParser_sysexBuffer[0];
            patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].prob 	= frontParser_sysexBuffer[1];
            patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].note 	= frontParser_sysexBuffer[2];
            patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].param1Nr = frontParser_sysexBuffer[3];
            patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].param1Val 	= frontParser_sysexBuffer[4];
            patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].param2Nr 	= frontParser_sysexBuffer[5];
            patternSet->pat_subStepPattern[currentPattern][currentTrack][currentStep].param2Val 	= frontParser_sysexBuffer[6];
            //signal that a new data chunk is available
            //frontParser_newSeqDataAvailable = 1;
            //reset receive counter for next chunk
            frontParser_rxCnt = 0;
            
            if(frontParser_sysexSeqStepNr<127)
            {
               frontParser_sysexSeqStepNr++;
               frontPanelSending_sendSysexStepAck();
            }
            else
            {
               frontPanelSending_sendSysexBeginPatternTransmitAck();
            }
         }
         break;  
      
      case SYSEX_ACTIVE_MODE_NONE:
      default:
      
      //we received a mode message -> set the active mode
         frontParser_sysexActive = data;
         frontParser_sysexSeqStepNr = 0;
         frontParser_rxCnt = 0;
         break;
   }
}
static void frontParser_handleVoiceMorph(uint8_t slot, uint8_t payload)
{
   uint8_t voice;

   if(slot < 6)
   {
      frontParser_voiceMorphLsb[slot] = (uint8_t)(payload & 0x7f);
      return;
   }

   if(slot < 12)
   {
      uint8_t morphAmount;

      voice = (uint8_t)(slot - 6);
      morphAmount = (uint8_t)(frontParser_voiceMorphLsb[voice]
         | ((payload & 0x01) << 7));
      seq_setVoiceMorphAmount(voice, morphAmount);
   }
}

/* Handle a fully assembled front-panel message after byte decoding. */
void frontParser_handleMidiMessage(void)
{
   switch(frontParser_command.status)
   {
      case PARAM_RESTORE_READY:
         preset_tmpKitHandshakeReady = 1;
         return;

      case PARAM_RESTORE_ACK:
         preset_tmpKitHandshakeAck = 1;
         return;

      default:
         break;
   }

   switch(frontParser_command.status)
   {
      case PRF_RESTORE_PARAM_CC:
      case PRF_RESTORE_PARAM_CC2:
         {
            uint16_t paramNr = frontParser_command.data1;
            if(frontParser_command.status == PRF_RESTORE_PARAM_CC2)
               paramNr += 128;

            /* PRF_RESTORE_PARAM_* carries raw kit/front endpoint bytes. These
               are not live low MIDI CC numbers, so do not apply the +1 offset. */
            preset_storeParameterIngress(paramNr, frontParser_command.data2);
         }
         break;

      case PRF_RESTORE_MORPH_CC:
      case PRF_RESTORE_MORPH_CC2:
         {
            uint16_t paramNr = frontParser_command.data1;
            if(frontParser_command.status == PRF_RESTORE_MORPH_CC2)
               paramNr += 128;

            /* RESTORE: Store morph parameter endpoint bytes into the endpoint image
               selected by the current transfer context. These raw selector bytes are
               distinct from the resolved automation target sideband messages. */
            uint8_t currentTarget = preset_getIngressTarget();
            if(paramNr < END_OF_SOUND_PARAMETERS)
            {
               if(currentTarget == SEQ_PARAM_INGRESS_CURRENT_IMAGE)
               {
                  preset_storeMorphParameterIngress(paramNr, frontParser_command.data2);
               }
               else
               {
                  preset_normalKitState.morphEndpointParams[paramNr] = frontParser_command.data2;
               }
            }
         }
         break;

      case FRONT_SEQ_VOICE_MORPH:
         frontParser_handleVoiceMorph(frontParser_command.data1,
                                      frontParser_command.data2);
         break;

      case FRONT_CC_MACRO_TARGET: //frontParser_command.status
         {
            /* Session 025: legacy macro packets are intentionally ignored while
               the macro feature is being deprecated. Keep the opcode comment
               block here only as historical context for later removal. */
#if 0
            uint8_t applyLive = preset_shouldApplyIngressToLive();

            /* MACRO_CC message structure
            byte1 - status byte 0xaa as above
            byte2, data1 byte: xtta aa-b : tt= top level macro value sent (2 macros exist now, we can do 2 more if we want)
                                           aaa= macro destination value sent (4 destinations exist now, can do 8)
                                           b=macro mod target value top bit
                                           I have left a blank bit above this to make it easier to make more than 255 kit parameters
                                           if we ever want to take on that can of worms

            byte3, data2 byte: xbbb bbbb : b=macro mod target value lower 7 bits or top level value full
            */

            uint8_t upper = frontParser_command.data1;
            uint8_t lower = frontParser_command.data2;
            //sequencer_sendVMorph(0,(uint8_t)(lower*macroModulators[0].amount));
            if (upper&0x20)
            {
               float value = ((float)(lower))/127.f;
               // top level macro amount message received
               if(applyLive)
               {
                  modNode_updateValue(&macroModulators[0],(value));
                  modNode_updateValue(&macroModulators[1],(value));
               }
            }
            else if (upper&0x40)
            {
               float value = ((float)(lower))/127.f;
               // top level macro amount message received
               if(applyLive)
               {
                  modNode_updateValue(&macroModulators[2],(value));
                  modNode_updateValue(&macroModulators[3],(value));
               }
            }
            else
            {
               // macro destination message
               uint16_t value = (uint16_t)( ( (upper&0x03)<<8) | lower);
               uint8_t whichModDest = (uint8_t)( 0x07&(upper>>2) ); // whichModDest 0,1,2,3 mac1d1,mac1d2,mac2d1,mac2d2
               preset_storeMacroDestinationIngress(whichModDest, value);
               if(applyLive)
               {
                  modNode_setDestination(&macroModulators[whichModDest], value);
                  modNode_updateValue(&macroModulators[whichModDest],macroModulators[whichModDest].lastVal);
               }
            }
#endif
         
         }
         break; // case FRONT_CC_LFO_TARGET
      //SEQ MESSAGES
      case FRONT_SEQ_CC: // frontParser_command.status
         frontParser_handleSeqCC();
         break;
   
      case SAMPLE_CC:
         switch(frontParser_command.data1)
         {
         
            case FRONT_SAMPLE_START_UPLOAD:
            {
               /* Blocking STM-side import. The AVR releases its SD SPI pins
                  before sending this command; STM stops the sequencer before
                  erasing/programming internal flash sectors 8..11, initializes
                  SD access for the import, then reports completion with an
                  explicit SAMPLE_UPLOAD_RESULT packet. */
               seq_setRunning(0);
               sampleMemory_init();
               uint8_t sampleStatus = sampleMemory_loadSamples();
               FLASH_Lock();
            
               frontPanelSending_sendSampleUploadResult(sampleStatus);
               break;
            }
         
            case FRONT_SAMPLE_COUNT:
               /* Count reply lets the AVR waveform menu know exactly how many
                  dense sample slots are valid after import. */
               frontPanelSending_sendSampleCountReply();
               break;
         
            default:
               break;
         }
         break;
   
      /* SOUND PARAMETER 0..127 (plain or menu-originated)
         WHAT: MIDI_CC and FRONT_CC_MENU (Session 039 P3) share one body. The
               live message is forced to status MIDI_CC, because
               frontParser_applyParameterCommand() treats any other status as
               CC2. A menu edit that was applied live is also queued for MIDI TX.
         WHY:  identical behaviour for both opcodes; only menu edits may be
               transmitted. Stored-only values (temp playback) are not sent.
         INPUT:  data1 = raw id, data2 = value.
         OUTPUT: store + live apply (+1 apply id) + automation record, or
                 normal-image store; TX queue.
         ACCESSORS: AVR avrComms_sendData(MIDI_CC / MIDI_CC_MENU, ...).
         AFFILIATES: preset_storeParameterIngress(),
                     frontParser_applyParameterCommand(), seq_recordAutomation(),
                     outputControl_queueMenuParameterTx(). */
      case FRONT_CC_MENU:
      case MIDI_CC: //frontParser_command.status
         // this is for parameters below 128
         {
            const uint8_t isMenuEdit = (uint8_t)(frontParser_command.status == FRONT_CC_MENU);
            uint8_t rawParam = frontParser_command.data1;
            MidiMsg liveMsg = frontParser_command;

            liveMsg.status = MIDI_CC;
            liveMsg.data1 = (uint8_t)((rawParam + 1) & 0x7f);

            if(preset_shouldApplyIngressToLive())
            {
               preset_storeParameterIngress(rawParam, frontParser_command.data2);
               frontParser_applyParameterCommand(liveMsg,0);

            //record automation if record is turned on
               seq_recordAutomation(frontParser_activeTrack, rawParam, frontParser_command.data2);

               if(isMenuEdit)
                  outputControl_queueMenuParameterTx(rawParam, frontParser_command.data2);
            }
            else
            {
               /* FILE LOAD: Store normal-image params without changing the
                  temporary sound when the temporary pattern is active. */
               preset_storeParameterIngress(rawParam, frontParser_command.data2);
            }
         }
         break;
   
      /* SOUND PARAMETER 128+ (plain or menu-originated)
         WHAT: FRONT_CC_2 and FRONT_CC_2_MENU (Session 039 P3) share one body.
               The message is normalised to FRONT_CC_2 for the apply path; a
               menu edit that was applied live is also queued for MIDI TX.
         WHY:  identical sound/storage/record behaviour for both opcodes; only
               menu edits may be transmitted (loads/reverts use FRONT_CC_2).
               Values only stored (temp playback active) are not audible and
               are not transmitted.
         INPUT:  data1 = raw id - 128, data2 = value.
         OUTPUT: live apply + automation record, or normal-image store; TX queue.
         ACCESSORS: AVR avrComms_sendData(CC_2 / CC_2_MENU, ...).
         AFFILIATES: frontParser_applyParameterCommand(), seq_recordAutomation(),
                     outputControl_queueMenuParameterTx(). */
      case FRONT_CC_2_MENU:
      case FRONT_CC_2: // frontParser_command.status
         {
            const uint8_t isMenuEdit = (uint8_t)(frontParser_command.status == FRONT_CC_2_MENU);
            MidiMsg cc2Msg = frontParser_command;

            cc2Msg.status = FRONT_CC_2;

            if(preset_shouldApplyIngressToLive())
            {
               frontParser_applyParameterCommand(cc2Msg,1);
               //record automation if record is turned on
               seq_recordAutomation(frontParser_activeTrack, frontParser_command.data1+128, frontParser_command.data2);

               if(isMenuEdit)
                  outputControl_queueMenuParameterTx((uint16_t)(frontParser_command.data1 + 128),
                                                     frontParser_command.data2);
            }
            else
            {
               /* FILE LOAD: Store normal-image params without changing the
                  temporary sound when the temporary pattern is active. */
               preset_storeParameterIngress(frontParser_command.data1+128, frontParser_command.data2);
            }
         }
         break;
   
   
      case FRONT_CC_LFO_TARGET: //frontParser_command.status
         {
            uint8_t upper = frontParser_command.data1;
            uint8_t lower = frontParser_command.data2;
            uint8_t applyLive = preset_shouldApplyIngressToLive();
         // --AS **PATROT note that the only valid values for the following are listed in
         // the modTargets array in the AVR code
            uint8_t value = ((upper&0x01)<<7) | lower;
         
            uint8_t lfoNr = (upper&0xfe)>>1;
            preset_storeLfoDestinationIngress(lfoNr, value);
            
            if(!applyLive)
            {
               /* RESTORE: Endpoint-copy automation target sidebands are stored only.
                  They must not touch the currently sounding modulation nodes. */
            }
            else
            {
               switch(lfoNr)
               {
                  case 0:
                  case 1:
                  case 2:	modNode_setDestination(&voiceArray[lfoNr].lfo.modTarget, value);
                     break;
                  case 3:	modNode_setDestination(&snareVoice.lfo.modTarget,value);		
                     break;
                  case 4:	modNode_setDestination(&cymbalVoice.lfo.modTarget, value);		
                     break;
                  case 5:	modNode_setDestination(&hatVoice.lfo.modTarget, value);			
                     break;
                  default:
                     break;
               }
            }
         }
         break; // case FRONT_CC_LFO_TARGET
   
      case FRONT_SET_P1_DEST: 
         { // frontParser_command.status
            uint8_t hi = frontParser_command.data1;
            uint8_t lo = frontParser_command.data2;
            uint8_t val = (hi<<7)|lo;
            seq_setStepAutomationDestination(frontParser_shownPattern,
                                             frontParser_activeTrack,
                                             seq_selectedStep,
                                             0,
                                             val);
         }
         break;
      case FRONT_SET_P2_DEST: 
         { // frontParser_command.status
            uint8_t hi = frontParser_command.data1;
            uint8_t lo = frontParser_command.data2;
            uint8_t val = (hi<<7)|lo;
            seq_setStepAutomationDestination(frontParser_shownPattern,
                                             frontParser_activeTrack,
                                             seq_selectedStep,
                                             1,
                                             val);
         }
         break;
      case FRONT_SET_P1_VAL: 
         { // frontParser_command.status
            uint8_t stepNr = frontParser_command.data1;
            uint8_t value = frontParser_command.data2;
            pat_getStepPtr(frontParser_shownPattern, frontParser_activeTrack, stepNr)->param1Val = value;
         
         }
         break;
      case FRONT_SET_P2_VAL: 
         { // frontParser_command.status
            uint8_t stepNr = frontParser_command.data1;
            uint8_t value = frontParser_command.data2;
            pat_getStepPtr(frontParser_shownPattern, frontParser_activeTrack, stepNr)->param2Val = value;
         }
         break;
   
      case FRONT_ARM_AUTOMATION_STEP: // frontParser_command.status
         {
            const uint8_t stepNr 	= frontParser_command.data1;
            const uint8_t onOff 	= frontParser_command.data2 & 0x40;
            const uint8_t trackNr 	=  frontParser_command.data2 & 0x3f;
         
            seq_armAutomationStep(stepNr,trackNr,onOff);
         
         }
         break;
   
      case FRONT_MAIN_STEP_CC: // frontParser_command.status
         {
         //data 1 = track und pattern nr
         //data 2 = step nr
            uint8_t voiceNr 	= frontParser_command.data1 >> 4;
            uint8_t patternNr 	= pat_normalizePatternNumber(frontParser_command.data1 & 0x0f);
            uint8_t stepNr 		= frontParser_command.data2 & 0x1f;
         
         
            if (frontParser_command.data2 & 0x40) // flag for force ON
            {
               pat_setMainStep(patternNr, voiceNr, stepNr, 1);
            }
            else if (frontParser_command.data2 & 0x20) // flag for force OFF
            {
               pat_setMainStep(patternNr, voiceNr, stepNr, 0);
            }
            
            else  //toggle the step in the seq
            {
               pat_toggleMainStep(voiceNr, stepNr, patternNr);
            }   
         
         //if step active send led on message to front
            frontPanelSending_sendMainStepLedReply(voiceNr, stepNr, patternNr);
         }
         break;
   
      case FRONT_STEP_CC: // frontParser_command.status
         {
         //data 1 = track und pattern nr
         //data 2 = step nr
            uint8_t voiceNr 	= frontParser_command.data1 >> 4;
            uint8_t patternNr 	= pat_normalizePatternNumber(frontParser_command.data1 & 0x0f);
            uint8_t stepNr 		= frontParser_command.data2;
         
         //toggle the step in the seq
            pat_toggleStep(voiceNr, stepNr, patternNr);
         
         }
         break;
   
      case FRONT_CC_VELO_TARGET: // frontParser_command.status
         {
            uint8_t upper = frontParser_command.data1;
            uint8_t lower = frontParser_command.data2;
            uint8_t applyLive = preset_shouldApplyIngressToLive();
         // --AS **PATROT note that the only valid values for the following are listed in
         // the modTargets array in the AVR code
            uint8_t value = ((upper&0x01)<<7) | lower;
            uint8_t velModNr = (upper&0xfe)>>1;
            preset_storeVelocityDestinationIngress(velModNr, value);
            if(!applyLive)
            {
               /* RESTORE: Endpoint-copy automation target sidebands are stored only.
                  They must not touch the currently sounding modulation nodes. */
            }
            else
            {
               /* Voice morph is not a live velocity modulation target; keep
                  PAR_MORPH_* out of the generic velocity node. */
               if(value >= PAR_MORPH_DRUM1 && value <= PAR_MORPH_HIHAT)
                  modNode_setDestination(&velocityModulators[velModNr], PAR_NONE);
               else
                  modNode_setDestination(&velocityModulators[velModNr], value);
            }
	         }
         break;
   
   //VOICE option Messages
      case VOICE_CC: // frontParser_command.status
         break;
   
   //BPM MESSAGE
      case FRONT_SET_BPM: 
         { // frontParser_command.status
            uint16_t bpm = frontParser_command.data1 |(uint16_t)(frontParser_command.data2<<7);
            if(bpm == 0) {
               seq_setExtSync(1);
            }
            else {
               seq_setBpm(bpm);
            
               seq_setExtSync(0);
            }
         }
         break;
   
   //LED MESSAGES
      case FRONT_STEP_LED_STATUS_BYTE: // frontParser_command.status
         switch(frontParser_command.data1)
         {
         //send all active step numbers to frontpanel to light up corresponding LEDs
            case FRONT_LED_QUERY_SEQ_TRACK:
            {
                  uint8_t trackNr = frontParser_command.data2 >> 4;
                  uint8_t patternNr = pat_normalizePatternNumber(frontParser_command.data2 & 0x0f);
               
                  frontParser_activeFrontTrack = trackNr;
                  frontPanelSending_updateTrackLeds(trackNr, patternNr, frontParser_activeStep);
               
               //send track length back
                  frontPanelSending_sendTrackLengthReply(trackNr);
               
               // **PATROT send track rotation value back
                  frontPanelSending_sendTrackRotationReply(trackNr);
               
            }
               break;
            case FRONT_LED_ALL_SUBSTEP:
               {
                  uint8_t trackNr = frontParser_command.data2 >> 4;
                  uint8_t patternNr = pat_normalizePatternNumber(frontParser_command.data2 & 0x0f);
               
                  frontPanelSending_updateSubStepLeds(trackNr, patternNr, frontParser_activeStep);               
               }
               break;
         
            default:
               break;
         }
         break;
   } // frontParser_command.status

   frontParser_flowMessageApplied();
}
//------------------------------------------------------
// Sequencer message handler
// This is called when we have received a message with status FRONT_SEQ_CC
static void frontParser_handleSeqCC()
{
   switch(frontParser_command.data1)
   {
      case FRONT_SEQ_FLOW_BEGIN:
         if(frontParser_command.data2 == FLOW_CH_LOAD_SESSION)
         {
            comm_loadSessionActive = 1;
            comm_quietUi = 1;
            comm_flowActive = 0;
            comm_flowBudgetRemaining = 0;
            frontParser_sendFlowGrant(FLOW_CH_LOAD_SESSION, FLOW_ACK_CREDITS);
         }
         else
         {
            comm_flowActive = 1;
            comm_flowChannel = (uint8_t)(frontParser_command.data2 & 0x07);
            comm_flowBudgetRemaining = FLOW_INITIAL_GRANT;
            frontParser_sendFlowGrant(comm_flowChannel, FLOW_INITIAL_GRANT);
         }
         break;

      case FRONT_SEQ_OSC_WAVE_INTERPOLATION:
         modNode_setWaveInterpEnabled(frontParser_command.data2);
         break;

      case FRONT_SEQ_FLOW_END:
         if(frontParser_command.data2 == FLOW_CH_LOAD_SESSION)
         {
            comm_loadSessionActive = 0;
            comm_quietUi = 0;
            comm_flowActive = 0;
            comm_flowBudgetRemaining = 0;
            frontParser_sendFlowGrant(FLOW_CH_LOAD_SESSION, FLOW_ACK_CREDITS);
         }
         else
         {
            uint8_t channel = (uint8_t)(frontParser_command.data2 & 0x07);
            if(comm_flowActive && (comm_flowChannel == channel))
            {
               comm_flowActive = 0;
               comm_flowBudgetRemaining = 0;
            }
            frontParser_sendFlowGrant(channel, FLOW_ACK_CREDITS);
         }
         break;

      case FRONT_SEQ_FLOW_ABORT:
         comm_loadSessionActive = 0;
         comm_quietUi = 0;
         comm_flowActive = 0;
         comm_flowBudgetRemaining = 0;
         frontParser_endFileLoadIngress();
         break;

      case FRONT_SEQ_FLOW_GRANT:
         break;

#if 0
      case FRONT_SEQ_PRF_CACHE_BEGIN:
         /* The PRF cache/background-load regime is deprecated. File loads now
            write directly to normal storage; AVR may keep probing this opcode
            until its initiators are removed, so reject it cleanly. */
         frontParser_sendPrfCacheStatus(FRONT_SEQ_PRF_CACHE_BEGIN, PRF_CACHE_REJECTED);
         frontParser_sendFlowGrant(FLOW_CH_LOAD_SESSION, FLOW_ACK_CREDITS);
         break;

      case FRONT_SEQ_PRF_PENDING_BEGIN:
         frontParser_sendFlowGrant(FLOW_CH_LOAD_SESSION, FLOW_ACK_CREDITS);
         break;

      case FRONT_SEQ_PRF_PENDING_DONE:
         frontParser_sendFlowGrant(FLOW_CH_LOAD_SESSION, FLOW_ACK_CREDITS);
         break;

      case FRONT_SEQ_PRF_CACHE_ABORT:
         frontParser_endFileLoadIngress();
         frontParser_sendFlowGrant(FLOW_CH_LOAD_SESSION, FLOW_ACK_CREDITS);
         break;

      case FRONT_SEQ_PRF_AVR_SNAPSHOT_BEGIN:
         frontParser_sendFlowGrant(FLOW_CH_LOAD_SESSION, FLOW_ACK_CREDITS);
         break;

      case FRONT_SEQ_PRF_AVR_SNAPSHOT_END:
         frontParser_sendFlowGrant(FLOW_CH_LOAD_SESSION, FLOW_ACK_CREDITS);
         break;

      case FRONT_SEQ_PRF_RESTORE_AVR_LIVE:
         frontParser_sendFlowGrantWait(FLOW_CH_LOAD_SESSION, FLOW_ACK_CREDITS);
         break;
#endif

      case FRONT_SEQ_TMP_KIT_ENDPOINT_BEGIN:
      {
         uint8_t endpointMode = frontParser_command.data2;
         frontParser_fileLoadEndpointMode = endpointMode;

         /* RESTORE: Switch ingress target to normal kit endpoint buffer.
            Subsequent parameter and target messages will populate preset_normalKitState endpoint images.
            The data byte selects which endpoint group is being refreshed, so file
            loads can update only the kit/front endpoint or only the morph parameter
            endpoint without clearing the other side. */
         preset_setIngressTarget(SEQ_PARAM_INGRESS_NORMAL_KIT_ENDPOINT);
         preset_setAutomationIngressTarget(SEQ_AUTOMATION_INGRESS_NONE);

         if(endpointMode != FRONT_SEQ_TMP_KIT_ENDPOINT_MORPH_ONLY)
         {
            memset(preset_normalKitState.kitEndpointParams, 0, END_OF_SOUND_PARAMETERS);
            /* File/restore endpoint refresh starts from a zeroed image, but
               global SampleRt's canonical default is 127. Seed it here so an
               older or partial file that never sends this parameter cannot
               leave the STM baseline at the broken zero default. */
            preset_normalKitState.kitEndpointParams[PAR_VOICE_DECIMATION_ALL] = 127;
            preset_normalKitState.interpolatedParams[PAR_VOICE_DECIMATION_ALL] = 127;
            memset(&preset_normalKitState.frontPanelAutomationTargets,
                   0,
                   sizeof(preset_normalKitState.frontPanelAutomationTargets));
         }

         if(endpointMode != FRONT_SEQ_TMP_KIT_ENDPOINT_FRONT_ONLY)
         {
            memset(preset_normalKitState.morphEndpointParams, 0, END_OF_SOUND_PARAMETERS);
            preset_normalKitState.morphEndpointParams[PAR_VOICE_DECIMATION_ALL] = 127;
            memset(&preset_normalKitState.morphParameterEndpointAutomationTargets,
                   0,
                   sizeof(preset_normalKitState.morphParameterEndpointAutomationTargets));
         }
         break;
      }

      case FRONT_SEQ_TMP_KIT_AUTOMATION_PHASE:
         /* RESTORE: Inside the copy-to-temp endpoint bracket, raw param opcodes
            identify parameter_values[] vs parameters2[]. Resolved automation target
            sidebands need this explicit phase marker so the STM stores them with
            the matching endpoint image and does not apply them to live audio. */
         if(frontParser_command.data2 == FRONT_SEQ_TMP_KIT_AUTOMATION_FRONT_ENDPOINT)
            preset_setAutomationIngressTarget(SEQ_AUTOMATION_INGRESS_FRONT_ENDPOINT);
         else if(frontParser_command.data2 == FRONT_SEQ_TMP_KIT_AUTOMATION_MORPH_ENDPOINT)
            preset_setAutomationIngressTarget(SEQ_AUTOMATION_INGRESS_MORPH_ENDPOINT);
         else
            preset_setAutomationIngressTarget(SEQ_AUTOMATION_INGRESS_NONE);
         break;

      case FRONT_SEQ_TMP_KIT_ENDPOINT_END:
         preset_applyNormalEndpointAutomationTargets();
         if(frontParser_fileLoadMirrorToTmp)
            frontParser_mirrorNormalPresetEndpointsToTmp(frontParser_fileLoadEndpointMode);

         /* RESTORE: Switch ingress target back to the surrounding context. During
            file load, endpoint dumps are nested inside normal kit-endpoint ingress. */
         preset_setIngressTarget(frontParser_fileLoadIngressActive
                                ? SEQ_PARAM_INGRESS_NORMAL_KIT_ENDPOINT
                                : SEQ_PARAM_INGRESS_CURRENT_IMAGE);
         preset_setAutomationIngressTarget(SEQ_AUTOMATION_INGRESS_NONE);
         break;

      case FRONT_SEQ_SET_GLOBAL_MORPH:
         /* AVR menu/global morph resets all STM per-voice morph amounts. Per
            voice step automation or modulation may later overwrite individual
            voices without asking AVR to recompute morph. */
         seq_setGlobalMorphAmount(frontParser_command.data2);
         break;

      case FRONT_SEQ_SET_GLOBAL_MORPH_LSB:
         frontParser_globalMorphLsb = (uint8_t)(frontParser_command.data2 & 0x7f);
         break;

      /* GLOBAL MORPH (PERF MENU) + MENU TX  (Session 039 P3)
         WHAT: assemble the 8-bit amount, apply it, then queue it for MIDI TX
               as Global CC1 (user decision 6; converted to amount >> 1 in
               outputControl_queueMenuPerfTx()).
         WHY:  SEQ_SET_GLOBAL_MORPH_LSB/MSB are sent only by AVR
               menu_parseGlobalParam() PAR_MORPH. Global CC1 on receive drives
               global morph (v*2, 127 -> 255), so a DAW recording plays back.
         INPUT:  frontParser_globalMorphLsb, data2 bit 0.
         OUTPUT: global morph applied; queued TX.
         AFFILIATES: seq_setGlobalMorphAmount(), preset_morphAutomationValueToAmount(). */
      case FRONT_SEQ_SET_GLOBAL_MORPH_MSB:
      {
         uint8_t morphAmount =
            (uint8_t)(frontParser_globalMorphLsb
               | ((frontParser_command.data2 & 0x01) << 7));
         seq_setGlobalMorphAmount(morphAmount);
         outputControl_queueMenuPerfTx(OUTPUT_MENU_TX_GLOBAL_MORPH, morphAmount);
         break;
      }

      case FRONT_SEQ_REQUEST_PATTERN_PARAMS:

      /* send back bar change and next pattern params from requested pattern*/
         frontPanelSending_sendPatternParamsReply(frontParser_shownPattern);
      
         break;
      case FRONT_SEQ_SET_PAT_BEAT:
         pat_getPatternSettingPtr(frontParser_shownPattern)->changeBar = frontParser_command.data2;
         break;
      case FRONT_SEQ_SET_PAT_NEXT:
         pat_getPatternSettingPtr(frontParser_shownPattern)->nextPattern = frontParser_command.data2;
         break;
   
      case FRONT_SEQ_REC_ON_OFF:
         seq_setRecordingMode(frontParser_command.data2);
         break;
   
      case FRONT_SEQ_ERASE_ON_OFF:
         seq_setErasingMode(frontParser_command.data2);
         break;
   
      case FRONT_SEQ_NOTE:
         pat_getStepPtr(frontParser_shownPattern, frontParser_activeTrack, frontParser_activeStep)->note = frontParser_command.data2;
         break;
   
      case FRONT_SEQ_VOLUME:
         pat_getStepPtr(frontParser_shownPattern, frontParser_activeTrack, frontParser_activeStep)->volume &= ~(0x7f);
         pat_getStepPtr(frontParser_shownPattern, frontParser_activeTrack, frontParser_activeStep)->volume |= (frontParser_command.data2&0x7f);
         break;
   
      case FRONT_SEQ_PROB:
      
         pat_getStepPtr(frontParser_shownPattern, frontParser_activeTrack, frontParser_activeStep)->prob = frontParser_command.data2;
         break;
         
      case FRONT_SEQ_EUKLID_RESET:
         {
         /* `FRONT_SEQ_EUKLID_RESET` now multiplexes legacy file-load cache
            clears and the explicit SHIFT+PERF page-visit control. Reusing the
            existing opcode avoids another transport command while still giving
            the STM an exact begin/leave/restore signal for temp-track
            snapshots on the Euclid page. */
            switch(frontParser_command.data2)
            {
               case FRONT_SEQ_EUKLID_RESET_CLEAR_ROTATION:
                  euklid_clearRotation();
                  frontParser_endEuklidSnapshotVisit();
                  break;

               case FRONT_SEQ_EUKLID_RESET_BEGIN_VISIT:
                  frontParser_beginEuklidSnapshotVisit();
                  break;

               case FRONT_SEQ_EUKLID_RESET_END_VISIT:
                  /* Leaving SHIFT+PERF commits the current normal-pattern
                     edits by discarding the per-visit temp-backup bookkeeping.
                     Returning later must start a fresh snapshot cycle, so the
                     old touched-track mask cannot survive across page exits. */
                  frontParser_endEuklidSnapshotVisit();
                  break;

               case FRONT_SEQ_EUKLID_RESET_RESTORE_VISIT:
                  frontParser_restoreEuklidSnapshotTracks();
                  break;

               default:
                  break;
            }
         }
         break;
   
      case FRONT_SEQ_EUKLID_LENGTH:
         {
            uint8_t pattern = (frontParser_shownPattern == SEQ_TMP_PATTERN) ? SEQ_TMP_PATTERN : (frontParser_command.data2&0x07);
            uint8_t length 	= frontParser_command.data2 >> 3;
            length += 1;
         /* Every Euclid-page mutation shares the same contract: preserve the
            untouched normal track in temp before the first edit of this visit,
            then mutate only the normal track until the user either leaves the
            page or explicitly re-selects SHIFT+PERF to restore from the saved
            temp copies. */
            frontParser_snapshotEuklidTrackIfNeeded(frontParser_activeTrack);
            euklid_setLength(frontParser_activeTrack, pattern, length);
            frontParser_activeFrontTrack = frontParser_activeTrack;
            frontPanelSending_updateTrackLeds(frontParser_activeTrack, pattern, frontParser_activeStep);
         }
         break;
   
      case FRONT_SEQ_EUKLID_STEPS:
         {
            uint8_t steps 	= frontParser_command.data2 >> 3;
            steps += 1;
            uint8_t pattern = (frontParser_shownPattern == SEQ_TMP_PATTERN) ? SEQ_TMP_PATTERN : (frontParser_command.data2 & 0x7);
            frontParser_snapshotEuklidTrackIfNeeded(frontParser_activeTrack);
            euklid_setSteps(frontParser_activeTrack,steps,pattern);
            frontParser_activeFrontTrack = frontParser_activeTrack;
            frontPanelSending_updateTrackLeds(frontParser_activeTrack, pattern, frontParser_activeStep);
         }
         break;
   
      case FRONT_SEQ_EUKLID_ROTATION:
         {
            uint8_t rotation = frontParser_command.data2 >> 3;
         //rotation += 1;
            uint8_t pattern = (frontParser_shownPattern == SEQ_TMP_PATTERN) ? SEQ_TMP_PATTERN : (frontParser_command.data2 & 0x7);
            frontParser_snapshotEuklidTrackIfNeeded(frontParser_activeTrack);
            euklid_setRotation(frontParser_activeTrack,rotation,pattern);
            frontParser_activeFrontTrack = frontParser_activeTrack;
            frontPanelSending_updateTrackLeds(frontParser_activeTrack, pattern, frontParser_activeStep);
         }
         break;
         
      case FRONT_SEQ_EUKLID_SUBSTEP_ROTATION:
         {
            uint8_t rotation = frontParser_command.data2 >> 3;
         //rotation += 1;
            uint8_t pattern = (frontParser_shownPattern == SEQ_TMP_PATTERN) ? SEQ_TMP_PATTERN : (frontParser_command.data2 & 0x7);
            frontParser_snapshotEuklidTrackIfNeeded(frontParser_activeTrack);
            euklid_setSubStepRotation(frontParser_activeTrack,rotation,pattern);
            frontParser_activeFrontTrack = frontParser_activeTrack;
            frontPanelSending_updateTrackLeds(frontParser_activeTrack, pattern, frontParser_activeStep);
         }
         break;   
   
      case FRONT_SEQ_CLEAR_TRACK: 
         {
            pat_clearTrack(frontParser_command.data2, frontParser_shownPattern);
         }
         break;
   
      case FRONT_SEQ_CLEAR_PATTERN:
         pat_clearPattern(frontParser_command.data2);
         break;
   
      case FRONT_SEQ_POSX:
         som_setX(frontParser_command.data2);
         break;
   
      case FRONT_SEQ_POSY:
         som_setY(frontParser_command.data2);
         break;
   
      case FRONT_SEQ_FLUX:
         som_setFlux(frontParser_command.data2/127.f);
         break;
   
      case FRONT_SEQ_SOM_FREQ:
         som_setFreq(frontParser_command.data2,frontParser_activeTrack);
         break;
   
      case FRONT_SEQ_MIDI_CHAN:
         {
            uint8_t voice = (frontParser_command.data2 >> 4)&0x07;
            uint8_t channel = (frontParser_command.data2&0x0f)+1;
            
            // --AS if midi channel changed, and a note was playing on old channel, turn it off
            if(voice < 8 && midi_MidiChannels[voice] != channel)
            {
               /* MIDI channel changes invalidate the parser-owned held-note
                  map before the old route can lose its matching note-off. */
               midiParser_clearMidiRollHolds();
               if(voice < 7)
                  voiceControl_noteOff(voice);
            }
               
            midi_MidiChannels[voice] = channel;
            
         }
         break;
         
      case FRONT_SEQ_MIDI_CHAN_OFF:
         {
            if(frontParser_command.data2 < 8)
            {
               /* Disabling a MIDI channel removes the route that would
                  receive release messages, so drop MIDI-owned roll holds. */
               if(midi_MidiChannels[frontParser_command.data2] != 0)
                  midiParser_clearMidiRollHolds();
               midi_MidiChannels[frontParser_command.data2]=0;
            }
         }
         break;
   
      // --AS not used anymore
      //case FRONT_SEQ_MIDI_MODE:
      //	midi_mode = frontParser_command.data2;
      //	break;
      
      
      //voice nr (0xf0) + autom track nr (0x0f)
      case FRONT_SEQ_CLEAR_AUTOM:
         {
            const uint8_t voice 		= frontParser_command.data2 >> 4;
            const uint8_t automTrack 	= frontParser_command.data2 &  0x0f;
            if(voice < NUM_TRACKS
               && frontParser_shownPattern == seq_perTrackActivePattern[voice])
            {
               seq_releaseAutomationLane(voice, automTrack ? 1 : 0);
            }
            pat_clearAutomation(voice, frontParser_shownPattern, automTrack);
         }
         break;
   
      case FRONT_SEQ_COPY_TRACK:
         {
            const uint8_t src = frontParser_command.data2>>4;
            const uint8_t dst = frontParser_command.data2&0xf;
            pat_copyTrack(src,dst,frontParser_shownPattern);
         }
         break;
   
      case FRONT_SEQ_COPY_PATTERN:
         {
            const uint8_t src = frontParser_command.data2>>4;
            const uint8_t dst = frontParser_command.data2&0xf;
            if(dst == SEQ_TMP_PATTERN)
               pat_copyToTmpPattern(src);
            else
               pat_copyPattern(src,dst);
         }
         break;
         
      case FRONT_SEQ_COPY_TRACK_PATTERN:
         {
            const uint8_t srcNr = frontParser_command.data2>>4;
            const uint8_t dstPat = frontParser_command.data2&0xf;
            pat_copyTrackPattern(srcNr,dstPat,frontParser_shownPattern);
         }
         break;
         
      case FRONT_SEQ_COPY_SRC:
         {
            frontParser_stepCopySource = frontParser_command.data2;
         }
         break;
      
      case FRONT_SEQ_COPY_DST:
         {
            pat_copySubStep(frontParser_stepCopySource,frontParser_command.data2,frontParser_activeTrack);
         }
         break;
   
      case FRONT_SEQ_TRACK_LENGTH:
         pat_setTrackLength(frontParser_activeTrack,frontParser_command.data2);
         break;
         
      case FRONT_SEQ_TRACK_SCALE:
         pat_setTrackScale(frontParser_activeTrack,frontParser_command.data2);
         break;
   
      case FRONT_SEQ_TRACK_ROTATION: //**PATROT handle incoming track rotation. apply to active track
         pat_setTrackRotation(frontParser_activeTrack,frontParser_command.data2);
         break;
   
      case FRONT_SEQ_SHUFFLE:
         seq_setShuffle(frontParser_command.data2/127.f);
         break;
   
      case FRONT_SEQ_SELECT_ACTIVE_STEP:
         seq_selectedStep = frontParser_command.data2;
         break;
   
      case FRONT_SEQ_SET_AUTOM_TRACK:
         seq_setActiveAutomationTrack(frontParser_command.data2);
         break;
   
      case FRONT_SEQ_SET_QUANT:
         seq_setQuantisation(frontParser_command.data2);
         break;
   
      case FRONT_SEQ_REQUEST_EUKLID_PARAMS:
         frontPanelSending_sendEuklidParamsReply(frontParser_command.data2);
         break;
   
      case FRONT_SEQ_SET_SHOWN_PATTERN:
         if (frontParser_command.data2==frontParser_shownPattern)
         {
            if(!preset_consumeTmpBoundaryPatternSwitchAck())
               seq_realign();
         }
         else
         {
            (void)preset_consumeTmpBoundaryPatternSwitchAck();
            frontParser_shownPattern = pat_normalizePatternNumber(frontParser_command.data2);
         }
         break;   
      case FRONT_SEQ_SET_ACTIVE_TRACK:
         if ( (frontParser_activeTrack==frontParser_command.data2)&&(!seq_isRunning()) )
            seq_triggerVoice(frontParser_activeTrack, seq_rollVelocity, seq_rollNote);
            
         frontParser_activeTrack = frontParser_command.data2;
         frontPanelSending_sendActiveTrackReply(frontParser_activeTrack);
         
         break;
   
      case FRONT_SEQ_REQUEST_STEP_PARAMS:
         {
         /* send back probability, volume and note nr*/
            frontPanelSending_sendStepParamsReply(frontParser_shownPattern,
                                                 frontParser_activeTrack,
                                                 frontParser_command.data2);
         
         
         
            frontParser_activeStep = frontParser_command.data2;
         }
         break;
   
      /* PERF ROLL CONTROLS + MENU TX  (Session 039 P3)
         WHAT: apply the roll setting, then queue it for MIDI TX as Global
               NRPN 93/94/95 (user decision 6).
         WHY:  these opcodes are sent only from AVR menu_parseGlobalParam()
               (encoder/knob edits; the armed-step revert after DISARM sends
               the restored value, which is harmless). Never from load or boot
               (menu_sendAllGlobals() starts at PAR_BEGINNING_OF_GLOBALS).
         INPUT:  data2 - rate 0..15 / note / velocity.
         OUTPUT: sequencer roll state; queued TX (gate in MidiOutputControl.c).
         AFFILIATES: outputControl_queueMenuPerfTx(), GlobalMidiParser NRPN
                     93-95 receive (same numbers). */
      case FRONT_SEQ_ROLL_RATE:
         seq_setRollRate(frontParser_command.data2);
         outputControl_queueMenuPerfTx(OUTPUT_MENU_TX_ROLL_RATE, frontParser_command.data2);
         break;
      case FRONT_SEQ_ROLL_NOTE:
         seq_setRollNote(frontParser_command.data2);
         outputControl_queueMenuPerfTx(OUTPUT_MENU_TX_ROLL_NOTE, frontParser_command.data2);
         break;
      case FRONT_SEQ_ROLL_VELOCITY:
         seq_setRollVelocity(frontParser_command.data2);
         outputControl_queueMenuPerfTx(OUTPUT_MENU_TX_ROLL_VELOCITY, frontParser_command.data2);
         break;
      case FRONT_SEQ_ROLL_MODE:
         // if(frontParser_command.data2==ROLL_MODE_FIRST_ON)
         //    seq_skipFirstRoll=0;
         // else if(frontParser_command.data2==ROLL_MODE_FIRST_OFF)
         //    seq_skipFirstRoll=1;   
         // else
         seq_rollMode = frontParser_command.data2;
         break;
      /* Update the external MIDI roll-note offset. Existing MIDI-held rolls
         are released by MidiParser when the mapping changes. */
      case FRONT_SEQ_ROLL_NOTE_OFFSET:
         midiParser_setRollNoteOffset(frontParser_command.data2);
         break;
      case FRONT_SEQ_TRANSPOSE:
         seq_transpose_voiceAmount[frontParser_activeTrack]=frontParser_command.data2;
         break;
      case FRONT_SEQ_TRANSPOSE_ON_OFF:
         if (frontParser_command.data2==0x0f)
            seq_writeTranspose();
         else if (frontParser_command.data2<=1)
            seq_transposeOnOff = frontParser_command.data2;
         break;
   
      case FRONT_SEQ_ROLL_ON_OFF:
         {
         
            const uint8_t onOff = frontParser_command.data2 >> 4;
            const uint8_t voice = frontParser_command.data2 & 0xf;
            seq_rollChange(voice,onOff);
         }
         break;
   
      case FRONT_SEQ_CHANGE_PAT:
         //switch to one of the 8 patterns on the next pattern start
         seq_setNextPattern( (frontParser_command.data2&0x07),(frontParser_command.data2>>3) );
         break;

      case FRONT_SEQ_CHANGE_TMP_PAT:
         seq_setNextPattern(SEQ_TMP_PATTERN, (frontParser_command.data2>>3));
         break;
   
   
      case FRONT_SEQ_RUN_STOP:
         seq_setRunning(frontParser_command.data2);
         break;
   
      case FRONT_SEQ_MUTE_TRACK:
         seq_setMute(frontParser_command.data2,1);
         break;
   
      case FRONT_SEQ_UNMUTE_TRACK:
         seq_setMute(frontParser_command.data2,0);
         break;
      case FRONT_SEQ_MIDI_ROUTING:
         midi_setRouting(frontParser_command.data2);
         break;
      case FRONT_SEQ_MIDI_TX_FILTER:
      case FRONT_SEQ_MIDI_RX_FILTER:
         midi_setFilter(frontParser_command.data1==FRONT_SEQ_MIDI_TX_FILTER, frontParser_command.data2);
         break;
      case FRONT_SEQ_BAR_RESET_MODE:
      // --AS a setting of 0 is default (keep track of bars in song), a setting of 1 is
      // to reset the bar counter when a manual pattern change occurs
         seq_resetBarOnPatternChange = frontParser_command.data2;
         break;
      case FRONT_SEQ_PC_TIME_MODE:
      // a setting of 0 is default (pattern changes at end of current bar
      // a setting of 1 causes pattern to change on next step
         switchOnNextStep = frontParser_command.data2;
         break;
      case FRONT_SEQ_TRIGGER_IN_PPQ:
         switch(frontParser_command.data2)
         {
            case 0:
               trigger_prescalerClockInput = PRE_1_PPQ;
               break;
         
            default:
            case 1:
               trigger_prescalerClockInput = PRE_4_PPQ;
               break;
         
            case 2:
               trigger_prescalerClockInput = PRE_8_PPQ;
               break;
            case 3:
               trigger_prescalerClockInput = PRE_16_PPQ;
               break;
            case 4:
               trigger_prescalerClockInput = PRE_32_PPQ;
               break;
         }
         break;
      case FRONT_SEQ_TRIGGER_OUT1_PPQ:
         switch(frontParser_command.data2)
         {
            case 0:
               trigger_dividerClockOut1 = PRE_1_PPQ;
               break;
         
            default:
            case 1:
               trigger_dividerClockOut1 = PRE_4_PPQ;
               break;
         
            case 2:
               trigger_dividerClockOut1 = PRE_8_PPQ;
               break;
            case 3:
               trigger_dividerClockOut1 = PRE_16_PPQ;
               break;
            case 4:
               trigger_dividerClockOut1 = PRE_32_PPQ;
               break;
         }
      
         break;
   
      case FRONT_SEQ_TRIGGER_OUT2_PPQ:
         switch(frontParser_command.data2)
         {
            case 0:
               trigger_dividerClockOut2 = PRE_1_PPQ;
               break;
         
            default:
            case 1:
               trigger_dividerClockOut2 = PRE_4_PPQ;
               break;
         
            case 2:
               trigger_dividerClockOut2 = PRE_8_PPQ;
               break;
            case 3:
               trigger_dividerClockOut2 = PRE_16_PPQ;
               break;
            case 4:
               trigger_dividerClockOut2 = PRE_32_PPQ;
               break;
               break;
         }
         break;
   
      case FRONT_SEQ_TRIGGER_GATE_MODE:
         trigger_setGatemode(frontParser_command.data2);
         break;
         
      case FRONT_SEQ_SET_LOOP:
         pat_setLoop(frontParser_command.data2);
         break;
      case FRONT_SEQ_LOAD_VOICE:
         frontParser_beginFileLoadIngress(0);
         seq_voicesLoading |= (0x01<<frontParser_command.data2);
         break;
      case FRONT_SEQ_UNHOLD_VOICE:
         frontParser_clearVoiceLoading(frontParser_command.data2);
         if(!frontParser_fileLoadBracketActive && !seq_voicesLoading)
            frontParser_endFileLoadIngress();
         break;
      case FRONT_SEQ_LOAD_FAST:
         seq_loadFastMode=frontParser_command.data2;
         break;
      case FRONT_SEQ_BACKGROUND_SWAP_BEGIN:
         pat_copyToTmpPattern(seq_activePattern);
         seq_setNextPattern(SEQ_TMP_PATTERN, 0x0f);
         preset_tempPlaybackSwitchState.forceInstantSwitch = 1;
         preset_tempPlaybackSwitchState.patternOnlyTempPlayback =
            (frontParser_command.data2 == FRONT_FILE_DONE_TYPE_PATTERN);
         frontParser_backgroundSwapPrepared = 1;
         frontParser_backgroundSwapPreparedType = frontParser_command.data2;
         frontParser_backgroundSwapPending = 1;
         frontParser_backgroundSwapFileType = frontParser_command.data2;
         frontParser_backgroundSwapAckDelayActive = 0;
         break;
      case FRONT_SEQ_FILE_BEGIN:
         frontParser_beginFileLoadIngress(1);
         frontParser_fileLoadMirrorToTmp = 0;
         frontParser_fileLoadPreserveTempPreset = 0;
         if(frontParser_fileLoadHasPresetEndpoints(frontParser_command.data2))
         {
            if(frontParser_fileLoadShouldPreserveTempPreset(frontParser_command.data2))
            {
               /* Protected .prf/.all background loads intentionally do not
                  mirror into temp while the file bytes are arriving. Temp is
                  still the audible old sound and only becomes the new snapshot
                  after playback later returns to normal. */
               frontParser_fileLoadPreserveTempPreset = 1;
               preset_tempPlaybackSwitchState.resnapshotTmpFromNormalPending = 1;
               preset_setTempPresetPlaybackActive(1);
            }
            else
            {
               frontParser_fileLoadMirrorToTmp = 1;
            }
         }
         frontParser_backgroundSwapPrepared = 0;
         if((frontParser_command.data2 == FRONT_FILE_DONE_TYPE_PERFORMANCE)
            || (frontParser_command.data2 == FRONT_FILE_DONE_TYPE_ALL))
         {
            seq_resetLiveMorphApplyCache();
            preset_morphLoadDisabled = 1;
            preset_vMorphFlag = 0;
         }
         break;
      case FRONT_SEQ_FILE_DONE:
         if((frontParser_command.data2 == FRONT_FILE_DONE_TYPE_PERFORMANCE)
            || (frontParser_command.data2 == FRONT_FILE_DONE_TYPE_ALL))
         {
            preset_morphLoadDisabled = 0;
            preset_vMorphFlag = 0;
         }
         frontParser_fileLoadMirrorToTmp = 0;
         frontParser_fileLoadPreserveTempPreset = 0;
         seq_voicesLoading=0;
         frontParser_endFileLoadIngress();
         break;
      case FRONT_SEQ_TRACK_NOTE1:
      case FRONT_SEQ_TRACK_NOTE2:
      case FRONT_SEQ_TRACK_NOTE3:
      case FRONT_SEQ_TRACK_NOTE4:
      case FRONT_SEQ_TRACK_NOTE5:
      case FRONT_SEQ_TRACK_NOTE6:
      case FRONT_SEQ_TRACK_NOTE7:
         {
            const uint8_t voice =
               (uint8_t)(frontParser_command.data1-FRONT_SEQ_TRACK_NOTE1);
            /* Note overrides define both normal MIDI triggers and shifted roll
               triggers. Release old MIDI-held rolls before changing them. */
            if(voice < 7 && midi_NoteOverride[voice] != frontParser_command.data2)
            {
               midiParser_clearMidiRollHolds();
               midi_NoteOverride[voice] = frontParser_command.data2;
            }
         }
         break;
      default:
         break;
   }
}
