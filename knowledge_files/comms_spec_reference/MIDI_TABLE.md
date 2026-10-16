# MIDI Table

Date: 2026-10-16
Status: current after Session 039 (¢ RX-filter CC38 swap and NRPN 96; RPN-null swallow; NRPN 94/95 roll note/velocity with PERF display reports; RX filter C gating all incoming CCs; TX filter C transmitting menu edits as Global CC/NRPN; whole-message DIN/USB MIDI out with the USB IN completion/timeout fix), on top of the Session 038 stuck-MIDI-roll fix (NOTE_ON velocity 0 is a note-off; per-key MIDI roll ownership; stop release; stopped-transport MIDI rolls) and the 2026-09-17 MIDI roll-trigger implementation. This is the durable MIDI reference for external DIN/USB MIDI input parsing, realtime dispatch timing, and menu-originated MIDI transmit on STM32. Full Session 039 rationale: `knowledge_files/log_archive/039_SESSION_HANDOFF_LOG.md`.

## Ownership And Routing

External MIDI byte parsing starts in `mainboard/LxrStm32/src/MIDI/MidiParser.c`.

Important ownership boundaries:

- `MidiParser.c` owns byte-stream parsing, routing/filtering, system message dispatch, note/program-change routing, the high-level CC router, the DWT timestamp helper used by realtime capture, the saved MIDI roll-note offset, and the MIDI roll held-key table (`(channel, note)` -> claimed voice mask, Session 038). The sequencer owns only the per-voice MIDI ownership bit (`seq_rollMidiHeld`, via `seq_rollMidiChange()`), which stays separate from manual front-panel roll ownership.
- `uARTFrontSYX/Uart.c` and `Hardware/USB/usb_midi_core.c` each own one source-specific, 16-event SPSC realtime capture queue. `main.c` drains DIN then USB at the audio render deadline before rendering the next block.
- `ChannelMidiParser.c` owns the existing per-voice/channel CC implementation, plus the preserved CC0 bank-change and CC1 morph behavior.
- `GlobalMidiParser.c` owns system clock/MTC handling and, after Session 029, the separate Global-channel CC/NRPN table for CC2-127. Session 039: it also owns the NRPN protocol state (selection, ¢ CC38 swap flag `globalMidiParser_cc38Swap`, RPN-null window), the special NRPNs 93-96 (numbers defined in `GlobalMidiParser.h`), and the TX reverse address table `globalMidiParser_getTxAddress()`.
- `MidiOutputControl.c` owns MIDI output fan-out (`outputControl_sendMidi()` to USB and DIN) and, since Session 039, the menu MIDI-TX gate, coalescing queue and `outputControl_serviceMenuTx()` (called from the `main.c` loop before `usb_tick()`).
- `Hardware/USB/usb_manager.c` / `usb_midi_core.c` own USB MIDI out: whole-event bounds check, `usbd_midi_DataIn()` completion flag, 5 ms busy timeout, discard while not `USB_OTG_CONFIGURED` (Session 039). `uARTFrontSYX/Uart.c` owns DIN MIDI out (`uart_sendMidi()` whole-message-or-drop, Session 039).
- `frontPanelReceivingProtocol.c` owns the internal `MIDI_CC` / `MIDI_CC2` parameter apply ladder through `frontParser_applyParameterCommand()`. The new Global CC/NRPN parser translates into this existing internal apply path rather than duplicating DSP apply code.

Global-channel CC pre-emption:

- If an incoming MIDI CC is on `midi_MidiChannels[7]`, the Global channel wins over any overlapping voice-channel assignment.
- Global CC0 and CC1 are explicit exceptions: they still run through the current `ChannelMidiParser.c` bank/morph behavior.
- Global CC2-127 run only through the new `GlobalMidiParser.c` table and do not also run through the voice-channel parser.
- Non-global CC messages continue to use the existing `ChannelMidiParser.c` implementation.

RX filter and routing (Session 039 P2):

- `mrt` routing runs first in `midiParser_parseMidiMessage()` and is never affected by the RX filter. A filtered message is still forwarded if routed.
- RX letters gate what the LXR responds to: N `0x01` notes (including MIDI rolls), R `0x02` clock/start/stop/continue/MTC, C `0x04` **all** incoming CC traffic (every channel; CC0 bank, CC1 morph, NRPN, both CC tables), P `0x08` program change.
- C is gated once at the `MIDI_CC` entry. The older `0x04` tests inside the CC ladders are now redundant guards.
- TX C (`0x40`) transmits menu-originated parameter edits as Global CC/NRPN (Session 039 P3); see `## MIDI Transmit` below. TX N/R/P gate sequencer note-ons, clock/start/stop and pattern program changes; note-offs are never gated.

### Global Parameter-ID Domains

The Global CC and NRPN numbers in this document are fixed external legacy
assignments. Internally, `GlobalMidiParser.c` lookup tables use canonical raw
`PAR_*` IDs, which are the IDs used by Preset storage, AVR parameter packets,
and pattern automation. For low parameters only, the Global parser converts
that raw ID to the separate legacy MIDI apply ID immediately before calling the
live DSP switch. No raw Preset/AVR/automation ID is incremented; this is an
implementation boundary, not a change to any published CC or NRPN assignment.

## Non-CC Messages

### System Messages

For system-realtime statuses `0xf8..0xff`, DIN classifies at USART2 RXNE and
USB classifies at the completed MIDI OUT callback. Both capture a DWT cycle
timestamp into their own 16-event queue. `serviceAudioRenderDeadline()` drains
DIN first, then USB, immediately before `calcNextSampleBlock()`; the queue is
therefore a bounded main-context timing path, not an ISR execution path. The
existing filter and external-sync behavior below is unchanged. Timestamps are
diagnostics, not sample-offset compensation, and fixed serial/USB-host,
ping-pong, I2S, and DAC latency remains.

| Message | Current behavior |
|---|---|
| MIDI clock `0xf8` | If RX clock filter is enabled and external sync is enabled, call `seq_sync()`. |
| MIDI start `0xfa` | If RX clock filter is enabled and external sync is enabled, call `sync_midiStartStop(1)`. |
| MIDI continue `0xfb` | Same as start. |
| MIDI stop `0xfc` | If RX clock filter is enabled and external sync is enabled, call `sync_midiStartStop(0)`. Like every `seq_setRunning(0)`, this also releases all MIDI roll holds (Session 038). |
| MTC quarter frame `0xf1` | Global MTC start detection starts sequencer only after a complete zero-position frame set; `midiParser_checkMtc()` stops if MTC goes stale. |
| SysEx payload | Routed/passed through as transport data where configured; this parser does not interpret arbitrary SysEx payloads. |

### Notes

| Input channel | Current behavior |
|---|---|
| Global channel | Note-on/off targets the active front-panel track. If that track has no note override, chromatic note mode is used for the active track. If the active track has note override enabled, the parser scans all seven track note overrides and triggers matching tracks. |
| Voice/track channels | Note-on/off triggers each track whose `midi_MidiChannels[track]` matches the incoming channel. Track note override filters incoming notes when configured. |
| Note-off classification | A message is a note-off if its status is NOTE_OFF **or** it is NOTE_ON with velocity `0` (MIDI 1.0). Such messages take the `channelMidiParser_noteOff()` route on normal paths and the release branch on the roll path. For normal notes this is behaviourally identical to the old NOTE_ON route. (Session 038) |
| Recording side effect | When a note is accepted with `do_rec = 1`, the parser records it to the sequencer and echoes MIDI note output for the voice channel. |
| Note-off recording placement | `channelMidiParser_noteOff()` forces `vel = 0` and delegates to `channelMidiParser_noteOn()`, which calls `seq_addNote(voice, vel, note, 1)`. That trailing `1` is `isNoteOff`, and it is what allows a note-off to keep its **true, un-quantized** position (a zero-velocity "ghost" step marking where the note was released). **This MIDI path is the only caller permitted to pass `isNoteOff = 1`.** Every internal trigger — roll hits, loop re-record — passes `0` and is always written to the quantized slot, even at velocity 0. Placement must never again be inferred from velocity alone; see Session 037. |

MIDI roll notes use the saved global roll-note offset. Raw `0` disables the
feature and displays as `off`; raw `1..127` is a positive semitone offset above
the normal trigger note. With a note override, the normal route is tried first
and roll notes are evaluated only if it did not consume the message. On
chromatic-mode tracks (no note override), the offset acts as a keyboard split:
notes below it play normally, notes at or above it are roll notes only. Nothing
rolls while the offset is `off`. Notes that would shift outside MIDI `0..127`
do nothing.

A NOTE_ON with velocity `0` is a note-off (MIDI 1.0) on every note route,
including the roll path (Session 038; the earlier "literal status" rule is
withdrawn). Each held roll key (channel + note) claims the voices it matched at
note-on, and its note-off releases exactly those voices. A voice keeps rolling
while any held key claims it. A repeated note-on for a held key is not counted
twice; unmatched note-offs are ignored; at most 16 roll keys can be held at
once.

All MIDI roll holds are released on sequencer stop (every stop request),
roll-offset change, MIDI channel change or off, track note-override change, RX
note-filter disable, and parser cache clear. While the sequencer is stopped, a
roll key still starts a roll immediately. It repeats at the current tempo and
roll rate, using roll velocity and roll note regardless of roll mode; a one-shot
rate fires once per press. If the key is still held at start, the roll
continues on the normal quantized roll engine, with a single first hit at the
first quantize boundary. Front-panel (manual) rolls remain silent while
stopped.

### Program Change

| Input channel | Current behavior |
|---|---|
| Global channel | If program number `< 16`, select next pattern `program & 0x07` for all tracks. Program values `8..15` also set `seq_newVoiceAvailable = 0x7f` to reload kit voices. |
| Voice/track channels | For each track whose MIDI channel matches, select next pattern `program & 0x07` for that individual track. |

## Channel MIDI CC Table

This table documents the existing non-global channel parser in `ChannelMidiParser.c`. The same CC number has different destinations depending on which configured voice/track MIDI channel receives it.

`HH A` means `midi_MidiChannels[5]`; `HH B` means `midi_MidiChannels[6]`. Both hihat channel assignments currently target the same hihat sound parameters. For CC0/CC1, either hihat channel contributes both bank/morph bits `BANK_6 | BANK_7`.

| CC | Function | Drum 1 | Drum 2 | Drum 3 | Snare | Cymbal | HH A | HH B |
|---:|---|---|---|---|---|---|---|---|
| 0 | Bank change | bank mask `BANK_1` | `BANK_2` | `BANK_3` | `BANK_4` | `BANK_5` | `BANK_6|BANK_7` | `BANK_6|BANK_7` |
| 1 | Morph/mod wheel | voice morph mask `BANK_1` | `BANK_2` | `BANK_3` | `BANK_4` | `BANK_5` | `BANK_6|BANK_7` | `BANK_6|BANK_7` |
| 7 | Voice volume | `VOL1` | `VOL2` | `VOL3` | `VOL4` | `VOL5` | `VOL6` | `VOL6` |
| 9 | Osc 1 waveform | `OSC_WAVE_DRUM1` | `OSC_WAVE_DRUM2` | `OSC_WAVE_DRUM3` | `OSC_WAVE_SNARE` | `CYM_WAVE1` | `WAVE1_HH` | `WAVE1_HH` |
| 10 | Pan | `PAN1` | `PAN2` | `PAN3` | `PAN4` | `PAN5` | `PAN6` | `PAN6` |
| 12 | Decimation | `VOICE_DECIMATION1` | `VOICE_DECIMATION2` | `VOICE_DECIMATION3` | `VOICE_DECIMATION4` | `VOICE_DECIMATION5` | `VOICE_DECIMATION6` | `VOICE_DECIMATION6` |
| 13 | Distortion | `OSC1_DIST` | `OSC2_DIST` | `OSC3_DIST` | `SNARE_DISTORTION` | `CYMBAL_DISTORTION` | `HAT_DISTORTION` | `HAT_DISTORTION` |
| 14 | Osc 1 coarse tune | `F_OSC1_COARSE` | `F_OSC2_COARSE` | `F_OSC3_COARSE` | `F_OSC4_COARSE` | `F_OSC5_COARSE` | `F_OSC6_COARSE` | `F_OSC6_COARSE` |
| 16 | Filter frequency | `FILTER_FREQ_DRUM1` | `FILTER_FREQ_DRUM2` | `FILTER_FREQ_DRUM3` | `SNARE_FILTER_F` | `CYM_FIL_FREQ` | `HAT_FILTER_F` | `HAT_FILTER_F` |
| 17 | Filter resonance | `RESO_DRUM1` | `RESO_DRUM2` | `RESO_DRUM3` | `SNARE_RESO` | `CYM_RESO` | `HAT_RESO` | `HAT_RESO` |
| 18 | Filter drive | `128+CC2_FILTER_DRIVE_1` | `128+CC2_FILTER_DRIVE_2` | `128+CC2_FILTER_DRIVE_3` | `128+CC2_FILTER_DRIVE_4` | `128+CC2_FILTER_DRIVE_5` | `128+CC2_FILTER_DRIVE_6` | `128+CC2_FILTER_DRIVE_6` |
| 19 | Filter type | `128+CC2_FILTER_TYPE_1` | `128+CC2_FILTER_TYPE_2` | `128+CC2_FILTER_TYPE_3` | `128+CC2_FILTER_TYPE_4` | `128+CC2_FILTER_TYPE_5` | `128+CC2_FILTER_TYPE_6` | `128+CC2_FILTER_TYPE_6` |
| 20 | Snare mix | -- | -- | -- | `SNARE_MIX` | -- | -- | -- |
| 21 | Snare noise frequency | -- | -- | -- | `SNARE_NOISE_F` | -- | -- | -- |
| 22 | Mix/mod select | `128+CC2_MIX_MOD_1` | `128+CC2_MIX_MOD_2` | `128+CC2_MIX_MOD_3` | -- | -- | -- | -- |
| 23 | Velocity volume mod on/off | `128+CC2_VOLUME_MOD_ON_OFF1` | `128+CC2_VOLUME_MOD_ON_OFF2` | `128+CC2_VOLUME_MOD_ON_OFF3` | `128+CC2_VOLUME_MOD_ON_OFF4` | `128+CC2_VOLUME_MOD_ON_OFF5` | `128+CC2_VOLUME_MOD_ON_OFF6` | `128+CC2_VOLUME_MOD_ON_OFF6` |
| 24 | Velocity mod amount | `128+CC2_VELO_MOD_AMT_1` | `128+CC2_VELO_MOD_AMT_2` | `128+CC2_VELO_MOD_AMT_3` | `128+CC2_VELO_MOD_AMT_4` | `128+CC2_VELO_MOD_AMT_5` | `128+CC2_VELO_MOD_AMT_6` | `128+CC2_VELO_MOD_AMT_6` |
| 25 | Velocity mod destination | `128+CC2_VEL_DEST_1` | `128+CC2_VEL_DEST_2` | `128+CC2_VEL_DEST_3` | `128+CC2_VEL_DEST_4` | `128+CC2_VEL_DEST_5` | `128+CC2_VEL_DEST_6` | `128+CC2_VEL_DEST_6` |
| 26 | Transient volume | `128+CC2_TRANS1_VOL` | `128+CC2_TRANS2_VOL` | `128+CC2_TRANS3_VOL` | `128+CC2_TRANS4_VOL` | `128+CC2_TRANS5_VOL` | `128+CC2_TRANS6_VOL` | `128+CC2_TRANS6_VOL` |
| 27 | Transient waveform | `128+CC2_TRANS1_WAVE` | `128+CC2_TRANS2_WAVE` | `128+CC2_TRANS3_WAVE` | `128+CC2_TRANS4_WAVE` | `128+CC2_TRANS5_WAVE` | `128+CC2_TRANS6_WAVE` | `128+CC2_TRANS6_WAVE` |
| 28 | Transient frequency | `128+CC2_TRANS1_FREQ` | `128+CC2_TRANS2_FREQ` | `128+CC2_TRANS3_FREQ` | `128+CC2_TRANS4_FREQ` | `128+CC2_TRANS5_FREQ` | `128+CC2_TRANS6_FREQ` | `128+CC2_TRANS6_FREQ` |
| 46 | Osc 1 fine tune | `F_OSC1_FINE` | `F_OSC2_FINE` | `F_OSC3_FINE` | `F_OSC4_FINE` | `F_OSC5_FINE` | `F_OSC6_FINE` | `F_OSC6_FINE` |
| 70 | Volume envelope repeat | -- | -- | -- | `REPEAT1` | `CYM_REPEAT` | -- | -- |
| 72 | Open hihat decay | -- | -- | -- | -- | -- | `VELOD6_OPEN` | `VELOD6_OPEN` |
| 73 | Volume envelope attack | `VELOA1` | `VELOA2` | `VELOA3` | `VELOA4` | `VELOA5` | `VELOA6` | `VELOA6` |
| 74 | Volume envelope slope | `VOL_SLOPE1` | `VOL_SLOPE2` | `VOL_SLOPE3` | `EG_SNARE1_SLOPE` | `CYM_SLOPE` | `VOL_SLOPE6` | `VOL_SLOPE6` |
| 75 | Volume envelope decay | `VELOD1` | `VELOD2` | `VELOD3` | `VELOD4` | `VELOD5` | `VELOD6` | `VELOD6` |
| 76 | LFO frequency | `FREQ_LFO1` | `FREQ_LFO2` | `FREQ_LFO3` | `FREQ_LFO4` | `FREQ_LFO5` | `FREQ_LFO6` | `FREQ_LFO6` |
| 77 | LFO amount | `AMOUNT_LFO1` | `AMOUNT_LFO2` | `AMOUNT_LFO3` | `AMOUNT_LFO4` | `AMOUNT_LFO5` | `AMOUNT_LFO6` | `AMOUNT_LFO6` |
| 78 | LFO phase offset | `128+CC2_OFFSET_LFO1` | `128+CC2_OFFSET_LFO2` | `128+CC2_OFFSET_LFO3` | `128+CC2_OFFSET_LFO4` | `128+CC2_OFFSET_LFO5` | `128+CC2_OFFSET_LFO6` | `128+CC2_OFFSET_LFO6` |
| 79 | LFO waveform | `128+CC2_WAVE_LFO1` | `128+CC2_WAVE_LFO2` | `128+CC2_WAVE_LFO3` | `128+CC2_WAVE_LFO4` | `128+CC2_WAVE_LFO5` | `128+CC2_WAVE_LFO6` | `128+CC2_WAVE_LFO6` |
| 80 | LFO target voice selector | `128+CC2_VOICE_LFO1` | `128+CC2_VOICE_LFO2` | `128+CC2_VOICE_LFO3` | `128+CC2_VOICE_LFO4` | `128+CC2_VOICE_LFO5` | `128+CC2_VOICE_LFO6` | `128+CC2_VOICE_LFO6` |
| 81 | LFO target selector | `128+CC2_TARGET_LFO1` | `128+CC2_TARGET_LFO2` | `128+CC2_TARGET_LFO3` | `128+CC2_TARGET_LFO4` | `128+CC2_TARGET_LFO5` | `128+CC2_TARGET_LFO6` | `128+CC2_TARGET_LFO6` |
| 82 | LFO retrigger | `128+CC2_RETRIGGER_LFO1` | `128+CC2_RETRIGGER_LFO2` | `128+CC2_RETRIGGER_LFO3` | `128+CC2_RETRIGGER_LFO4` | `128+CC2_RETRIGGER_LFO5` | `128+CC2_RETRIGGER_LFO6` | `128+CC2_RETRIGGER_LFO6` |
| 83 | LFO sync | `128+CC2_SYNC_LFO1` | `128+CC2_SYNC_LFO2` | `128+CC2_SYNC_LFO3` | `128+CC2_SYNC_LFO4` | `128+CC2_SYNC_LFO5` | `128+CC2_SYNC_LFO5` | `128+CC2_SYNC_LFO5` |
| 84 | Mod envelope decay | `PITCHD1` | `PITCHD2` | `PITCHD3` | `PITCHD4` | -- | -- | -- |
| 85 | Envelope mod amount | `MODAMNT1` | `MODAMNT2` | `MODAMNT3` | `MODAMNT4` | -- | -- | -- |
| 86 | Mod envelope slope | `PITCH_SLOPE1` | `PITCH_SLOPE2` | `PITCH_SLOPE3` | `PITCH_SLOPE4` | -- | -- | -- |
| 89 | Audio output routing | `128+CC2_AUDIO_OUT1` | `128+CC2_AUDIO_OUT2` | `128+CC2_AUDIO_OUT3` | `128+CC2_AUDIO_OUT4` | `128+CC2_AUDIO_OUT5` | `128+CC2_AUDIO_OUT6` | `128+CC2_AUDIO_OUT6` |
| 90 | Envelope position reset | `128+CC2_ENVELOPE_POSITION_1` | `128+CC2_ENVELOPE_POSITION_2` | `128+CC2_ENVELOPE_POSITION_3` | `128+CC2_ENVELOPE_POSITION_4` | `128+CC2_ENVELOPE_POSITION_5` | `128+CC2_ENVELOPE_POSITION_6` | `128+CC2_ENVELOPE_POSITION_6` |
| 102 | Mod oscillator waveform | `MOD_WAVE_DRUM1` | `MOD_WAVE_DRUM2` | `MOD_WAVE_DRUM3` | -- | -- | -- | -- |
| 103 | FM/mod oscillator coarse tune | `FMDTN1` | `FMDTN2` | `FMDTN3` | -- | -- | -- | -- |
| 104 | FM amount | `FMAMNT1` | `FMAMNT2` | `FMAMNT3` | -- | -- | -- | -- |
| 105 | Mod oscillator waveform 1 | -- | -- | -- | -- | `CYM_WAVE2` | `WAVE2_HH` | `WAVE2_HH` |
| 106 | Mod oscillator gain 1 | -- | -- | -- | -- | `CYM_MOD_OSC_GAIN1` | `MOD_OSC_GAIN1` | `MOD_OSC_GAIN1` |
| 107 | Mod oscillator coarse tune 1 | -- | -- | -- | -- | `CYM_MOD_OSC_F1` | `MOD_OSC_F1` | `MOD_OSC_F1` |
| 108 | Mod oscillator waveform 2 | -- | -- | -- | -- | `CYM_WAVE3` | `WAVE3_HH` | `WAVE3_HH` |
| 109 | Mod oscillator gain 2 | -- | -- | -- | -- | `CYM_MOD_OSC_GAIN2` | `MOD_OSC_GAIN2` | `MOD_OSC_GAIN2` |
| 110 | Mod oscillator coarse tune 2 | -- | -- | -- | -- | `CYM_MOD_OSC_F2` | `MOD_OSC_F2` | `MOD_OSC_F2` |
| 120 | Track mute | `128+CC2_MUTE_1` | `128+CC2_MUTE_2` | `128+CC2_MUTE_3` | `128+CC2_MUTE_4` | `128+CC2_MUTE_5` | `128+CC2_MUTE_6` | `128+CC2_MUTE_6` |

Notes:

- Channel CC1 remains a 7-bit morph input. If the incoming channel is the Global channel, it calls `seq_setGlobalMorphAutomationValue()`. Otherwise it applies a voice mask through `seq_setVoiceMorphMaskAutomationValue()`.
- Channel CC120 sets mute off when value is `0`; any nonzero value mutes.
- Hihat channel CC83 currently applies hihat LFO sync but stores/echoes `128+CC2_SYNC_LFO5` in the channel parser for both hihat channel assignments. This is documented as current behavior, not as a recommended future mapping.
- Channel MIDI has no separate NRPN parser after Session 029. High-parameter destinations in the channel table are reached directly by the listed CC numbers.

## Global MIDI CC Table

This table applies only on the configured Global MIDI channel. CC0 and CC1 are preserved current behavior; CC2-127 use the separate Session 029 Global parser table.

| CC | Function | Voice/scope | Internal target |
|---:|---|---|---|
| 0 | Bank change | Global/current behavior | Existing `ChannelMidiParser.c` CC0 path, global bank mask `0x3f` |
| 1 | Mod wheel/global morph | Global/current behavior | Existing `ChannelMidiParser.c` CC1 path, `seq_setGlobalMorphAutomationValue()` |
| 2 | Waveform Osc 1 | Drum 1 | `OSC_WAVE_DRUM1` |
| 3 | Waveform Osc 1 | Drum 2 | `OSC_WAVE_DRUM2` |
| 4 | Waveform Osc 1 | Drum 3 | `OSC_WAVE_DRUM3` |
| 5 | Waveform Osc 1 | Snare | `OSC_WAVE_SNARE` |
| 6 | NRPN data entry | Global NRPN state | Apply selected NRPN |
| 7 | Waveform Osc 1 | Cymbal | `CYM_WAVE1` |
| 8 | Waveform Osc 1 | Hihat | `WAVE1_HH` |
| 9 | Coarse tune Osc 1 | Drum 1 | `F_OSC1_COARSE` |
| 10 | Fine tune Osc 1 | Drum 1 | `F_OSC1_FINE` |
| 11 | Coarse tune Osc 1 | Drum 2 | `F_OSC2_COARSE` |
| 12 | Fine tune Osc 1 | Drum 2 | `F_OSC2_FINE` |
| 13 | Coarse tune Osc 1 | Drum 3 | `F_OSC3_COARSE` |
| 14 | Fine tune Osc 1 | Drum 3 | `F_OSC3_FINE` |
| 15 | Coarse tune Osc 1 | Snare | `F_OSC4_COARSE` |
| 16 | Fine tune Osc 1 | Snare | `F_OSC4_FINE` |
| 17 | Coarse tune Osc 1 | Cymbal | `F_OSC5_COARSE` |
| 18 | Fine tune Osc 1 | Cymbal | `F_OSC5_FINE` |
| 19 | Coarse tune Osc 1 | Hihat | `F_OSC6_COARSE` |
| 20 | Fine tune Osc 1 | Hihat | `F_OSC6_FINE` |
| 21 | Waveform mod Osc | Drum 1 | `MOD_WAVE_DRUM1` |
| 22 | Waveform mod Osc | Drum 2 | `MOD_WAVE_DRUM2` |
| 23 | Waveform mod Osc | Drum 3 | `MOD_WAVE_DRUM3` |
| 24 | Waveform mod Osc 1 | Cymbal | `CYM_WAVE2` |
| 25 | Waveform mod Osc 2 | Cymbal | `CYM_WAVE3` |
| 26 | Waveform mod Osc 1 | Hihat | `WAVE2_HH` |
| 27 | Waveform mod Osc 2 | Hihat | `WAVE3_HH` |
| 28 | Noise frequency | Snare | `SNARE_NOISE_F` |
| 29 | Mix osc/noise | Snare | `SNARE_MIX` |
| 30 | Coarse tune mod Osc 1 | Cymbal | `CYM_MOD_OSC_F1` |
| 31 | Coarse tune mod Osc 2 | Cymbal | `CYM_MOD_OSC_F2` |
| 32 | Gain mod Osc 1 | Cymbal | `CYM_MOD_OSC_GAIN1` |
| 33 | Gain mod Osc 2 | Cymbal | `CYM_MOD_OSC_GAIN2` |
| 34 | Coarse tune mod Osc 1 | Hihat | `MOD_OSC_F1` |
| 35 | Coarse tune mod Osc 2 | Hihat | `MOD_OSC_F2` |
| 36 | Gain mod Osc 1 | Hihat | `MOD_OSC_GAIN1` |
| 37 | Gain mod Osc 2 | Hihat | `MOD_OSC_GAIN2` |
| 38 | Filter frequency (ignored in ¢ RX-filter modes; use NRPN 96) | Drum 1 | `FILTER_FREQ_DRUM1` |
| 39 | Filter frequency | Drum 2 | `FILTER_FREQ_DRUM2` |
| 40 | Filter frequency | Drum 3 | `FILTER_FREQ_DRUM3` |
| 41 | Filter frequency | Snare | `SNARE_FILTER_F` |
| 42 | Filter frequency | Cymbal | `CYM_FIL_FREQ` |
| 43 | Filter frequency | Hihat | `HAT_FILTER_F` |
| 44 | Filter resonance | Drum 1 | `RESO_DRUM1` |
| 45 | Filter resonance | Drum 2 | `RESO_DRUM2` |
| 46 | Filter resonance | Drum 3 | `RESO_DRUM3` |
| 47 | Filter resonance | Snare | `SNARE_RESO` |
| 48 | Filter resonance | Cymbal | `CYM_RESO` |
| 49 | Filter resonance | Hihat | `HAT_RESO` |
| 50 | Volume envelope attack | Drum 1 | `VELOA1` |
| 51 | Volume envelope decay | Drum 1 | `VELOD1` |
| 52 | Volume envelope attack | Drum 2 | `VELOA2` |
| 53 | Volume envelope decay | Drum 2 | `VELOD2` |
| 54 | Volume envelope attack | Drum 3 | `VELOA3` |
| 55 | Volume envelope decay | Drum 3 | `VELOD3` |
| 56 | Volume envelope attack | Snare | `VELOA4` |
| 57 | Volume envelope decay | Snare | `VELOD4` |
| 58 | Volume envelope attack | Cymbal | `VELOA5` |
| 59 | Volume envelope decay | Cymbal | `VELOD5` |
| 60 | Volume envelope attack | Hihat | `VELOA6` |
| 61 | Closed hihat decay time | Hihat | `VELOD6` |
| 62 | Open hihat decay time | Hihat | `VELOD6_OPEN` |
| 63 | Volume envelope slope | Drum 1 | `VOL_SLOPE1` |
| 64 | Volume envelope slope | Drum 2 | `VOL_SLOPE2` |
| 65 | Volume envelope slope | Drum 3 | `VOL_SLOPE3` |
| 66 | Volume envelope slope | Snare | `EG_SNARE1_SLOPE` |
| 67 | Volume envelope slope | Cymbal | `CYM_SLOPE` |
| 68 | Volume envelope slope | Hihat | `VOL_SLOPE6` |
| 69 | Volume envelope repeat count | Snare | `REPEAT1` |
| 70 | Volume envelope repeat count | Cymbal | `CYM_REPEAT` |
| 71 | Mod envelope decay | Drum 1 | `PITCHD1` |
| 72 | Mod envelope decay | Drum 2 | `PITCHD2` |
| 73 | Mod envelope decay | Drum 3 | `PITCHD3` |
| 74 | Mod envelope decay | Snare | `PITCHD4` |
| 75 | Envelope mod amount | Drum 1 | `MODAMNT1` |
| 76 | Envelope mod amount | Drum 2 | `MODAMNT2` |
| 77 | Envelope mod amount | Drum 3 | `MODAMNT3` |
| 78 | Envelope mod amount | Snare | `MODAMNT4` |
| 79 | Mod envelope slope | Drum 1 | `PITCH_SLOPE1` |
| 80 | Mod envelope slope | Drum 2 | `PITCH_SLOPE2` |
| 81 | Mod envelope slope | Drum 3 | `PITCH_SLOPE3` |
| 82 | Mod envelope slope | Snare | `PITCH_SLOPE4` |
| 83 | FM amount | Drum 1 | `FMAMNT1` |
| 84 | FM frequency | Drum 1 | `FMDTN1` |
| 85 | FM amount | Drum 2 | `FMAMNT2` |
| 86 | FM frequency | Drum 2 | `FMDTN2` |
| 87 | FM amount | Drum 3 | `FMAMNT3` |
| 88 | FM frequency | Drum 3 | `FMDTN3` |
| 89 | Voice volume | Drum 1 | `VOL1` |
| 90 | Voice volume | Drum 2 | `VOL2` |
| 91 | Voice volume | Drum 3 | `VOL3` |
| 92 | Voice volume | Snare | `VOL4` |
| 93 | Voice volume | Cymbal | `VOL5` |
| 94 | Voice volume | Hihat | `VOL6` |
| 95 | Voice pan | Drum 1 | `PAN1` |
| 96 | Voice pan | Drum 2 | `PAN2` |
| 97 | Voice pan | Drum 3 | `PAN3` |
| 98 | NRPN fine/LSB selector | Global NRPN state | Set active NRPN low 7 bits |
| 99 | NRPN coarse/MSB selector | Global NRPN state | Set active NRPN high 7 bits |
| 100 | Voice pan | Snare | `PAN4`; value 127 directly after an NRPN data entry is consumed as RPN-null |
| 101 | Voice pan | Cymbal | `PAN5`; value 127 directly after an NRPN data entry is consumed as RPN-null |
| 102 | Voice pan | Hihat | `PAN6` |
| 103 | Distortion | Drum 1 | `OSC1_DIST` |
| 104 | Distortion | Drum 2 | `OSC2_DIST` |
| 105 | Distortion | Drum 3 | `OSC3_DIST` |
| 106 | Distortion | Snare | `SNARE_DISTORTION` |
| 107 | Distortion | Cymbal | `CYMBAL_DISTORTION` |
| 108 | Distortion | Hihat | `HAT_DISTORTION` |
| 109 | Decimation | Drum 1 | `VOICE_DECIMATION1` |
| 110 | Decimation | Drum 2 | `VOICE_DECIMATION2` |
| 111 | Decimation | Drum 3 | `VOICE_DECIMATION3` |
| 112 | Decimation | Snare | `VOICE_DECIMATION4` |
| 113 | Decimation | Cymbal | `VOICE_DECIMATION5` |
| 114 | Decimation | Hihat | `VOICE_DECIMATION6` |
| 115 | Decimation | All voices | `VOICE_DECIMATION_ALL` |
| 116 | LFO frequency | Drum 1 | `FREQ_LFO1` |
| 117 | LFO frequency | Drum 2 | `FREQ_LFO2` |
| 118 | LFO frequency | Drum 3 | `FREQ_LFO3` |
| 119 | LFO frequency | Snare | `FREQ_LFO4` |
| 120 | LFO frequency | Cymbal | `FREQ_LFO5` |
| 121 | LFO frequency | Hihat | `FREQ_LFO6` |
| 122 | LFO amount | Drum 1 | `AMOUNT_LFO1` |
| 123 | LFO amount | Drum 2 | `AMOUNT_LFO2` |
| 124 | LFO amount | Drum 3 | `AMOUNT_LFO3` |
| 125 | LFO amount | Snare | `AMOUNT_LFO4` |
| 126 | LFO amount | Cymbal | `AMOUNT_LFO5` |
| 127 | LFO amount | Hihat | `AMOUNT_LFO6` |

## Global MIDI NRPN Table

NRPN sequence:

1. Send CC99 with the coarse/MSB selector.
2. Send CC98 with the fine/LSB selector.
3. Send CC6 with the value to apply.

Valid Global NRPN numbers are 0..95, plus 96 in ¢ RX-filter modes. CC6 before a selector is ignored. Out-of-range selected NRPN numbers are ignored. CC100/CC101 = 127 directly after a data entry deselects the NRPN (RPN null) and is not applied as pan. In ¢ modes CC38 (data-entry LSB) is consumed.

| NRPN | Function | Voice/scope | Internal target |
|---:|---|---|---|
| 0 | Filter drive | Drum 1 | `128+CC2_FILTER_DRIVE_1` |
| 1 | Filter drive | Drum 2 | `128+CC2_FILTER_DRIVE_2` |
| 2 | Filter drive | Drum 3 | `128+CC2_FILTER_DRIVE_3` |
| 3 | Filter drive | Snare | `128+CC2_FILTER_DRIVE_4` |
| 4 | Filter drive | Cymbal | `128+CC2_FILTER_DRIVE_5` |
| 5 | Filter drive | Hihat | `128+CC2_FILTER_DRIVE_6` |
| 6 | Mix/Mod select | Drum 1 | `128+CC2_MIX_MOD_1` |
| 7 | Mix/Mod select | Drum 2 | `128+CC2_MIX_MOD_2` |
| 8 | Mix/Mod select | Drum 3 | `128+CC2_MIX_MOD_3` |
| 9 | Velocity volume mod on/off | Drum 1 | `128+CC2_VOLUME_MOD_ON_OFF1` |
| 10 | Velocity volume mod on/off | Drum 2 | `128+CC2_VOLUME_MOD_ON_OFF2` |
| 11 | Velocity volume mod on/off | Drum 3 | `128+CC2_VOLUME_MOD_ON_OFF3` |
| 12 | Velocity volume mod on/off | Snare | `128+CC2_VOLUME_MOD_ON_OFF4` |
| 13 | Velocity volume mod on/off | Cymbal | `128+CC2_VOLUME_MOD_ON_OFF5` |
| 14 | Velocity volume mod on/off | Hihat | `128+CC2_VOLUME_MOD_ON_OFF6` |
| 15 | Velocity mod amount | Drum 1 | `128+CC2_VELO_MOD_AMT_1` |
| 16 | Velocity mod amount | Drum 2 | `128+CC2_VELO_MOD_AMT_2` |
| 17 | Velocity mod amount | Drum 3 | `128+CC2_VELO_MOD_AMT_3` |
| 18 | Velocity mod amount | Snare | `128+CC2_VELO_MOD_AMT_4` |
| 19 | Velocity mod amount | Cymbal | `128+CC2_VELO_MOD_AMT_5` |
| 20 | Velocity mod amount | Hihat | `128+CC2_VELO_MOD_AMT_6` |
| 21 | Velocity mod destination | Drum 1 | `128+CC2_VEL_DEST_1` |
| 22 | Velocity mod destination | Drum 2 | `128+CC2_VEL_DEST_2` |
| 23 | Velocity mod destination | Drum 3 | `128+CC2_VEL_DEST_3` |
| 24 | Velocity mod destination | Snare | `128+CC2_VEL_DEST_4` |
| 25 | Velocity mod destination | Cymbal | `128+CC2_VEL_DEST_5` |
| 26 | Velocity mod destination | Hihat | `128+CC2_VEL_DEST_6` |
| 27 | LFO waveform | Drum 1 | `128+CC2_WAVE_LFO1` |
| 28 | LFO waveform | Drum 2 | `128+CC2_WAVE_LFO2` |
| 29 | LFO waveform | Drum 3 | `128+CC2_WAVE_LFO3` |
| 30 | LFO waveform | Snare | `128+CC2_WAVE_LFO4` |
| 31 | LFO waveform | Cymbal | `128+CC2_WAVE_LFO5` |
| 32 | LFO waveform | Hihat | `128+CC2_WAVE_LFO6` |
| 33 | LFO target voice | Drum 1 | `128+CC2_VOICE_LFO1` |
| 34 | LFO target voice | Drum 2 | `128+CC2_VOICE_LFO2` |
| 35 | LFO target voice | Drum 3 | `128+CC2_VOICE_LFO3` |
| 36 | LFO target voice | Snare | `128+CC2_VOICE_LFO4` |
| 37 | LFO target voice | Cymbal | `128+CC2_VOICE_LFO5` |
| 38 | LFO target voice | Hihat | `128+CC2_VOICE_LFO6` |
| 39 | LFO target | Drum 1 | `128+CC2_TARGET_LFO1` |
| 40 | LFO target | Drum 2 | `128+CC2_TARGET_LFO2` |
| 41 | LFO target | Drum 3 | `128+CC2_TARGET_LFO3` |
| 42 | LFO target | Snare | `128+CC2_TARGET_LFO4` |
| 43 | LFO target | Cymbal | `128+CC2_TARGET_LFO5` |
| 44 | LFO target | Hihat | `128+CC2_TARGET_LFO6` |
| 45 | LFO retrigger | Drum 1 | `128+CC2_RETRIGGER_LFO1` |
| 46 | LFO retrigger | Drum 2 | `128+CC2_RETRIGGER_LFO2` |
| 47 | LFO retrigger | Drum 3 | `128+CC2_RETRIGGER_LFO3` |
| 48 | LFO retrigger | Snare | `128+CC2_RETRIGGER_LFO4` |
| 49 | LFO retrigger | Cymbal | `128+CC2_RETRIGGER_LFO5` |
| 50 | LFO retrigger | Hihat | `128+CC2_RETRIGGER_LFO6` |
| 51 | LFO sync | Drum 1 | `128+CC2_SYNC_LFO1` |
| 52 | LFO sync | Drum 2 | `128+CC2_SYNC_LFO2` |
| 53 | LFO sync | Drum 3 | `128+CC2_SYNC_LFO3` |
| 54 | LFO sync | Snare | `128+CC2_SYNC_LFO4` |
| 55 | LFO sync | Cymbal | `128+CC2_SYNC_LFO5` |
| 56 | LFO sync | Hihat | `128+CC2_SYNC_LFO6` |
| 57 | LFO offset | Drum 1 | `128+CC2_OFFSET_LFO1` |
| 58 | LFO offset | Drum 2 | `128+CC2_OFFSET_LFO2` |
| 59 | LFO offset | Drum 3 | `128+CC2_OFFSET_LFO3` |
| 60 | LFO offset | Snare | `128+CC2_OFFSET_LFO4` |
| 61 | LFO offset | Cymbal | `128+CC2_OFFSET_LFO5` |
| 62 | LFO offset | Hihat | `128+CC2_OFFSET_LFO6` |
| 63 | Filter type | Drum 1 | `128+CC2_FILTER_TYPE_1` |
| 64 | Filter type | Drum 2 | `128+CC2_FILTER_TYPE_2` |
| 65 | Filter type | Drum 3 | `128+CC2_FILTER_TYPE_3` |
| 66 | Filter type | Snare | `128+CC2_FILTER_TYPE_4` |
| 67 | Filter type | Cymbal | `128+CC2_FILTER_TYPE_5` |
| 68 | Filter type | Hihat | `128+CC2_FILTER_TYPE_6` |
| 69 | Transient generator volume | Drum 1 | `128+CC2_TRANS1_VOL` |
| 70 | Transient generator volume | Drum 2 | `128+CC2_TRANS2_VOL` |
| 71 | Transient generator volume | Drum 3 | `128+CC2_TRANS3_VOL` |
| 72 | Transient generator volume | Snare | `128+CC2_TRANS4_VOL` |
| 73 | Transient generator volume | Cymbal | `128+CC2_TRANS5_VOL` |
| 74 | Transient generator volume | Hihat | `128+CC2_TRANS6_VOL` |
| 75 | Transient generator waveform | Drum 1 | `128+CC2_TRANS1_WAVE` |
| 76 | Transient generator waveform | Drum 2 | `128+CC2_TRANS2_WAVE` |
| 77 | Transient generator waveform | Drum 3 | `128+CC2_TRANS3_WAVE` |
| 78 | Transient generator waveform | Snare | `128+CC2_TRANS4_WAVE` |
| 79 | Transient generator waveform | Cymbal | `128+CC2_TRANS5_WAVE` |
| 80 | Transient generator waveform | Hihat | `128+CC2_TRANS6_WAVE` |
| 81 | Transient generator frequency | Drum 1 | `128+CC2_TRANS1_FREQ` |
| 82 | Transient generator frequency | Drum 2 | `128+CC2_TRANS2_FREQ` |
| 83 | Transient generator frequency | Drum 3 | `128+CC2_TRANS3_FREQ` |
| 84 | Transient generator frequency | Snare | `128+CC2_TRANS4_FREQ` |
| 85 | Transient generator frequency | Cymbal | `128+CC2_TRANS5_FREQ` |
| 86 | Transient generator frequency | Hihat | `128+CC2_TRANS6_FREQ` |
| 87 | Audio output routing | Drum 1 | `128+CC2_AUDIO_OUT1` |
| 88 | Audio output routing | Drum 2 | `128+CC2_AUDIO_OUT2` |
| 89 | Audio output routing | Drum 3 | `128+CC2_AUDIO_OUT3` |
| 90 | Audio output routing | Snare | `128+CC2_AUDIO_OUT4` |
| 91 | Audio output routing | Cymbal | `128+CC2_AUDIO_OUT5` |
| 92 | Audio output routing | Hihat | `128+CC2_AUDIO_OUT6` |
| 93 | Roll rate | Global performance | `seq_setRollRate(value clamped 0..15)`; reported to AVR PERF page |
| 94 | Roll note | Global performance | `seq_setRollNote(value)`; reported to AVR PERF page |
| 95 | Roll velocity | Global performance | `seq_setRollVelocity(value)`; reported to AVR PERF page |
| 96 | Filter frequency (¢ RX-filter modes only) | Drum 1 | `PAR_FILTER_FREQ_1` (replaces Global CC38) |

## RX Filter ¢ Modes

Session 039 adds 8 "¢" (cent sign) RX-filter twins to the existing 16 entries. They are encoder-only on the AVR (pots cannot reach them; TX filter is unchanged). Each ¢ entry behaves like the C-entry it shadows plus two differences: Global CC38 is consumed (Elektron data-entry LSB), and NRPN 96 controls Drum 1 filter cutoff (`PAR_FILTER_FREQ_1`).

| Menu value | Name | RX bits | CC38 swap | NRPN 96 |
|---:|---|---|---|---|
| 16 | `¢` | `0x04` | yes | yes |
| 17 | `¢N` | `0x05` | yes | yes |
| 18 | `¢R` | `0x06` | yes | yes |
| 19 | `¢RN` | `0x07` | yes | yes |
| 20 | `P¢` | `0x0C` | yes | yes |
| 21 | `P¢N` | `0x0D` | yes | yes |
| 22 | `P¢R` | `0x0E` | yes | yes |
| 23 | `al¢` | `0x0F` | yes | yes |

Wire encoding (AVR → STM on `SEQ_MIDI_FILT_RX`): values 0..15 are sent unchanged. Values 16..23 encode as `0x04 | (k & 3) | ((k & 4) << 1) | 0x10` where `k = value - 16`. Bit 4 (`0x10`, `MIDI_RX_FILTER_CC38_SWAP`) is stripped by `midi_setFilter()` before the RX nibble is stored and forwarded to `globalMidiParser_setCc38Swap()`.

AVR rules:
- `midiFilterNames` rows are `[4]`; the count byte stays `{16}`, so pots and TxFilter see 16 entries.
- `menu_getDtypeMenuEntryCount()` returns 24 for `PAR_MIDI_FILT_RX` (encoder paths only).
- `menu_parseKnobValue()` ignores the RxFilter pot while the value is ≥ 16.
- `menu_parseGlobalParam()` is the single encode point; values ≥ 24 are repaired to 15 (`all`).
- Globals save byte-for-byte; no file-format change.

Glyph: `MENU_CHAR_CENT` is `"\\"` (LCD ROM 0x5C, cent sign on this display per the user; a standard HD44780 A00 ROM has `¥` there, so this is unverified). Fallback: a CGRAM glyph via `lcd_generatechar()` in slot 1 (`"\x01"`; slot 0 cannot be used in strings).

NRPN protocol details (`globalMidiParser_handleNrpnControl()`):
- CC98/99 select and close the RPN-null window. CC6 opens it only when an NRPN is selected; CC6 with no selection is consumed and ignored.
- In ¢ mode CC38 is consumed and leaves the window open (Elektron sends CC38 between CC6 and any null).
- CC100/101 = 127 inside the window clears the selection and is consumed (both null bytes). Any other CC closes the window.
- Accepted cost: a deliberate Pan 4/5 = 127 sent immediately after an NRPN data entry is dropped.

---

## MIDI Transmit (TX filter C, Session 039 P3)

When `txf` includes **C** (bit 6, `0x40`) and the **Global MIDI channel** is set (1–16, not 0), every parameter change made with the menu encoder or knobs is transmitted on the Global channel as the CC or NRPN that controls that parameter on receive.

### Gate

TX is suppressed when any of these is true:

- TX filter C is off.
- Global MIDI channel is 0 (off) or > 16.
- An automation step is armed (`seq_isAutomationStepArmed()`).
- The parameter has no Global MIDI mapping (returns `GLOBAL_MIDI_TX_NONE`).

### Address derivation

The transmit address is the **inverse of the receive tables** (`globalMidiParser_ccToRawParam[]` and `globalMidiParser_nrpnToRawParam[]`). CCs are preferred over NRPNs when both tables map a parameter (3 bytes instead of 9). The reverse table is built lazily on first use.

### Exclusions (return GLOBAL_MIDI_TX_NONE)

- **18 target selectors:** `PAR_VEL_DEST_1..6`, `PAR_VOICE_LFO1..6`, `PAR_TARGET_LFO1..6` — their receive value domain is unverified against the menu's `modTargets` index.
- Parameters with no Global mapping: envelope position 1–6, per-voice morph, kit version, reserved slots.
- CC0/1/6/98/99 never come out of the reverse table because those receive-table slots are `PAR_NONE`.

### ¢ mirror

When the RX filter is a ¢ entry (CC38 swap active), Drum 1 filter cutoff (`PAR_FILTER_FREQ_1`) is sent as **NRPN 96** instead of CC38, matching what a ¢-mode receiver accepts.

### PERF controls

| Control | Address | Value |
|---|---|---|
| Roll rate | NRPN 93 | 0..15 (selector) |
| Roll note | NRPN 94 | 0..127 |
| Roll velocity | NRPN 95 | 0..127 |
| Global morph | CC1 | amount >> 1 (inverse of receive: v*2, 127 → 255) |

### NRPN framing

CC99 = 0, CC98 = number, CC6 = value. No CC38 (data-entry LSB), no RPN-null tail.

### CC100/101 = 127 guard

CC100 (Snare pan) and CC101 (Cymbal pan) with value 127 would be taken by an LXR receiver as an RPN-null deselect if it followed an NRPN data entry (P1 rule). Before emitting CC100/101 = 127, TX first emits CC99 = 0, which closes the receiver's null window.

### Coalescing and pacing

A coalescing queue holds one slot per raw sound parameter plus 4 PERF slots. Repeated edits keep only the latest value. `outputControl_serviceMenuTx()` emits at most one message per main-loop pass, round-robin, only when both DIN and USB outputs can take the whole message. This prevents knob sweeps from saturating DIN (31,250 baud) or splitting messages.

### Whole-message DIN/USB

DIN: `uart_sendMidi()` drops the entire message when `fifo_freeSpace()` is insufficient, never splits. This applies to every DIN sender (sequencer notes/clock, routing, menu TX). USB: `usb_sendMidi()` drops the 4-byte event when `usb_midiTxFree() < 4`, never writes past the current ping-pong half. `usb_flushMidi()` starts the next IN transfer only after `usbd_midi_DataIn()` clears `usb_midiTxBusy`, or after `USB_MIDI_TX_TIMEOUT_TICKS` (20 SysTicks = 5 ms). It discards pending bytes while the device is not `USB_OTG_CONFIGURED`. Drop/timeout counters: `uart_midiTxDropCount`, `usb_midiTxDropCount`, `usb_midiTxTimeoutCount` (debugger-visible).

### Not transmitted

Received MIDI (DIN or USB) is never re-transmitted (except by `mrt` routing). Only menu-originated changes trigger TX: sound-parameter edits tagged with AVR opcodes `0xca`/`0xcb`, and the menu-only PERF opcodes `FRONT_SEQ_ROLL_RATE/_NOTE/_VELOCITY` and `FRONT_SEQ_SET_GLOBAL_MORPH_MSB`. Values only stored while temporary playback is active (not applied live) are not transmitted. File loads, voice loads, automation playback, the armed-step revert, LFOs, and preset restores all use the plain opcodes (`MIDI_CC`/`CC_2`) and are never transmitted.
