# S039 P3 — MIDI Transmit of Menu Parameter Changes (TX "C")

Branch `dev-roll-midi`, on top of S039 P1 and P2 (both reviewed clean). Status: **plan with all user decisions settled (2026-10-16). Line-cited schedule: `S039_P3_IMPLEMENTATION.md`.**

## 1. Goal

When `txf` includes **C** and the **Global MIDI channel is not 0**, every parameter change made **in the menu** is sent on the Global channel as the CC or NRPN that controls that parameter on receive. "In the menu" means an encoder edit or a knob turn. The purpose is to record knob motions into an external DAW and play them back into the LXR.

Nothing else transmits CCs:
- not incoming MIDI,
- not file/kit/morph loads or restores,
- not sequencer automation, LFOs or morph interpolation,
- not armed-automation-step edits.

## 2. Settled decisions

| # | Decision |
|---|---|
| 1 | **Global channel only**, using the Global CC/NRPN tables. If the Global channel is `0`, nothing is transmitted. |
| 2 | **No TX for armed-automation-step edits.** |
| 3 | **TX for edits made during live record.** |
| 4 | **Target selectors excluded** (`PAR_VEL_DEST_1..6` NRPN 21-26, `PAR_VOICE_LFO1..6` NRPN 33-38, `PAR_TARGET_LFO1..6` NRPN 39-44). |
| 5 | **¢ mirror:** while the RX filter is a ¢ entry, Drum 1 filter cutoff is sent as **NRPN 96**; otherwise as **CC38**. This way a DAW recording plays back correctly under the same settings. |
| 6 | **PERF controls included:** roll rate/note/velocity as **NRPN 93/94/95**, and global morph as **CC1**. |
| 7 | **NRPN framing:** CC99 = 0, CC98 = number, CC6 = value, every time. Never CC38, never an RPN-null tail. |
| 8 | **Coalescing queue:** the latest value per parameter wins, with paced, whole-message sends. |
| 9 | **Fix USB MIDI out overflow** as part of P3. |
| 10 | No release note needed. |

## 3. Current state (audited)

| Fact | Where |
|---|---|
| The STM owns MIDI out. `outputControl_sendMidi()` writes the same message to USB and DIN. TX letters N/R/P are gated at their senders. TX C (`0x40`) has no reader. | `MIDI/MidiOutputControl.c:59`, `Sequencer/sequencer.c:2533-2586` |
| The AVR sends menu sound-parameter edits as `MIDI_CC` (`0xb0`) / `CC_2` (`0xb6`). | `menu.c:3108/3115` (encoder), `:3336/3343` (shift-encoder, non-morph branch), `:4397/4399` (knob, `DTYPE_AUTOM_TARGET` non-PERF), `:4410/4412` (knob default) |
| The **same opcodes come from non-menu code**: the armed-step revert (`buttonHandler.c:284/292`), the per-voice preset load (`presetManager.c:2226/2230`) and the audio-out send during load (`presetManager.c:694`). The opcode alone cannot identify a menu edit. | as cited |
| PERF global morph is sent only from `menu_parseGlobalParam()` `PAR_MORPH`, as `SEQ_SET_GLOBAL_MORPH_LSB`/`_MSB` (`menu.c:3857-3858`). Roll rate/note/velocity are sent only from `menu_parseGlobalParam()` as `SEQ_ROLL_RATE`/`_NOTE`/`_VELOCITY` (`:3896-3906`). `menu_parseGlobalParam()` is called from the encoder (`:3119`, `:3348`), the knob (`:4401`, `:4414`) and the armed-step revert (`buttonHandler.c:297`). `menu_sendAllGlobals()` starts at `PAR_BEGINNING_OF_GLOBALS`, so it never sends morph or roll. **These opcodes are therefore menu-only.** | as cited |
| STM handlers: `MIDI_CC` / `FRONT_CC_2` (`frontPanelReceivingProtocol.c:1994-2035`), `FRONT_SEQ_SET_GLOBAL_MORPH_MSB` (`:2424-2431`, which assembles the 8-bit amount), and `FRONT_SEQ_ROLL_RATE/_NOTE/_VELOCITY` (`:2741-2749`). | as cited |
| The receive side of global morph CC1 converts with `preset_morphAutomationValueToAmount()`: `v*2`, and `127 → 255` (`MorphEngine.c:289-295`). | |
| The STM tracks armed-step state as `seq_armedArmedAutomationStep` (static, `sequencer.c:141`; set and cleared in `seq_armAutomationStep()` `:1210-1220`). The AVR sends DISARM **before** the revert (`buttonHandler.c:277` → `:284`). | |
| Front-panel framing is a fixed 3 bytes per non-SysEx status. **`0xca` and `0xcb` are unused** on both MCUs. | `frontPanelReceivingProtocol.c:1503-1521` |
| The Global receive tables cover 216 sound parameters (`ccToRawParam[128]`, `nrpnToRawParam[93]`). Not mapped: envelope position 1-6 and per-voice morph. | `GlobalMidiParser.c` |
| **DIN out:** a 256-byte `fifo_midiTx` with byte-wise silent drop, so a message can be split. | `Uart.c:215-235`, `FIFO.c:47` |
| **USB out (defect):** `usb_MidiInBuff` is 2×64-byte ping-pong halves. `usb_sendByte()` has **no bounds check**: more than 64 bytes between flushes run into the half that is being transmitted, and more than 128 wrap around. The `uint8_t len` pointer difference in `usb_flushMidi()` is then wrong. `usb_flushMidi()` is called every main-loop pass and calls `DCD_EP_Flush()` + `DCD_EP_Tx()` **without checking whether the previous transfer completed**, so it can abort IN data still in the endpoint FIFO. `usbd_midi_DataIn()` (the transfer-complete callback, compiled and registered) is an empty stub. | `usb_manager.c:97-126`, `usb_midi_core.c:127-162, 396-401` |

## 4. Design

### 4.1 Tag menu sound-parameter edits at the source

New AVR→STM statuses, payload identical to the existing pair:

| AVR / STM name | Value | Payload |
|---|---|---|
| `MIDI_CC_MENU` / `FRONT_CC_MENU` | `0xca` | data1 = raw id 0-127, data2 = value |
| `CC_2_MENU` / `FRONT_CC_2_MENU` | `0xcb` | data1 = raw id − 128, data2 = value |

- **AVR:** add a helper, `menu_sendSoundParameter(paramNr, value)`, and use it at the four menu send-site pairs. Load, revert and restore keep the plain opcodes. Internal UART traffic is unchanged. Flash cost is roughly 50-80 bytes (798 bytes free).
- **STM:** factor the `MIDI_CC`/`FRONT_CC_2` bodies into `frontParser_handleSoundParameterIngress(rawParam, value)`, which returns 1 if the value was applied live. The plain cases behave exactly as today. The `_MENU` cases call the same helper and, if it was applied live, queue TX.
- Global morph and roll need **no new opcodes**: their front opcodes are already menu-only (§3), so the STM queues TX directly in those cases.

### 4.2 Single TX gate (STM)

`midiTx_isMenuTxAllowed()` returns true only when all of these hold:
1. `midiParser_txRxFilter & 0x40` (TX C);
2. `midi_MidiChannels[7] != 0` (Global channel set);
3. no automation step is armed, via a new accessor `seq_isAutomationStepArmed()` (decision 2).

The armed-step check sits on the STM because the STM sees ARM/DISARM in order with the edits, and the morph/roll paths have no AVR helper.

Side effect: a revert of a PERF morph/roll value after DISARM transmits the restored value. That is harmless, because it equals what the DAW already holds.

### 4.3 Mapping (one source of truth with receive)

`globalMidiParser_getTxAddress(rawParam, &kind, &number)` in `GlobalMidiParser.c`:
- It uses a reverse table `uint8_t[END_OF_SOUND_PARAMETERS]`, built once and lazily from `ccToRawParam[]` and `nrpnToRawParam[]`. The CC wins if both map a parameter.
- **Excluded:** protocol CCs 0, 1, 6, 38, 98-101 and the 18 target selectors (decision 4).
- **¢ mirror (decision 5):** for `PAR_FILTER_FREQ_1`, return NRPN 96 when `globalMidiParser_cc38Swap`, else CC38.
- Fixed TX-only addresses outside the tables:
  - roll rate/note/velocity → NRPN 93/94/95;
  - global morph → CC1, with value `amount >= 254 ? 127 : amount >> 1`, the inverse of the receive `v*2` / `127 → 255` (odd amounts round down by 1 on playback).

### 4.4 Wire format

- **CC:** `0xB0 | (globalCh-1), cc, value`.
- **NRPN:** `CC99 0`, `CC98 nrpn`, `CC6 value`. No CC38 and no null (decision 7).
- **Values:**
  - sound parameters: data2 as received from the AVR, which is the 7-bit raw domain the receive path stores, so it round-trips exactly;
  - roll rate: the 0-15 selector index, the same as receive NRPN 93.

### 4.5 Coalescing queue and pacing (decision 8)

Lives in `MIDI/MidiOutputControl.c`:
- **Slots:** `END_OF_SOUND_PARAMETERS` sound slots plus 4 extra (roll rate, roll note, roll velocity, global morph). Storage is `uint8_t value[]` plus a dirty bitmap, about 280 bytes of STM RAM.
- **`outputControl_queueMenuTx(slot, value)`:** if the gate (§4.2) is open, store the value and set the dirty bit. The latest value wins.
- **`outputControl_serviceMenuTx()`:** called from the `main.c` main loop next to `usb_tick()`. Each pass it emits **at most one** dirty slot, round-robin, and only if **both** outputs can take the **whole** message: DIN free ≥ 3 or 9 bytes, and USB free ≥ 4 or 12 bytes. Otherwise it tries again next pass.
- **Gate re-checked at emit:** a slot left dirty after TX C is turned off, or after the channel is set to 0, is dropped.
- Sequencer notes and clock keep their direct path.

### 4.6 USB MIDI out fix (decision 9)

In `Hardware/USB/usb_manager.c`, `usb_midi_core.c` and `usb_manager.h`:
- **`usb_sendMidi()`:** add a **whole-packet bounds check**. If `(WrPtr − RdPtr) + 4 > TOTAL_MIDI_BUF_SIZE`, drop the packet and count it (`usb_midiTxDropCount`) instead of overrunning into the in-flight half or wrapping. New `usb_midiTxFree()` returns the bytes left in the current half, for the §4.5 pacing.
- **Transfer-in-flight flag:** `usb_midiTxBusy` is set when `usb_flushMidi()` starts `DCD_EP_Tx()` and cleared in `usbd_midi_DataIn()`.
  - `usb_flushMidi()` skips while busy, so it no longer `DCD_EP_Flush()`es in-flight data.
  - **Timeout:** if busy for longer than `USB_MIDI_TX_TIMEOUT` (e.g. 5 ms of `systick_ticks`, about 20 SysTicks at 250 µs), clear it and continue. This covers a host that stops polling.
  - Keep the existing suspended-state check.
- **Benefit beyond P3:** this also protects existing USB traffic (routing, notes, clock).

### 4.7 Loops

- The receive path never transmits. A DAW playing back into the LXR is applied once and not re-sent, so a recording is not doubled.
- `mrt` routing is unchanged and independent.

## 5. Scope

| Source | Transmit? |
|---|---|
| Encoder or knob edit of a mapped sound parameter on any page, gate open | **yes** (CC or NRPN) |
| Same, during live record | **yes** |
| Same, while an automation step is armed | **no** (STM gate) |
| Shift-edit of the morph endpoint (`menu_sendMorphParameterEndpoint()`) | no |
| Edit stored but not applied live (temp playback active) | no |
| PERF roll rate / note / velocity | **yes**: NRPN 93/94/95 |
| PERF global morph | **yes**: CC1 |
| PERF per-voice morph, envelope position | no (no Global mapping) |
| Target selectors | no (decision 4) |
| Loads, restores, background swap, automation, LFO, MIDI in, armed-step revert of sound parameters | **never** |

## 6. Changes (summary)

| Area | File | Change |
|---|---|---|
| AVR | `avrComms/avrCommsSendingProtocol.h` (or `ReceivingProtocol.h`, wherever `MIDI_CC`/`CC_2` are defined for sending) | `MIDI_CC_MENU 0xca`, `CC_2_MENU 0xcb` |
| AVR | `Menu/menu.c` | `menu_sendSoundParameter()`; replace the 4 send-site pairs |
| STM | `uARTFrontSYX/frontPanelReceivingProtocol.h/.c` | `FRONT_CC_MENU`, `FRONT_CC_2_MENU`; ingress helper; `_MENU` cases; TX queue calls in `FRONT_SEQ_SET_GLOBAL_MORPH_MSB` and `FRONT_SEQ_ROLL_*` |
| STM | `MIDI/GlobalMidiParser.c/.h` | `globalMidiParser_getTxAddress()` with reverse table, exclusions and ¢ mirror |
| STM | `MIDI/MidiOutputControl.c/.h` | Gate, coalescing queue, service function, CC/NRPN emitters |
| STM | `Sequencer/sequencer.c/.h` | `seq_isAutomationStepArmed()` accessor |
| STM | `uARTFrontSYX/Uart.c/.h`, `Hardware/FIFO.c/.h` | `fifo_freeSpace()`, `uart_midiTxFree()` |
| STM | `Hardware/USB/usb_manager.c/.h`, `usb_midi_core.c` | Bounds check, `usb_midiTxFree()`, busy flag + DataIn clear + timeout, drop counter |
| STM | `main.c` | Call `outputControl_serviceMenuTx()` in the main loop |
| Docs | `MIDI_TABLE.md`, `COMMS_FLOW_SPEC.md`, `MEMORY.md` | TX semantics, opcodes `0xca/0xcb`, exclusions, USB fix |
| Manual | `05_operation_modes.md` (`txf` row and filter text), `07_midi.md` (transmit section), `latex/midi_tables.tex` (note) | "With C on and a Global channel set, menu changes are sent as the listed CC/NRPN on the Global channel"; exclusions; ¢ mirror; DAW record/playback use |

## 7. Interactions

- **P1 ¢:** mirrored (decision 5).
- **P2 RX C:** for DAW playback, the LXR's `rxf` must include C (or ¢) or the recorded CCs are ignored. Document this.
- **Boot:** the STM `txRxFilter` default is `0xFF` until globals arrive, but no menu edit can happen before then.
- **Global morph via CC1 on receive** goes through the channel parser path (Global CC0/CC1 exception). Playback works with RX C on.

## 8. Risks

| Risk | Mitigation |
|---|---|
| Output saturation, split messages | Coalescing plus whole-message pacing on both outputs |
| USB IN stall if `DataIn` never fires | Busy timeout |
| Changing USB flush timing affects existing USB MIDI | Hardware test of USB notes/clock/routing under load (§9) |
| TX/RX mapping divergence | Reverse table derived from the receive tables |
| AVR flash | About 50-80 bytes against 798 free; build gate ≤ 57,344 |
| Opcode collision | `0xca`/`0xcb` verified unused |

## 9. Verification (outline)

- **Build:** STM and AVR builds, AVR ≤ 57,344 bytes, both binaries embedded in `FIRMWARE.BIN`.
- **Host harness:**
  - The reverse table equals the inverse of the receive tables, with exclusions and the ¢ toggle.
  - Morph TX/RX round trip is within 1 step for every amount 0-255.
  - Coalescer: the latest value wins, there are no partial messages, and it respects DIN/USB free space.
  - USB ring model: nothing is ever written past the current half.
- **Hardware** (MIDI monitor on DIN and USB, then DAW record and playback):
  1. A voice-page knob or encoder sends the right CC or NRPN triplet on the Global channel.
  2. Nothing is sent when TX C is off, when the Global channel is 0, or while a step is armed.
  3. Drum 1 cutoff is sent as CC38, or as NRPN 96 in ¢ mode.
  4. PERF roll sends NRPN 93-95 and global morph sends CC1.
  5. File load, morph load, automation, LFO and MIDI-in playback send **nothing**.
  6. A fast four-knob sweep gives valid streams on both outputs that end on the final values.
  7. Record a sweep in a DAW and play it back into the LXR with RX C on: the motion is reproduced and the PERF/menu display follows.
  8. USB stress: a high-rate sequencer plus routing gives no corrupted USB MIDI, and USB recovers after a host stops reading (timeout) and after suspend/resume.

## 10. Settled follow-ups (2026-10-16)

1. **DIN whole-message rule for all outputs: yes.** `uart_sendMidi()` becomes whole-message-or-drop for every caller (notes, clock, routing, menu TX), matching the USB fix. It never queues a partial message.
2. **USB busy timeout: 5 ms**, i.e. `USB_MIDI_TX_TIMEOUT_TICKS = 20` SysTicks at 250 µs.

The line-cited schedule is `S039_P3_IMPLEMENTATION.md`.
