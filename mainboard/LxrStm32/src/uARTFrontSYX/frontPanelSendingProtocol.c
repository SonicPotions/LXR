/*
 * frontPanelSendingProtocol.c
 *
 * Packet composition helpers for outbound front-panel protocol traffic.
 * This file owns reusable packet families and the parser-facing reply helpers
 * so the receive code does not need to open-code packet triples.
 */

#include "frontPanelSendingProtocol.h"

#include "Uart.h"
#include "Preset/MorphEngine.h"
#include "SampleRom/SampleMemory.h"
#include "Sequencer/sequencer.h"
#include "Sequencer/Pattern/EuklidGenerator.h"

/* Raw front-panel transport wrappers. These are the only helpers that talk to
   the UART primitives directly. */
void frontPanelSending_sendByte(uint8_t data)
{
   uart_sendFrontpanelByte(data);
}

void frontPanelSending_sendPriorityByte(uint8_t data)
{
   uart_sendFrontpanelPriorityByte(data);
}

void frontPanelSending_sendPriorityByteWait(uint8_t data)
{
   uart_sendFrontpanelPriorityByteWait(data);
}

void frontPanelSending_sendSysExByte(uint8_t data)
{
   uart_sendFrontpanelSysExByte(data);
}

/* Small packet builders used by the parser, sequencer, and restore code. */
void frontPanelSending_sendTriplet(uint8_t status, uint8_t data1, uint8_t data2)
{
   frontPanelSending_sendByte(status);
   frontPanelSending_sendByte(data1);
   frontPanelSending_sendByte(data2);
}

void frontPanelSending_sendPriorityTriplet(uint8_t status, uint8_t data1, uint8_t data2)
{
   frontPanelSending_sendPriorityByte(status);
   frontPanelSending_sendPriorityByte(data1);
   frontPanelSending_sendPriorityByte(data2);
}

void frontPanelSending_sendPriorityTripletWait(uint8_t status, uint8_t data1, uint8_t data2)
{
   frontPanelSending_sendPriorityByteWait(status);
   frontPanelSending_sendPriorityByteWait(data1);
   frontPanelSending_sendPriorityByteWait(data2);
}

/* Short protocol acknowledgements and query replies. */
void frontPanelSending_sendCallbackAck(void)
{
   frontPanelSending_sendPriorityByte(FRONT_CALLBACK_ACK);
}

void frontPanelSending_sendSampleUploadResult(uint8_t statusFlags)
{
   /* Mask to documented status bits so stale local flags cannot leak into the
      AVR parser. Output packet shape: SAMPLE_CC, RESULT opcode, flags. Use the
      priority wait path because the AVR wait loop has no timeout fallback; this
      sparse completion packet is the only import-complete signal. */
   frontPanelSending_sendPriorityTripletWait(
      SAMPLE_CC,
      FRONT_SAMPLE_UPLOAD_RESULT,
      (uint8_t)(statusFlags & (SAMPLE_UPLOAD_STATUS_COUNT_LIMIT |
                               SAMPLE_UPLOAD_STATUS_FLASH_LIMIT |
                               SAMPLE_UPLOAD_STATUS_READ_ERROR)));
}

void frontPanelSending_sendSampleUploadProgress(uint8_t looped, uint8_t index)
{
   uint8_t progressOpcode = looped ? FRONT_SAMPLE_UPLOAD_LOOP_PROGRESS :
                                     FRONT_SAMPLE_UPLOAD_SAMPLE_PROGRESS;

   /* Progress packets are sparse, user-facing protocol triples.
      Inputs: looped selects sample vs loop wording on the AVR; index is the
      one-based number to display. Output: SAMPLE_CC, progress opcode, index.
      Use the priority wait transport so a full UART FIFO cannot drop one byte
      and leave the AVR displaying an older number. No final marker is sent
      here: every progress packet represents a real accepted file. */
   frontPanelSending_sendPriorityTripletWait(SAMPLE_CC, progressOpcode, index);
}

void frontPanelSending_sendSampleCountReply(void)
{
   /* Reply with the current sample count. */
   frontPanelSending_sendByte(SAMPLE_CC);
   frontPanelSending_sendByte(FRONT_SAMPLE_COUNT);
   frontPanelSending_sendByte(sampleMemory_getNumSamples());
}

void frontPanelSending_sendTrackLengthReply(uint8_t trackNr)
{
   /* Reply with the current track length. */
   frontPanelSending_sendByte(FRONT_SEQ_CC);
   frontPanelSending_sendByte(FRONT_SEQ_TRACK_LENGTH);
   frontPanelSending_sendByte(pat_getTrackLength(trackNr));
}

void frontPanelSending_sendTrackRotationReply(uint8_t trackNr)
{
   /* Reply with the current track rotation. */
   frontPanelSending_sendTrackRotationValue(pat_getTrackRotation(trackNr));
}

void frontPanelSending_sendTrackRotationValue(uint8_t rotation)
{
   /* Emit the track-rotation value packet. */
   frontPanelSending_sendByte(FRONT_SEQ_CC);
   frontPanelSending_sendByte(FRONT_SEQ_TRACK_ROTATION);
   frontPanelSending_sendByte(rotation);
}

void frontPanelSending_sendBankChange(uint8_t bankCode, uint8_t value)
{
   /* Bank-change echo packet used by the MIDI channel parser. */
   frontPanelSending_sendByte(BANK_CHANGE_CC);
   frontPanelSending_sendByte(bankCode);
   frontPanelSending_sendByte(value);
}

void frontPanelSending_sendParameterEcho(uint16_t paramNr, uint8_t value)
{
   /* Parameter echo packet with the live low/high parameter split preserved. */
   if(paramNr < 128)
   {
      frontPanelSending_sendByte(PARAM_CC);
      frontPanelSending_sendByte((uint8_t)(paramNr - 1));
   }
   else
   {
      frontPanelSending_sendByte(PARAM_CC2);
      frontPanelSending_sendByte((uint8_t)(paramNr - 128));
   }

   frontPanelSending_sendByte(value);
}

void frontPanelSending_sendPatchReset(void)
{
   /* Legacy outbound compatibility helper only. PATCH_RESET now belongs to the
      AVR->STM reload path and must not be used to trigger AVR-side snapshot
      restore behavior. */
   frontPanelSending_sendSysExByte(PATCH_RESET);
}

void frontPanelSending_sendVoiceActivity(uint8_t voice)
{
   /* Voice LED pulse for the current note-on event. */
   frontPanelSending_sendTriplet(NOTE_ON, voice, 0);
}

/* Pattern and step information replies used by the front-panel UI. */
void frontPanelSending_sendPatternParamsReply(uint8_t patternNr)
{
   PatternSetting *patternSetting = pat_getPatternSettingPtr(patternNr);

   frontPanelSending_sendByte(FRONT_SEQ_CC);
   frontPanelSending_sendByte(FRONT_SEQ_SET_PAT_BEAT);
   frontPanelSending_sendByte(patternSetting->changeBar);

   frontPanelSending_sendByte(FRONT_SEQ_CC);
   frontPanelSending_sendByte(FRONT_SEQ_SET_PAT_NEXT);
   frontPanelSending_sendByte(patternSetting->nextPattern);
}

void frontPanelSending_sendPatternDataReply(uint8_t patternNr)
{
   /* Pattern data reply used by front-panel pattern dumps. */
   frontPanelSending_sendSysExByte(pat_patternSet.pat_patternSettings[patternNr].nextPattern);
   frontPanelSending_sendSysExByte(pat_patternSet.pat_patternSettings[patternNr].changeBar);
}

void frontPanelSending_sendEuklidParamsReply(uint8_t trackNr)
{
   /* Euclidean-pattern reply packet family. */
   frontPanelSending_sendByte(FRONT_SEQ_CC);
   frontPanelSending_sendByte(FRONT_SEQ_EUKLID_LENGTH);
   frontPanelSending_sendByte(euklid_getLength(trackNr));

   frontPanelSending_sendByte(FRONT_SEQ_CC);
   frontPanelSending_sendByte(FRONT_SEQ_EUKLID_STEPS);
   frontPanelSending_sendByte(euklid_getSteps(trackNr));

   frontPanelSending_sendByte(FRONT_SEQ_CC);
   frontPanelSending_sendByte(FRONT_SEQ_EUKLID_ROTATION);
   frontPanelSending_sendByte(euklid_getRotation(trackNr));

   frontPanelSending_sendByte(FRONT_SEQ_CC);
   frontPanelSending_sendByte(FRONT_SEQ_EUKLID_SUBSTEP_ROTATION);
   frontPanelSending_sendByte(euklid_getSubStepRotation(trackNr));

   frontPanelSending_sendByte(FRONT_SEQ_CC);
   frontPanelSending_sendByte(FRONT_SEQ_TRACK_SCALE);
   frontPanelSending_sendByte(pat_getTrackScale(trackNr));
}

void frontPanelSending_sendActiveTrackReply(uint8_t trackNr)
{
   /* Active-track reply combines the current rotation and transpose state. */
   frontPanelSending_sendTrackRotationReply(trackNr);

   frontPanelSending_sendByte(FRONT_SEQ_CC);
   frontPanelSending_sendByte(FRONT_SEQ_TRANSPOSE);
   frontPanelSending_sendByte(seq_transpose_voiceAmount[trackNr]);
}

void frontPanelSending_sendMainStepLedReply(uint8_t trackNr, uint8_t stepNr, uint8_t patternNr)
{
   /* Main-step LED reply. */
   if(pat_isMainStepActive(trackNr, stepNr, patternNr))
   {
      frontPanelSending_sendTriplet(FRONT_STEP_LED_STATUS_BYTE,
                                    FRONT_LED_SEQ_BUTTON,
                                    (uint8_t)(stepNr * 8));
   }
}

void frontPanelSending_sendStepParamsReply(uint8_t patternNr, uint8_t trackNr, uint8_t stepNr)
{
   /* Full step-parameter reply used by the front-panel step editor. */
   Step *step = pat_getStepPtr(patternNr, trackNr, stepNr);
   uint8_t hi;
   uint8_t lo;
   uint8_t dest;

   frontPanelSending_sendByte(FRONT_SEQ_CC);
   frontPanelSending_sendByte(FRONT_SEQ_VOLUME);
   frontPanelSending_sendByte(step->volume & STEP_VOLUME_MASK);

   frontPanelSending_sendByte(FRONT_SEQ_CC);
   frontPanelSending_sendByte(FRONT_SEQ_NOTE);
   frontPanelSending_sendByte(step->note);

   frontPanelSending_sendByte(FRONT_SEQ_CC);
   frontPanelSending_sendByte(FRONT_SEQ_PROB);
   frontPanelSending_sendByte(step->prob);

   dest = step->param1Nr;
   hi = (uint8_t)(dest >> 7);
   lo = (uint8_t)(dest & 0x7f);
   frontPanelSending_sendByte(FRONT_SET_P1_DEST);
   frontPanelSending_sendByte(hi);
   frontPanelSending_sendByte(lo);

   hi = (uint8_t)(step->param1Val >> 7);
   lo = (uint8_t)(step->param1Val & 0x7f);
   frontPanelSending_sendByte(FRONT_SET_P1_VAL);
   frontPanelSending_sendByte(hi);
   frontPanelSending_sendByte(lo);

   dest = step->param2Nr;
   hi = (uint8_t)(dest >> 7);
   lo = (uint8_t)(dest & 0x7f);
   frontPanelSending_sendByte(FRONT_SET_P2_DEST);
   frontPanelSending_sendByte(hi);
   frontPanelSending_sendByte(lo);

   hi = (uint8_t)(step->param2Val >> 7);
   lo = (uint8_t)(step->param2Val & 0x7f);
   frontPanelSending_sendByte(FRONT_SET_P2_VAL);
   frontPanelSending_sendByte(hi);
   frontPanelSending_sendByte(lo);
}

/* SysEx handshake acknowledgements. */
void frontPanelSending_sendSysexStartAck(void)
{
   frontPanelSending_sendSysExByte(SYSEX_START);
}

void frontPanelSending_sendSysexEndAck(void)
{
   /* Echo the sysex end marker back to the front panel. */
   frontPanelSending_sendSysExByte(SYSEX_END);
}

void frontPanelSending_sendSysexReceiveAck(uint8_t mode)
{
   /* Report the current sysex receive mode. */
   frontPanelSending_sendSysExByte(mode);
}

void frontPanelSending_sendSysexStepAck(void)
{
   /* Ack for incoming sequencer step data. */
   frontPanelSending_sendSysExByte(SYSEX_STEP_ACK);
}

void frontPanelSending_sendSysexBeginPatternTransmitAck(void)
{
   /* Ack for starting a pattern transmit SysEx session. */
   frontPanelSending_sendSysExByte(SYSEX_BEGIN_PATTERN_TRANSMIT);
}

/* Restore-mode packet families. */
void frontPanelSending_sendRestoreBegin(void)
{
   frontPanelSending_sendPriorityByteWait(PARAM_RESTORE_BEGIN);
   frontPanelSending_sendPriorityByteWait(0);
   frontPanelSending_sendPriorityByteWait(0);
}

void frontPanelSending_sendRestoreDone(void)
{
   /* Close the restore packet sequence. */
   frontPanelSending_sendPriorityByteWait(PARAM_RESTORE_DONE);
   frontPanelSending_sendPriorityByteWait(0);
   frontPanelSending_sendPriorityByteWait(0);
}

void frontPanelSending_sendRestoreParam(uint16_t param, uint8_t value)
{
   /* Restore one raw parameter using the stored parameter index. */
   if(param >= END_OF_SOUND_PARAMETERS)
      return;

   if(param < 128)
   {
      frontPanelSending_sendPriorityByteWait(PRF_RESTORE_PARAM_CC);
      frontPanelSending_sendPriorityByteWait((uint8_t)param);
   }
   else
   {
      frontPanelSending_sendPriorityByteWait(PRF_RESTORE_PARAM_CC2);
      frontPanelSending_sendPriorityByteWait((uint8_t)(param - 128));
   }

   frontPanelSending_sendPriorityByteWait(value);
}

void frontPanelSending_sendRestoreMorphParam(uint16_t param, uint8_t value)
{
   /* Restore one morph parameter using the stored parameter index. */
   if(param >= END_OF_SOUND_PARAMETERS)
      return;

   if(param < 128)
   {
      frontPanelSending_sendPriorityByteWait(PRF_RESTORE_MORPH_CC);
      frontPanelSending_sendPriorityByteWait((uint8_t)param);
   }
   else
   {
      frontPanelSending_sendPriorityByteWait(PRF_RESTORE_MORPH_CC2);
      frontPanelSending_sendPriorityByteWait((uint8_t)(param - 128));
   }

   frontPanelSending_sendPriorityByteWait(value);
}

void frontPanelSending_sendGlobalMorphReport(uint8_t amount)
{
   /* Report the current global morph amount to the front panel. */
   frontPanelSending_sendPriorityByteWait(FRONT_SEQ_CC);
   frontPanelSending_sendPriorityByteWait(FRONT_SEQ_REPORT_GLOBAL_MORPH_LSB);
   frontPanelSending_sendPriorityByteWait((uint8_t)(amount & 0x7f));

   frontPanelSending_sendPriorityByteWait(FRONT_SEQ_CC);
   frontPanelSending_sendPriorityByteWait(FRONT_SEQ_REPORT_GLOBAL_MORPH_MSB);
   frontPanelSending_sendPriorityByteWait((uint8_t)((amount >> 7) & 0x01));
}

void frontPanelSending_sendGlobalMorphRuntimeReport(uint8_t amount)
{
   frontPanelSending_sendByte(FRONT_SEQ_CC);
   frontPanelSending_sendByte(FRONT_SEQ_REPORT_GLOBAL_MORPH_LSB);
   frontPanelSending_sendByte((uint8_t)(amount & 0x7f));

   frontPanelSending_sendByte(FRONT_SEQ_CC);
   frontPanelSending_sendByte(FRONT_SEQ_REPORT_GLOBAL_MORPH_MSB);
   frontPanelSending_sendByte((uint8_t)((amount >> 7) & 0x01));
}

/* PERF ROLL DISPLAY REPORT  (Session 039)
   WHAT/WHY: see frontPanelSendingProtocol.h. Display-only: the AVR writes
             parameter_values[PAR_ROLL*] and repaints; it does not echo back.
   INPUT:  subOpcode (FRONT_SEQ_REPORT_ROLL_*), value.
   OUTPUT: FRONT_SEQ_CC, subOpcode, value & 0x7f on the normal (non-priority)
           front-panel queue, the same path as the runtime morph report.
   ACCESSORS: globalMidiParser_applySpecialNrpn().
   AFFILIATES: frontPanelSending_sendTriplet(),
               frontPanelSending_sendGlobalMorphRuntimeReport() (precedent). */
void frontPanelSending_sendRollParamReport(uint8_t subOpcode, uint8_t value)
{
   frontPanelSending_sendTriplet(FRONT_SEQ_CC, subOpcode, (uint8_t)(value & 0x7f));
}

void frontPanelSending_sendVoiceMorphReport(uint8_t voice, uint8_t amount)
{
   if(voice >= PRESET_SYNTH_VOICES)
      return;

   frontPanelSending_sendPriorityByteWait(FRONT_SEQ_VOICE_MORPH);
   frontPanelSending_sendPriorityByteWait(voice);
   frontPanelSending_sendPriorityByteWait((uint8_t)(amount & 0x7f));

   frontPanelSending_sendPriorityByteWait(FRONT_SEQ_VOICE_MORPH);
   frontPanelSending_sendPriorityByteWait((uint8_t)(voice + PRESET_SYNTH_VOICES));
   frontPanelSending_sendPriorityByteWait((uint8_t)((amount >> 7) & 0x01));
}

void frontPanelSending_sendVoiceMorphRuntimeReport(uint8_t voice, uint8_t amount)
{
   if(voice >= PRESET_SYNTH_VOICES)
      return;

   frontPanelSending_sendByte(FRONT_SEQ_VOICE_MORPH);
   frontPanelSending_sendByte(voice);
   frontPanelSending_sendByte((uint8_t)(amount & 0x7f));

   frontPanelSending_sendByte(FRONT_SEQ_VOICE_MORPH);
   frontPanelSending_sendByte((uint8_t)(voice + PRESET_SYNTH_VOICES));
   frontPanelSending_sendByte((uint8_t)((amount >> 7) & 0x01));
}

void frontPanelSending_sendVoiceMorphRuntimeReports(void)
{
   uint8_t voice;

   for(voice=0;voice<PRESET_SYNTH_VOICES;voice++)
      frontPanelSending_sendVoiceMorphRuntimeReport(voice, preset_getVoiceMorphAmount(voice));
}

void frontPanelSending_sendVoiceMorphReports(void)
{
   uint8_t voice;

   for(voice=0;voice<PRESET_SYNTH_VOICES;voice++)
      frontPanelSending_sendVoiceMorphReport(voice, preset_getVoiceMorphAmount(voice));
}

void frontPanelSending_sendVoiceMorphReportsFromKit(const PresetKitState *kit)
{
   uint8_t voice;

   if(!kit)
      return;

   for(voice=0;voice<PRESET_SYNTH_VOICES;voice++)
      frontPanelSending_sendVoiceMorphReport(voice, kit->voiceMorphAmount[voice]);
}

/* Runtime feedback helpers for the sequencer and transport LEDs. */
void frontPanelSending_sendPatternChange(uint8_t pattern)
{
   /* Notify the front panel that the active pattern changed. */
   frontPanelSending_sendByte(FRONT_SEQ_CC);
   frontPanelSending_sendByte(FRONT_SEQ_CHANGE_PAT);
   frontPanelSending_sendByte(pattern);
}

void frontPanelSending_sendRunStop(uint8_t isRunning)
{
   /* Report sequencer run/stop transitions to the front panel. */
   frontPanelSending_sendByte(FRONT_SEQ_CC);
   frontPanelSending_sendByte(FRONT_SEQ_RUN_STOP);
   frontPanelSending_sendByte(isRunning);
}

void frontPanelSending_sendBeatLed(uint8_t onOff)
{
   /* Beat pulse LED helper. */
   frontPanelSending_sendTriplet(FRONT_STEP_LED_STATUS_BYTE,
                                 FRONT_LED_PULSE_BEAT,
                                 onOff);
}

void frontPanelSending_sendCurrentStepLed(uint8_t stepNr)
{
   /* Current-step LED helper. */
   frontPanelSending_sendTriplet(FRONT_STEP_LED_STATUS_BYTE,
                                 FRONT_CURRENT_STEP_NUMBER_CC,
                                 stepNr);
}

void frontPanelSending_sendMainStepInfo(uint16_t stepNr)
{
   /* Serialize the pattern main-step info into a SysEx reply. */
   const uint8_t currentPattern = (uint8_t)(stepNr / 7);
   const uint8_t currentTrack = (uint8_t)(stepNr - (currentPattern * 7));
   uint16_t dataToSend = pat_patternSet.pat_mainSteps[currentPattern][currentTrack];

   frontPanelSending_sendSysExByte((uint8_t)(dataToSend & 0x7f));
   frontPanelSending_sendSysExByte((uint8_t)((dataToSend >> 7) & 0x7f));
   frontPanelSending_sendSysExByte((uint8_t)((dataToSend >> 14) & 0x7f));
   frontPanelSending_sendSysExByte(pat_patternSet.pat_patternLengthRotate[currentPattern][currentTrack].length);
   frontPanelSending_sendSysExByte(pat_patternSet.pat_patternLengthRotate[currentPattern][currentTrack].scale);
}

void frontPanelSending_sendStepInfo(uint16_t stepNr)
{
   /* Serialize the full step editor state into a SysEx reply. */
   const uint8_t absPat = (uint8_t)(stepNr / 128);
   const uint8_t currentTrack = (uint8_t)(absPat / 8);
   const uint8_t currentPattern = (uint8_t)(absPat - (currentTrack * 8));
   const uint8_t currentStep = (uint8_t)(stepNr - (absPat * 128));
   Step *dataToSend = &pat_patternSet.pat_subStepPattern[currentPattern][currentTrack][currentStep];

   frontPanelSending_sendSysExByte((uint8_t)(dataToSend->volume & 0x7f));
   frontPanelSending_sendSysExByte((uint8_t)(dataToSend->prob & 0x7f));
   frontPanelSending_sendSysExByte((uint8_t)(dataToSend->note & 0x7f));
   frontPanelSending_sendSysExByte((uint8_t)(dataToSend->param1Nr & 0x7f));
   frontPanelSending_sendSysExByte((uint8_t)(dataToSend->param1Val & 0x7f));
   frontPanelSending_sendSysExByte((uint8_t)(dataToSend->param2Nr & 0x7f));
   frontPanelSending_sendSysExByte((uint8_t)(dataToSend->param2Val & 0x7f));
   frontPanelSending_sendSysExByte((uint8_t)((((dataToSend->volume & 0x80) >> 7) |
                                             ((dataToSend->prob & 0x80) >> 6) |
                                             ((dataToSend->note & 0x80) >> 5) |
                                             ((dataToSend->param1Nr & 0x80) >> 4) |
                                             ((dataToSend->param1Val & 0x80) >> 3) |
                                             ((dataToSend->param2Nr & 0x80) >> 2) |
                                             ((dataToSend->param2Val & 0x80) >> 1)) & 0x7f));
}

/* LED display helpers for track and sub-step updates. */
void frontPanelSending_updateTrackLeds(uint8_t trackNr, uint8_t patternNr, uint8_t activeStep)
{
   /* Rebuild the track LED status row from the current sequencer state. */
   uint8_t ledByte = 0x00;
   uint8_t i;
   uint8_t k;

   if(trackNr > 6)
      return;

   for(k=0;k<4;k++)
   {
      for(i=0;i<4;i++)
      {
         if(pat_isMainStepActive(trackNr,(uint8_t)((k<<2) + i),patternNr))
            ledByte |= (0x01<<i);
      }

      frontPanelSending_sendTriplet(FRONT_STEP_LED_STATUS_BYTE,
                                    (uint8_t)(FRONT_LED_SEQ_MAIN_ONE+k),
                                    ledByte);
      ledByte = 0x00;
   }

   frontPanelSending_updateSubStepLeds(trackNr, patternNr, activeStep);
}

void frontPanelSending_updateSubStepLeds(uint8_t trackNr, uint8_t patternNr, uint8_t activeStep)
{
   /* Rebuild the sub-step LED status rows from the current sequencer state. */
   uint8_t start = activeStep & 0x78;
   uint8_t ledByte = 0x00;
   uint8_t i;

   for(i=0;i<4;i++)
   {
      if(pat_isStepActive(trackNr,(start+i),patternNr))
         ledByte |= (0x01<<i);
   }

   frontPanelSending_sendTriplet(FRONT_STEP_LED_STATUS_BYTE,
                                 FRONT_LED_SEQ_SUB_STEP_LOWER,
                                 ledByte);

   ledByte = 0x00;

   for(i=0;i<4;i++)
   {
      if(pat_isStepActive(trackNr,(start+4+i),patternNr))
         ledByte |= (0x01<<i);
   }

   frontPanelSending_sendTriplet(FRONT_STEP_LED_STATUS_BYTE,
                                 FRONT_LED_SEQ_SUB_STEP_UPPER,
                                 ledByte);
}

static uint8_t frontPanelSending_flowGrantByte(uint8_t channel, uint8_t credits)
{
   return (uint8_t)(((channel & 0x07) << 4) | (credits & 0x0f));
}

/* Flow-control helpers shared with the parser receive path. */
void frontPanelSending_sendFlowGrant(uint8_t channel, uint8_t credits)
{
   /* Emit a non-blocking flow grant packet. */
   uart_sendFrontpanelPriorityByte(FRONT_SEQ_CC);
   uart_sendFrontpanelPriorityByte(FRONT_SEQ_FLOW_GRANT);
   uart_sendFrontpanelPriorityByte(frontPanelSending_flowGrantByte(channel, credits));
}

//#if 0
//void frontPanelSending_sendFlowGrantWait(uint8_t channel, uint8_t credits)
//{
//   /* Emit a blocking flow grant packet. */
//   uart_sendFrontpanelPriorityByteWait(FRONT_SEQ_CC);
//   uart_sendFrontpanelPriorityByteWait(FRONT_SEQ_FLOW_GRANT);
//   uart_sendFrontpanelPriorityByteWait(frontPanelSending_flowGrantByte(channel, credits));
//}
//#endif

void frontPanelSending_sendFlowAbort(uint8_t channel)
{
   /* Abort the current flow-control session. */
   uart_sendFrontpanelPriorityByte(FRONT_SEQ_CC);
   uart_sendFrontpanelPriorityByte(FRONT_SEQ_FLOW_ABORT);
   uart_sendFrontpanelPriorityByte(channel);
}

//#if 0
//void frontPanelSending_sendPrfCacheStatus(uint8_t command, uint8_t status)
//{
//   /* Report the current PRF cache command/status pair. */
//   uart_sendFrontpanelPriorityByte(PRF_CACHE_STATUS);
//   uart_sendFrontpanelPriorityByte(command);
//   uart_sendFrontpanelPriorityByte(status);
//}
//#endif
