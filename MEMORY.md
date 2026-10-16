# LXR -bc- Enhanced Firmware — Project Context

This file is the working memory for you, the Codex/Claude Code/LLM agent performing a task on this project.
Read it fully at the start of every session before touching any code.

---

## Quick Start

```bash
# Build
make clean
make firmware

# Build outputs
# - mainboard/LxrStm32/LxrStm32.bin
# - front/LxrAvr/LxrAvr.bin
# - firmware image/FIRMWARE.BIN
```

**Current working source**: repository root. At Session 039 close the active branch is `dev-roll-midi`. HEAD is `cdc5242` (Session 038 fix in `13d40eb`; Session 039 P1/P2 and plans in `cdc5242`); the Session 039 P3 implementation, manual edits, README/firmware version 1.03 and the rebuilt `FIRMWARE.BIN` are staged but uncommitted. `master` is the main branch for PRs.

**Canonical comms naming**: STM-side front-panel comms live under `mainboard/LxrStm32/src/uARTFrontSYX/` and keep the `frontPanel*` naming. AVR-side comms live under `front/LxrAvr/avrComms/` and use the `avrComms*` and `avrCommsParser*` naming. On the AVR side, `frontPanel` and `frontParser` are historical terminology only and should not be used for new code or docs.

**Session 001 close status**: full top-level build succeeds in this repo with `make clean && make firmware` (warnings remain, no fatal errors).

**Current status after Session 009 closeout (2026-06-10)**: Phase 2 of the architectural refactor is complete. The morph engine, interpolation worker, and live-apply suppression cache have been moved into the new `mainboard/LxrStm32/src/Preset/MorphEngine` module. All relocated logic was renamed with the `preset_` prefix, and a new authoritative DSP bridge was established in `ParameterIngress.c`. `Sequencer` remains a compatibility façade. Build is verified, and the refactor continues according to the phased plan that followed this handoff. Session 010 is expanding the Phase 3 plan so the next implementation pass can move endpoint restore, temp switching, and background-load finalization into `Preset`.

**Current status after Session 013 closeout (2026-06-13)**: Phase 6 cleanup/naming is complete and `make -C mainboard/LxrStm32 -j4 stm32` is green. `mainboard/LxrStm32/src/uARTFrontSYX/` owns the front-panel UART transport and parser, `uARTFrontSYX/FrontPanelProtocol.h` owns the front-panel opcode namespace through a compatibility include from `MidiMessages.h`, and `PresetLoadCache` still exposes the shared `presetLoad_*` background-load/session API while `mainboard/LxrStm32/src/uARTFrontSYX/frontPanelParser.c` still carries a parser-local duplicate helper block. `SeqKitState` uses endpoint-oriented field names, the AVR snapshot buffers were renamed to match their file-load/menu-snapshot role, and the session 013 handoff now points at `PRESET_CONSOLIDATION_AUDIT.md`, `COMMS_FLOW_SPEC.md`, and `TEMPORARY_PAT_PARAM_LOAD_SPEC.md` as the active next-step docs. Session 012 smoke-tested `.ALL` load, morph, parameter change, copy to temp, load new `.ALL`, and switch back; MIDI front-panel verification was already confirmed before the closeout pass.

**Current status after Session 014 implementation pass (2026-06-13)**: `Sequencer`, `MidiParser`, and `MidiVoiceControl` now read live voice/pattern/morph state directly from the owner modules instead of the shared cache header. `presetLoad_finalizeTempBackgroundLoad()` is exposed through the `TempPlaybackSwitch` interface so `sequencer.c` no longer needs `PresetLoadCache.h` for the finalizer call. `PresetLoadCache.c/.h` were deleted, and the remaining parser/session bridge now lives inside `mainboard/LxrStm32/src/uARTFrontSYX/frontPanelParser.c` as the transitional holdover. `make -C mainboard/LxrStm32 -j4 stm32` is green after the cutover, and the user re-test confirmed the Phase 7 cutover behaves correctly.

**Current status after Session 015 implementation pass (2026-06-13)**: Phase 8 has started in code. `mainboard/LxrStm32/src/Preset/ParameterArray.c/.h` now owns the parameter table plus the classification helpers that used to live in `ParameterMap.c`, `ParameterMap.c` and `ParameterMap.h` have both been deleted, `mainboard/LxrStm32/src/MIDI/ParameterArray.h` has been deleted, and `TempPlaybackSwitch` now stores its transition flags inside one explicit state struct with legacy names preserved only as macros. Phase 9 now covers the prefix cleanup: the temp-switch state object is `preset_tempPlaybackSwitchState`, the AVR restore handshake mailboxes are `preset_tmpKitHandshakeReady` / `preset_tmpKitHandshakeAck`, and `make -C mainboard/LxrStm32 -j4 stm32` is green again after a clean rebuild. The remaining Phase 9 judgment call is whether `ParameterIngress` should stay as the final narrow router or be collapsed further in a later pass; Phase 10 will extract the live-apply helpers, Phase 11 will rename the remaining Preset exports to `preset_`, and Phase 12 will move the remaining Preset-state accessors out of `Sequencer`. The remaining stale `seq_` / `Seq` ownership names are being cataloged separately, starting with the restore queue/cursor block and the push-disable flag. Later in Session 015, the AVR front-panel encoder was reworked onto a Timer1 compare polling path, the dead 2-step API was removed, and the user re-tested it successfully even though reversals and fast spins still feel rough.

**Current status after Session 016 implementation pass (2026-06-13)**: The AVR front-panel main encoder is now hardware-approved with the final Timer1 16 kHz rest-phase FSM. The only supported rotation read is `encode_stableRead4()`, `front/LxrAvr/encoder.h` exposes only `encode_init()`, `encode_stableRead4()`, and `encode_readButton()`, and `front/LxrAvr/config.h` no longer has `ENC_USE_STABLE_DRIVER`. The physical rest phase is fixed at `AB=11` (`ENCODER_REST_STATE = 0x03`), and the decoder emits detents only after a legal full sequence leaves and returns to that rest phase. Do not reintroduce legacy read modes, PCINT decoding, Timer0 encoder sampling, or the temporary LCD/debug hooks used during Session 016. `make -C front/LxrAvr avr -j4` and `make firmware` are green, with the usual AVR IO-register warnings.

**Current status after Session 017 closeout (2026-06-14)**: Phase 10/11 Preset cleanup is complete and the remaining Phase 12 review is retrospective only. `KitState`, `EndpointRestore`, `MorphEngine`, `ParameterIngress`, `TempPlaybackSwitch`, `sequencer.c`, and the parser/MIDI call sites were switched to the canonical `Preset*` / `preset_*` names, the last Preset-owned `seq_` aliases were removed, and the UART send-side work is now clearly tracked as protocol refactor work rather than more Preset ownership cleanup. The STM32 build was re-run successfully after the rename sweep, and the user re-tested the firmware and reported nothing obviously broken.

**Current status after Session 018 closeout (2026-06-14)**: The front-panel send helpers are now consolidated behind `uARTFrontSYX/frontPanelSendingProtocol`, the MIDI parser is split into `ChannelMidiParser` and `GlobalMidiParser`, and the transitional front-panel load/session bridge now lives in `Preset/PresetLoadCache` instead of `frontPanelParser.c`. The STM32 build was re-run successfully after the split work, and the user re-tested the key front-panel and MIDI paths: menu parameter change, file load, copy-to-temp, temp switch, MIDI clock sync, global CC1, voice CC1, and voice note trigger.

**Current status after Session 019 closeout (2026-06-14)**: The AVR front-panel main encoder was retuned after slow counter-clockwise missed decrements returned. The current hardware-approved implementation still uses only Timer1 compare A and raw `encode_stableRead4()` detents, but now samples at roughly 32.05 kHz with six stable phase samples, fixed `AB=11` rest anchoring, narrow rest-jump recovery for filtered `11 -> 00 -> adjacent -> 11` contact sequences, and 192-sample button debounce. Edit-mode parameter acceleration is config-driven from complete emitted detents only, with `ENC_ACCEL_MIN_REV_PER_SEC = 1`, `ENC_ACCEL_MAX_REV_PER_SEC = 2`, and final hardware-tested `ENC_ACCEL_MAX_MULT = 4`. Acceleration is applied only to menu edit-mode value changes; navigation/load/save/copy-clear selection remain unaccelerated. `make -C front/LxrAvr avr -j4` and `make firmware` are green with the usual AVR warnings, and the user reported the final encoder behavior is good.

**Current status after Session 020 closeout (2026-06-14)**: The refactor finalization pass corrected the Session 018 planning regression and completed the cache/protocol split objectives. `mainboard/LxrStm32/src/Preset/PresetLoadCache.c/.h` are gone again, no active `presetLoad_*` cache API remains, file loads route directly to normal Preset/Pattern storage, and the existing normal/temp Preset and Pattern switching remains the only supported staging model. STM front-panel receive is now `mainboard/LxrStm32/src/uARTFrontSYX/frontPanelReceivingProtocol.c/.h`, STM front-panel send is `mainboard/LxrStm32/src/uARTFrontSYX/frontPanelSendingProtocol.c/.h`, and the old STM `FrontPanelProtocol.h` / `frontPanelParser.h` shim headers were removed after include redirection. At that point the AVR side was still using the older `frontPanelReceivingProtocol.c/.h` and `frontPanelSendingProtocol.c/.h` names; Session 021 later renamed that layer to `front/LxrAvr/avrComms/` with `avrComms*` / `avrCommsParser*` prefixes and removed the AVR `frontPanelParser.h` shim. The final cleanup also renamed front-panel receive state from `frontParser_midiMsg` to `frontParser_command`, moved the internal CC/CC2 parameter-apply ladder from `MIDI/MidiParser.c` to `uARTFrontSYX/frontPanelReceivingProtocol.c` as `frontParser_applyParameterCommand()`, renamed `MidiVoiceControl.c/.h` to `MidiOutputControl.c/.h`, and moved Sequencer MIDI fan-out behind `outputControl_*` helpers. `make -C mainboard/LxrStm32 -j4 stm32`, `make -C front/LxrAvr avr -j4`, and `make firmware` were verified, and the user hardware-tested Steps 1, 2, and 3 successfully.

**Current status after Session 021 closeout (2026-06-15)**: The AVR comms layer was renamed and consolidated under `front/LxrAvr/avrComms/` with `avrCommsReceivingProtocol.c/.h`, `avrCommsSendingProtocol.c/.h`, and `avrCommsPanelParser.h`. AVR-side identifiers now use the `avrComms*` / `avrCommsParser*` prefixes, while STM-side front-panel ownership remains under `mainboard/LxrStm32/src/uARTFrontSYX/` with `frontPanel*` names. This session was a naming and documentation consolidation pass, so no firmware behavior changed; the most recent compiled verification remains the prior AVR build.

**Current status after Session 022 closeout (2026-06-16)**: Copy-to-temp now snapshots the actually playing per-track pattern sources instead of a generic pending-pattern view, and the temp-pattern repeat path is keyed off `SEQ_TMP_PATTERN` so the temp slot keeps holding until the user makes a manual change. The `Sequencer` API surface was documented in-place in both `sequencer.h` and `sequencer.c`, the stale Sequencer temp-boundary declaration was removed, Pattern/PatternData naming and comments were cleaned up, and `make -C mainboard/LxrStm32 -j4 stm32` was verified again. This pass was implementation plus documentation cleanup only; no comms spec updates were needed.

**Current status after Session 023 closeout (2026-06-16)**: The AVR load-page control was renamed to the 5-state background-load selector `PAR_FILE_LOAD_BACKGROUND`, with `TEXT_FILE_LOAD_BACKGROUND`, `backgroundLoadNames`, and `SEQ_LOAD_BACKGROUND` updated on the AVR side while STM runtime behavior stayed untouched. The packed menu-ID table used the free `MENU_FILE_LOAD_BACKGROUND = 0` slot because the existing dtype encoding only has four bits for the menu ID, and the globals file continues to round-trip the same raw byte without a serializer change. `make -C front/LxrAvr avr -j4`, `make -C mainboard/LxrStm32 -j4 stm32`, and `make firmware` all passed again after the rename. `TEMP_LOAD_MENU_AUDIT.md`, `knowledge_files/log_archive/000_SESSION_INDEX.md`, `knowledge_files/log_archive/023_SESSION_HANDOFF_LOG.md`, `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md`, and `knowledge_files/comms_spec_reference/TEMPORARY_PAT_PARAM_LOAD_SPEC.md` were updated to keep the new terminology canonical.

**Current status after Session 024 closeout (2026-06-17)**: The opcode audit closeout commented out the stale AVR and STM opcode constants plus the thin cache-only helper surface, while leaving the live non-cache file-load/session path untouched. `front/LxrAvr/avrComms/` and `mainboard/LxrStm32/src/uARTFrontSYX/` now keep the old cache handshake only as commented-out historical context, the durable audit details were folded into `knowledge_files/log_archive/024_SESSION_HANDOFF_LOG.md`, and `OPCODE_AUDIT.md` was removed as part of the handoff. This was a documentation and cleanup pass rather than a new firmware feature, so no new hardware behavior was introduced.

**Current status after Session 025 closeout (2026-06-17)**: The legacy macro path is intentionally disabled. AVR file loads zero the macro slots before the live arrays see them, the AVR performance-page macro send/apply branches are commented out, the STM front-panel macro receive/apply branches and CC2 macro-amount handler are disabled, and the Preset macro storage/replay helpers are inert stubs. The visible menu no longer shows the macro controls, the comms/spec docs now treat `MACRO_CC` as deprecated historical context, and the implementation audit was promoted into `knowledge_files/log_archive/025_SESSION_HANDOFF_LOG.md` so `AUDIT_MACRO_REMOVAL.md` can be deleted later.

**Current status after Session 026 closeout (2026-06-18)**: The PERF-page individual voice morph controls are connected end-to-end. AVR edits for `PAR_MORPH_DRUM1` through `PAR_MORPH_HIHAT` use full `0..255` values and dedicated low/high `VOICE_MORPH` traffic instead of generic `CC_2`; STM receives those packets through `FRONT_SEQ_VOICE_MORPH` and writes direct per-voice current morph state for the morph worker. Global morph remains authoritative and synchronizes all six individual voice morph display slots. MIDI CC1 global/voice morph paths remain 7-bit inputs but report the resulting full-range display values to AVR. Velocity-to-voice-morph is trigger-time only via `preset_applyVelocityVoiceMorphOnTrigger()` from `voiceControl_noteOn()`; generic velocity modulation nodes must not own or apply `TYPE_UINT8_VMORPH`. LFO-to-voice-morph remains the async morph-drain overlay and should not be rewritten. The encoder `DTYPE_MENU` list mismatch is unresolved; invalid attempted fixes were reverted and the useful findings are in root `ENC_LIST_MISMATCH_ERROR.md`.

**Current status after Session 027 closeout (2026-06-19)**: Front-panel live recording of parameter edits now writes the correct step automation destination. `Step.param1Nr` / `param2Nr` use raw AVR/menu `PAR_*` ids consistently; front-panel live recording and manual step-destination edits both write raw ids, automation playback converts raw low destinations to the low `MIDI_CC` apply-domain form immediately before `frontParser_applyParameterCommand()`, and external MIDI recording uses `seq_recordAutomationMidiDestination()` to convert MIDI-domain ids back to raw step storage. The hardware-confirmed example was Drum 2 volume no longer playing back as Drum 1 volume. The clarified encoder `DTYPE_MENU` issue was also fixed: encoder edit mode was selecting the correct parameter, but out-of-range raw byte values were clamped to the last list item; both encoder edit paths now normalize `DTYPE_MENU` values into compact menu-list index range before applying the encoder delta. A final clear-menu patch makes encoder press execute the selected SHIFT+COPY clear target instead of falling through to the generic shifted encoder press behavior.

**Current status after Session 028 closeout (2026-06-21)**: Background loading via temporary data is feature-complete and hardware-confirmed. AVR `.pat`, `.prf`, and `.all` loads use the 5-state `PAR_FILE_LOAD_BACKGROUND` policy to decide whether to send the active `SEQ_BACKGROUND_SWAP_BEGIN` / `SEQ_BACKGROUND_SWAP_DONE` handshake (`0x6d/0x6e`). STM copies the currently audible pattern/kit state to temp, forces an immediate switch to `SEQ_TMP_PATTERN`, waits for readiness plus the ACK delay, then lets the ordinary file load write normal storage while playback continues from temp. `.pat` background loading is pattern-only and keeps preset parameters normal. File loads no longer reset current global/per-voice morph amounts, `.prf` / `.all` no longer zero the AVR per-voice morph menu values, repeated loads while already playing temp skip another copy-to-temp, and AVR LED/UI hints show temp playback while disabling `SHIFT+PERF`.

**Current status after Session 029 closeout (2026-06-21)**: The Global-channel MIDI CC/NRPN implementation is complete and user-tested. For MIDI CC messages, the configured Global channel now pre-empts overlapping voice-channel assignments. Global CC0 bank change and Global CC1 morph intentionally keep the existing `ChannelMidiParser.c` behavior; Global CC2-127 now route only through `GlobalMidiParser.c` and its separate lookup table. Global NRPN uses CC99/CC98 as coarse/fine selectors and CC6 as data entry for NRPN0-92, translating high targets into the existing internal `MIDI_CC2` apply path. Non-global Channel MIDI behavior remains in `ChannelMidiParser.c`. The complete durable external MIDI table now lives in `knowledge_files/comms_spec_reference/MIDI_TABLE.md`.

**Current status after Session 030 closeout (2026-06-26)**: Sample import phases 1-3 are complete and user-tested. STM sample import is bounded to internal flash sectors 8-11, PCM writes are capped before the `SampleInfo` table, and the committed sample count is written last. `SampleInfo` remains a 12-byte on-flash ABI with bit 31 of `size` used as the loop flag. Imported-sample oscillator playback uses 32-bit sample positions, 17-bit fractional advance, trigger-time reset, one-shot end gating, and loop wrapping. `Load: Samples` now imports dense `/samples` one-shots first and optional `/loops` loops second; invalid/empty/odd/too-large/unreadable WAV candidates do not consume waveform slots. AVR waits for parsed `SAMPLE_UPLOAD_RESULT` while processing numbered sample/loop progress packets instead of waiting for a raw ACK. The final sample-import details live in `knowledge_files/log_archive/030_SESSION_HANDOFF_LOG.md` and `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md`.

**Current status after Session 031 closeout (2026-06-28)**: Sample import/display was redone cleanly after reverting the experimental work from the session. STM now builds a sorted cached 8.3 WAV-name list per active folder, sorts digit runs numerically while treating `_`, `-`, and spaces as separators, sends immediate real one-based progress for the current accepted file, and uses 128-frame PCM transfer blocks. STM sample progress/result packets use priority-wait transport. AVR sample upload has no fixed timeout and no `Load timeout`; it waits for parsed `SAMPLE_UPLOAD_RESULT` and switches locally to `Writing Flash` dots after numbered progress idles for roughly two seconds. `SAMPLE_LOAD_REDO_AUDIT.md` is superseded by `knowledge_files/log_archive/031_SESSION_HANDOFF_LOG.md`.

**Current status after Session 032 closeout (2026-06-28)**: Oscillator waveform interpolation implementation is complete and wired to the global `PAR_OSC_WAVE_INTERPOLATION` menu parameter. Waveform parameters now route through `osc_setWaveInterp` to write fractional blend targets to the oscillators, capped at a single active slot per block via `OSC_WAVE_INTERP_MAX_ACTIVE`. Double-render crossfade blocks correctly manage target `phaseInc` updates (`osc_setFreq`) to prevent pitch doubling and explicitly merge `samplePosition` updates back into standard oscillator state to fix looping squeals for samples. The experimental `OSC_WAVE_INTERP_FINALIZE.md` document has been superseded and removed.

**Current status after Session 033 pass (2026-06-29)**: The committed Session 033 change (`43f5006`) restored `SHIFT+PLAY` around STM temporary preset storage. Ordinary `.snd` kit loads, instrument loads, morph loads, `.prf`, and `.all` loads now mirror the appropriate endpoint subset into temp; protected `.prf` / `.all` background loads still preserve the old audible temp preset image only until playback returns to normal, then `preset_resnapshotTemporaryPresetFromNormal()` refreshes temp from the newly loaded normal preset. `PATCH_RESET` now means STM-owned temp-to-normal preset endpoint restore and is ignored while protected temp preset playback is still active. `PAR_VOICE_DECIMATION_ALL` now clamps imported `0` to `127`, and AVR startup seeds that menu value to `127`. The current local branch also carries uncommitted Euclid-page rollback WIP: hold `SHIFT`, then press `PERF` enters `SELECT_MODE_PAT_GEN` / `EUKLID_PAGE`; the first edit to each touched track snapshots that normal track into `seq_tmpPattern` once per visit, leaving the page commits, and pressing `SHIFT+PERF` again restores all touched tracks via the multiplexed `SEQ_EUKLID_RESET` visit-control path. AVR and STM sub-builds passed for this WIP; fresh hardware confirmation is still pending.

**Current status after Session 034 closeout (2026-07-04)**: Global SHIFT toggle is now treated as an effective button-level latch: non-toggler code should use `buttonHandler_getShift()` and must not read raw SHIFT DIN state or `shiftState`. Step automation is now a one-step override released from STM `/Preset/` baselines: ordinary parameters and voice morph automation release on the next active step for the track, immediate lane target change/clear, stop, or pattern change. `PAR_VOICE_DECIMATION_ALL` / global SampleRt step automation is special-cased as shared state with last-owner track/lane release. STM canonical Preset storage now also protects global decimation: startup and endpoint-restore zeroing seed it to `127`, stored endpoint `0` normalizes to `127`, and live/menu shared-parameter writes refresh `interpolatedParams[]` so step automation releases to the current menu baseline. The user spot-tested global decimation automation and accepted the final restore-baseline behavior; SHIFT-toggle still wants a final hardware smoke test if not already covered outside the transcript.

**Current status after Session 035 closeout (2026-07-25)**: External DIN and USB system-realtime statuses (`0xf8..0xff`) now bypass their ordinary input queues at their receive boundaries, enter separate 16-event SPSC queues with DWT cycle timestamps, and are dispatched DIN-first immediately before the next audio block render. `NVIC_PriorityGroup_1` assigns audio DMA `0/0`, DIN USART2 `0/1`, and USB `1/3`; capture remains timestamp-and-queue only, with parsing/routing/sequencer work in main context. The initial DIN queue-only waveform test showed little difference; the final timestamp/priority/USB extension is build-verified but still needs the documented hardware timing matrix. It reduces cooperative-loop jitter only: fixed serial, USB host-frame, ping-pong buffer, I2S, and DAC latency remain uncompensated.

**Current status after Session 036 closeout (2026-08-20)**: Investigated a user-reported bug (loading a saved song ignores step probability on the Clap/Cym track) via file forensics on the attached `.ALL`/`.PRF`/`.SND` and a full AVR/STM data-path trace; the full record is root `PROBABILITY_INVESTIGATION.md`. Confirmed a standalone defect in `avrCommsReceivingProtocol.c`'s `SEQ_CHANGE_PAT` ACK handler: while the front panel is on `PATTERN_SETTINGS_PAGE` (hold `SHIFT` while already on the PERF page — **not** the separate `EUKLID_PAGE`/`SELECT_MODE_PAT_GEN` Euclid generator page, see the existing note below on that exact mix-up), the handler updated only the local `menu_shownPattern` and never sent `SEQ_SET_SHOWN_PATTERN` to the STM, despite a comment claiming the update was deferred for a "shift button release handler" flush that never actually existed. This let `frontParser_shownPattern` on STM (the pattern step-parameter edit opcodes `FRONT_SEQ_PROB`/`VOLUME`/`NOTE`/`FRONT_SET_P1_VAL`/`P2_VAL` all resolve against) get stuck on a stale pattern — including `SEQ_TMP_PATTERN` across a background-load temp/normal boundary crossing — with no bounded-time recovery, silently stranding step edits where neither playback nor Save would ever see them. Fixed by making the `PATTERN_SETTINGS_PAGE` branch call the existing `menu_setShownPattern(patMsg)` helper (used elsewhere already) instead of a raw local assignment; the fix deliberately keeps the `if`/`else` split so that branch's LED-refresh calls (which would otherwise clobber the page's repurposed rotation-indicator LEDs) stay skipped on that page. No STM-side change was needed. `make -C front/LxrAvr avr -j4` and `make firmware` both verified clean; **not yet hardware-tested**. Full detail: `knowledge_files/log_archive/036_SESSION_HANDOFF_LOG.md`, `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md` (`### 4c`), and `knowledge_files/comms_spec_reference/BACKGROUND_LOAD_TEMPORARY.md`. **Post-closeout correction, same session**: the original bug report's actual root cause is a *different* defect, found after this closeout when the user pointed out `P075.ALL` genuinely does contain a non-default probability (25) on Clap/Cym pattern 1 step 12 that has no effect — Part 1's original forensic scan missed it by only checking `STEP_ACTIVE_MASK`-active steps. The real mechanism: the front panel's step-edit addressing (`buttonHandler_selectActiveStep()` and siblings, `buttonHandler.c`) always targets raw sub-step index `mainStepIndex*8` (row 0 of that step's 8-slot group) with no awareness of sub-step rotation, while this track has a baked-in `+1` sub-step rotation (`euklid_setSubStepRotation()`/`euklid_rotatePattern()`) that correctly moved the live note to row 1 of each group without moving the main-step on/off bitmask. Editing "step 12" therefore silently writes probability to the vacated, inactive row-0 slot instead of the row-1 slot that's actually playing. Verified as the sole explanation for every non-default `prob` byte in the file (4 total, all in this track/pattern, all `STEP_ACTIVE_MASK`-inactive, all one raw slot before their track's actually-active row). Not yet fixed — see `PROBABILITY_INVESTIGATION.md` Part 8 for the full trace and the open question of whether the fix should be dynamic active-row resolution or a UX indicator; it touches the same three front-panel call sites (`buttonHandler_selectActiveStep()`, `buttonHandler_setRemoveStep()`, and the `SELECT_MODE_PAT_GEN` step-select variant) and needs a decision before implementation. **Second post-closeout addendum, same session**: the user separately reported two more rotation bugs, both investigated and fixed (`PROBABILITY_INVESTIGATION.md` Part 9, `knowledge_files/log_archive/036_SESSION_HANDOFF_LOG.md`). (1) Every `PAR_EUKLID_ROTATION`/`PAR_EUKLID_SUBSTEP_ROTATION` encoder nudge retriggered the voice while the sequencer was stopped — caused by a redundant `SEQ_SET_ACTIVE_TRACK` resend (`menu.c`) before each rotation opcode coinciding with STM's documented "preview the already-selected voice while stopped" gesture (`FRONT_SEQ_SET_ACTIVE_TRACK`, `frontPanelReceivingProtocol.c`); fixed by removing the redundant resend from exactly those two handlers (safe because track selection already happens once via the Euclid-page voice-button press before either handler can run). (2) Sub-step rotation could non-deterministically rotate the main-step on/off bitmask too — caused by `PAR_EUKLID_SUBSTEP_ROTATION`'s dial allowing 0-15 while `euklid_rotatePattern()` (`EuklidGenerator.c`) folds any delta `>7` into a whole main-step rotation; fixed by clamping the parameter to 0-7 (the user's own proposed fix, verified algebraically sufficient — every possible delta between two 0-7 values stays within ±7, so the fold path is never triggered). `menu.h`'s `Datatypes` dtype enum is hard-capped at exactly 16 entries (4-bit-packed) and had no free slot for a dedicated 0-7 dtype, so the clamp is a `paramNr == PAR_EUKLID_SUBSTEP_ROTATION` special case inside the existing `DTYPE_0B15` branch at its three `menu.c` clamp sites. Both fixes build-verified (`make -C front/LxrAvr avr -j4`, `make firmware`); **not yet hardware-tested**.

**Current status after Session 037 closeout (2026-08-22)**: Session 037 **disproved and fully reverted** Session 036's live-record roll fix, then found and fixed the real cause, which the user has now **hardware-verified**. Method note worth reusing: two rounds of static analysis both accepted a plausible-but-impossible mechanism; what resolved it was copying `seq_quantize()`, `seq_addNote()`, `seq_rollTrig()`, `seq_setRoll()` and `seq_checkRollStep()` **verbatim** into a host-side C harness driven by a replica of `seq_process()`'s per-tick ordering, then sweeping roll mode x roll rate x quantize x track scale x press timing x hold length. That sweep found **0 of 1070** symptom-producing configurations under the reported settings (proving the roll path cannot write sub-step 1 at all with quantization on) and additionally showed the Session 036 fix had introduced a regression that silently dropped notes for taps landing 2-3 sub-steps after a boundary. **Real root cause**: `seq_addNote()` chose its destination slot on velocity alone, treating any `vel == 0` event as a MIDI note-off and writing it at the raw un-quantized playhead index. A roll hit can legitimately carry velocity 0 (roll velocity dialled to 0, or `ROLL_MODE_TRIG`/`_NOTE` reading a zero-volume ghost step), and that raw index is always `mainStep*8 + 1` because `seq_stepIndex[track]` is advanced at the top of `seq_process()`'s per-track loop while `seq_stepIndex[NUM_TRACKS]` (which drives the quantize-boundary test) is only advanced at the very end — so the stray write always landed on sub-step 1, and `pat_setMainStep()` then un-masked the default-active sub-step 0, firing both. Self-sustaining, because the written step holds `0 | STEP_ACTIVE_MASK` and reads back as velocity 0 forever. **Fixed** by adding an explicit `isNoteOff` parameter to `seq_addNote()`; only `channelMidiParser_noteOn()`/`noteOff()` pass `1`, every internal trigger passes `0`. `seq_rollTrig()` is back to its original single-parameter form. Corroborated by the user's own decode of `P075.ALL` (all 16 notes at `8k+1` with sub-step 0 cleared, `prob=127` — the unmistakable `seq_addNote()` signature), which also supplies the missing half of the Session 036 probability bug: the probability bytes sit at `8k` where the step editor addresses, while the audible notes sit at `8k+1`. Full detail: `knowledge_files/log_archive/037_SESSION_HANDOFF_LOG.md` (which also **preserves the complete durable content of the now-deletable root docs `PROBABILITY_INVESTIGATION.md` and `LIVE_REC_DUPLICATE_SUBSTEP_BUG.md`**, including the file-forensics tables, offset verification, and a correction to an arithmetic error in the former's main-step bitmask claim). Also updated: `knowledge_files/comms_spec_reference/MIDI_TABLE.md` and `COMMS_FLOW_SPEC.md`.

**Current consolidation / protocol planning artifacts**: the durable finalization notes now live in `knowledge_files/log_archive/038_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/037_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/036_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/035_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/034_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/033_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/032_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/031_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/030_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/029_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/028_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/027_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/026_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/025_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/024_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/023_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/021_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/020_SESSION_HANDOFF_LOG.md`, `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md`, `knowledge_files/comms_spec_reference/BACKGROUND_LOAD_TEMPORARY.md`, and `knowledge_files/comms_spec_reference/MIDI_TABLE.md`. Current realtime MIDI timing behavior lives in `knowledge_files/log_archive/035_SESSION_HANDOFF_LOG.md`, `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md`, and `knowledge_files/comms_spec_reference/MIDI_TABLE.md`; root `SESSION_LOG_LATENCY_FIX.md` is the completed working trail and can be deleted once the archive is retained. Current step-automation and global shift-toggle behavior lives in `knowledge_files/log_archive/034_SESSION_HANDOFF_LOG.md`, `SHIFT_TOGGLE_AUDIT.md`, and `STEP_AUTOMATION_AUDIT.md`. Current sample-import behavior lives in `knowledge_files/log_archive/031_SESSION_HANDOFF_LOG.md` and `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md`; `SAMPLE_LOAD_REDO_AUDIT.md` is superseded and can be deleted after this closeout. Root files `SAMPLE_PLAN_PHASE_1.md`, `SAMPLE_PLAN_PHASE_1_POST_BUGS.md`, `SAMPLE_PLAN_PHASE_2.md`, `SAMPLE_PLAN_PHASE_3.md`, `SAMPLE_PLAN_PHASE_3_AUDIT_INVESTIGATION_1.md`, `SAMPLE_PLAN_PHASE_3_AUDIT_INVESTIGATION_2.md`, and `PHASE_3_CORRECTIONS_PART_2.md` were temporary Session 030 planning/debug docs and can be deleted if still present. Current background-load and reload behavior lives in `knowledge_files/log_archive/033_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/028_SESSION_HANDOFF_LOG.md`, `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md`, and `knowledge_files/comms_spec_reference/BACKGROUND_LOAD_TEMPORARY.md`. Current external MIDI behavior lives in `knowledge_files/log_archive/035_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/029_SESSION_HANDOFF_LOG.md`, and `knowledge_files/comms_spec_reference/MIDI_TABLE.md`; root files `GLOBAL_MIDI_AUDIT.md`, `GLOBAL_MIDI_TABLE_REQUEST.md`, and `GLOBAL_MIDI_TABLE_TO_CREATE.md` are temporary Session 029 planning docs and can be deleted after this closeout. The temporary root audits `TEMP_LOAD_MENU_AUDIT.md`, `TEMP_LOAD_ACK_AUDIT.md`, `FILE_LOAD_SWAP_PART2_AUDIT.md`, `FILE_LOAD_MORPH_CHANGE_BUG_AUDIT.md`, `BG_LOAD_SWITCH_AUDIT.md`, `AUDIT_PER_VOICE_MORPH_PAT_BG_TWEAKS.md`, and `AUDIT_BG_LOAD_LED_HINT.md` can be deleted after Session 028 closeout. `KIT_RELOAD_AUDIT.md` and `PAT_REALIGN_AUDIT.md` are the Session 033 root audit trails; the handoff log is now the durable summary, but those two files still hold detailed implementation notes and should be kept until the user decides whether to prune them. Current encoder hardware details live in `knowledge_files/log_archive/019_SESSION_HANDOFF_LOG.md` and supersede the temporary `ENCODER_AUDIT.md`; the resolved encoder `DTYPE_MENU` value-domain fix lives in `knowledge_files/log_archive/027_SESSION_HANDOFF_LOG.md`. `AUDIT_LIVE_REC_WRONG_PARAMS.md` and `ENC_DTYPE_MENU_WRONG.md` were temporary Session 027 working docs and can be deleted after this closeout. `VOICE_MORPH_CONTROLS_AUDIT2.md` was the temporary working doc for Session 026 and can be deleted after the closeout pass if the archived handoff is sufficient. `OPCODE_AUDIT.md` was the temporary working doc for Session 024 and was removed after the handoff. `REFACTOR_FINALIZATION.md`, `REFACTOR_MY_MESS_CLEANUP.md`, `PRESET_CONSOLIDATION_AUDIT.md`, and `MIDI_UART_SPLIT_AUDIT_EDIT.md` were temporary working docs and may be deleted after Session 020. Root `PROBABILITY_INVESTIGATION.md` and `LIVE_REC_DUPLICATE_SUBSTEP_BUG.md` were the Session 036/037 working audit trails. **Their full durable content — forensic data tables, offset verification, code citations, and the reverted-fix record — was migrated into `knowledge_files/log_archive/037_SESSION_HANDOFF_LOG.md` at Session 037 closeout specifically so both root files can be deleted.** Read the 037 log, not those files, for that material; the 037 log also corrects an arithmetic error in `PROBABILITY_INVESTIGATION.md`'s main-step bitmask claim (`0x1800` sets bits 11 and 12 only, i.e. main steps 12 and 13 — not bits 5/8/9/11 as that document stated). The Session 038 root working docs (`S038_STUCK_MIDI_ROLL_BUG.md`, `S038_STUCK_ROLL_IMPLEMENTATION.md`) and the roll-trigger design docs (`MIDI_ROLL_TRIGGER.md`, `MIDI_ROLL_TRIGGER_IMPLEMENTATION.md`) were deleted in commit `7a21412`; their durable content is in `knowledge_files/log_archive/038_SESSION_HANDOFF_LOG.md` (they remain readable in git history). The six Session 039 root working docs (`S039_P1_FM_BUGFIX.md`, `S039_P1_IMPLEMENTATION.md`, `S039_P2_CC_RECEP_MIDI_FIX.md`, `S039_P2_IMPLEMENTATION.md`, `S039_P3_MIDI_TX_CC.md`, `S039_P3_IMPLEMENTATION.md`) are superseded by `knowledge_files/log_archive/039_SESSION_HANDOFF_LOG.md` plus `MIDI_TABLE.md` / `COMMS_FLOW_SPEC.md`, and are being deleted by the user (also preserved in commit `cdc5242`).

**Current status after Session 038 closeout (2026-09-26)**: The user hardware-tested the MIDI roll-trigger feature (planned and implemented 2026-09-17 in commits `86dde2a`/`52d4ca4`/`9c9c67f`/`9576ae5` outside a numbered session; design in root `MIDI_ROLL_TRIGGER.md` / `MIDI_ROLL_TRIGGER_IMPLEMENTATION.md`) from an Octatrack. Every MIDI-triggered roll stuck until any MIDI channel was changed. **Root cause**: the feature read note statuses literally (its Settled Decision #4), so the Octatrack's NOTE_ON-velocity-0 release was counted as a second press by per-voice hold counters that then never returned to zero. This was reproduced on the pre-fix code with a verbatim host harness. **Fixed** (user decisions D1-D4): NOTE_ON velocity 0 is now a note-off on every external note route. MIDI roll ownership is a 16-slot `(channel, note)` held-key table in `MidiParser.c`: a release frees exactly the voice mask its press captured, duplicate presses are idempotent, unmatched releases are ignored, and the release lookup is ungated. Sequencer stop (every `seq_setRunning(0)`) clears all MIDI roll holds, as does disabling the RX note filter. MIDI-held rolls play while stopped on a private tempo-locked clock (`seq_tickStoppedRolls()`: immediate hit, then `seq_tempRate` repeats, roll velocity/note regardless of roll mode). Start hands a still-held roll to the normal quantized engine. Manual front-panel rolls are unchanged, and silent while stopped. **Post-review fix**: `seq_setRoll()`'s early-roll window now uses `(uint8_t)seq_stepIndex[NUM_TRACKS]`, because the transport-start master index `-1` fired a spurious early hit, so any roll held through start flammed on the downbeat (hits 0,1,9,17 -> now 1,9,17). Verified by an independent verbatim host harness (34/34), an STM32 build with only the pre-existing warnings, and `FIRMWARE.BIN` confirmed to embed the rebuilt 245628-byte STM binary. **Not yet hardware-tested.** `MIDI_TABLE.md` now also documents that, with the offset on, chromatic-mode tracks split at the roll offset. Full detail: `knowledge_files/log_archive/038_SESSION_HANDOFF_LOG.md` (the durable record; the root S038 working docs were deleted in `7a21412`), `knowledge_files/comms_spec_reference/MIDI_TABLE.md`, and `COMMS_FLOW_SPEC.md` (`#### Session 038 MIDI roll ownership`).

**Current status after Session 039 closeout (2026-10-16)**: Three phases implemented on `dev-roll-midi`, **none hardware-tested**.
- **P1:**
  - The user-reported "NRPN 93 attenuates one track" was never NRPN 93: NRPN senders such as Elektron append a CC38 data-entry LSB, and Global CC38 is Drum 1 filter cutoff (applied and stored). Fixed opt-in by 8 encoder-only, pot-locked **¢ RxFilter twins** (menu 16..23, wire bit `0x10`) that consume Global CC38 and map **NRPN 96** to Drum 1 cutoff.
  - **RPN-null** (CC100/101 = 127 right after an NRPN data entry) is swallowed.
  - **NRPN 94/95** are roll note/velocity, with STM->AVR PERF display reports `SEQ_REPORT_ROLL_*` `0x71-0x73` for NRPN 93-95.
  - **Imported-sample FM** is real phase modulation (`calcUserSampleOscFmBlock()`, `<<14`).
  - **Hihat modulator** samples re-arm on trigger.
- **P2:** RX filter **C** now gates all incoming CC/NRPN at the parser entry. Before, CCs were always applied and C only gated front-panel echo/recording.
- **P3:**
  - TX filter **C** transmits **menu** edits (new AVR->STM opcodes `0xca`/`0xcb`), PERF roll (NRPN 93-95) and global morph (CC1) as Global-channel CC/NRPN.
  - It uses a coalescing paced queue, with addresses from the inverse of the receive tables, a ¢ mirror (Drum 1 cutoff -> NRPN 96) and a CC100/101 = 127 guard.
  - Also: whole-message-or-drop DIN and USB MIDI out, and a USB IN fix: `DataIn` busy flag, 5 ms timeout, discard when unconfigured.
- **Builds:** STM 248,020; AVR 56,570 of a 57,344 limit; `FIRMWARE.BIN` 305,408. The verbatim P3 host harness passes. Firmware version 1.03.
- The root `S039_*.md` working docs are superseded: their content is in `knowledge_files/log_archive/039_SESSION_HANDOFF_LOG.md`, `MIDI_TABLE.md` and `COMMS_FLOW_SPEC.md`, and the user is deleting them.

Canonical current WIP docs:
- `knowledge_files/log_archive/039_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/038_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/037_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/036_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/035_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/034_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/033_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/032_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/031_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/030_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/029_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/028_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/027_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/026_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/024_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/025_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/023_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/022_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/021_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/020_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/019_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/018_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/017_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/016_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/015_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/014_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/013_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/012_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/011_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/010_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/009_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/008_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/007_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/006_SESSION_HANDOFF_LOG.md`
- `knowledge_files/log_archive/000_SESSION_INDEX.md`
- `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md`
- `knowledge_files/comms_spec_reference/BACKGROUND_LOAD_TEMPORARY.md`
- `knowledge_files/comms_spec_reference/MIDI_TABLE.md`

Session 005 closeout / remaining follow-up:
- Encoder follow-up was most recently completed in Session 019. The hardware-approved implementation is the Timer1 ~32.05 kHz fixed-`AB=11` rest-phase FSM with six stable phase samples, narrow rest-jump recovery, 192-sample button debounce, raw `encode_stableRead4()` rotation reads, and edit-mode-only acceleration. Keep it unchanged unless new hardware testing reveals a regression.
- Background loading via temporary data was completed in Session 028. Keep the active `0x6d/0x6e` background-swap handshake and do not revive the old cache/load-fast model.
- Keep the temporary SEQ16 pattern keyhole in place until after the future preset/morph refactor.
- `PATCH_RESET` is now an STM-owned temp-to-normal preset reload request. It must stay disabled while protected `.prf` / `.all` temp preset playback is still active.
- `PAR_VOICE_DECIMATION_ALL` must never survive as canonical stored `0`; AVR file import and STM Preset endpoint/interpolated storage normalize it to `127`, and both AVR menu startup and STM Preset image startup seed it to `127`. Live step automation values are not normalized.
- The real Euclid page is `SELECT_MODE_PAT_GEN` / `EUKLID_PAGE` from holding `SHIFT` then pressing `PERF`; do not confuse it with the separate pattern-settings page flow.
- The separate pattern-settings page (`PATTERN_SETTINGS_PAGE`, from holding `SHIFT` while already on the PERF page, `menu_shiftPerf()`) must always flush its shown-pattern state to STM via `menu_setShownPattern()` on every `SEQ_CHANGE_PAT` ACK, even though its LED-refresh calls stay skipped on that page (Session 036 fix). Do not reintroduce a direct `menu_shownPattern = ...` assignment in that branch.

The `knowledge_files/session_in_flight/` directory was cleaned up (post-Session 020). The following audits no longer exist on disk and should not be referenced or recreated:
- `knowledge_files/session_in_flight/AUDIT_MORPH_MOVE.md`
- `knowledge_files/session_in_flight/TMP_VARS_AUDIT.md`
- `knowledge_files/session_in_flight/PRF_ALL_LOAD_FIX_AUDIT-IN_FLIGHT.md`
- `knowledge_files/session_in_flight/COMMS_FLOW_AUDIT-IN_FLIGHT.md`
- `knowledge_files/session_in_flight/TEMP_CACHE_LOAD-IN_FLIGHT-POST_MORPH_MOVE.md`
- `knowledge_files/session_in_flight/COMMS_FLOW_AUDIT-IN_FLIGHT-POST_MORPH_MOVE.md`

User-referenced checkpoints:
- Commit `90d3f08` is the checkpoint where `.ALL` and `.PRF` load their parameters correctly provided there is no morph automation and background loading into the temp slot is turned off for `.PRF`.
- Session 003 final code is the checkpoint where standard STM-owned morph sounds correct, but normal/temp parameter exchange is broken.

---

## Repository Layout

```text
./LXR/
├── LICENSE.txt
├── MEMORY.md
├── Makefile
├── README.md
├── conflicts.txt
├── firmware image/
│   └── FIRMWARE.BIN
├── front/
│   ├── LxrAvr/
│   │   ├── Hardware/
│   │   ├── IO/
│   │   ├── Menu/
│   │   ├── Preset/
│   │   ├── avrComms/
│   │   ├── Makefile
│   │   └── README.md
│   └── LxrAvr_bootloader/
│       ├── Bootloader/
│       ├── default/
│       ├── elmChan/
│       ├── lcd/
│       └── README.md
├── knowledge_files/
│   ├── SESSION_HANDOFF_TEMPLATE.md
│   ├── comms_spec_reference/
│   ├── hardware_archive/
│   │   ├── front/
│   │   │   ├── AVR_SETUP_ALLOCATION.md
│   │   │   └── AVR_HARDWARE.md
│   │   ├── main/
│   │   │   ├── STM32F4_SETUP_ALLOCATION.md
│   │   │   └── STM32F4_HARDWARE.md
│   │   └── ATMEGA_STM32F4_COMMS_AUDIT.md
│   ├── log_archive/
│   │   ├── 000_SESSION_INDEX.md
│   │   ├── 001_SESSION_HANDOFF_LOG.md
│   │   └── ...
│   └── reference_material/
├── mainboard/
│   ├── LxrStm32/
│   │   ├── Libraries/
│   │   ├── Makefile
│   │   ├── src/
│   │   │   ├── AudioCodecManager/
│   │   │   ├── DSPAudio/
│   │   │   ├── Hardware/
│   │   │   ├── MIDI/
│   │   │   ├── Preset/
│   │   │   ├── SampleRom/
│   │   │   ├── Sequencer/
│   │   │   └── uARTFrontSYX/
│   │   └── stm32_flash.ld
│   └── LxrStm32_bootloader/
│       ├── Libraries/
│       ├── Release/
│       ├── src/
│       └── stm32_flash.ld
├── requirements.txt
└── tools/
    ├── FirmwareImageBuilder/
    ├── file_decode.py            # user-authored: decode .pat/.prf/.all to readable text
    ├── rotation_correction.py    # user-authored: rotate step data / main-step masks in saved files
    └── bin/
        ├── FirmwareImageBuilder
        ├── FirmwareImageBuilder.exe
        └── makeFirmware.bat
```

### Where to look for things

| Question | File |
|----------|------|
| Which session introduced a fix? | `knowledge_files/log_archive/000_SESSION_INDEX.md` |
| Full details of a fix or decision? | `knowledge_files/log_archive/00x_SESSION_HANDOFF_LOG.md` |
| Session handoff template / formatting | `knowledge_files/SESSION_HANDOFF_TEMPLATE.md` |
| Confirmed hardware/protocol notes? | `knowledge_files/hardware_archive/` |
| AVR hardware and setup allocation | `knowledge_files/hardware_archive/front/AVR_HARDWARE.md`, `knowledge_files/hardware_archive/front/AVR_SETUP_ALLOCATION.md` |
| STM32 hardware and setup allocation | `knowledge_files/hardware_archive/main/STM32F4_HARDWARE.md`, `knowledge_files/hardware_archive/main/STM32F4_SETUP_ALLOCATION.md` |
| Current comms specs | `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md`, `knowledge_files/comms_spec_reference/BACKGROUND_LOAD_TEMPORARY.md`, `knowledge_files/comms_spec_reference/MIDI_TABLE.md` |
| Reference material snapshots | `knowledge_files/reference_material/` |
| Current known issues and reminders? | `MEMORY.md` |
| Project introduction for new users and end-user semantics? | `README.md` |
| Session 003 STM morph move details | `knowledge_files/log_archive/003_SESSION_HANDOFF_LOG.md` |
| Session 004 temp/background loading closeout | `knowledge_files/log_archive/004_SESSION_HANDOFF_LOG.md` |
| Session 006 refactor planning details | `knowledge_files/log_archive/006_SESSION_HANDOFF_LOG.md` |
| Session 007 refactor Phase 1 details | `knowledge_files/log_archive/007_SESSION_HANDOFF_LOG.md` |
| Current preset/morph refactor knowledge | `knowledge_files/log_archive/020_SESSION_HANDOFF_LOG.md`, `knowledge_files/comms_spec_reference/BACKGROUND_LOAD_TEMPORARY.md`, `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md` |
| Current comms/protocol knowledge | `knowledge_files/log_archive/024_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/021_SESSION_HANDOFF_LOG.md`, `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md`, `knowledge_files/hardware_archive/ATMEGA_STM32F4_COMMS_AUDIT.md` |
| Current PERF voice morph behavior | `knowledge_files/log_archive/026_SESSION_HANDOFF_LOG.md`, `VOICE_MORPH_CONTROLS_AUDIT2.md`, `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md` |
| Current live-record step automation destination convention | `knowledge_files/log_archive/027_SESSION_HANDOFF_LOG.md`, `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md`, `knowledge_files/comms_spec_reference/BACKGROUND_LOAD_TEMPORARY.md` |
| Current step automation one-step release behavior | `knowledge_files/log_archive/034_SESSION_HANDOFF_LOG.md`, `STEP_AUTOMATION_AUDIT.md`, `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md` |
| Current global SHIFT-toggle behavior | `knowledge_files/log_archive/034_SESSION_HANDOFF_LOG.md`, `SHIFT_TOGGLE_AUDIT.md` |
| Current encoder `DTYPE_MENU` value-domain fix | `knowledge_files/log_archive/027_SESSION_HANDOFF_LOG.md` |
| Current background `.pat` / `.prf` / `.all` loading via temporary data | `knowledge_files/log_archive/033_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/028_SESSION_HANDOFF_LOG.md`, `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md`, `knowledge_files/comms_spec_reference/BACKGROUND_LOAD_TEMPORARY.md` |
| Current external MIDI routing and realtime timing | `knowledge_files/log_archive/035_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/029_SESSION_HANDOFF_LOG.md`, `knowledge_files/comms_spec_reference/MIDI_TABLE.md`, `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md` |
| Current sample import and imported-sample playback | `knowledge_files/log_archive/031_SESSION_HANDOFF_LOG.md`, `knowledge_files/log_archive/030_SESSION_HANDOFF_LOG.md`, `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md` |
| Current front-panel shown-pattern sync (`frontParser_shownPattern`/`menu_shownPattern`), step-parameter edit routing, and the `PATTERN_SETTINGS_PAGE` fix | `PROBABILITY_INVESTIGATION.md`, `knowledge_files/log_archive/036_SESSION_HANDOFF_LOG.md`, `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md` (`### 4c`), `knowledge_files/comms_spec_reference/BACKGROUND_LOAD_TEMPORARY.md` |
| Live-record step placement, `seq_addNote()`'s `isNoteOff` contract, the per-track/master step-index skew, and the reverted Session 036 roll fix | `knowledge_files/log_archive/037_SESSION_HANDOFF_LOG.md`, `knowledge_files/comms_spec_reference/MIDI_TABLE.md`, `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md` |
| MIDI roll trigger: roll-note offset, NOTE_ON-velocity-0 release, `(channel, note)` held-key ownership, stop release, stopped-transport MIDI rolls, chromatic-mode split, NRPN 93 roll rate | `knowledge_files/log_archive/038_SESSION_HANDOFF_LOG.md`, `knowledge_files/comms_spec_reference/MIDI_TABLE.md`, `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md`, root `MIDI_ROLL_TRIGGER.md` (historical design) |
| Repairing songs whose notes sit on the wrong sub-step row (firmware fixes never repair stored data) | `tools/rotation_correction.py` (self-documented; user-authored) |
| Session 036 probability/rotation forensic record (source docs deleted) | `knowledge_files/log_archive/037_SESSION_HANDOFF_LOG.md` -> "Preserved Record" section |
| ¢ RxFilter CC38 swap / NRPN 96, RPN-null window, NRPN 93-95 roll + PERF display reports, the "NRPN 93 attenuation" investigation | `knowledge_files/log_archive/039_SESSION_HANDOFF_LOG.md`, `knowledge_files/comms_spec_reference/MIDI_TABLE.md` (`## RX Filter ¢ Modes`, NRPN table) |
| RX/TX filter letters, `mrt` routing order, menu MIDI transmit (`0xca`/`0xcb`, coalescing queue, `getTxAddress()`), DIN/USB MIDI-out robustness | `knowledge_files/log_archive/039_SESSION_HANDOFF_LOG.md`, `knowledge_files/comms_spec_reference/MIDI_TABLE.md` (`## MIDI Transmit`), `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md` (`#### Session 039 MIDI filters and menu transmit`) |
| Imported-sample FM scaling and the Hihat modulator re-arm | `knowledge_files/log_archive/039_SESSION_HANDOFF_LOG.md` (Part 2.4-2.5) |

---

## Hardware Overview

- **Audio/control MCU:** STM32F407 (`mainboard/LxrStm32`) — DSP, sequencer, MIDI, USB
- **Front-panel MCU:** ATmega644 (`front/LxrAvr`) — LCD, encoders, buttons, LEDs, SD card preset I/O
- **Inter-MCU link:** UART-based protocol at 500,000 baud (`mainboard/LxrStm32/src/uARTFrontSYX/` on the STM side, `front/LxrAvr/avrComms/` on the AVR side)

### MCU Naming Conventions

STM-side front-panel comms live under `mainboard/LxrStm32/src/uARTFrontSYX/` and use the `frontPanel*` naming. AVR-side comms live under `front/LxrAvr/avrComms/` and use the `avrComms*` / `avrCommsParser*` naming. The terms `frontPanel` and `frontParser` on the AVR side are historical only and should not be used for new code or documentation.

### Encoder

AVR main encoder: Timer1 CTC at ~32.05 kHz, fixed `AB=11` rest phase, six stable-sample phase filter, 192-sample button debounce, narrow rest-jump recovery, edit-mode-only acceleration (1–2 rev/s → 1–4× multiplier). See `knowledge_files/log_archive/019_SESSION_HANDOFF_LOG.md` for the full hardware-approved implementation record. Do not reintroduce legacy read modes, PCINT decoding, or Timer0 encoder sampling.

Global SHIFT toggle after Session 034 is an effective button-level latch. Only
the shift-button toggler should care about raw physical SHIFT press/release;
all other AVR UI code should call `buttonHandler_getShift()`.

---

## Project Goal

- Maintain and extend the LXR firmware while preserving reliable dual-MCU operation (ATmega front panel + STM32 audio engine).
- Keep build reproducible with current toolchains (`arm-none-eabi-gcc`, `avr-gcc`) and preserve firmware image output flow.
- This folder is the repository/codebase.
- In normal maintenance, only session logs under `knowledge_files/log_archive/` should be changed inside `knowledge_files/`; broader documentation passes may update `comms_spec_reference/`, `hardware_archive/`, or `reference_material/` when a session explicitly requires alignment.
- Some reference files formerly at repo root were moved under `knowledge_files/reference_material/` during Session 002 cleanup (`Changelog.txt`, `Readme - firmware additions to v.36.txt`, `linux_build_guide.txt`, `lxr-midi-assign.txt`, `mod_targ_lineup.xls`).
- Root files `P000.ALL`, `P000.PRF`, `P005.PRF`, and `P000.SND` are temporary test files and are expected to be removed later.

## General Process Reminders

- Always verify the local working repository directory before writing code.
- Validate error logs against the exact local source snapshot before patching.
- Prefer source-level compatibility fixes over compiler-flag masking.
- Blocking for 1ms anywhere in the main loop or any ISR at priority <= 4 is unacceptable.

---

## Hardware

### MCU: <add details>

- Main audio/control MCU: STM32F407 (`mainboard/LxrStm32`).
- Front-panel/UI MCU: ATmega644 (`front/LxrAvr`).
- Inter-MCU link: UART-based protocol at 500000 baud.
- Front panel handles LCD, encoders, buttons, LEDs, SD card preset I/O; STM32 handles DSP/sequencer/MIDI/USB.

### Flash Sector Layout (single-bank, confirmed via memtest)

| Sector | Range | Size | Use |
|--------|-------|------|-----|
| <to add later> | <to add later> | <to add later> | <to add later> |

### Confirmed GPIO

| Peripheral | Pins | Notes |
|-----------|------|-------|
| STM32 <-> AVR UART | <to add from hardware audit/code grep> | Known to run 500000 baud in current firmware |
| OUT jack detect mapping | PD6/PD7/PB4/PB6 | Carried forward from prior notes; re-validate in this branch if touched |

### IRQ Assignments

| IRQ | Priority | Handler | Function |
|-----|----------|---------|----------|
| <to add later> | <to add later> | <to add later> | <to add later> |

---

## Audio Pipeline <add later>

**Main loop pattern:**
```c
/* TODO: capture from mainboard/LxrStm32/src/main.c in a dedicated audit session */
```

**OUTPUT_DMA_SIZE = xx** <to be audited later>

**DSP render must remain in the main loop.**

---

## SD Card Architecture

Current high-level architecture (from source tree):
- STM32 side: `mainboard/LxrStm32/src/Hardware/SD_FAT/`
- AVR side: `front/LxrAvr/Hardware/SD/`
- Shared FAT-style flow with board-local SPI/SD routines.

**Architecture:**
```text
ATmega644 UI/Preset manager <-> SD (front/LxrAvr/Hardware/SD)
STM32 sample/preset-related storage services <-> SD_FAT (mainboard/LxrStm32/src/Hardware/SD_FAT)
```

**Key design points:**
- `startBlock`, `totalBlocks`, `SDHC_flag`, `cardType` are global SD state symbols and must remain single-defined (`extern` in headers, one C definition).
- SD layers are build-critical on both MCUs; header ownership mistakes cause cross-TU link failures.

**ISR migration path** <to be audited>

**Approaches confirmed wrong** (do not retry):
- Defining SD state globals directly in headers included by multiple C files.

---

## Sample Flash Loading

<to audit another session>

Sample flash map:

| Region | Range |
|--------|-------|
| <to be completed> | <to be completed> |

---

## Encoder

- Session 019 supersedes older encoder timing/tuning notes. See `knowledge_files/log_archive/019_SESSION_HANDOFF_LOG.md`.
- Final AVR main encoder implementation:
  - Wiring is immutable: phase A = PC0, phase B = PC1, button = PC2.
  - Timer1 CTC samples at roughly 32.05 kHz from `TIMER1_COMPA_vect` using prescaler 8 and `OCR1A = 77` at 20 MHz.
  - Phase filtering requires six stable samples per phase.
  - Button debounce uses a 192-sample integrator in the same Timer1 ISR.
  - Physical detent rest is fixed as `AB=11` (`ENCODER_REST_STATE = 0x03`).
  - The rest-phase FSM emits one detent only after a same-direction sequence leaves rest and returns to rest.
  - A narrow rest-jump recovery rescues filtered `11 -> 00 -> adjacent -> 11` contact sequences while all other illegal transitions still reset.
  - `encode_stableRead4()` is the raw supported rotation read and drains complete detents atomically.
  - `encode_readButton()` is the only button read.
  - `encode_getAccelerationMultiplier()` exposes a complete-detent speed multiplier for menu edit acceleration.
  - Acceleration config lives in `front/LxrAvr/config.h`: min 1 rev/s, max 2 rev/s, max multiplier 4.
  - Acceleration is applied only in menu edit-mode parameter changes, not navigation/load/save/copy-clear selection.
- Do not reintroduce `encode_stableRead1()`, `encode_read1()`, `encode_read4()`, `encode_stableRead2()`, `ENC_USE_STABLE_DRIVER`, PCINT decoding, Timer0 encoder sampling, or temporary LCD/debug hooks.

---

## Digital Input (DIN)

- **Initialization Requirements**: `din_inputData[]` software mirror must be synchronized with the physical shift registers during `din_init()`.
- **Logic**: 0 = Pressed, 1 = Released.
- **Initialization Bug (Fixed Session 008)**: Initializing mirror to `0` with `memset` caused ghost release events on the first scan. Sequential polling (10 buttons/loop) allowed SELECT buttons to be processed while SHIFT state was still `0` (interpreted as 'Pressed'), triggering phantom 'shifted release' actions.

---

## Display / Menu

- **LCD queue**: <to audit>
- **Knob repaint**: <to audit>
- **Encoder `DTYPE_MENU` edit values (fixed Session 027)**: encoder edit mode selects the correct parameter; menu-list failures here are value-domain failures, not parameter-index failures. `DTYPE_MENU` values must be compact list indices `0..numEntries-1` before applying encoder delta. The current AVR menu code normalizes out-of-range raw byte values proportionally into the compact list range in both regular and shift/morph encoder edit paths before clamping.
- **`Datatypes` dtype enum (`menu.h`) is hard-capped at exactly 16 entries** — it is packed into the low 4 bits of `parameter_dtypes[]` everywhere that array is read (`... & 0x0F`), and as of Session 036 all 16 slots are in use. There is no free slot for a new dtype without first reclaiming one (e.g. `DTYPE_AUTOM_TARGET` is legacy-macro-only per its own comment and may be reclaimable, but this has not been investigated). To give one parameter a narrower range than the dtype it shares with others, add a `paramNr`-based special case inside the existing dtype's switch branch instead of adding a new dtype — see the `PAR_EUKLID_SUBSTEP_ROTATION` clamp in `menu_encoderChangeParameter()`/`menu_encoderChangeShiftParameter()`/`getDtypeValue()` (`menu.c`, Session 036) for the pattern to follow.

---

## DSP / RNG Rules

- **Compiler**: modern GCC toolchains expose legacy `inline` linkage pitfalls.
- **FPU**: STM32F4 hardware FPU present; exact compile flag set should be captured in a later dedicated toolchain audit.
- **VLAs**: <to audit usage policy>
- `bufferTool_*` and `GetRngValue` must keep consistent declaration/definition linkage across headers and C files.
- `freq2PhaseIncr` currently uses `static inline` in header in this branch.

---

## Boot / Init Order (do not reorder)

```c
/* TODO: capture explicit init order from mainboard/LxrStm32/src/main.c */
```

### Sequencer / PATGEN Reminders

- **`seq_stepIndex[track]` runs exactly ONE SUB-STEP AHEAD of `seq_stepIndex[NUM_TRACKS]`** inside `seq_process()`: the per-track index is incremented at the top of the per-track loop, the master index only at the very end. Any code that writes at a raw per-track index while testing a boundary on the master index inherits this skew — a raw per-track index captured at a quantize boundary is `mainStep*8 + 1`, not `mainStep*8`. This caused the Session 037 duplicate-sub-step bug. (Session 037)
- **Live-record placement is decided by `seq_addNote()`'s `isNoteOff` parameter, never by velocity.** Only `channelMidiParser_noteOn()`/`noteOff()` (`MIDI/ChannelMidiParser.c`) may pass `1`, so only a genuine MIDI note-off keeps its raw un-quantized position. Roll hits (all five `ROLL_MODE_*` branches of `seq_rollTrig()`) and the `seq_process()` loop re-record pass `0` and always land on the quantize grid, even when their velocity resolves to 0. Do not reintroduce a bare `if (vel == 0)` placement test. (Session 037)
- **Zero-velocity active steps are intentional.** `volume == 0 | STEP_ACTIVE_MASK` is a "ghost step" (ratchet placeholder, muted accent, silent control event) — see the `STEP_VOLUME_MASK > 0` guard in `seq_process()`'s automation-release block. Never treat velocity 0 as "no step" or as an implicit note-off. A roll in `ROLL_MODE_TRIG`/`_NOTE` reads its velocity straight from the step under the playhead, so it can legitimately be 0.
- **MIDI roll holds are keyed per MIDI key (channel + note) in `MidiParser.c`**, and each release frees exactly the voice mask its own note-on captured, never a mask recomputed from current routing. The note-off lookup is deliberately ungated (no `normalConsumed` / offset / routing check). A duplicate press keeps the original mask and only re-asserts it; re-asserting on every press is what re-fires one-shot rolls and re-arms stopped rolls. A NOTE_ON with velocity 0 is a note-off on every route (MIDI 1.0). Never count it as a press; the Session 038 stuck-roll bug came from that. Any event that invalidates MIDI note routing must call `midiParser_clearMidiRollHolds()` (the parser table), not merely clear `seq_rollMidiHeld`; a stale table entry makes the next press look like a duplicate. Sequencer stop (every `seq_setRunning(0)`, including a repeated stop) clears all MIDI roll holds. While stopped, MIDI-held rolls play on the private `seq_tickStoppedRolls()` clock (roll velocity/note regardless of `seq_rollMode`, `seq_tempRate`, systick-timed at `seq_tempo`). Manual rolls stay silent while stopped. (Session 038)
- **`seq_setStepIndexToStart()` leaves `seq_stepIndex[NUM_TRACKS]` at `-1`** (rotation 0) after start and after playhead-reset pattern changes, and signed `%` on it yields `-1`. `seq_setRoll()`'s early-roll window therefore casts to `uint8_t`; keep that cast, or a roll held through start double-hits the downbeat. The boundary test `!(idx % q)` is unaffected. Quantized roll hits still land at per-track `8k+1` (the Session 037 skew); that is known and not addressed. (Session 038)
- **Do NOT re-suppress the early-roll hit's recording in `seq_setRoll()`.** Session 036 did this and it silently dropped notes: that early hit records the *correctly quantized* position (snapped back to the boundary that just passed), while the later on-time hit lands a full 16th later. The call site now carries a comment saying so. (Reverted in Session 037)
- **Firmware fixes do not repair already-damaged saved songs.** Steps written to the wrong sub-step row are stored data. Use `tools/rotation_correction.py` (user-authored, self-documented, rotates step data and main-step gate masks in `.pat`/`.prf`/`.all` in place) or clear and re-record the affected main steps.
- **`LengthRotate` (`Sequencer/Pattern/PatternData.h`) is mis-declared and unfixed**: `length:4 + scale:3 + rotate:4` = 11 bits inside a union with a `uint8_t value`, so `sizeof(LengthRotate)` is **4, not 1**, and `value` aliases only the low byte — `rotate`'s upper 3 bits live outside it entirely. Any code treating `value` as the whole packed state silently loses rotation >= 2. `scale` is *not* corrupted by rotation writes (rotation goes through the bitfield, not `value`) — verified — so this caused none of the bugs investigated so far, but it deserves its own review, especially around file save/load. (Found Session 037, not fixed)
- **`seq_skipFirstRoll`** (`sequencer.c`) is permanently 0 — its only setter is commented out in the `FRONT_SEQ_ROLL_MODE` handler, so `seq_setRoll()`'s early-roll branch can never be disabled from the front panel. Not a defect; noted because it means that branch is always live.
- **Pre-existing roll behaviour, deliberately unchanged**: a roll tap landing in the *second half* of a quantize window and released before the next boundary records nothing at all. This is a trade-off in `seq_setRoll()`'s early-window condition `< (seq_stepsPerQuant/2 - 1)`, not a regression.
- **Investigation technique that worked**: when static analysis gives a confident answer that hardware contradicts, copy the real sequencer functions **verbatim** into a host-side C harness driven by a replica of `seq_process()`'s per-tick ordering and sweep the parameter space. Session 037 used this to disprove a plausible-but-wrong root cause in one pass after two rounds of reading had accepted it. Paraphrasing the functions would only re-encode the same misreading.
- **Forensic caution**: before trusting any "no X is set anywhere in the file" conclusion, scan **all** steps for the field, not just `STEP_ACTIVE_MASK`-active ones. Session 036 reached a wrong conclusion this way and the user had to correct it.

- Step-parameter edit opcodes (`FRONT_SEQ_PROB`, `FRONT_SEQ_VOLUME`, `FRONT_SEQ_NOTE`, `FRONT_SET_P1_VAL`, `FRONT_SET_P2_VAL`) always resolve against `frontParser_shownPattern` (the pattern the front panel is currently displaying), never against `seq_perTrackActivePattern[track]` (the per-track pattern that actually drives playback) or `seq_activePattern`. Playback (`seq_liveStepForTrack()`) and Save (`frontPanelSending_sendStepInfo()`) both use `seq_perTrackActivePattern[]`/raw indices and never consult `frontParser_shownPattern`. If the two diverge, a step edit is accepted by the menu and looks correct but never affects playback or gets saved. See `PROBABILITY_INVESTIGATION.md` and `COMMS_FLOW_SPEC.md` `### 4c`.
- `frontParser_shownPattern` (STM) / `menu_shownPattern` (AVR) is kept in sync only by explicit `SEQ_SET_SHOWN_PATTERN` traffic: on every `SEQ_CHANGE_PAT` ACK when `PAR_FOLLOW` is on or a temp/normal boundary was just crossed (`tempBoundaryAck`, computed independent of Follow so the boundary case is covered even with Follow off), and unconditionally on every file load (`menu_setShownPattern(menu_playedPattern)`, called twice). It can legitimately equal `SEQ_TMP_PATTERN`; `pat_getStepPtr()` routes writes for that value into the scratch `pat_tmpPattern` struct, which Save's per-pattern loop (patterns 0-7 only) never visits — a step edit made while shown-pattern is stuck on `SEQ_TMP_PATTERN` is invisible to Save on every real pattern.
- Per-track pattern following (hold a VOICE button, press a PATTERN button — `seq_setNextPattern(pattern, voice)` with `voice != 0x0f`) reassigns only that one track's `seq_perTrackActivePattern[track]`; the global `seq_activePattern`/pattern-change ACK is unaffected, so `frontParser_shownPattern` cannot track a per-track-only reassignment. This is expected feature behavior, not a bug — but it does mean step edits on that track will not land where it's actually playing until the per-track assignment is cleared.
- Session 036 fixed the one confirmed defect in this area: the `PATTERN_SETTINGS_PAGE` branch of the `SEQ_CHANGE_PAT` ACK handler used to update only local AVR state, never flushing to STM (despite a stale comment claiming it would be flushed on shift-release — no such flush existed). See the pattern-settings-page bullet above and `PROBABILITY_INVESTIGATION.md` for the full record. Mechanism B (editing while shown-pattern is `SEQ_TMP_PATTERN` outside of that now-fixed stuck state, e.g. during the live streaming window of an actual background load) remains an open, lower-priority hardening item.
- **Separate, unfixed issue found the same session (`PROBABILITY_INVESTIGATION.md` Part 8):** step-parameter edit addressing (`buttonHandler_selectActiveStep()`, `buttonHandler_setRemoveStep()`, and the `SELECT_MODE_PAT_GEN` step-select path, all `buttonHandler.c`) always computes the raw sub-step index as `mainStepIndex*8` (row 0 of that main step's 8-slot group) and has no awareness of sub-step rotation (`PAR_EUKLID_SUBSTEP_ROTATION`/`euklid_rotatePattern()`, which physically moves the live note to a different row within the group without moving the main-step on/off bitmask). On a rotated track, editing a step's probability/note/volume through the normal front-panel workflow silently writes to the vacated, inactive row instead of the row that's actually playing. This is the verified root cause of the originally reported Song-75 bug (confirmed against every non-default `prob` byte in the attached file). Not yet fixed; needs a decision on approach (dynamic active-row resolution vs. a UX indicator) before touching the three call sites.
- **Fixed in Session 036 (`PROBABILITY_INVESTIGATION.md` Part 9):** `PAR_EUKLID_ROTATION`/`PAR_EUKLID_SUBSTEP_ROTATION` no longer resend `SEQ_SET_ACTIVE_TRACK` before their opcode (`menu.c`) — that redundant resend previously retriggered the voice on every encoder nudge while the sequencer was stopped, since STM's `FRONT_SEQ_SET_ACTIVE_TRACK` treats "same track, stopped" as the documented voice-preview gesture. The identical redundant-resend idiom still exists on `PAR_EUKLID_LENGTH`, `PAR_EUKLID_STEPS`, `PAR_POS_X`, `PAR_POS_Y`, `PAR_FLUX`, `PAR_SOM_FREQ`, `PAR_TRACK_LENGTH`, and `PAR_TRACK_SCALE` and was intentionally left unfixed (only rotation was reported).
- **Fixed in Session 036:** `PAR_EUKLID_SUBSTEP_ROTATION` is now clamped to `0-7` (not the shared `DTYPE_0B15` dtype's normal `0-15`) at all three `menu.c` value-clamp sites, because `euklid_rotatePattern()` folds any sub-step delta with `|delta| > 7` into a whole main-step rotation (correct for genuine multi-main-step moves, but was previously reachable from an ordinary sub-step-only dial move since the dial allowed up to 15). Do not raise this parameter's ceiling back above 7 without re-deriving the delta-safety math in `PROBABILITY_INVESTIGATION.md` Part 9. `PAR_EUKLID_ROTATION` correctly keeps its real `0-15` range.

### MIDI / Clock Reminders

- Inter-MCU UART protocol uses MIDI-like status/data framing and sysex-like bulk transfer behavior.
- Communications audit warns about silent FIFO overflow and blocking ACK wait patterns; review `knowledge_files/hardware_archive/ATMEGA_STM32F4_COMMS_AUDIT.md` before touching parser/transport logic.
- Session 004 comms knowledge is consolidated in `knowledge_files/hardware_archive/ATMEGA_STM32F4_COMMS_AUDIT.md`. Implemented checkpoint behavior includes acknowledged load sessions, STM32 quiet mode, credit-metered globals/voice/meta bursts, endpoint restore on every normal/temp switch, and removal/bypass of obsolete first-switch voice hold/unhold delays. Old SysEx/callback waits still need timeout recovery in a later pass.
- Session 027 automation destination convention: `Step.param1Nr` / `param2Nr` store raw AVR/menu `PAR_*` ids. Front-panel live recording and manual step automation destination editing write raw ids. Automation playback converts raw low destinations to `MIDI_CC data1 = destination + 1` for `frontParser_applyParameterCommand()`. External MIDI recording must use `seq_recordAutomationMidiDestination()` because MIDI parser call sites start from MIDI-domain/apply-domain ids.
- Session 034 automation lifetime convention: step automation is a one-step override. Release ordinary destinations from STM `/Preset/` `interpolatedParams[]` baselines on the next active step for the track, immediate lane target change/clear, stop, or pattern change. Do not restore from the AVR/front-panel original-value mirror.
- Session 034 global decimation exception: `PAR_VOICE_DECIMATION_ALL` step automation is shared global state and must use the dedicated last-owner track/lane release path, not generic per-lane `AutomationNode` ownership.
- Session 035 realtime rule: DIN/USB statuses `0xf8..0xff` bypass ordinary input queues into separate source-owned 16-event SPSC queues and are drained DIN-first immediately before `calcNextSampleBlock()`. Preserve source semantics: DIN uses the realtime helper; USB reconstructs a source-tagged `MidiMsg` for the normal parser.
- Do not move sequencer, routing, DSP, or front-panel work into the USART/USB capture context. Realtime timestamps measure queue-to-dispatch latency only; they do not compensate fixed serial, USB host-frame, ping-pong buffer, I2S, or DAC delay. Keep audio DMA `0/0`, DIN `0/1`, and USB `1/3` under `NVIC_PriorityGroup_1` unless the full interrupt-latency model is re-audited.
- If build errors reference functions/line numbers missing in local files, treat as snapshot mismatch and confirm active repository first.
- External MIDI note-off = NOTE_OFF **or** NOTE_ON velocity 0 (`isNoteOff` in `midiParser_parseMidiMessage()`, Session 038). Both statuses are behaviourally identical on normal note routes. With the roll offset on, chromatic-mode tracks (no note override) split at the offset: notes below play, notes at or above are roll notes only. Nothing rolls while the offset is `off`. Global NRPN 93 = roll rate (0..15), 94 = roll note, 95 = roll velocity, all reported to the AVR PERF page (STM->AVR SEQ_CC 0x71-0x73). NRPN 96 = Drum 1 cutoff in ¢ RxFilter modes only, where Global CC38 is consumed. CC100/101 = 127 directly after an NRPN data entry is an RPN-null deselect, not pan. RX filter C (0x04) gates all incoming CCs at the midiParser_parseMidiMessage() MIDI_CC entry (Session 039 P2); before P2 it only gated front-panel echo/recording. mrt routing always forwards before RX filtering. TX C (0x40) transmits menu edits only (FRONT_CC_MENU/FRONT_CC_2_MENU, PERF roll/morph) as Global-channel CC/NRPN via the coalescing queue in MidiOutputControl.c (Session 039 P3); DIN and USB MIDI out are whole-message-or-drop; USB IN waits for DataIn with a 5 ms timeout.
- Session 039 opcode guardrail: only menu encoder/knob sound-parameter edits use `MIDI_CC_MENU`/`CC_2_MENU` (`0xca`/`0xcb`). Loads, per-voice preset sends, endpoint restore and the armed-step revert must keep plain `MIDI_CC`/`CC_2`, or they would be transmitted as MIDI. PERF roll/global-morph front opcodes are menu-only and also queue TX.
- Menu TX address must remain the inverse of the Global receive tables (`globalMidiParser_getTxAddress()`; CC preferred; 18 target selectors excluded, their receive value domain is unverified). Never emit CC38 after an NRPN. Keep the CC99 = 0 guard before CC100/101 = 127.
- STM->AVR display reports for parameters at or above `END_OF_SOUND_PARAMETERS` need dedicated `SEQ_CC` report sub-opcodes. `PARAM_CC2` cannot address raw > 255, and its AVR handler writes `parameters2[]`, which is only `END_OF_SOUND_PARAMETERS` long.
- "Echo" in the CC ladders (`channelMidiParser_sendParameterEcho()`) means the front-panel display update, never MIDI output.

### Sample Import Reminders

- Session 030/031 sample import uses STM32 internal flash sectors 8-11 only. PCM writes must stay below `SAMPLE_INFO_START_ADDRESS`, and the committed sample count at `SAMPLE_ROM_START_ADDRESS` is written last.
- AVR normally owns the SD card. During `Load: Samples`, AVR must call `spi_deInit()` before sending `SAMPLE_START_UPLOAD`; STM must deinitialize SPI on every import exit.
- Do not wait for a raw ACK or fixed timeout during sample import. AVR must keep parsing UART traffic until `SAMPLE_UPLOAD_RESULT` latches completion, while progress packets update the LCD.
- Import order is `/samples` one-shots first, optional `/loops` loops second, using sorted cached 8.3 WAV names on STM. Invalid, empty, odd-length, too-large, or unreadable files must not increment `loadedSamples` or create silent waveform slots.
- Sample progress packets are immediate real one-based current-file numbers. Do not delay progress by one file, do not send `data2 == 0` final markers, and do not add a separate STM `Writing Flash` signal.
- AVR owns the display-only `Writing Flash` idle spinner after numbered progress goes quiet while waiting for `SAMPLE_UPLOAD_RESULT`.
- `SampleInfo` is a 12-byte flash ABI. Bit 31 of `size` is the loop flag, bits 30..0 are int16 frame count, and the struct size is compile-time guarded.
- Imported-sample oscillator playback uses `samplePosition`, `sampleFraction`, and `sampleActive`; all trigger paths that can play imported samples must call `osc_resetSamplePlayback()`.
- Imported-sample FM is read-offset phase modulation (`calcUserSampleOscFmBlock()`, `<<14` into the 17-bit frame domain). The play head advances unmodulated, so FM never changes one-shot length; keep it in step with `calcUserSampleOscBlock()`.

---

### Morph / Endless-Pot Reminders

- Standard morph operation is STM-owned after Session 003.
- Normal/temp parameter switching is STM-owned after Session 004. AVR/front panel owns menu display, file I/O, and endpoint-byte transport/display, but must not compute or stream standard live morph or own canonical temp staging.
- Raw endpoint parameter storage uses AVR/menu indices on both MCUs. Low `+1` is only for applying an ordinary low live CC through `frontParser_applyParameterCommand(...)`.
- Raw step automation destination storage also uses AVR/menu indices after Session 027. Do not store low apply-domain ids in `Step.param1Nr` / `param2Nr`.
- Stored/canonical `PAR_VOICE_DECIMATION_ALL == 0` means `127` after Session 034. This applies to STM Preset endpoint/interpolated storage and AVR file import. It does not mean live step automation value `0` is invalid.
- Global morph writes all six STM per-voice morph amounts; actual interpolation uses per-voice morph amounts.
- Per-voice morph may come from global morph, MIDI, step automation, or LFO/velocity modulation destinations.
- Velocity-to-voice-morph is a one-shot trigger-time write to the active normal/temp per-voice morph value. Do not route velocity-to-voice-morph through the generic modulation matrix.
- LFO-to-voice-morph is serviced by the sequencer morph drain using `modNode_lastVal` style LFO state. Do not route it through generic high-frequency modulation paths; that caused audio breakup.
- `seq_serviceMorphInterpolation()` runs one parameter per STM main-loop pass and is the only intended writer of `interpolatedParams[]`.
- `seq_serviceMorphInterpolation()` now has phase 0 for standard morph and optional later phases for source voices whose LFO targets voice morph. Each main-loop pass must still perform at most one interpolation/application unit.
- Automation selector parameters, morph amount parameters, and LFO target voice selector parameters are carved out of ordinary interpolation and return kit/front endpoint values.
- Automation target endpoint bytes and resolved sideband structures must stay coherent. For LFO target ingress, raw selector bytes must be written to `kitEndpointParams[]` for kit endpoints or `morphEndpointParams[]` for morph endpoints, and the matching `PresetAutomationTargets` sideband must be refreshed. If only the sideband is changed, normal/temp switching can later restore stale/off raw bytes and drop the assignment.

---

## Known Issues / Technical Debt

### Resolved in Session 008

- **AVR Startup Substep Toggle Bug**: Root cause identified as a mismatch between the `din_inputData` software mirror (initialized to 'all-pressed' `0`) and the actual hardware state (all-released `1`), combined with a sequential polling order that processed SELECT buttons before the SHIFT button. This triggered a phantom 'shifted release' event on boot, sending 8 `STEP_CC` toggle messages to the STM32. Fixed by synchronizing `din_inputData` with hardware in `din_init()` before the main loop starts.

### Resolved in Session 001
- Header-defined globals causing multiple definitions (`MidiMessages.h`, `MidiVoiceControl.h`, SD headers).
- Legacy inline linkage breakage causing undefined references (`BufferTools`, `GetRngValue`).
- Full build now completes to `firmware image/FIRMWARE.BIN` in this repo.

### High Priority

- Current repository state is a functional post-morph temp/background-load baseline after Session 004.
- Current repository state now also includes the Session 024 opcode-surface cleanup, where the stale PRF/cache handshake names were commented out instead of deleted.
- Standard morph and normal/temp switching are STM-owned and hardware-verified for ordinary operation, including LFO target to voice morph.
- Session 005 closed the remaining global morph menu-sync issue; the remaining small workflow items are tracked below and before the larger preset/morph refactor.
- SEQ16 temp pattern observation bodge remains in place until after the future refactor.
- Preserve STM morph invariants while making Session 005 fixes: no AVR live morph fallback, no per-parameter valid arrays, no direct file/AVR writes into interpolation arrays.
- **Session 038 MIDI roll fix awaits hardware verification** (Octatrack release with NOTE_ON velocity 0, a real `0x8n` sender, NRPN 93, stopped rolls, stop/start handover, a single first hit when a roll is held through Play), then commit on `dev-roll-midi`. Checklists are in `knowledge_files/log_archive/038_SESSION_HANDOFF_LOG.md`.
- **Session 039 awaits hardware verification**: the ¢ glyph (LCD ROM 0x5C vs `¥`; CGRAM slot-1 fallback), ¢ UI, CC38 swap/NRPN 96, RPN null, roll NRPN 93-95 + PERF sync, imported-sample FM, hat modulator re-arm, RX C gate (routed but no effect), menu TX on DIN+USB including DAW record/playback, and **USB MIDI-out stress** (busy/DataIn/5 ms timeout, unplug, suspend; highest hardware-only risk). Consolidated checklist: `knowledge_files/log_archive/039_SESSION_HANDOFF_LOG.md`. Then commit P3.

### Current Temp Pattern / `.PRF` WIP Reminders

- SEQ16 is used as a SELECT button for `SEQ_TMP_PATTERN`.
- Session 004 made normal/temporary pattern and parameter exchange functional again after the Session 003 morph move.
- AVR-to-STM endpoint dump (Opcodes 0x65/0x66) is implemented and captures `parameter_values_fileLoadSnapshot`, `parameters2_fileLoadSnapshot`, and all 16 mod targets.
- STM-side `preset_normalKitState` and `preset_tmpKitState` are the source of truth for menu pushbacks.
- STM-to-AVR parameter pushback on temp-pattern transitions must happen on every normal/temp switch so the menu stays in sync. Global morph push-up on those boundaries is now handled by display-only report traffic.
- Endpoint storage uses raw AVR/menu parameter indices. Do not apply low `+1/-1` offsets to endpoint arrays or PRF restore opcodes.
- Sound engine live morph path uses `interpolatedParams` after the STM worker writes it; menu pushback uses `kitEndpointParams`.
- File payloads must target normal parameter storage and normal pattern storage only. The Session 028 background-swap prelude may copy currently audible data into temporary storage before file bytes arrive, but the file payload itself must not write temp storage.
- File loads must only write endpoint arrays, not interpolation arrays. Automation target sideband caches may be refreshed on file load, but parameter bytes still flow into endpoint storage only.
- Sound parameter arrays are always valid from zero init. Do not reintroduce `kitEndpointParamsValid[]`, `morphEndpointParamsValid[]`, or `interpolatedParamsValid[]`.
- No extraneous LCD/debug writes should occur during copy/paste, temp/normal switching, endpoint restore, or file-load operations.
- Do not make the temp pattern loadable/saveable unless explicitly requested.
- Background-load automation is complete as of Session 028 and uses the STM-owned normal/temp image model. Do not revive AVR-owned temp staging or the old cache/load-fast model.
- The old PRF cache/state-machine work was retired in Session 014. Treat any remaining `SEQ_PRF_CACHE_*` mentions, live-pattern getter references, or pending-counter notes as historical context only.

### Morph Move Completed In Session 003

- Use exact terms: "morph parameter endpoint" and "morph automation target endpoint"; do not use ambiguous shorthand for these concepts.
- Per-voice morph is displayed and directly set from the PERF menu as of Session 026 using full `0..255` `VOICE_MORPH` traffic; older notes saying it is never displayed are stale.
- Per-voice morph value conversion: `0` is valid, `0-126` maps to `value * 2`, and `127` maps to exact full morph `255`.
- STM-side morph interpolation/modulation is always serviced; pending morph work gets exactly one parameter interpolation per STM main-loop pass.
- During normal/temp pattern changeover, receipt of AVR/front-panel global morph may be blocked, but STM-side morph interpolation/modulation must not be paused.
- STM now has the AVR `modTargets[]` selector mapping needed to resolve interpolated automation destinations and apply voice morph modulation.
- The live-apply cache is not a validity system. It only prevents repeated DSP setter calls for unchanged values while still allowing first-time zero values from file load to land in DSP.

### Medium Priority

- Capture definitive boot/init order and IRQ priority table in this file.
- Expand directory and subsystem references with owner/entrypoint notes.
- `seq_init()` pre-existing out-of-bounds writes (compiler-flagged, not yet fixed): `midi_envPosition[i]` written for `i < NUM_TRACKS` (7) into `uint8_t midi_envPosition[6]`, and `memset(seq_lastMasterStep, 0, NUM_TRACKS+1)` into `uint8_t seq_lastMasterStep[NUM_TRACKS]`. Each writes one zero byte past the end at init.
- Stale Session 039 code comments: `MidiParser.h` (`MIDI TX/RX FILTER SETTER`, `MIDI FILTER STATE`) still say TX C is reserved / "0x40 has no transmitter"; false since P3.
- Cosmetic Session 039 leftovers: the duplicate `usb_flushMidi()` comment header in `usb_manager.c`; re-encoded `ä` in two `FIFO.h` comments; ¢ constants placed inside the `MenuText.h` menu-id define list.
- Dead code: `calcNextOscSampleFm()` / `calcNextOscSample()` (and `calcPeriodicInterpFm()` / `calcPeriodicInterp()`) return 0 for imported samples but have no callers; all rendering uses the block paths. Waveform interpolation can include imported samples (CRASH→sample, sample→sample) and FMs both sides with the same offset before crossfading. Known artifact: sample→sample blends share one playback position (the further-along copy wins), so two looped samples of different lengths can jump or click after the longer loop wraps.

### Lower Priority

- Normalize older warning patterns where safe (fallthrough annotations, const duplicates, legacy macro style).

---

## Failed Approaches — Do Not Retry

- Applying patches from an `err.txt` log without confirming the log matches current local source snapshot.
- Working in a similarly named sibling repository without verifying `pwd` and branch first.
- Using compiler workarounds alone (`-fcommon`, `-fgnu89-inline`) as a substitute for real source ownership/linkage fixes.
- Reintroducing unbounded STM32 front RX full-drain as a first communications fix; it previously caused a known-good `.ALL` load freeze.
- Changing PAT_CHAIN to pair-level ACK without changing the AVR sender; current protocol expects byte-by-byte callbacks.
- Using `midiParser_originalCcValues` as the source of truth for current loaded STM voice parameters.
- Using `parameterArray` to reconstruct raw menu parameter bytes for the temp cache; it points into live converted/modulated DSP state.
- Reintroducing per-parameter valid arrays in `PresetKitState`; sound parameter arrays are always-defined from zero init.
- Letting file load or AVR endpoint restore write directly into `interpolatedParams[]`; the STM morph worker owns interpolation arrays.
- Using `interpolatedParams[]` equality as a proxy for live DSP state; this skipped zero-valued file parameters when DSP init defaults were nonzero.
- Applying every parameter to DSP on every morph scan pass; this caused a loud continuous low-Hz overlay.
- Restoring AVR live morph computation to paper over temp-cache/load bugs; standard morph is STM-owned after Session 003.
- Reading external MIDI note statuses literally (treating NOTE_ON velocity 0 as a press) or keeping MIDI roll holds as per-voice counters whose release recomputes voices from current routing; both caused or enabled the Session 038 stuck-roll bug.
- Trusting `make firmware` alone after source edits; it repackaged a stale STM binary in Session 038.
- Treating `MIDI_CC`/`CC_2` front-panel traffic as "menu edits": loads and the armed-step revert use the same opcodes. Use the Session 039 `_MENU` opcodes.
- Reusing `PARAM_CC2` to report non-sound parameters (e.g. roll note, raw 270) to the AVR: it cannot address them, and it overflows `parameters2[]`.

---

## Toolchain

```text
Required:
- arm-none-eabi-gcc
- avr-gcc (+ avr-libc)
- make
- host C++ compiler (for tools/bin/FirmwareImageBuilder)
```

Notes:
- Reference install guidance: `requirements.txt`.
- Top-level build command: `make firmware`.
- **`make firmware` does not rebuild changed sources**: the top-level `Makefile` gives `$(ARM_BINARY)` / `$(AVR_BINARY)` no source prerequisites, so it only repackages existing `.bin` files unless they are missing. After editing code, run `make -C mainboard/LxrStm32 -j8 stm32` and/or `make -C front/LxrAvr avr -j4` first, then `make firmware`. Session 038 caught a stale STM binary this way; confirm by checking that `LxrStm32.bin` is embedded in `FIRMWARE.BIN` (it starts at offset 57344 with the current AVR size).
- Bootloader flash flow: copy `FIRMWARE.BIN` to SD root, then hold main encoder while powering on.
- **AVR app must stay ≤ 57,344 bytes** (AVR bootloader linked at `-Ttext=0xE000`, `front/LxrAvr_bootloader/default/Makefile`). At Session 039 close `LxrAvr.bin` is 56,570 bytes (774 free). Check the size after every AVR change; prefer arithmetic over PROGMEM tables, and narrow string-table rows.

---

## Governance Rules

### 1. Identity And Communication

- Be technical, concise, and direct.
- No greetings, apologies, filler, or meta-commentary.
- State facts, risks, and next actions. Keep code-adjacent documentation comments brief: why, ownership, timing.
- **User-manual entries (`manual/chapters/*.md`, `manual/latex/*.tex`) must be terse**: short rows and short sentences, no tutorial prose or long explanations. The user judged the Session 039 manual text too verbose.

### 2. Security And Boundaries

- Stay inside the workspace root, read permissions are always granted in root. 
- No network, sudo, system config, or destructive commands unless explicitly requested.
- No mutating git unless explicitly requested. Read-only git inspection is allowed when needed.
- Never expose credentials. Ask before any command that can alter external state.

### 3. Coding Standards

- Make the smallest correct change to functional code possible for the request, preferring existing helpers and protocols over new abstractions.
- All exported functions and variables should be accompanied by a commented-out note adjacent the code describing its function. All frequently-accessed variables in code files that are not function-local should have similar comments describing function.

### 4. Design Philosophy

- Preserve STM ownership of sound state and AVR ownership of UI, SD file I/O, and menu display state.
- Protocol direction matters. Do not reuse a command name in the opposite direction unless the intent is explicit.

### 5. Reasoning And Self-Review

- Before a complex change, give a short reasoning summary: challenge, affected modules, edge cases, timing risks, verification.
- After drafting code, do a red-team pass for scope creep, ownership errors, timing spikes, null or bounds risk, stale docs, and missing verification.
