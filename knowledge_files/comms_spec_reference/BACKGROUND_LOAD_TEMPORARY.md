# TEMPORARY / PATTERN / PARAMETER LOAD SPEC

Date: 2026-10-16
Status: current storage and switching spec (Session 039 added the menu-opcode guardrail note below) after Session 034 made step automation release through STM `/Preset/` baselines, added last-writer ownership for global decimation automation, and extended the `PAR_VOICE_DECIMATION_ALL` `0 -> 127` guard into STM canonical endpoint/interpolated storage. Session 033 restored `SHIFT+PLAY` around STM temporary preset storage and added Euclid-page one-visit temp track backups on the `SHIFT+PERF` page, while keeping the Session 028 background-load model intact. `PresetLoadCache` and the active `presetLoad_*` cache API are gone; file loads route directly to normal Preset/Pattern storage; normal/temp Preset and Pattern switching remains the only supported staging model. Internal CC/CC2-shaped parameter application is owned by STM front-panel receive/protocol code, not `MIDI/MidiParser.c`. Session 024 commented out the stale PRF/cache opcode surface without changing the live non-cache file-load path, Session 025 made the legacy macro slots zero-on-load plus inert on the apply/replay side, Session 026 connected per-voice morph display/control values to the active kit image via dedicated voice-morph traffic, Session 027 made step automation destinations raw AVR/menu `PAR_*` ids in pattern storage, Session 028 finished the `0x6d/0x6e` background-swap handshake so `.pat`, `.prf`, and `.all` loads can write normal storage while playback continues from temp, and Session 033 made temp preset storage the authoritative reload image for `PATCH_RESET`.

Naming note: STM-side front-panel ownership stays under `mainboard/LxrStm32/src/uARTFrontSYX/` with `frontPanel*` names. AVR-side comms now live under `front/LxrAvr/avrComms/` with `avrComms*` names. Older AVR `frontPanel*` references are historical only.

Session 023 note: the AVR load-page control is now the 5-state background-load
selector `PAR_FILE_LOAD_BACKGROUND` / `TEXT_FILE_LOAD_BACKGROUND`, but it still
persists and transmits the same raw byte value. STM behavior was intentionally
left unchanged in that pass.

Session 025 note: the legacy macro slots are now zeroed during AVR file import,
and the remaining macro storage/replay helpers are inert. Treat the macro
parameter slots as compatibility baggage only.

Session 026 note: individual voice morph values are now live PERF controls and
part of display sync. Global morph remains authoritative and overwrites all six
voice morph values; endpoint restore reports both global morph and the six
per-voice morph display values. Velocity-to-voice-morph is a trigger-time
current morph write, not a generic velocity modulation-node destination.

Session 027 note: `Step.param1Nr` and `Step.param2Nr` now store raw AVR/menu
`PAR_*` automation destinations. Front-panel live recording and manual step
destination edits both write raw ids; automation playback converts raw low
destinations to apply-domain low `MIDI_CC` ids immediately before
`frontParser_applyParameterCommand()`. External MIDI recording uses
`seq_recordAutomationMidiDestination()` to convert MIDI-domain ids back to raw
step storage.

Session 028 note: active background loading is the
`SEQ_BACKGROUND_SWAP_BEGIN` / `SEQ_BACKGROUND_SWAP_DONE` handshake (`0x6d` /
`0x6e`). STM copies the currently audible Pattern/Preset state into temporary
storage, forces an immediate switch to `SEQ_TMP_PATTERN`, waits until temp
playback readiness is true plus the final ACK delay, and then ACKs the AVR so
the ordinary file load can write normal storage. `.pat` background loading is
pattern-only: it uses temp pattern playback but keeps preset parameters normal.
File loads do not reset current global or per-voice morph amounts.

Session 033 note: temp preset storage is now also the authoritative “last
loaded preset” image. Ordinary `.snd` kit loads, instrument loads, morph
loads, `.prf`, and `.all` loads mirror the loaded endpoint subset into temp
immediately. Protected `.prf` / `.all` background loads are still the one
exception during the load itself: temp keeps the old audible preset until
playback later returns to normal, then STM resnapshots temp from the newly
loaded normal preset. `PATCH_RESET` now means “restore normal preset endpoints
from temp” and must be ignored while protected temp preset playback is still
active.

Session 033 note: `PAR_VOICE_DECIMATION_ALL` must never survive file import as
`0`. AVR now clamps imported `0` to `127` before the value can propagate into
normal storage, temp storage, or re-saved files, and startup seeds the menu
copy to `127`.

Session 034 note: STM `/Preset/` also protects global decimation storage.
Endpoint-restore zeroing and STM startup seed `PAR_VOICE_DECIMATION_ALL` to
`127`, stored endpoint value `0` is normalized to `127` when endpoint images
are rebuilt, and live/menu shared-parameter writes update
`interpolatedParams[]` immediately so step automation releases to the current
menu value.

Session 039 note: menu-originated sound-parameter edits now use the AVR->STM
statuses `MIDI_CC_MENU` / `FRONT_CC_MENU` (`0xca`) and `CC_2_MENU` /
`FRONT_CC_2_MENU` (`0xcb`), which the STM may transmit as Global MIDI CC/NRPN
when TX filter C is on. File loads, per-voice preset loads, endpoint restore and
the armed-step revert must keep the plain `MIDI_CC` / `CC_2` opcodes so load
traffic is never transmitted. Ingress storage semantics are unchanged: both
opcode pairs share the same STM case bodies, and while temp playback is active
(`preset_shouldApplyIngressToLive()` false) a menu edit is stored to the normal
image only and is not transmitted.

## Purpose

This document defines how the firmware should think about:

- normal versus temp parameter storage;
- normal versus temp pattern storage;
- background-load routing;
- copy-to-temp;
- normal/temp boundary switching;
- the `.pat` exception that must never move parameter read/write away from normal storage.

The goal is to keep one clear model for what is stored where and which operation is allowed to touch it.

## Ownership Model

### Parameter storage

`mainboard/LxrStm32/src/Preset/KitState.c/.h` owns the runtime parameter images:

- `preset_normalKitState`
- `preset_tmpKitState`

Each kit image carries:

- kit/front endpoint bytes;
- morph endpoint bytes;
- interpolated worker-owned bytes;
- resolved automation target images;
- global morph amount and per-voice morph values.

The exposed type names are now `PresetKitState` and `PresetAutomationTargets`.
The storage behavior did not change during the rename sweep; only the public
names did.

### Pattern storage

`mainboard/LxrStm32/src/Sequencer/Pattern/` owns pattern storage:

- `seq_patternSet`
- `seq_tmpPattern`

Pattern storage is separate from parameter storage.
Copying one does not automatically mean the other changed, unless the caller explicitly says so.

Step automation lanes are part of pattern storage. Their destination fields
(`param1Nr` / `param2Nr`) use raw AVR/menu `PAR_*` ids; playback and external
MIDI recording perform the necessary low-bank conversion at their respective
boundaries instead of storing mixed apply-domain ids in the pattern.

### Source selection

`mainboard/LxrStm32/src/Preset/TempPlaybackSwitch.c/.h` owns the persistent runtime decision about which image is audible.

The important state is:

- `preset_tmpKitActive`
- `preset_voiceSourceState[]`
- the boundary-switch flags that tell the sequencer when a source change is pending or committed
- `preset_tempPlaybackSwitchState.forceInstantSwitch`
- `preset_tempPlaybackSwitchState.patternOnlyTempPlayback`

That module answers the question, "which image is active?"
It does not own a second copy of the storage model itself.

### File-load ingress

`mainboard/LxrStm32/src/uARTFrontSYX/frontPanelReceivingProtocol.c`
receives the old AVR file-transfer envelope and routes file bytes to the real
storage owners.

Session 020 removed the recreated
`mainboard/LxrStm32/src/Preset/PresetLoadCache.c/.h` files and the active
`presetLoad_*` cache API. The receive protocol now keeps only a tiny
file-load ingress bracket that forces parameter ingress to normal-kit endpoint
mode while the file envelope is active. That bracket is not a cache and must not
grow into one.

File-load-while-playing behavior now uses the existing temp parameter/pattern
copy and playback mechanisms, not a separate load-cache authority. The active
entry point is `FRONT_SEQ_BACKGROUND_SWAP_BEGIN` in
`frontPanelReceivingProtocol.c`; the non-blocking service point is
`frontParser_serviceBackgroundSwapAck()` from the STM main loop.

Current background-swap preparation:

- `pat_copyToTmpPattern(seq_activePattern)` snapshots the currently audible
  pattern state. If the sequencer is running, it copies each track from
  `seq_perTrackActivePattern[track]`; if stopped, it uses the selected source
  pattern.
- `pat_copyToTmpPattern()` also calls `preset_captureTmpKitState()`, copying
  the normal kit image into `preset_tmpKitState`.
- `seq_setNextPattern(SEQ_TMP_PATTERN, 0x0f)` requests temp pattern playback
  for all tracks.
- `preset_tempPlaybackSwitchState.forceInstantSwitch = 1` makes the switch
  immediate after the copy completes, independent of the user instant-switch
  global.
- `preset_tempPlaybackSwitchState.patternOnlyTempPlayback` is set for `.pat`
  loads so the sequencer switches pattern playback to temp without moving
  preset parameter ownership to temp.

The same receive/protocol file owns live internal parameter application through
`frontParser_applyParameterCommand()`. AVR/front-panel parameter commands,
automation-node restores, and final single-parameter emits all use that helper.
External MIDI remains free to parse DIN/USB CC traffic, but shared internal
parameter application should flow into this front-panel/protocol helper rather
than calling back into `MidiParser.c`.

## Core Rules

### 1. Copy-to-temp clones normal into temp

When the user explicitly asks for copy-to-temp, or when the background-swap
prelude needs to protect currently audible playback:

- copy the current normal/audible pattern into `seq_tmpPattern`;
- copy the current normal kit image into `preset_tmpKitState`;
- copy endpoint bytes, interpolated bytes, automation targets, and morph values as part of that image copy;
- mark the temp image valid only after the full copy is complete.

Copy-to-temp is a storage operation, not a transport operation. For background
loads, the transport only requests the operation; Pattern and Preset own the
actual storage copy.

### 2. Temp switching changes source only

When the user switches between normal and temp playback:

- do not recopy storage;
- do not re-run file loading;
- do not use the switch itself to mutate endpoint arrays;
- only change which already-existing image is considered active.

That distinction matters because the active image and the stored image are not the same thing.

### 3. Background file loads write normal storage

`.pat`, `.prf`, and `.all` loads refresh the normal storage images while temp playback can keep running.

The current rule is:

- load normal parameter storage;
- load normal pattern storage;
- keep temp playback isolated unless the background-swap prelude or the user
  explicitly requests a temp switch/copy-to-temp;
- never create a second sound-authority cache just because a file is being loaded.
- never stage through `PresetLoadCache` or a replacement cache.
- when temp playback is already active, a later `.pat`, `.prf`, or `.all` load
  skips another copy-to-temp and proceeds directly as an ordinary normal-storage
  file load.

Implementation reminder:

- `.pat` is the special case that should never switch parameter read/write away
  from normal storage;
- the temp/background storage rules are about where bytes land, not about
  changing which module owns the storage model;
- `TempPlaybackSwitch` still owns the audible-image selection state.
- current global/per-voice morph amounts are live performance state and must
  not be reset by any file load.

### 4. Pattern and parameter temp ownership stay separate

Pattern background loading and parameter background loading are distinct ownership domains.

That means:

- a pattern load may stage pattern data into pattern temp structures;
- parameter ingress should remain normal unless the file kind explicitly requires a parameter-bearing load path;
- the pattern temp path must not be used as an excuse to redirect parameter writes;
- the parameter temp path must not be used as an excuse to steal pattern ownership.

This is the rule that prevents the `.pat` wrinkle from accidentally turning into a general parameter switch.

### 5. `.pat` background loading never switches parameter read/write away from normal storage

This is the special case that must stay true even if the interface changes later.

For `.pat` background loads:

- pattern data may be staged in pattern-owned temp structures;
- parameter ingress must remain on normal storage;
- no parameter temp switch should be implied by pattern background loading;
- the loader distinguishes this case with
  `preset_tempPlaybackSwitchState.patternOnlyTempPlayback`.

This rule is easy to violate if the code only knows "background load" and does not know "pattern-only background load."

## Current Flow Shape

### Copy-to-temp

The current copy flow is:

1. User requests temp capture.
2. Pattern code copies normal/audible pattern images into `seq_tmpPattern`.
3. Preset code copies normal parameter images into `preset_tmpKitState`.
4. Temp image is marked valid.
5. The active source can then switch to temp when the boundary logic says so.

For background swap, STM reaches this same copy model through
`FRONT_SEQ_BACKGROUND_SWAP_BEGIN`:

```text
pat_copyToTmpPattern(seq_activePattern)
-> running sequencer: copy each track from seq_perTrackActivePattern[track]
-> copy current pattern settings and temp hold settings
-> preset_captureTmpKitState()
```

### Ordinary File Loads And `PATCH_RESET`

Session 033 tightened the non-background rules:

- `.snd` full-kit loads and individual instrument loads mirror only the
  kit/front endpoint subset into temp;
- morph loads mirror only the morph endpoint subset into temp;
- `.prf` and `.all` loads without background loading mirror both endpoint
  groups into temp immediately;
- `PATCH_RESET` copies both kit/front and morph endpoint groups from temp back
  into normal storage and reapplies the live normal image;
- `PATCH_RESET` must stay disabled while protected `.prf` / `.all` temp preset
  playback is still active, because temp is intentionally holding the old
  audible sound during that window.

### Normal/temp boundary switch

The current boundary switch is:

1. Sequencer/pattern logic decides the next pattern source.
2. `TempPlaybackSwitch` updates the active source selection.
3. `Preset` uses that source selection to choose the correct image for live routing.
4. Endpoint restore traffic, if required, keeps the AVR menu coherent.

Switching should never be confused with copying.

### Background `.prf` / `.all` load

The current background-load flow is:

1. AVR validates the file header and checks `preset_backgroundSwapNeeded()`.
2. If background swap is needed, AVR sends `SEQ_BACKGROUND_SWAP_BEGIN`.
3. STM copies current normal/audible Pattern and Preset state into temp.
4. STM sets `forceInstantSwitch` and requests `SEQ_TMP_PATTERN`.
5. Sequencer immediately switches playback to temp after copy, preserving the
   current sequencer position through the existing instant switch path.
6. STM waits until `seq_activePattern`, all `seq_perTrackActivePattern[]`, and
   all preset voice sources report the expected temp state, then waits the
   final ACK delay and sends `SEQ_BACKGROUND_SWAP_DONE`.
7. AVR continues the ordinary `.prf` / `.all` file load into normal storage.
8. Temp playback continues from the copied temp image while normal storage is
   overwritten by the new file.
9. On completion, no cache promotion occurs. Normal storage now contains the
   newly loaded file; temp storage remains the audible protected image until
   the user switches away from temp.
10. Once playback later returns to normal, STM runs
    `preset_resnapshotTemporaryPresetFromNormal()` so temp stops holding the
    old sound and becomes the new last-loaded preset snapshot again.

### Background `.pat` load

The current background-load flow for `.pat` is narrower:

1. AVR validates the file header and checks `preset_backgroundSwapNeeded()`.
2. If background swap is needed, AVR sends `SEQ_BACKGROUND_SWAP_BEGIN`.
3. STM copies current audible pattern data to `seq_tmpPattern`.
4. STM sets `patternOnlyTempPlayback = 1`.
5. Sequencer switches pattern playback to `SEQ_TMP_PATTERN` but skips
   `preset_setTempPlaybackActive()` and skips
   `preset_updateVoiceSourcesForPatternChange()`.
6. Parameter storage/source remains normal.
7. STM ACK readiness requires pattern playback on temp and all preset voice
   sources still normal.
8. AVR continues the ordinary `.pat` file load into normal pattern storage.
9. The load completes without switching parameter read/write away from normal storage.

This flow is why `patternOnlyTempPlayback` must remain separate from the broad
"currently playing temp pattern" state.

### Repeated File Load While Temp Playback Is Active

The AVR tracks successful background swaps with
`preset_backgroundTempPlaybackActive`.

Current behavior:

1. First matching `.pat`, `.prf`, or `.all` load while playing normal may send
   `SEQ_BACKGROUND_SWAP_BEGIN`.
2. After STM sends `SEQ_BACKGROUND_SWAP_DONE`, AVR sets
   `preset_backgroundTempPlaybackActive = 1`.
3. Any later `.pat`, `.prf`, or `.all` load before returning to a normal played
   pattern skips `SEQ_BACKGROUND_SWAP_BEGIN`.
4. That later load proceeds as an ordinary file load into normal storage while
   temp playback remains audible.
5. `SEQ_CHANGE_PAT` with a non-`SEQ_TMP_PATTERN` played pattern clears the AVR
   temp-playback flag.

This prevents a second background load from copying partially overwritten
normal storage back into the still-audible temp image.

**Session 036 note**: every `SEQ_CHANGE_PAT` ACK -- including the one that
reports a temp/normal boundary crossing described above -- also drives a
second, separate piece of AVR/STM state: which pattern the front panel is
showing and editing (`menu_shownPattern` / `frontParser_shownPattern`). Step-
parameter edits (probability, volume, note) resolve against that shown-
pattern state, not against `preset_backgroundTempPlaybackActive` or
`seq_perTrackActivePattern[]`. Session 036 found and fixed a defect where
that resync could be silently dropped while the front panel was on
`PATTERN_SETTINGS_PAGE`, which could strand edits on `SEQ_TMP_PATTERN`
(invisible to Save) across a background-load boundary crossing. See
`knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md` `### 4c` for the
full mechanism and fix, and root `PROBABILITY_INVESTIGATION.md` for the
investigation that found it.

### Euclid Page Temp Track Backups

Session 033 also reuses `seq_tmpPattern` as a page-local edit backup on the
real `SHIFT+PERF` Euclid page (`SELECT_MODE_PAT_GEN` / `EUKLID_PAGE`):

1. entering the page begins one visit window;
2. the first Euclid edit to each touched track copies that shown normal track
   into the corresponding temp track slot;
3. later edits to that same track during the same visit do not copy again;
4. leaving the page commits by clearing the visit bookkeeping without restore;
5. pressing `SHIFT+PERF` again during the same visit restores every touched
   track from temp back into the shown normal pattern.

This path is explicitly source/destination addressed and is separate from
background temp playback. It is not active when the shown/edit pattern is
already `SEQ_TMP_PATTERN`.

### Morph Amount Preservation During File Loads

Current global morph and per-voice morph amounts are performance state. They
are not file-backed kit bytes and must not be reset by file load begin/done.

Current behavior:

- `.pat` load begin has no morph side effect.
- `.prf` / `.all` load begin may invalidate the live morph apply cache so
  loaded endpoints are recomputed from the existing morph amounts.
- `.prf` / `.all` load begin must not reset global morph or per-voice morph
  amounts.
- User edits to global or per-voice morph while temp is active mirror into
  normal storage too, so returning to normal does not resurrect stale morph
  amounts.
- AVR file-backed kit loops and endpoint dumps use `END_OF_KIT_PARAMETERS`.
- `PAR_MORPH_DRUM1..PAR_MORPH_HIHAT` remain before
  `END_OF_SOUND_PARAMETERS` so runtime automation/modulation domains still see
  those parameters.
- AVR `rxDisable` guards prevent protected file-load restore traffic from
  zeroing the per-voice morph display slots.

## Current Functions

These functions matter for the current normal/temp model:

- `preset_captureTmpKitState()`
- `preset_setTmpKitActive()`
- `preset_getCurrentImageKitState()`
- `preset_getMorphKitForImage()`
- `preset_setGlobalMorphAmount()`
- `preset_setVoiceMorphAmount()`
- `preset_setVoiceMorphLiveAmount()`
- `preset_getVoiceMorphAmount()`
- `preset_storeParameterIngress()`
- `preset_storeMorphParameterIngress()`
- `preset_storeLfoDestinationIngress()`
- `preset_storeVelocityDestinationIngress()`
- `preset_storeMacroDestinationIngress()` - legacy inert compatibility stub
- `frontParser_applyParameterCommand()`
- `preset_updateVoiceSourcesForPatternChange()`
- `preset_allVoiceSourcesUseTmp()`
- `preset_allVoiceSourcesUseNormal()`
- `preset_isTempPresetPlaybackActive()`
- `preset_reloadNormalFromTemporaryPreset()`
- `preset_resnapshotTemporaryPresetFromNormal()`
- `preset_copyKitEndpoints()`
- `pat_copyToTmpPattern()`
- `frontParser_serviceBackgroundSwapAck()`
- `preset_backgroundSwapNeeded()`
- `preset_performBackgroundSwapWait()`
- `preset_backgroundSwapDoneFromStm()`
- `preset_notePlayedPatternChanged()`
- `buttonHandler_refreshTempPlaybackLedHint()`

`presetLoad_*` is not a current ownership surface. Do not add new callers or
recreate those APIs.

## Design Note: Why The Temp Switch Survived And The Load Cache Did Not

The reason these mechanisms felt split was simple:

- `TempPlaybackSwitch` answers "which image is active?";
- the old load cache answered "what in-flight file/session work still needs to
  be staged or finalized?"

That distinction made sense while background-load flow still had special
session machinery.

Session 020 removed the need for a separate load cache by making the
temp/parameter ownership rules carry the real state directly. The receive
protocol may bracket the old file-transfer envelope, but it must not become a
second staging owner.

## Do-Not-Do List

- Do not reintroduce per-parameter validity arrays.
- Do not use background load as a second sound-authority cache.
- Do not let `.pat` loading switch parameter read/write away from normal storage.
- Do not merge pattern staging and parameter staging into one ambiguous flag.
- Do not let temp switching recopy storage when a simple source selection change is enough.
- Do not allow file-transfer compatibility code to become a permanent second
  owner.
- Do not reintroduce the old PRF cache opcode surface as an active staging model.
- Do not treat `SEQ_LOAD_BACKGROUND` / `FRONT_SEQ_LOAD_FAST` at `0x50` as the
  active background-load mechanism; active background swap is `0x6d/0x6e`.
- Do not reset current global morph or per-voice morph amounts from any file
  load.
- Do not copy normal storage into temp again while AVR temp playback state is
  already active.
- Do not use `END_OF_SOUND_PARAMETERS` for file-backed kit bytes if the loop is
  meant to exclude non-file-backed live morph amount controls; use
  `END_OF_KIT_PARAMETERS`.
- Do not let imported or STM-stored `PAR_VOICE_DECIMATION_ALL == 0` survive as
  the canonical Preset baseline; AVR file import and STM endpoint/interpolated
  storage both normalize it to `127`.
