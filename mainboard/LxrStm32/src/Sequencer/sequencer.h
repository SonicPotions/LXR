/*
 * sequencer.h
 *
 *  Created on: 11.04.2012
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


#ifndef SEQUENCER_H_
#define SEQUENCER_H_



#include "stm32f4xx.h"
#include "globals.h"
#include "Preset/KitState.h"
#include "Preset/ParameterArray.h"
#include "Preset/ParameterIngress.h"
#include "Preset/MorphEngine.h"
#include "Preset/TempPlaybackSwitch.h"
#include "PatternData.h"
#include "EuklidGenerator.h"

/* Pattern-chain sentinel values used to request randomized follow-up
   selection when a pattern-chain entry is resolved. */
#define SEQ_NEXT_RANDOM 		0x08
#define SEQ_NEXT_RANDOM_PREV 	0x09

/* Default roll velocity used when the roll engine substitutes its own hit
   volume instead of replaying the pattern volume unchanged. */
#define ROLL_VOLUME 100

enum Seq_RollModeEnum //0=trig, 1=nte, 2=vel, 3=bth, 4=all
{
	ROLL_MODE_TRIG,
   ROLL_MODE_NOTE,
   ROLL_MODE_VELOCITY,
   ROLL_MODE_BOTH,
   ROLL_MODE_ALL,
   // ROLL_MODE_FIRST_ON,
   // ROLL_MODE_FIRST_OFF,
};

enum Seq_QuantisationEnum
{
	NO_QUANTISATION,
	QUANT_8,
	QUANT_16,
	QUANT_32,
	QUANT_64,
};

/* Sequencer-owned live state exported for other modules.
   These values are read by the front-panel UI, transport, recording, and
   pattern-ownership helpers to keep playback and editing synchronized. */
/* Index of the currently active pattern slot. This is the transport-visible
   pattern number that the sequencer and front-panel currently play back. */
extern uint8_t seq_activePattern;
/* Per-track source pattern table for the currently active pattern view.
   Each entry selects the normal or temp pattern slot that the corresponding
   track should read from during playback and editing. */
extern uint8_t seq_perTrackActivePattern[7];
/* Per-track playback cursor plus one shared master reference in the final
   array slot. The individual track entries move independently while the last
   entry tracks the transport master position used for pattern timing. */
extern int8_t seq_stepIndex[NUM_TRACKS+1];
/* Non-zero while live recording is enabled. The sequencer and pattern-edit
   helpers check this flag before writing captured notes or automation into
   the active source pattern. */
extern uint8_t seq_recordActive;

//extern PatternSet* seq_activePatternSetPtr;

/* Per-track transpose offsets. Each entry stores the current semitone offset
   for the corresponding track, using 63 as the neutral value. */
extern uint8_t seq_transpose_voiceAmount[7];
/* Global transpose enable flag. When zero, the stored per-track offsets are
   ignored and playback uses the source notes unchanged. */
extern uint8_t seq_transposeOnOff;

/* Last step selected in the UI. Used by the front-panel and editor logic to
   keep the current step focus stable while transport and pattern state move. */
extern uint8_t seq_selectedStep;

/* Pattern-change bar reset option. When set, a pattern switch resets the bar
   counter so chained changes stay aligned to a fresh bar boundary. */
extern uint8_t seq_resetBarOnPatternChange;

/* Front-panel "switch on next step" toggle. A non-zero value requests the
   next queued pattern change to happen immediately on the next transport step
   instead of waiting for the next bar boundary. */
extern uint8_t switchOnNextStep;

/* Voice-loading state exported for the preset/temp-switch workflow.
   seq_voicesLoading tracks whether a voice image is currently being restored.
   seq_newVoiceAvailable marks which voices need a source refresh.
   seq_tracksLocked prevents live edits while a load is in flight.
   seq_loadFastMode tells the sequencer to apply the next source change without
   waiting for the slower boundary restore path. */
extern uint8_t seq_voicesLoading;
extern uint8_t seq_newVoiceAvailable;
extern uint8_t seq_tracksLocked;
extern uint8_t seq_loadFastMode;

/* Roll playback state.
   seq_rollMode selects which roll behavior to use.
   seq_rollNote and seq_rollVelocity provide the override note and velocity
   used by the note/velocity/both roll modes.
   The requested roll state is the aggregate of manual/front-panel and
   parser-owned MIDI roll-note holds. MIDI holds are keyed per MIDI key in
   MidiParser.c and are released on note-off (including NOTE_ON velocity 0)
   and on sequencer stop. While stopped, MIDI-held rolls play on a private
   stopped roll clock using seq_rollVelocity / seq_rollNote (Session 038).
   seq_kitResetFlag and seq_skipFirstRoll hold the current roll-control flags
   used by the front-panel transport and step-quantized roll trigger path. */
extern uint8_t seq_rollMode;
extern uint8_t seq_rollNote;
extern uint8_t seq_rollVelocity;
extern uint8_t seq_kitResetFlag;
extern uint8_t seq_skipFirstRoll;

/* Push a live morph packet to the front-panel/DSP bridge. */
void sequencer_sendVMorph(uint8_t voiceArray, uint8_t morphAmount);
/* Trigger one voice from the current sequencer state.
   voiceNr: track index to read.
   vol: velocity to send to the synth/MIDI bridge.
   note: MIDI note value to send after transpose and overrides are applied. */
void seq_triggerVoice(uint8_t voiceNr, uint8_t vol, uint8_t note);
/* Update the global shuffle amount used by the transport clock. */
void seq_setShuffle(float shuffle);
/* Preset now owns the core sound-state images and parameter routing logic.
   Keep the old seq_* names alive here as compatibility wrappers while the
   rest of the tree migrates to the new Preset public API. */
#define SEQ_SYNTH_VOICES PRESET_SYNTH_VOICES
#define SEQ_VOICE_PARAM_LENGTH PRESET_VOICE_PARAM_LENGTH
#define SEQ_MORPH_IMAGE_NORMAL PRESET_MORPH_IMAGE_NORMAL
#define SEQ_MORPH_IMAGE_TMP PRESET_MORPH_IMAGE_TMP
#define SEQ_MORPH_IMAGE_COUNT PRESET_MORPH_IMAGE_COUNT
#define SEQ_VOICE_SOURCE_NORMAL PRESET_VOICE_SOURCE_NORMAL
#define SEQ_VOICE_SOURCE_TMP PRESET_VOICE_SOURCE_TMP
#define SEQ_PARAM_INGRESS_CURRENT_IMAGE PRESET_PARAM_INGRESS_CURRENT_IMAGE
#define SEQ_PARAM_INGRESS_NORMAL_KIT_ENDPOINT PRESET_PARAM_INGRESS_NORMAL_KIT_ENDPOINT
#define SEQ_AUTOMATION_INGRESS_NONE PRESET_AUTOMATION_INGRESS_NONE
#define SEQ_AUTOMATION_INGRESS_FRONT_ENDPOINT PRESET_AUTOMATION_INGRESS_FRONT_ENDPOINT
#define SEQ_AUTOMATION_INGRESS_MORPH_ENDPOINT PRESET_AUTOMATION_INGRESS_MORPH_ENDPOINT

/* Compatibility wrapper for the active kit-image selector. This preserves the
   old `seq_*` name while delegating the actual temp-vs-normal choice to
   `Preset`, which now owns the routing policy. */
static inline PresetKitState* seq_getCurrentImageKitState(void)
{
   return preset_getCurrentImageKitState();
}

/* Compatibility wrapper for the image-to-kit lookup helper. It keeps callers
   from learning the new file layout while still routing through `Preset`. */
static inline PresetKitState* seq_getMorphKitForImage(uint8_t image)
{
   return preset_getMorphKitForImage(image);
}

/* Compatibility wrapper for the per-voice image selection helper. The voice
   source state is now owned by `Preset`, but the old name remains available for
   callers that have not switched over yet. */
static inline uint8_t seq_getMorphImageForVoice(uint8_t synthVoice)
{
   return preset_getMorphImageForVoice(synthVoice);
}

/* Compatibility wrapper for reading a voice's source marker. This keeps the
   old `seq_*` entry point alive while the source-state array physically lives
   in `Preset`. */
static inline uint8_t seq_getVoiceSourceState(uint8_t synthVoice)
{
   return preset_getVoiceSourceState(synthVoice);
}

/* Compatibility wrapper for updating a voice source marker. Later phases can
   keep using the same call shape while `Preset` remains the owner of the array
   that the router consults. */
static inline void seq_setVoiceSourceState(uint8_t synthVoice, uint8_t sourceState)
{
   preset_setVoiceSourceState(synthVoice, sourceState);
}

/* Compatibility wrapper for the canonical-parameter hook. The function is
   currently an identity mapping, but keeping the wrapper makes future table
   normalization a single change in `Preset`. */
static inline uint16_t seq_canonicalParamFromVoiceMask(uint16_t param)
{
   return preset_canonicalParamFromVoiceMask(param);
}

/* Compatibility wrapper for the first-bit scan helper used by the voice-mask
   logic. */
static inline uint8_t seq_firstVoiceForMask(uint8_t voiceMask)
{
   return preset_firstVoiceForMask(voiceMask);
}

/* Compatibility wrapper for the parameter-to-voice-mask lookup. */
static inline uint8_t seq_voiceMaskForParameter(uint16_t param)
{
   return preset_voiceMaskForParameter(param);
}

/* Compatibility wrapper for the voice-parameter predicate used by the live
   cache and restore code. */
static inline uint8_t seq_isVoiceParameter(uint16_t param)
{
   return preset_isVoiceParameter(param);
}

/* Compatibility wrapper that maps selector bytes back to canonical destination
   parameters. */
static inline uint16_t seq_resolveAutomationTargetSelector(uint8_t selector)
{
   return preset_resolveAutomationTargetSelector(selector);
}

/* Compatibility wrapper for the reverse destination-to-selector lookup. */
static inline uint8_t seq_selectorForAutomationTargetDestination(uint16_t destination)
{
   return preset_selectorForAutomationTargetDestination(destination);
}

/* Compatibility wrapper that preserves the old voice-selector helper name
   while delegating the actual lookup into `Preset`. */
static inline uint8_t seq_voiceSelectorForAutomationTargetDestination(uint16_t destination,
                                                                      uint8_t fallback)
{
   return preset_voiceSelectorForAutomationTargetDestination(destination,
                                                             fallback);
}

/* Compatibility wrapper for the selector-parameter predicate. */
static inline uint8_t seq_isAutomationTargetSelectorParam(uint16_t param)
{
   return preset_isAutomationTargetSelectorParam(param);
}

/* Compatibility wrapper for the morph-amount predicate. */
static inline uint8_t seq_isMorphAmountParam(uint16_t param)
{
   return preset_isMorphAmountParam(param);
}

/* Compatibility wrapper that maps morph parameters back to voice indices. */
static inline uint8_t seq_morphVoiceForParam(uint16_t param)
{
   return preset_morphVoiceForParam(param);
}

/* Compatibility wrapper for the helper that updates interpolated automation
   targets only. The real implementation now lives in `Preset`, but the old
   call shape stays intact for callers migrating in smaller steps. */
static inline void seq_updateInterpolatedAutomationTarget(PresetKitState *kit,
                                                          uint16_t param,
                                                          uint8_t selector)
{
   preset_updateInterpolatedAutomationTarget(kit, param, selector);
}

/* Compatibility wrapper for the helper that keeps the front-panel and
   interpolated automation targets coherent together. */
static inline void seq_updateFrontAndInterpolatedAutomationTargets(PresetKitState *kit,
                                                                   uint16_t param,
                                                                   uint8_t selector)
{
   preset_updateFrontAndInterpolatedAutomationTargets(kit, param, selector);
}

/* Compatibility wrapper for updating the live shared-parameter cache. */
static inline void seq_updateLiveSharedParameterCache(uint16_t param, uint8_t value)
{
   preset_updateLiveSharedParameterCache(param, value);
}

/* Compatibility wrapper for ingress target selection. The actual mode state is
   owned by `Preset`, but old callers can keep using the `seq_*` name while the
   refactor lands. */
static inline void seq_setIngressTarget(uint8_t target)
{
   preset_setIngressTarget(target);
}

/* Compatibility wrapper that exposes the current ingress target flag through
   the legacy Sequencer API surface. */
static inline uint8_t seq_getIngressTarget(void)
{
   return preset_getIngressTarget();
}

/* Compatibility wrapper for the live-vs-restore ingress predicate. */
static inline uint8_t seq_shouldApplyIngressToLive(void)
{
   return preset_shouldApplyIngressToLive();
}

/* Compatibility wrapper for the automation-sideband ingress mode. */
static inline void seq_setAutomationIngressTarget(uint8_t target)
{
   preset_setAutomationIngressTarget(target);
}

/* Compatibility wrapper that forwards raw endpoint bytes into `Preset`. */
static inline void seq_storeParameterIngress(uint16_t param, uint8_t value)
{
   preset_storeParameterIngress(param, value);
}

/* Compatibility wrapper that forwards morph-endpoint bytes into `Preset`. */
static inline void seq_storeMorphParameterIngress(uint16_t param, uint8_t value)
{
   preset_storeMorphParameterIngress(param, value);
}

/* Compatibility wrapper for LFO destination ingress. */
static inline void seq_storeLfoDestinationIngress(uint8_t voice, uint16_t destination)
{
   preset_storeLfoDestinationIngress(voice, destination);
}

/* Compatibility wrapper for velocity destination ingress. */
static inline void seq_storeVelocityDestinationIngress(uint8_t voice, uint16_t destination)
{
   preset_storeVelocityDestinationIngress(voice, destination);
}

/* Compatibility wrapper for macro destination ingress. */
static inline void seq_storeMacroDestinationIngress(uint8_t destinationNr, uint16_t destination)
{
   preset_storeMacroDestinationIngress(destinationNr, destination);
}

/* Initialize the sequencer runtime, transport, and live playback caches. */
void seq_init();
/* Advance the sequencer one timing quantum and process due playback.
   While stopped, also services the MIDI-only stopped roll clock
   (Session 038); transport timing is not affected by that service. */
void seq_tick();

/* Forward morph interpolation work into the Preset-owned engine. */
static inline void seq_serviceMorphInterpolation(void)
{
   preset_serviceMorphInterpolation();
}

/* Process pending endpoint-restore work for the current transport frame. */
void seq_serviceEndpointRestore();
/* Return non-zero while endpoint restore is still running. */
uint8_t seq_endpointRestoreBusy();

/* Set the global morph amount shared by all voices. */
static inline void seq_setGlobalMorphAmount(uint8_t morphAmount)
{
   preset_setGlobalMorphAmount(morphAmount);
}

/* Reset every voice morph amount to the global value. */
static inline void seq_resetVoiceMorphAmountsToGlobal(void)
{
   preset_resetVoiceMorphAmountsToGlobal();
}

/* Reset the live morph-apply cache. */
static inline void seq_resetLiveMorphApplyCache(void)
{
   preset_resetLiveMorphApplyCache();
}

/* Set the global morph automation value. */
static inline void seq_setGlobalMorphAutomationValue(uint8_t morphValue)
{
   preset_setGlobalMorphAutomationValue(morphValue);
}

/* Set the morph amount for one voice. */
static inline void seq_setVoiceMorphAmount(uint8_t synthVoice, uint8_t morphAmount)
{
   preset_setVoiceMorphAmount(synthVoice, morphAmount);
}

/* Set the morph automation value for one voice. */
static inline void seq_setVoiceMorphAutomationValue(uint8_t synthVoice, uint8_t morphValue)
{
   preset_setVoiceMorphAutomationValue(synthVoice, morphValue);
}

/* Set the morph automation value for a voice mask. */
static inline void seq_setVoiceMorphMaskAutomationValue(uint8_t voiceMask, uint8_t morphValue)
{
   preset_setVoiceMorphMaskAutomationValue(voiceMask, morphValue);
}

/* Apply a proportional modulation delta to one voice's morph amount. */
static inline void seq_modulateVoiceMorphAmount(uint8_t synthVoice, float amount, float value)
{
   preset_modulateVoiceMorphAmount(synthVoice, amount, value);
}

/* Transitional wrapper retained for older Sequencer-facing call sites while
   the live parameter emit path is fully owned by ParameterIngress. New code
   should call `preset_applySingleParameterValue()` directly. */
static inline void seq_applySingleParameterValue(uint16_t param, uint8_t value)
{
   preset_applySingleParameterValue(param, value);
}
/* Arm or clear a pending automation capture step.
   stepNr: step index to capture into when the armed state is enabled.
   track: track index whose automation lane is being armed.
   isArmed: non-zero to arm the capture point, zero to clear it. */
void seq_armAutomationStep(uint8_t stepNr, uint8_t track,uint8_t isArmed);
/* AUTOMATION STEP ARMED QUERY  (Session 039 P3)
   WHAT: non-zero while a front-panel automation step is armed (step and
         track both set by seq_armAutomationStep()).
   WHY:  menu edits made while a step is armed are temporary step data that
         the AVR reverts on disarm; MIDI TX must not send them (user decision).
         The STM sees ARM/DISARM in order with the edits, so it is the single
         place to gate.
   OUTPUT: 1 armed, 0 not armed.
   ACCESSORS: outputControl_queueMenuParameterTx(),
              outputControl_queueMenuPerfTx() (MidiOutputControl.c).
   AFFILIATES: seq_armAutomationStep(), seq_recordAutomation(). */
uint8_t seq_isAutomationStepArmed(void);
/* Recalculate the transport delta after a clock jump.
   This is used by clock jumps, sync restarts, and other timing changes that
   force the next step to be recomputed immediately. */
void seq_resetDeltaAndTick();
/* Realign transport and track cursors to the current playback position.
   This keeps the per-track cursors and bar counter coherent after pattern or
   rotation changes that should not restart playback from step zero. */
void seq_realign();
/* Override the current transport delta in milliseconds.
   delta: next-step interval to use until the transport recalculates it. */
void seq_setDeltaT(float delta);
/* Advance the external-clock master step marker by one step size.
   stepSize: number of sequencer steps represented by the current external
   clock tick. */
void seq_triggerNextMasterStep(uint8_t stepSize);
/* Set the transport tempo in beats per minute.
   bpm: requested transport tempo; the sequencer uses it to rebuild sync
   timing and related LFO timing. */
void seq_setBpm(uint16_t bpm);
/* Read the current transport tempo in beats per minute.
   Returns the live transport tempo used by the sequencer clock. */
uint16_t seq_getBpm();
/* Service sync state and advance the transport if needed.
   This is the transport's periodic sync hook and is safe to call repeatedly
   from the main loop. */
void seq_sync();
/* Read the current external-sync enable state.
   Returns non-zero when external clocking is active and the sequencer should
   wait for external transport ticks. */
uint8_t seq_getExtSync();
/* Select the quantisation grid used for timing-sensitive capture.
   value: one of the quantisation enum values; the sequencer converts it into
   a step interval used by recording, roll, and capture code. */
void seq_setQuantisation(uint8_t value);
/* Enable or disable external sync mode.
   isExt: non-zero to follow external MIDI clock, zero to run internally. */
void seq_setExtSync(uint8_t isExt);
/* Queue a pattern switch.
   patNr: requested destination pattern, including random or temp sentinels.
   voice: target track selector, or 0x0f to apply the request to every track.
   The function stages the switch request in the temp-playback state machine
   and does not immediately change the active playback source. */
void seq_setNextPattern(const uint8_t patNr, uint8_t voice);
/* Start or stop the sequencer transport.
   isRunning: non-zero starts playback, zero stops playback and resets the
   transport state to its stopped baseline.
   Stop also releases every MIDI-held roll (midiParser_clearMidiRollHolds()),
   and does so on every stop request, so it doubles as a MIDI-roll panic.
   Start hands any MIDI roll pressed during the stop to the normal quantized
   roll engine. Manual roll behaviour is unchanged (Session 038). */
void seq_setRunning(uint8_t isRunning);
/* Read whether the transport is currently running.
   Returns non-zero while the transport is active. */
uint8_t seq_isRunning();
/* Mute or unmute a track, or all tracks when trackNr is 7.
   trackNr: target track index, or 7 for the global mute/all-notes-off path.
   isMuted: non-zero to mute, zero to unmute. */
void seq_setMute(uint8_t trackNr, uint8_t isMuted);
/* Read whether a track is currently muted.
   Returns non-zero when the track's mute bit is set. */
uint8_t seq_isTrackMuted(uint8_t trackNr);
/* Send the full step payload for one step index to the front panel.
   stepNr: absolute step index to serialize for the UI. */
void seq_sendStepInfoToFront(uint16_t stepNr);
/* Send the main-step mask and length information to the front panel.
   stepNr: step index whose main-step mask should be displayed. */
void seq_sendMainStepInfoToFront(uint16_t stepNr);
/* Record a change in roll button state for one voice.
   voice: track index whose roll button changed.
   onOff: non-zero when the button is pressed, zero when released. */
void seq_rollChange(uint8_t voice, uint8_t onOff);
/* Record a MIDI-owned roll hold for one voice.
   voice: track index 0..6. onOff: non-zero = held by MIDI, zero = released.
   MIDI ownership is separate from front-panel roll buttons; either source
   keeps the aggregate roll request active until both release. Called only by
   MidiParser's held-key synchroniser: on every roll-key press (re-assert,
   which re-fires one-shot rolls and, while stopped, re-arms an immediate
   hit), on a voice's final release, and on every hold clear, including
   sequencer stop. While the transport is stopped, MIDI-held voices are
   played by a private stopped roll clock serviced from seq_tick().
   See sequencer.c seq_tickStoppedRolls() (Session 038). */
void seq_rollMidiChange(uint8_t voice, uint8_t onOff);
/* Apply the current roll state for one voice and report whether it triggered.
   voice: track index whose roll state should be updated.
   onOff: non-zero to enable roll, zero to release it. The return value is
   non-zero when the call immediately triggered playback. */
uint8_t seq_setRoll(uint8_t voice, uint8_t onOff);
/* Check whether a roll trigger should fire on the current step.
   voice: track index whose roll counter should be evaluated.
   Returns non-zero when the roll counter fired on this step. */
uint8_t seq_checkRollStep(uint8_t voice);
/* Trigger a roll event immediately for one voice.
   voice: track index to trigger.
   Returns non-zero when the voice was actually fired. */
uint8_t seq_rollTrig(uint8_t voice);
/* Set the roll playback rate.
   rate: UI-facing roll rate selector that is converted into the internal
   step countdown used by the roll engine. */
void seq_setRollRate(uint8_t rate);
/* Set the roll playback note.
   note: override note value used by roll modes that substitute pitch. */
void seq_setRollNote(uint8_t note);
/* Set the roll playback velocity.
   velocity: override velocity used by roll modes that substitute dynamics. */
void seq_setRollVelocity(uint8_t velocity);
/* Record a note into the current pattern position.
   trackNr: track to write.
   vel: velocity to store.
   note: note value to store.
   isNoteOff: LIVE_REC_DUPLICATE_SUBSTEP_BUG.md fix -- pass 1 only from the
   MIDI channel-parser note path, where vel == 0 genuinely means "note
   released" and the event must keep its true un-quantized position. Every
   internal trigger (roll hits, loop re-record) passes 0 so that a
   zero-velocity event is still placed on the quantize grid instead of being
   mistaken for a note-off and written one sub-step off the beat. See the full
   rationale above seq_addNote() in sequencer.c. */
void seq_addNote(uint8_t trackNr,uint8_t vel, uint8_t note, uint8_t isNoteOff);
/* Enable or disable live recording.
   active: non-zero to arm recording writes into the current source pattern. */
void seq_setRecordingMode(uint8_t active);
/* Enable or disable live erasing.
   active: non-zero to clear notes during step capture instead of writing them. */
void seq_setErasingMode(uint8_t active);
/* Select which automation lane receives live recordings.
   trackNr: lane selector, usually 0 for param1 and non-zero for param2. */
void seq_setActiveAutomationTrack(uint8_t trackNr);
/* Store one step automation destination through Sequencer so currently held
   one-step overrides can be released when a lane target is overwritten. */
void seq_setStepAutomationDestination(uint8_t pattern,
                                      uint8_t track,
                                      uint8_t step,
                                      uint8_t lane,
                                      uint8_t dest);
/* Release a currently held one-step automation override for a track/lane.
   Clear-menu paths use this before deleting lane data from the active pattern. */
void seq_releaseAutomationLane(uint8_t track, uint8_t lane);
/* Record an automation value into the active pattern source.
   voice: track whose active pattern source receives the update.
   dest: raw automation destination parameter.
   value: automation value to store. */
void seq_recordAutomation(uint8_t voice, uint8_t dest, uint8_t value);
/* Record an automation value whose destination came from the MIDI CC enum.
   Low MIDI-domain ids are converted back to raw pattern parameter ids. */
void seq_recordAutomationMidiDestination(uint8_t voice, uint16_t dest, uint8_t value);
/* Apply transpose values to the active per-track source patterns.
   This mutates the currently selected source patterns in place, then resets
   the stored transpose offsets back to their neutral value. */
void seq_writeTranspose();
/* Quantize a step index for one track.
   step: raw step index to quantize.
   track: track index used to read the current pattern scale.
   Returns the quantized step index. */
int8_t seq_quantize(int8_t step, uint8_t track);
/* Send note-off messages to one MIDI channel or to every active channel.
   chan: MIDI channel index, or 0xff to release all active notes. */
void seq_midiNoteOff(uint8_t chan);
/* Send a MIDI note-on message with the requested channel, note, and velocity.
   channel: MIDI channel index.
   note: note value to emit.
   veloc: note velocity to emit. */
void seq_sendMidiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t veloc);

#endif /* SEQUENCER_H_ */
