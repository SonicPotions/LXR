/*
 * frontPanelSendingProtocol.h
 *
 * Front-panel outbound packet helpers owned by the uARTFrontSYX layer.
 * These helpers assemble reusable packet families so parser and sequencer
 * code do not need to rebuild the same byte triples inline.
 */

#ifndef UARTFRONTSYX_FRONTPANELSENDINGPROTOCOL_H_
#define UARTFRONTSYX_FRONTPANELSENDINGPROTOCOL_H_

#include <stdint.h>

#include "frontPanelReceivingProtocol.h"
#include "Preset/KitState.h"

/* Raw STM->AVR byte wrappers. Only this send-side protocol module should call
   the front-panel UART primitives directly for command traffic. */
void frontPanelSending_sendByte(uint8_t data);
void frontPanelSending_sendPriorityByte(uint8_t data);
void frontPanelSending_sendPriorityByteWait(uint8_t data);
void frontPanelSending_sendSysExByte(uint8_t data);

/* Emit ordinary three-byte STM->AVR command triples. */
void frontPanelSending_sendTriplet(uint8_t status, uint8_t data1, uint8_t data2);
/* Emit priority triples without waiting for UART drain. */
void frontPanelSending_sendPriorityTriplet(uint8_t status, uint8_t data1, uint8_t data2);
/* Emit priority triples and wait for each byte to be accepted. */
void frontPanelSending_sendPriorityTripletWait(uint8_t status, uint8_t data1, uint8_t data2);

/* Acknowledge an AVR callback request with FRONT_CALLBACK_ACK. */
void frontPanelSending_sendCallbackAck(void);
/* Report SAMPLE_START_UPLOAD completion. Input statusFlags is a bitmask of
   SAMPLE_UPLOAD_STATUS_*; output is SAMPLE_CC/FRONT_SAMPLE_UPLOAD_RESULT/data2.
   The AVR waits for this parsed packet, not a raw ACK byte. */
void frontPanelSending_sendSampleUploadResult(uint8_t statusFlags);
/* Report the 1-based sample or loop currently being imported. Inputs:
   looped=0 emits FRONT_SAMPLE_UPLOAD_SAMPLE_PROGRESS, looped=1 emits
   FRONT_SAMPLE_UPLOAD_LOOP_PROGRESS; index is the user-visible 1-based count.
   The implementation uses the priority wait transport because every progress
   packet represents a real accepted file and must not be dropped. */
void frontPanelSending_sendSampleUploadProgress(uint8_t looped, uint8_t index);
/* Reply to FRONT_SAMPLE_COUNT with the sample ROM count. */
void frontPanelSending_sendSampleCountReply(void);
/* Reply to a track-length query for trackNr. Caller must pass 0..6. */
void frontPanelSending_sendTrackLengthReply(uint8_t trackNr);
/* Reply to a track-rotation query for trackNr. Caller must pass 0..6. */
void frontPanelSending_sendTrackRotationReply(uint8_t trackNr);
/* Emit FRONT_SEQ_TRACK_ROTATION with an already computed rotation value. */
void frontPanelSending_sendTrackRotationValue(uint8_t rotation);
/* Echo a bank-change command/value pair back to the AVR. */
void frontPanelSending_sendBankChange(uint8_t bankCode, uint8_t value);
/* Echo an internal parameter value using PARAM_CC/PARAM_CC2 encoding. */
void frontPanelSending_sendParameterEcho(uint16_t paramNr, uint8_t value);
/* Legacy outbound PATCH_RESET helper. PATCH_RESET is now primarily an AVR->STM
   reload request path and must not be used to ask AVR to restore local
   snapshot arrays. */
void frontPanelSending_sendPatchReset(void);
/* Pulse one front-panel voice activity LED with NOTE_ON encoding. */
void frontPanelSending_sendVoiceActivity(uint8_t voice);
/* Reply with pattern next/change-bar settings for patternNr. */
void frontPanelSending_sendPatternParamsReply(uint8_t patternNr);
/* Send pattern metadata inside an active SysEx pattern transfer. */
void frontPanelSending_sendPatternDataReply(uint8_t patternNr);
/* Reply with Euclidean/scale settings for trackNr. Caller must pass 0..6. */
void frontPanelSending_sendEuklidParamsReply(uint8_t trackNr);
/* Reply with active-track rotation and transpose state for trackNr. */
void frontPanelSending_sendActiveTrackReply(uint8_t trackNr);
/* Send one main-step LED state when the requested step is active. */
void frontPanelSending_sendMainStepLedReply(uint8_t trackNr,
                                           uint8_t stepNr,
                                           uint8_t patternNr);
/* Reply with the editable step parameter set for pattern/track/step. */
void frontPanelSending_sendStepParamsReply(uint8_t patternNr,
                                          uint8_t trackNr,
                                          uint8_t stepNr);

/* SysEx transfer acknowledgements sent over the front-panel SysEx byte path. */
void frontPanelSending_sendSysexStartAck(void);
void frontPanelSending_sendSysexEndAck(void);
void frontPanelSending_sendSysexReceiveAck(uint8_t mode);
void frontPanelSending_sendSysexStepAck(void);
void frontPanelSending_sendSysexBeginPatternTransmitAck(void);

/* Begin/end canonical restore packets. These use priority wait writes so the
   AVR receives a coherent restore bracket. */
void frontPanelSending_sendRestoreBegin(void);
void frontPanelSending_sendRestoreDone(void);
/* Send one front-end restore parameter, raw-indexed 0..END_OF_SOUND_PARAMETERS-1. */
void frontPanelSending_sendRestoreParam(uint16_t param, uint8_t value);
/* Send one morph-endpoint restore parameter, raw-indexed 0..END_OF_SOUND_PARAMETERS-1. */
void frontPanelSending_sendRestoreMorphParam(uint16_t param, uint8_t value);
/* Report global morph amount as low/high 7-bit FRONT_SEQ_CC commands. */
void frontPanelSending_sendGlobalMorphReport(uint8_t amount);
void frontPanelSending_sendGlobalMorphRuntimeReport(uint8_t amount);
/* PERF ROLL DISPLAY REPORT  (Session 039)
   WHAT: queues one FRONT_SEQ_CC / FRONT_SEQ_REPORT_ROLL_* / value triplet to
         the AVR on the non-blocking front-panel UART path.
   WHY:  keeps the PERF page in step with MIDI-driven roll changes;
         non-blocking because it is called from the main-loop MIDI parse path.
   INPUT:  subOpcode - FRONT_SEQ_REPORT_ROLL_RATE/_NOTE/_VELOCITY;
           value - 0..127 (masked to 7 bits).
   OUTPUT: three bytes queued via frontPanelSending_sendTriplet().
   ACCESSORS: globalMidiParser_applySpecialNrpn() (GlobalMidiParser.c).
   AFFILIATES: AVR SEQ_REPORT_ROLL_* cases (avrCommsReceivingProtocol.c). */
void frontPanelSending_sendRollParamReport(uint8_t subOpcode, uint8_t value);
/* Report one or all current per-voice morph amounts as low/high packets. */
void frontPanelSending_sendVoiceMorphReport(uint8_t voice, uint8_t amount);
void frontPanelSending_sendVoiceMorphRuntimeReport(uint8_t voice, uint8_t amount);
void frontPanelSending_sendVoiceMorphRuntimeReports(void);
void frontPanelSending_sendVoiceMorphReports(void);
void frontPanelSending_sendVoiceMorphReportsFromKit(const PresetKitState *kit);

/* Runtime sequencer feedback commands for AVR UI state. */
void frontPanelSending_sendPatternChange(uint8_t pattern);
void frontPanelSending_sendRunStop(uint8_t isRunning);
void frontPanelSending_sendBeatLed(uint8_t onOff);
void frontPanelSending_sendCurrentStepLed(uint8_t stepNr);
/* Send compact main-step or sub-step data inside the active SysEx reply. */
void frontPanelSending_sendMainStepInfo(uint16_t stepNr);
void frontPanelSending_sendStepInfo(uint16_t stepNr);

/* Rebuild the visible 16-step main LED row and then refresh sub-step LEDs. */
void frontPanelSending_updateTrackLeds(uint8_t trackNr,
                                      uint8_t patternNr,
                                      uint8_t activeStep);
/* Rebuild the lower/upper four-bit sub-step LED rows around activeStep. */
void frontPanelSending_updateSubStepLeds(uint8_t trackNr,
                                        uint8_t patternNr,
                                        uint8_t activeStep);

/* Flow-control grants for long AVR->STM transfers. Non-waiting version is used
   during normal parser progress; wait version is used when bracketing needs a
   synchronous grant. */
void frontPanelSending_sendFlowGrant(uint8_t channel, uint8_t credits);
//#define FRONT_PANEL_SEND_FLOW_GRANT_WAIT_DISABLED
//#if 0
//void frontPanelSending_sendFlowGrantWait(uint8_t channel, uint8_t credits);
//#endif
/* Abort a long-transfer flow-control session for channel. */
void frontPanelSending_sendFlowAbort(uint8_t channel);
/* Deprecated PRF compatibility status only; there is no active cache owner. */
//#if 0
//void frontPanelSending_sendPrfCacheStatus(uint8_t command, uint8_t status);
//#endif

#endif /* UARTFRONTSYX_FRONTPANELSENDINGPROTOCOL_H_ */
