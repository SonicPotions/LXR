# COMMS FLOW SPEC - UART FRONT PANEL

Date: 2026-10-16
Status: current AVR<->STM comms reference after Session 039 (P1: ¢ RX-filter modes with CC38 swap via bit 4 on `SEQ_MIDI_FILT_RX`, NRPN 94/95/96, RPN-null swallow, roll PERF display reports `0x71-0x73`, imported-sample FM, hihat modulator re-arm; P2: RX filter C gates all incoming CCs at the STM parser entry; P3: menu-originated sound-parameter edits use the new AVR->STM statuses `MIDI_CC_MENU`/`FRONT_CC_MENU` `0xca` and `CC_2_MENU`/`FRONT_CC_2_MENU` `0xcb` so TX filter C can transmit them as Global CC/NRPN, plus whole-message DIN/USB MIDI out and a USB IN completion/timeout fix; see `#### Session 039 MIDI filters and menu transmit` and `knowledge_files/log_archive/039_SESSION_HANDOFF_LOG.md`) and Session 038 fixed stuck MIDI rolls (external NOTE_ON velocity 0 is now a note-off; MIDI roll ownership is a per-`(channel, note)` held-key table in `MidiParser.c`; sequencer stop releases all MIDI roll holds; MIDI rolls play on a private clock while stopped; see `### MIDI Boundary` and `MIDI_TABLE.md`). The 2026-09-17 MIDI roll-trigger feature added `SEQ_ROLL_NOTE_OFFSET` / `FRONT_SEQ_ROLL_NOTE_OFFSET` (`0x6f`, a `SEQ_CC` sub-opcode carrying the saved global roll-note offset). Before that, Session 036 fixed a defect in the `SEQ_CHANGE_PAT` shown-pattern resync (see new `### 4c` section below): the `PATTERN_SETTINGS_PAGE` branch now flushes `SEQ_SET_SHOWN_PATTERN` to the STM via `menu_setShownPattern()` instead of silently stashing the update locally with no later flush. Session 035 separated external DIN/USB system-realtime MIDI from ordinary input, timestamped it at the receive boundary, and dispatched it at the audio render deadline. Session 034 made step automation a one-step Preset-baseline override, special-cased global decimation step automation ownership, and extended the `PAR_VOICE_DECIMATION_ALL` `0 -> 127` guard into STM canonical Preset storage. Session 033 restored `SHIFT+PLAY` around STM temporary preset storage, made `PATCH_RESET` an STM-owned temp-to-normal preset reload, multiplexed `SEQ_EUKLID_RESET` / `FRONT_SEQ_EUKLID_RESET` (`0x47`) for `SHIFT+PERF` Euclid visit control, and kept the Session 031 sample-import redo, Session 029 Global MIDI split, and Session 028 background file loading model intact. STM and AVR both have explicit receive/send protocol files, legacy parser/protocol shim headers were removed, the obsolete `PresetLoadCache` model is gone, the internal CC/CC2 parameter apply layer belongs to front-panel receive/protocol ownership rather than `MIDI/MidiParser.c`, the old cache-only opcode helpers are commented out instead of active, `MACRO_CC` is deprecated historical context, individual PERF voice morph uses dedicated full-range `VOICE_MORPH` / `FRONT_SEQ_VOICE_MORPH` traffic rather than generic `CC_2`, step automation destinations are stored as raw AVR/menu `PAR_*` ids, and active background loading uses the `SEQ_BACKGROUND_SWAP_BEGIN` / `SEQ_BACKGROUND_SWAP_DONE` handshake (`0x6d/0x6e`) rather than the retired cache/load-fast model.

## Purpose

This document records the current and future shape of the UART link between the ATmega front panel and the STM32 audio engine.

Naming note: STM-side front-panel ownership stays under `mainboard/LxrStm32/src/uARTFrontSYX/` and keeps the `frontPanel*` names. AVR-side comms now live under `front/LxrAvr/avrComms/` and use the `avrComms*` names. Any AVR-side `frontPanel*` reference in older material is historical.

It answers four questions:

- What kinds of messages cross the link?
- Which side owns each message family?
- Which parts are transport, protocol, and storage/session semantics?
- What should the comms layer look like once the preset consolidation is finished?

This is the working comms map for the audit pass. Transport details, protocol parsing, and preset storage should stay separate even when they interact.

## Ownership Boundaries

### Transport

`mainboard/LxrStm32/src/uARTFrontSYX/Uart.c/.h` should stay transport-only.

Its job is to move bytes, manage FIFOs, and service IRQ plumbing.
It should not know about normal/temp storage, background-load policy, or preset image ownership.

### Protocol

STM protocol ownership is split explicitly:

- `mainboard/LxrStm32/src/uARTFrontSYX/frontPanelReceivingProtocol.c/.h`
  owns AVR UART receive parsing, SysEx ingress, load-session receive state, and
  the STM front-panel opcode namespace. It also owns internal CC/CC2-shaped
  parameter application through `frontParser_applyParameterCommand()`.
- `mainboard/LxrStm32/src/uARTFrontSYX/frontPanelSendingProtocol.c/.h`
  owns STM-to-AVR packet construction.

AVR protocol ownership mirrors this:

- `front/LxrAvr/avrComms/avrCommsReceivingProtocol.c/.h` owns STM response
  parsing, SysEx receive state, restore handling, long-operation receive
  state, and the AVR-side opcode namespace.
- `front/LxrAvr/avrComms/avrCommsSendingProtocol.c/.h` owns AVR-to-STM packet
  construction, send-side flow-control state, LED/query sends, legacy macro
  sends kept only as disabled compatibility context, and the now-commented-out
  PRF cache control compatibility stubs.

The old STM `FrontPanelProtocol.h`, STM `frontPanelParser.h`, and AVR
`frontPanelParser.h` shim headers were removed in the Session 020 wrap-up. The
AVR rename to `avrComms*` landed in Session 021.
Callers should include the receive or sending protocol header directly.

### Storage and Session Semantics

`Preset` owns:

- endpoint storage images;
- temp versus normal selection;
- endpoint restore policy;
- normal/temp image selection and routing.

`Pattern` owns pattern storage and pattern-specific temp behavior.

The comms layer should route bytes into those owners, not duplicate their state.

### MIDI Boundary

`mainboard/LxrStm32/src/MIDI/` owns external DIN/USB MIDI byte parsing, MIDI
routing/filter settings, channel/global MIDI interpretation, voice triggering,
and sequencer-originated MIDI output fan-out.

The complete current external MIDI input table lives in
`knowledge_files/comms_spec_reference/MIDI_TABLE.md`.

Internal parameter application is not owned by `MidiParser.c`. External MIDI may
parse a CC/CC2 message and then call the front-panel receive/protocol helper
when it needs to apply the internal parameter command cases. The dependency
direction is MIDI -> uARTFrontSYX for shared internal CC/CC2 application, not
uARTFrontSYX -> MIDI parser.

Session 029 routing rule: for MIDI CC messages, the configured Global channel
pre-empts overlapping voice-channel assignments. Global CC0 and CC1 intentionally
keep the existing `ChannelMidiParser.c` bank-change and morph behavior. Global
CC2-127 route only through `GlobalMidiParser.c` and its separate CC/NRPN table;
non-global CC messages continue to use the existing channel parser. This
pre-emption rule is specific to CC routing; note and program-change routing keep
their existing behavior.

#### Session 038 MIDI roll ownership

MIDI roll notes (the saved global roll-note offset, `PAR_ROLL_NOTE_OFFSET`,
sent as `SEQ_CC` / `SEQ_ROLL_NOTE_OFFSET` = `FRONT_SEQ_ROLL_NOTE_OFFSET`
`0x6f`) are an external-MIDI concern owned by `MidiParser.c`. The parser keeps
a 16-slot held-key table of `(channel, note)` -> claimed voice mask. A note-off
(NOTE_OFF or NOTE_ON velocity 0) releases exactly the mask its press captured,
and the release lookup is not gated by routing, offset or `normalConsumed`. The
parser reports per-voice MIDI ownership to the sequencer only through
`seq_rollMidiChange()`. Manual front-panel rolls (`SEQ_ROLL_ON_OFF` /
`FRONT_SEQ_ROLL_ON_OFF`, `0x10`) own a separate `seq_rollChange()` source, and
neither source can release the other.

Front-panel traffic that invalidates held MIDI roll keys calls
`midiParser_clearMidiRollHolds()` on STM receive. That traffic is:
`FRONT_SEQ_MIDI_CHAN` (`0x2d`) when a channel actually changes,
`FRONT_SEQ_MIDI_CHAN_OFF` (`0x59`), `FRONT_SEQ_TRACK_NOTE1..7` (`0x52..`) when an
override actually changes, and `FRONT_SEQ_ROLL_NOTE_OFFSET` (`0x6f`) when the
offset actually changes. Transport stop (`FRONT_SEQ_RUN_STOP` `0x01`, MIDI Stop,
MTC timeout, sample-import entry: anything reaching `seq_setRunning(0)`) also
clears them, and so does disabling the RX note filter. No AVR-side change or
acknowledgement is involved; roll state is never echoed to the AVR.

#### Session 039 MIDI filters and menu transmit

- **RX order** (`midiParser_parseMidiMessage()`):
  - `mrt` routing forwards first and unfiltered.
  - SysEx stops there.
  - System messages are gated by RX R.
  - Channel messages are gated by their RX letter: N notes, P program change, C control change.
  - RX C (`0x04`) gates **all** incoming CC/NRPN once at the `MIDI_CC` branch. The older `0x04` tests in the CC ladders are redundant guards.
- **¢ RX flag:** the AVR RxFilter values 16..23 send the RX nibble `| 0x10` on `SEQ_CC`/`SEQ_MIDI_FILT_RX`. `midi_setFilter()` strips bit 4 into `globalMidiParser_setCc38Swap()`. Swap on means Global CC38 is consumed and NRPN 96 maps to `PAR_FILTER_FREQ_1`.
- **Menu transmit (TX C `0x40`):**
  - The AVR sends menu encoder/knob sound-parameter edits as `MIDI_CC_MENU` (`0xca`) / `CC_2_MENU` (`0xcb`) via `menu_sendSoundParameter()`. The payload is identical to `MIDI_CC`/`CC_2`.
  - STM `frontParser_handleMidiMessage()` shares the `MIDI_CC`/`FRONT_CC_2` case bodies. It normalises the status for `frontParser_applyParameterCommand()` and, only when the value was applied live, calls `outputControl_queueMenuParameterTx()`.
  - The menu-only PERF opcodes `FRONT_SEQ_ROLL_RATE/_NOTE/_VELOCITY` and `FRONT_SEQ_SET_GLOBAL_MORPH_MSB` call `outputControl_queueMenuPerfTx()`.
  - Gate: TX C on, Global channel 1..16, `!seq_isAutomationStepArmed()`.
  - Addresses come from `globalMidiParser_getTxAddress()`, the inverse of the receive tables.
  - `outputControl_serviceMenuTx()` (main loop, before `usb_tick()`) emits at most one whole message per pass, only when DIN and USB both have room.
- **Guardrail:** file/voice loads, the armed-step revert (`buttonHandler.c`) and restores must keep the plain `MIDI_CC`/`CC_2` opcodes, or they would be transmitted as MIDI. The shift-encoder path never sends `MIDI_CC`/`CC_2` for sound parameters (they are always morph-endpoint edits).
- **MIDI out robustness:**
  - `uart_sendMidi()` is whole-message-or-drop for every DIN sender.
  - `usb_sendMidi()` is whole-event-or-drop within the current 64-byte ping-pong half.
  - `usb_flushMidi()` waits for `usbd_midi_DataIn()` (busy flag) with a 5 ms timeout, and discards pending bytes while the device is not `USB_OTG_CONFIGURED`.

#### Session 035 realtime timing exception

System-realtime statuses `0xf8..0xff` are the deliberate exception to ordinary
external MIDI delivery. DIN `USART2_IRQHandler` classifies them at RXNE and USB
`usbd_midi_DataOut()` classifies completed MIDI OUT packets at its callback
boundary. Each source captures a DWT cycle timestamp and writes the status plus
timestamp to its own 16-event single-producer/single-consumer queue. This keeps
the ISR/callback capture-only and avoids a multi-producer queue race.

`serviceAudioRenderDeadline()` drains the DIN queue first and then the USB queue
immediately before `calcNextSampleBlock()`. DIN uses
`midiParser_handleDinRealtime()`; USB reconstructs a source-tagged `MidiMsg` and
uses the normal MIDI parser, so existing clock/start/continue/stop filters,
external-sync semantics, and USB routing remain unchanged. Realtime bytes no
longer wait behind ordinary `uart_processMidi()` FIFO work or `usb_getMidi()`
packet work; ordinary MIDI and all AVR front-panel transport retain their old
paths.

The interrupt configuration uses `NVIC_PriorityGroup_1`: audio DMA is `0/0`,
DIN USART2 is `0/1`, and USB is `1/3`. Thus capture does not run DSP or
sequencer/routing work in an interrupt, DIN can pre-empt USB, and neither path
pre-empts an already running audio-DMA handler. Queue timestamp latency is a
diagnostic only, measured with wrap-safe unsigned subtraction at dispatch. It
does not provide sample-offset scheduling or cancel serial transfer, USB host
frame/bulk, ping-pong buffer, I2S, or DAC latency.

`mainboard/LxrStm32/src/MIDI/MidiOutputControl.c/.h` is the current MIDI
voice/output-control file. Existing `voiceControl_*` functions retain their
names; sequencer-originated MIDI output helpers use the `outputControl_*`
prefix. `Sequencer` must not own USB/DIN fan-out directly. Since Session 039 the same file also owns the menu MIDI-TX gate, coalescing queue (one slot per raw sound parameter plus 4 PERF slots) and `outputControl_serviceMenuTx()`.

### Outbound Send Contract

The STM send-side split has landed in `frontPanelSendingProtocol.c/.h`.
The AVR send-side split landed in `avrCommsSendingProtocol.c/.h`.
That module owns outbound AVR command framing so the rest of the STM code can
reuse packet helpers instead of rebuilding byte triples inline.

The reusable helper surface should cover:

- ordinary command/status byte triples;
- priority command/status byte triples for flow-control and restore traffic;
- wait-for-space variants for restore sequencing and any other blocking packet
  family;
- SysEx-aware packet emission for front-panel streams that stay open across
  multiple bytes;
- common packet families such as restore begin/done, restore param, restore
  morph param, flow grant/abort, pattern change, run-stop, LED updates, and
  query replies.

The send header should also document the transmit mode bits or flags clearly so
callers can tell whether a packet is:

- normal or priority;
- blocking or non-blocking;
- SysEx-safe or ordinary command traffic;
- allowed to bypass the quiet UI gate or not.

Direct front-panel UART byte emission for AVR command traffic is confined to
`Uart.c/.h` and `avrCommsSendingProtocol.c`. Callers such as Sequencer,
EndpointRestore, MIDI parser helpers, voice control, and receive protocol code
use the sending protocol helpers.

`Uart.c/.h` should remain transport-only. The MIDI transport helpers
`uart_sendMidi()` and `uart_sendMidiByte()` stay outside this split because
they are not AVR front-panel command traffic. Since Session 039 `uart_sendMidi()`
is whole-message-or-drop (`fifo_freeSpace()`), and `uart_midiTxFree()` exposes
DIN headroom for paced menu MIDI transmit.

## Current Message Families

### 1. Live Control and Parameter Ingress

These are the ordinary single-message control paths:

- `MIDI_CC`
- `CC_2`
- `SEQ_CC`
- `CC_LFO_TARGET`
- `CC_VELO_TARGET`
- `VOICE_MORPH` / `FRONT_SEQ_VOICE_MORPH` - full-range `0..255` per-voice morph amount traffic, encoded as low/high 7-bit-safe packet pairs
- `MACRO_CC` - deprecated legacy macro traffic; current firmware ignores it
- `SEQ_CC` / `SEQ_ROLL_NOTE_OFFSET` (`0x6f`) - saved global MIDI roll-note offset (raw `0` = off, `1..127` = semitone shift); STM releases all MIDI roll holds when the value changes
- `SEQ_CC` / `SEQ_MIDI_FILT_RX` (`0x31`) - RX filter nibble; bit 4 (`0x10`, Session 039) = ¢ mode (Global CC38 consumed, NRPN 96 = Drum 1 cutoff). STM strips it in `midi_setFilter()` and stores it in `GlobalMidiParser.c`. RX bit 2 (0x04, C) gates all incoming CC/NRPN at the STM MIDI_CC entry (Session 039 P2); routing (mrt) is unaffected.
- `MIDI_CC_MENU` / `FRONT_CC_MENU` (`0xca`) and `CC_2_MENU` / `FRONT_CC_2_MENU` (`0xcb`) - menu-originated sound-parameter edits; same payload and handling as `MIDI_CC`/`CC_2`, plus STM menu MIDI TX when txf C is on (Session 039 P3). Loads/reverts keep the plain opcodes.

Raw endpoint bytes are routed into `Preset` ingress helpers such as:

- `preset_storeParameterIngress()`
- `preset_storeMorphParameterIngress()`
- `preset_storeLfoDestinationIngress()`
- `preset_storeVelocityDestinationIngress()`
- `preset_storeMacroDestinationIngress()` - legacy inert compatibility stub

Important rule:

- Raw endpoint storage uses raw AVR/menu parameter indices.
- The low-CC `+1` conversion only applies when the value is being applied as an ordinary live CC to DSP/front-panel parameter apply logic.
- Do not apply that conversion to endpoint restore traffic.
- Step automation destination storage also uses raw AVR/menu parameter indices. Front-panel live recording and manual step-destination editing write raw ids to `Step.param1Nr` / `Step.param2Nr`; automation playback converts raw low destinations to `MIDI_CC data1 = destination + 1` immediately before calling `frontParser_applyParameterCommand()`.
- External DIN/USB MIDI automation recording starts from the MIDI-domain/apply-domain id. Those call sites must use `seq_recordAutomationMidiDestination()` so low MIDI-domain destinations are converted back to raw step storage.
- Live-record **step placement** is decided by `seq_addNote()`'s `isNoteOff` parameter, never by velocity. Only the MIDI channel-parser note path passes `1`, so only a genuine MIDI note-off keeps its raw un-quantized position; roll hits and loop re-record pass `0` and always land on the quantize grid even when their velocity resolves to 0. Note also that `seq_stepIndex[track]` runs exactly one sub-step ahead of `seq_stepIndex[NUM_TRACKS]` inside `seq_process()` (per-track index advanced at the top of the loop, master index only at the end), so any raw per-track index captured at a quantize boundary is `mainStep*8 + 1`, not `mainStep*8`. Session 037 fixed a duplicate-sub-step bug caused by these two facts combining.
- STM-to-AVR step-parameter replies send raw stored destinations unchanged. This is different from `frontPanelSending_sendParameterEcho()`, which still subtracts one for low parameters because its MIDI parser callers pass apply-domain ids.
- Step automation is a Sequencer one-step override. Release baselines come from STM `/Preset/` via `PresetKitState.interpolatedParams[]`, not from AVR/front-panel original-value mirrors. Ordinary automated parameters release on the next active step for the track, and manual/recorded target changes release the old held destination immediately.
- `PAR_VOICE_DECIMATION_ALL` is a shared global SampleRt parameter, so step automation bypasses generic per-track `AutomationNode` ownership. The last track/lane that writes global decimation owns release back to the retained pre-automation value.
- PERF individual voice morph amount edits are not ordinary `CC_2` parameter ingress. They use `VOICE_MORPH` low/high packets and land in the direct full-range Preset voice morph setter.
- MIDI CC1 morph remains a 7-bit input path; its resulting full-range global/voice morph amount may be reported back to AVR for display sync.

Per-voice morph packet shape:

```text
status = VOICE_MORPH / FRONT_SEQ_VOICE_MORPH
data1  = 0..5   for low 7 bits of Drum1, Drum2, Drum3, Snare, Cym, Hihat
data1  = 6..11  for high bit of the same six voices
data2  = 7-bit payload
```

The receiver caches the low packet and commits the full `0..255` value on the high packet.

### 2. Endpoint Restore and Display Sync

These messages push authoritative preset bytes back to the AVR so the menu matches the selected image:

- `SEQ_TMP_KIT_ENDPOINT_BEGIN`
- `SEQ_TMP_KIT_ENDPOINT_END`
- `PRF_RESTORE_PARAM_CC`
- `PRF_RESTORE_PARAM_CC2`
- `PRF_RESTORE_MORPH_CC`
- `PRF_RESTORE_MORPH_CC2`
- `SEQ_TMP_KIT_AUTOMATION_PHASE`
- `SEQ_REPORT_GLOBAL_MORPH_LSB`
- `SEQ_REPORT_GLOBAL_MORPH_MSB`
- `VOICE_MORPH` / `FRONT_SEQ_VOICE_MORPH` per-voice morph reports
- `SEQ_REPORT_ROLL_RATE` / `_NOTE` / `_VELOCITY` (`0x71-0x73`, STM -> AVR, Session 039)

The boundary restore path is still needed so the AVR menu stays coherent when the audible image changes.

The global morph and per-voice morph reports are display-only. Roll reports (`SEQ_REPORT_ROLL_*`) are also display-only: sent on every Global NRPN 93-95 data entry, the AVR writes `parameter_values[PAR_ROLL*]` and repaints without echoing back.

- it updates the AVR view of the selected kit image;
- global morph reports also synchronize all six individual voice morph display slots because global morph overrides the per-voice current values;
- individual voice morph reports update only the displayed per-voice amount;
- it must not feed back into DSP state as if it were a new file-load payload.

### 3. Flow-Control Session Mode

These messages meter high-volume transfers:

- `SEQ_FLOW_BEGIN`
- `SEQ_FLOW_GRANT`
- `SEQ_FLOW_END`
- `SEQ_FLOW_ABORT`

Known channels:

- `FLOW_CH_LOAD_SESSION`
- `FLOW_CH_GLOBALS`
- `FLOW_CH_VOICE_PARAM`
- `FLOW_CH_DRUM_META`

This layer exists to prevent bursts from overwhelming the link.
It is not a replacement for preset/session ownership.

### 4. File-Load Semantics

These messages tell STM that the incoming traffic belongs to a file load or background load:

- `SEQ_FILE_BEGIN`
- `SEQ_FILE_DONE`
- `SEQ_LOAD_VOICE`
- `SEQ_LOAD_BACKGROUND` - legacy/background-setting byte at `0x50`, currently
  still sent when the AVR global menu value changes and still received on STM
  as `FRONT_SEQ_LOAD_FAST`; this is not the active background-swap/load
  mechanism
- `SEQ_BACKGROUND_SWAP_BEGIN` / `FRONT_SEQ_BACKGROUND_SWAP_BEGIN` (`0x6d`)
- `SEQ_BACKGROUND_SWAP_DONE` / `FRONT_SEQ_BACKGROUND_SWAP_DONE` (`0x6e`)

On the AVR side, the user-facing control is the 5-state background-load
selector `PAR_FILE_LOAD_BACKGROUND` on the load page:

| Value | Label | File types that may request background swap |
|-------|-------|---------------------------------------------|
| `0` | `off` | none |
| `1` | `pat` | `.pat` / `WTYPE_PATTERN` |
| `2` | `prf` | `.prf` / `WTYPE_PERFORMANCE` |
| `3` | `all` | `.all` / `WTYPE_ALL` |
| `4` | `tot` | `.pat`, `.prf`, and `.all` |

That selector still persists as one raw byte in globals / `.cfg` storage. The
old `SEQ_LOAD_BACKGROUND` / `FRONT_SEQ_LOAD_FAST` setting-byte path at `0x50`
is still present in current code, but the completed background-loading feature
does not depend on it. The active background-load prelude is the `0x6d/0x6e`
swap handshake.

The old `PresetLoadCache` and `presetLoad_*` cache API were removed in Session
020. File-load receive paths now route bytes directly to normal Preset/Pattern
storage. `avrCommsReceivingProtocol.c` only keeps a tiny file-load ingress
bracket to ensure file bytes land in normal-kit endpoint mode during the old
AVR file-transfer envelope; it is not a cache or staging owner.

Current active background-load prelude:

```text
AVR file loader validates the selected file header
-> preset_backgroundSwapNeeded(fileType) checks sequencer-running state,
   PAR_FILE_LOAD_BACKGROUND, temp-playback guard, and SEQ_TMP_PATTERN guard
-> AVR displays "Bckgrnd Swap..."
-> AVR sends SEQ_CC / SEQ_BACKGROUND_SWAP_BEGIN / fileType
-> STM handles FRONT_SEQ_BACKGROUND_SWAP_BEGIN:
   - pat_copyToTmpPattern(seq_activePattern)
   - seq_setNextPattern(SEQ_TMP_PATTERN, 0x0f)
   - preset_tempPlaybackSwitchState.forceInstantSwitch = 1
   - preset_tempPlaybackSwitchState.patternOnlyTempPlayback = fileType is pattern
   - arm frontParser_backgroundSwapPending
-> frontParser_serviceBackgroundSwapAck() runs from the STM main loop
-> after temp playback readiness plus 400 ticks, STM sends
   FRONT_SEQ_BACKGROUND_SWAP_DONE / fileType as priority traffic
-> AVR accepts SEQ_BACKGROUND_SWAP_DONE even while avrCommsParser_rxDisable is true
-> AVR continues the ordinary SEQ_FILE_BEGIN / file payload / SEQ_FILE_DONE load
```

Readiness for the STM ACK is code-defined:

- `seq_activePattern == SEQ_TMP_PATTERN`;
- every `seq_perTrackActivePattern[track] == SEQ_TMP_PATTERN`;
- for `.pat`, `preset_allVoiceSourcesUseNormal()` must be true;
- for `.prf` / `.all`, `preset_allVoiceSourcesUseTmp()` must be true.

File-load traffic follows this broad rule:

- `.pat`, `.prf`, and `.all` loads refresh normal storage images while temp
  playback can keep running.
- file loads must not create a second sound-authority cache.
- file-load-while-temp-active behavior uses the existing normal/temp
  copy/playback model, not a revived load cache.
- when AVR already knows temp playback is active
  (`preset_backgroundTempPlaybackActive`), another `.pat`, `.prf`, or `.all`
  load skips `SEQ_BACKGROUND_SWAP_BEGIN` and proceeds as an ordinary load into
  normal storage.
- the temp-playback flag clears only when STM reports a non-`SEQ_TMP_PATTERN`
  played pattern through `SEQ_CHANGE_PAT`.

Morph rule:

- file loads must not reset or resend current global morph or current per-voice
  morph amounts;
- `.pat` load begin has no morph side effect;
- `.prf` / `.all` load begin may invalidate the live morph apply cache so newly
  loaded endpoints are recomputed from the existing current morph amounts, but
  it must not mutate those morph amounts;
- AVR file-backed kit reads/writes/dumps use `END_OF_KIT_PARAMETERS`, not
  `END_OF_SOUND_PARAMETERS`, so `PAR_MORPH_DRUM1..PAR_MORPH_HIHAT` stay live
  performance/display controls rather than saved kit bytes.

Session 033 load/reload additions:

- ordinary `.snd` kit loads and individual instrument loads now mirror the
  loaded kit/front endpoint subset into STM temp preset storage immediately;
- ordinary morph loads mirror only the morph endpoint subset into STM temp
  preset storage immediately;
- ordinary `.prf` / `.all` loads mirror both endpoint groups into STM temp
  preset storage immediately when they are not using protected background
  playback;
- protected background `.prf` / `.all` loads still preserve the old audible
  temp preset image during the load, but once playback returns to normal STM
  resnapshots temp from the newly loaded normal preset image;
- imported `PAR_VOICE_DECIMATION_ALL == 0` is clamped to `127` at the AVR
  file-import boundary before it can propagate into STM storage or future
  re-saves.

Session 034 storage-baseline additions:

- STM `/Preset/` also treats stored `PAR_VOICE_DECIMATION_ALL == 0` as `127`
  when endpoint bytes are stored or used to rebuild `interpolatedParams[]`;
- STM startup and endpoint-restore zeroing seed global decimation endpoint and
  interpolated slots to `127`, matching the mixer and AVR menu default;
- live/menu edits of shared parameters update `interpolatedParams[]`
  immediately, so step automation release reads the menu value the user just
  set.

### 4b. Reload And Euclid Temp-Snapshot Control

These controls reuse existing opcodes rather than adding new transport
messages:

- `PATCH_RESET` (`0xfe`) is now an AVR-to-STM one-byte reload request.
  STM interprets it as “copy temporary preset endpoints back into normal
  preset storage and reapply the live normal image.”
- `PATCH_RESET` must be ignored while protected `.prf` / `.all` temp preset
  playback is still active. Pattern-only `.pat` temp playback does not imply
  this block.
- `SEQ_EUKLID_RESET` / `FRONT_SEQ_EUKLID_RESET` (`0x47`) is now a multiplexed
  Euclid control opcode with these `data2` meanings:
  - `0x01`: legacy clear Euclid working caches for file/preset operations
  - `0x02`: begin one `SHIFT+PERF` Euclid-page visit
  - `0x03`: end/commit that visit when leaving the page
  - `0x04`: restore every track touched during that visit from temp back to
    the shown normal pattern

The Euclid-page visit is the real `SELECT_MODE_PAT_GEN` / `EUKLID_PAGE` path
entered by holding `SHIFT` and pressing `PERF`. It is not the separate “hold
SHIFT while already on PERF page” menu flow.

### 4c. Shown-Pattern Display Sync (`SEQ_CHANGE_PAT` / `SEQ_SET_SHOWN_PATTERN`)

This is the mechanism that keeps the AVR's idea of "which pattern is shown
and being edited" (`menu_shownPattern` on AVR, `frontParser_shownPattern` on
STM) aligned with the pattern actually driving playback. It governs where
step-parameter edit opcodes land: `FRONT_SEQ_PROB`, `FRONT_SEQ_VOLUME`,
`FRONT_SEQ_NOTE`, `FRONT_SET_P1_VAL`, and `FRONT_SET_P2_VAL` all resolve
against `frontParser_shownPattern`, not against `seq_perTrackActivePattern[]`
(the per-track table playback and Save both actually use). If the two ever
disagree, an edit is accepted by the menu, looks correct on the display, and
never affects anything audible or saved. Session 036's investigation
(`PROBABILITY_INVESTIGATION.md`, root) traces this in full; this section is
the durable protocol-level summary.

Flow, on every `SEQ_CHANGE_PAT` ACK (`avrCommsReceivingProtocol.c`):

```
STM active pattern changes (sequencer.c, any reason, including a
   temp/normal boundary crossing) -> unconditional SEQ_CHANGE_PAT ACK
   carrying seq_activePattern (frontPanelSending_sendPatternChange)
-> AVR computes tempBoundaryAck = did old/new played pattern cross the
   SEQ_TMP_PATTERN boundary (independent of PAR_FOLLOW, by design -- this
   is meant to force a resync at the boundary even with Follow off)
-> if(PAR_FOLLOW || tempBoundaryAck):
     if menu_activePage != PATTERN_SETTINGS_PAGE:
        menu_setShownPattern(patMsg)  -- updates menu_shownPattern AND
        sends SEQ_SET_SHOWN_PATTERN to STM, plus repaints the normal
        sequencer step LEDs (led_clearSequencerLeds / avrComms_updatePatternLeds)
     else:
        menu_setShownPattern(patMsg)  -- same state-sync call, but the LED
        repaint above is deliberately skipped so PATTERN_SETTINGS_PAGE's
        repurposed rotation-indicator LED display is not disturbed
-> STM's FRONT_SEQ_SET_SHOWN_PATTERN handler sets frontParser_shownPattern,
   and (only if the incoming value matches what STM already had, and no
   temp-boundary ack is pending) triggers seq_realign() -- this is the
   "press the same pattern button again to realign" feature; the pending
   temp-boundary-ack check exists specifically to suppress a spurious
   realign on a genuine boundary-crossing echo.
```

`PATTERN_SETTINGS_PAGE` (hold `SHIFT` while already on the PERF page,
`menu_shiftPerf()`) is **not** the Euclid page from `### 4b` above — see that
section's own warning. It repurposes the step LEDs to show the active
track's rotation value, which is why the LED-repaint calls are skipped for
it; the state-sync call is not skipped, and must never be, regardless of
which front-panel page is active.

**Session 036 defect and fix**: prior to Session 036, the
`PATTERN_SETTINGS_PAGE` branch assigned `menu_shownPattern` directly instead
of calling `menu_setShownPattern()`, so it updated the AVR's own display but
never sent `SEQ_SET_SHOWN_PATTERN` to the STM at all. The removed code's
comment claimed the update was deferred "for shift button release handler,"
but no such flush existed anywhere (`menu_shiftPerf(0)` just switches the
page back without touching `menu_shownPattern`/`SEQ_SET_SHOWN_PATTERN`).
This let `frontParser_shownPattern` on the STM get stuck on a stale pattern
-- including `SEQ_TMP_PATTERN` during a background-load temp/normal boundary
crossing -- with no bounded-time recovery, silently swallowing any step edit
made while stuck. The fix keeps the `if`/`else` LED-skip split intact (so
`PATTERN_SETTINGS_PAGE`'s LED display still isn't disturbed) but makes both
branches call `menu_setShownPattern(patMsg)`, so the state sync always
happens regardless of which page is active. See
`front/LxrAvr/avrComms/avrCommsReceivingProtocol.c` (`case SEQ_CHANGE_PAT`)
for the current code and its adjacent explanatory comment, and
`PROBABILITY_INVESTIGATION.md` Parts 5-7 for the full investigation, risk
assessment, and build verification.

### 5. Sample Import / Sample ROM Refresh

Sample import is a separate load-page operation from preset/pattern file load.
It refreshes STM32 internal flash sample ROM from SD-card folders and reports
progress/results over `SAMPLE_CC`.

Hardware safety:

- The SD card is normally AVR-owned; the AVR calls `spi_deInit()` before sending
  `SAMPLE_START_UPLOAD` so the STM SD loader can temporarily access the card.
- STM sample import stops the sequencer before erasing/programming internal
  flash sectors 8..11 (`0x08080000..0x080FFFFF`).
- PCM writes are bounded below the metadata table at `0x080F9400`; metadata and
  the committed sample count are written only after accepted PCM writes finish.
- STM sends progress/result packets with the priority wait transport so sparse
  user-facing sample-import status cannot be dropped by a full UART FIFO.
- The operation is blocking and can interrupt audio; it is not a background-load
  mechanism and must not be combined with the temp pattern/preset load model.

Packet namespace:

```text
status = SAMPLE_CC / FRONT SAMPLE_CC = 0xc0
byte 2 = sample subcommand
byte 3 = payload
```

AVR to STM:

| Subcommand | Value | Payload | Meaning |
|---|---:|---|---|
| `SAMPLE_START_UPLOAD` / `FRONT_SAMPLE_START_UPLOAD` | `0x01` | ignored / `0x00` | Start blocking STM import from SD into sample ROM |
| `SAMPLE_COUNT` / `FRONT_SAMPLE_COUNT` | `0x02` | ignored / `0x00` | Query committed sample count for waveform menus |

STM to AVR:

| Subcommand | Value | Payload | Meaning |
|---|---:|---|---|
| `SAMPLE_COUNT` / `FRONT_SAMPLE_COUNT` | `0x02` | `0..248` | Current dense sample count |
| `SAMPLE_UPLOAD_RESULT` / `FRONT_SAMPLE_UPLOAD_RESULT` | `0x03` | status bitmask | Final import result/warning packet |
| `SAMPLE_UPLOAD_SAMPLE_PROGRESS` / `FRONT_SAMPLE_UPLOAD_SAMPLE_PROGRESS` | `0x04` | 1-based count | Current `/samples` item being flashed |
| `SAMPLE_UPLOAD_LOOP_PROGRESS` / `FRONT_SAMPLE_UPLOAD_LOOP_PROGRESS` | `0x05` | 1-based count | Current `/loops` item being flashed |

Progress payloads are always real one-based item numbers. There is no
`data2 == 0` final marker and no separate STM `Writing Flash` signal.

Result flags:

| Flag | Value | Meaning |
|---|---:|---|
| `SAMPLE_UPLOAD_STATUS_OK` | `0x00` | No warning/error bits set |
| `SAMPLE_UPLOAD_STATUS_COUNT_LIMIT` | `0x01` | More than 248 WAV entries were found; only selectable range is imported |
| `SAMPLE_UPLOAD_STATUS_FLASH_LIMIT` | `0x02` | One or more candidates did not fit in reserved sample flash |
| `SAMPLE_UPLOAD_STATUS_READ_ERROR` | `0x04` | Invalid/short/odd-length WAV data or FatFs read failure occurred |

Current flow:

```text
AVR Load: Samples selected
-> AVR releases SD SPI with spi_deInit()
-> AVR displays "Sample upload / Started" and delays 250 ms for LCD visibility
-> AVR clears avrComms_sampleUploadDone/status
-> AVR sends SAMPLE_CC / SAMPLE_START_UPLOAD / 0
-> AVR waits without a wall-clock timeout while repeatedly calling
   uart_checkAndParse(), checking for progress idle, and animating local
   "Writing Flash" dots after roughly two seconds without numbered progress
-> STM receives FRONT_SAMPLE_START_UPLOAD
-> STM calls seq_setRunning(0), sampleMemory_init(), sampleMemory_loadSamples()
-> sampleMemory_loadSamples imports /samples first, then /loops, using a sorted
   cached 8.3 short-name list for each folder
-> STM sends each per-file progress packet immediately after accepting that file
   and before reading/writing its PCM payload
-> STM writes metadata/count last and sends final SAMPLE_UPLOAD_RESULT
-> AVR parser latches result status and menu displays any warning/error
-> AVR reinitializes preset/SD state and queries SAMPLE_COUNT
```

Important Session 030/031 rules:

- Do not use `uart_waitAck()` for sample import completion. The STM replies with
  normal `SAMPLE_CC` packets, so the AVR must keep parsing while waiting.
- Do not restore a fixed AVR upload timeout or `Load timeout` screen. Completion
  is the parsed `SAMPLE_UPLOAD_RESULT` packet.
- Do not create metadata before a candidate is known to be valid and fully
  written. Dense user slots (`s0`, `s1`, `s2`, ...) depend on writing metadata
  only after PCM success.
- Do not use indexed `sd_setActiveSample()` for the main importer. Current
  import uses `sd_openSampleFolder()` to build one sorted short-name cache for
  the active folder, then `sd_openNextSampleInFolder()` advances by cache index.
- Do not delay progress by one file and do not send a synthetic final progress
  marker. The LCD status must describe the file currently being imported; this
  is what lets the final loop/sample number appear before the silent final work.
- Sample filenames are sorted on STM by a small ASCII/natural comparator over
  short 8.3 names: digit runs compare numerically, case is ignored, and `_`,
  `-`, and spaces are treated as separators so names such as `01_cut.wav` sort
  before `01-unohit.wav`.
- The AVR owns only the display fallback for the silent final stage: if numbered
  progress has started and no new progress arrives for roughly two seconds while
  waiting for result, it switches to `Writing Flash` and animates dots locally.
- `/loops` is optional. Missing `/loops` is one-shot-only import, not a load
  failure.
- `SampleInfo.size` packs the loop bit in bit 31 and the int16 frame count in
  bits 30..0. The STM playback code uses that bit to distinguish one-shots from
  loops without growing the 12-byte on-flash metadata ABI.

### 6. Background Temp-Playback UI Feedback

The AVR owns the visible temp-playback hint because it already tracks
`preset_backgroundTempPlaybackActive`.

Relevant functions:

- `preset_isBackgroundTempPlaybackActive()`
- `buttonHandler_refreshTempPlaybackLedHint()`
- `preset_notePlayedPatternChanged()`

Current UI behavior while temp playback is active:

- outside `SELECT_MODE_PERF`, `LED_MODE2` / PERF flashes while the current mode
  LED remains active;
- inside `SELECT_MODE_PERF`, all eight SELECT LEDs flash;
- `SHIFT+PERF` is ignored so PAT_GEN cannot be entered while temp playback
  remains active;
- `SHIFT+PLAY` / `PATCH_RESET` is ignored only while temp preset playback is
  active for protected `.prf` / `.all` background loads;
- `NUM_OF_BLINKABLE_LEDS` is currently `8` to support all SELECT LEDs blinking.

### 7. Callback and Hold Primitives

Legacy wait primitives still exist:

- `CALLBACK_ACK`
- `avrComms_holdForBuffer()`

These are still narrow compatibility tools.
They should not become a general mechanism for future protocol growth.

## Historical AVR-Side Initiators That Fed The Load Path

The AVR side does not own the canonical session state. The names below are kept as historical reference only because Session 024 commented out the thin cache helper surface that used to feed the old background-load machinery.

### Front-panel protocol initiators

In `front/LxrAvr/avrComms/avrCommsSendingProtocol.c`:

- `avrComms_prfCacheBegin()` (commented out in Session 024)
- `avrComms_prfCacheControl()` (commented out in Session 024)
- `avrComms_flowBeginSession()`
- `avrComms_flowEndSession()`
- `avrComms_flowAbortSession()`
- `PRF_CACHE_STATUS` handling (commented out in Session 024)

These were compatibility entry points for the old file-transfer envelope.
Session 024 commented out the obsolete cache behavior; do not treat these
commands as a live background-load cache regime.

### AVR preset/file-load flows

In `front/LxrAvr/Preset/presetManager.c`:

- `preset_loadDrumset()`
- `preset_loadAll()`
- `preset_loadPerf()`
- `preset_loadPattern()`
- `preset_saveDrumset()`
- `preset_saveAll()`
- `preset_shouldPreserveMenuEndpointsDuringFileLoad()`
- `preset_saveMenuEndpointsDuringFileLoad()`
- `preset_restoreMenuEndpointsDuringFileLoad()`
- `preset_backgroundSwapNeeded()`
- `preset_performBackgroundSwapWait()`
- `preset_backgroundSwapDoneFromStm()`
- `preset_notePlayedPatternChanged()`
- `preset_isBackgroundTempPlaybackActive()`

These are the file-load and menu-preservation flows that start the transport/session traffic.

They are the right place to keep the user-facing initiation logic.
They are not the right place to own STM storage/session semantics. They do own
the AVR policy decision about whether the current file type should request the
STM background-swap prelude.

### STM background-swap service points

In `mainboard/LxrStm32/src/uARTFrontSYX/frontPanelReceivingProtocol.c`:

- `FRONT_SEQ_BACKGROUND_SWAP_BEGIN` is the receive-side entry point.
- `frontParser_serviceBackgroundSwapAck()` is the non-blocking main-loop
  service that waits for readiness and sends `FRONT_SEQ_BACKGROUND_SWAP_DONE`.
- `frontParser_backgroundSwapTempPlaybackReady()` defines when the ACK may arm
  its final delay.

In `mainboard/LxrStm32/src/main.c`:

- `frontParser_serviceBackgroundSwapAck()` is called after
  `uart_processFront()` each main-loop pass.

## Send-Side Completion State

The send-side migration is complete for the current audit scope:

- STM front-panel output goes through
  `mainboard/LxrStm32/src/uARTFrontSYX/frontPanelSendingProtocol.c/.h`.
- AVR front-panel output goes through
  `front/LxrAvr/avrComms/avrCommsSendingProtocol.c/.h`.
- Raw UART files remain byte transport only.
- MIDI echo paths use the STM sending protocol layer for display-facing
  feedback and do not parse AVR command semantics.

## The `.pat` Rule

Pattern-only background loading is the important wrinkle and is now active code.

`.pat` background loading should never switch preset parameter read/write away from normal storage.

That means:

- pattern data can be staged in pattern-owned temp structures;
- parameter ingress must remain on normal storage for `.pat` background loads;
- pattern temp/normal ownership and parameter temp/normal ownership must remain separate;
- the implementation uses `preset_tempPlaybackSwitchState.patternOnlyTempPlayback`
  to distinguish pattern-only background loads from parameter-bearing file loads.

This rule is easy to miss if the pattern loader and parameter loader share one broad session object.
That is exactly why the interface keeps the ownership boundaries explicit.

## Current Shape After The Audit

The comms layer now has this split:

- `Uart.c/.h` for byte transport only
- `avrCommsReceivingProtocol.c/.h` for parsing and receive-side dispatch
- `avrCommsSendingProtocol.c/.h` for outbound framing and status messages
- receive protocol headers for the opcode namespaces
- `Preset` for storage and session semantics
- `Pattern` for pattern storage and pattern temp behavior

The parser must not keep a second background-load model. It routes semantics
into `Preset` and `Pattern`, then gets out of the way.

The old PRF/cache initiator names are now commented out rather than active.
Keep their history in the log archive rather than reintroducing them in place,
and do not bring back a cache-as-authority model under the same names.

## Guardrails

- Do not broaden blocking waits just because the comms layer is now better organized.
- Do not let display-only restore traffic feed back into live DSP state.
- Do not let transport code inspect `SeqKitState` directly.
- Do not create a second load/session model beside the temp/normal storage model.
- Do not merge pattern-only background loading with parameter storage switching.
- Do not let send/receive protocol work blur the ownership boundary between
  parsing bytes and deciding what they mean.
- Do not treat `SEQ_LOAD_BACKGROUND` / `FRONT_SEQ_LOAD_FAST` at `0x50` as the
  active background-load mechanism; active background swap is `0x6d/0x6e`.
- Do not reset current global/per-voice morph amounts from any file-load begin
  or done message.
- Do not send another `SEQ_BACKGROUND_SWAP_BEGIN` while AVR temp playback state
  is already active.
- Do not revive raw-ACK waits for sample import; wait by parsing
  `SAMPLE_UPLOAD_RESULT`.
- Do not treat an external NOTE_ON with velocity 0 as a press anywhere, and
  never release a MIDI roll by recomputing voices from current routing; release
  the mask the key captured at press (Session 038).
- Any new event that invalidates MIDI note routing must call
  `midiParser_clearMidiRollHolds()` (the parser table), not merely clear the
  sequencer's `seq_rollMidiHeld` bits.
- Do not revive sample-upload wall-clock timeouts or `Load timeout`; the AVR
  owns only a display idle spinner while it waits for result.
- Do not delay sample progress by one file or send synthetic final progress
  markers; progress packets are real current-file one-based counts.
