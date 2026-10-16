# 039 Session Handoff Log — ¢ RX-Filter CC38 Swap, Roll NRPNs, RX/TX Filter "C", Menu MIDI Transmit, Imported-Sample FM, and USB/DIN MIDI-Out Fixes

**Date**: 2026-10-16
**Working repository**: repository root, branch `dev-roll-midi`.
- Commit `cdc5242` ("s039 p3 pre-implementation: tx midi parameter transmit from menu") contains P1, P2 and all six root S039 planning/schedule documents.
- The P3 implementation, the manual edits, `README.md`, the AVR firmware version bump and the rebuilt `FIRMWARE.BIN` are **staged but uncommitted** at session end.

**Status at end**:
- STM32 + AVR + aggregate firmware build clean. STM `LxrStm32.bin` 248,020 bytes; AVR `LxrAvr.bin` 56,570 bytes (774 bytes below the 57,344-byte bootloader limit); `FIRMWARE.BIN` 305,408 bytes. Both binaries verified byte-identical inside `FIRMWARE.BIN` (AVR at offset 512, STM at 57,344).
- Verbatim host harness for the P3 transmit path passes.
- **Nothing in this session has been hardware-tested.** Session 038's MIDI roll fix is also still awaiting hardware verification.

The six root working documents of this session are being deleted by the user:
- `S039_P1_FM_BUGFIX.md`
- `S039_P1_IMPLEMENTATION.md`
- `S039_P2_CC_RECEP_MIDI_FIX.md`
- `S039_P2_IMPLEMENTATION.md`
- `S039_P3_MIDI_TX_CC.md`
- `S039_P3_IMPLEMENTATION.md`

Their durable content is preserved in this log and in `knowledge_files/comms_spec_reference/MIDI_TABLE.md` / `COMMS_FLOW_SPEC.md`. The full in-source documentation is the `WHAT / WHY / INPUT / OUTPUT / ACCESSORS / AFFILIATES` comment blocks that were pasted next to every code change. The code itself is the authoritative reference for exact text.

---

## Session Goal

The session opened with three requests made after the MIDI roll-trigger work:

1. A user reported that **NRPN 93** "does something with attenuation on one track". Investigate what, why and how, and whether it could be extended to all tracks on NRPN 93-98.
2. Put the PERF-page **roll rate, roll note and roll level** on NRPNs. The first ask was 99/100/101; it was later revised to "the NRPNs after 093".
3. Bug: **no FM is performed when the main oscillator is set to an imported sample**, although FM works with a base waveform like SIN.

Through user decisions the scope evolved into three planned and implemented phases:

| Phase | Content |
|---|---|
| **P1** | ¢ RX-filter twins that swap Global CC38 to NRPN 96 (the real cause of the "NRPN 93 attenuation"); RPN-null swallow; NRPN 94/95 roll note/velocity with PERF display sync; imported-sample FM; Hihat modulator sample re-arm |
| **P2** | Make the RX filter's **C** (control change) letter actually gate incoming CCs |
| **P3** | Make the TX filter's **C** letter actually transmit: menu parameter edits sent as Global CC/NRPN, plus a USB MIDI-out overflow/abort fix and whole-message DIN MIDI out |

Manual chapters and LaTeX MIDI tables were updated along the way.

---

## Headline Outcome

- **The "NRPN 93 attenuation" was never NRPN 93.** NRPN 93 only sets roll rate. The best explanation is that NRPN senders such as Elektron devices append a **CC38 data-entry LSB** to every NRPN. Global CC38 is the published Drum 1 filter-cutoff mapping, so every NRPN closed Drum 1's filter and stored the change into the kit. The user chose an opt-in fix: new **¢ ("cent") RX-filter entries** that ignore Global CC38 and move Drum 1 cutoff to NRPN 96.
- **RX filter C was a no-op.** Incoming CCs were applied whatever the C letter said; C only gated the front-panel display echo and automation recording. Fixed by gating at the single CC entry point.
- **TX filter C had no transmitter at all.** It now transmits **menu** edits (encoder/knob), PERF roll and global morph as Global-channel CC/NRPN, designed for recording knob motion into a DAW and playing it back.
- **Imported-sample FM was a deliberate stub** left by the sample-import Phase 2 work. It is now real phase modulation with the same modulation index as the periodic waveforms.
- **USB MIDI output had a latent overflow and abort defect** affecting all USB MIDI out. Fixed, and DIN MIDI out became whole-message-or-drop.

---

## Part 0 — Investigation: "NRPN 93 Attenuates One Track"

### What NRPN 93 does

`globalMidiParser_handleNrpnControl()` (`MIDI/GlobalMidiParser.c`) handled, on the Global MIDI channel only:
- CC99 (`NRPN_COARSE`): NRPN MSB.
- CC98 (`NRPN_FINE`): NRPN LSB. Both set `nrpnSelected`.
- CC6 (`NRPN_DATA_ENTRY_COARSE`): data entry.
  - NRPN 93 → `seq_setRollRate(min(value,15))`, added by the 2026-09-17 roll-trigger feature.
  - NRPN 0..92 → `globalMidiParser_nrpnToRawParam[]`; anything else ignored.
  - The NRPN raw table has 93 entries, 0..92; 87-92 are Audio Out routing 1-6.
- **Every other CC** fell through to `globalMidiParser_ccToRawParam[]` and was applied, stored and echoed as an ordinary Global CC.

NRPN 93 can only change roll rate, which is inaudible unless a roll is held. Before 2026-09-17 it was out of range and ignored.

### Hypotheses (each implicates a different track)

| # | Mechanism | Lands on | Audible effect |
|---|---|---|---|
| **H1 (most likely)** | Sender appends **CC38** (data-entry LSB, 14-bit NRPN; **Elektron devices always send CC99, CC98, CC6, CC38**) | Global CC38 = `PAR_FILTER_FREQ_1` | **Drum 1 filter cutoff** jumps to the LSB (usually 0), so Drum 1 is muffled to near-silent. Applied **and stored** in the kit. |
| H2 | Sender closes with "RPN null" CC101 = 127, CC100 = 127 | `PAR_PAN5` / `PAR_PAN4` | Cymbal / Snare panned hard |
| H2b | Data increment/decrement CC96/97 | `PAR_PAN2` / `PAR_PAN3` | Drum 2/3 pan moves |
| H3 | Plain **CC 93** sent instead of NRPN 93 | Global CC93 = `PAR_VOL5` | Cymbal volume |
| H4 | Controller UI numbers NRPNs 1-based (shows 93, sends 92) | NRPN 92 = `PAR_AUDIO_OUT6` | Hihat routed away from the main outs |

Notes:
- The stray value is always applied and stored. It is echoed to the AVR menu only when the RX C bit is set (that was the pre-P2 behaviour of the `0x04` bit), so with C off the menu stayed stale.
- **User decision:** do not build the proposed "NRPN 93-98 = per-track volume" extension. Instead, add opt-in ¢ RX-filter entries that swap CC38 to NRPN 96. NRPN 93 stays roll rate, and **no existing CC/NRPN assignment changes**.

---

## Part 1 — User Decisions (all rounds)

| Round | Decision |
|---|---|
| P1 | RX-filter entries containing C get encoder-only twins displayed with `¢` in place of `C`. They are selectable only in encoder edit mode, shown in the 4-parameter view, and cannot be changed with a pot. Each twin swaps CC38 to NRPN 96 and is otherwise identical to its C twin. |
| P1 | `¢` sits at LCD ROM **0x5C** ("between `[` and `]`"), per the user. |
| P1 | Attenuation/NRPN 93-98 volume extension: **dropped**. |
| P1 | NRPN 93 stays roll rate. **NRPN 94 = roll note, NRPN 95 = roll velocity** ("the NRPNs after 093"). |
| P1 | `all` twin named **`al¢`**. **NRPN 96 active only in ¢ modes.** ¢ entries **follow `all`** in encoder order (not interleaved). |
| P1 | **RPN-null swallow: yes** (follow-up 3). **Hihat modulator re-arm: yes** (follow-up 4). |
| P2 | Settings must work as documented: `mrt` routes regardless of `rxf`; a filtered category may be routed but must not affect the sound. The RX C fix was approved. |
| P3 | Wire TX properly: **only menu changes** send their CC/NRPN when TX C is enabled. |
| P3 | 1. **Global channel only**; the Global channel must not be 0. 2. **No TX for armed-automation-step edits.** 3. **TX for live-record edits.** 4. **Target selectors excluded.** 5. **¢ mirror**: Drum 1 cutoff as NRPN 96 when using ¢; the intent is record-to-DAW and playback. 6. **PERF roll rate/note/velocity (NRPN 93-95) and global morph (CC1) included.** 7. **NRPN framing**: CC99 = 0, CC98, CC6 every time; no CC38, no null. 8. **Coalescing queue.** 9. **Fix the USB overflow.** 10. No release note needed. |
| P3 follow-ups | **DIN whole-message-or-drop for all output: yes.** **USB busy timeout 5 ms: yes.** |
| Docs | Manual text written this session was judged far too verbose. **Future manual entries must be terse** (recorded in `MEMORY.md` governance). The existing text was left as is. |

---

## Part 2 — P1 Implementation

### 2.1 ¢ RX-filter twins (AVR)

`PAR_MIDI_FILT_RX` ("RxFilter", global settings page 1) is `DTYPE_MENU` over `midiFilterNames` (`Menu/MenuText.h`). The value is the RX bit nibble: N `0x01`, R `0x02`, C `0x04`, P `0x08`; values 0..15 = `off N R RN C CN CR CRN P PN PR PRN PC PCN PCR all`.

| Menu value | Name | RX bits on the wire | Twin of |
|---:|---|---|---|
| 16 | `¢` | `0x04 \| 0x10` | `C` |
| 17 | `¢N` | `0x05 \| 0x10` | `CN` |
| 18 | `¢R` | `0x06 \| 0x10` | `CR` |
| 19 | `¢RN` | `0x07 \| 0x10` | `CRN` |
| 20 | `P¢` | `0x0C \| 0x10` | `PC` |
| 21 | `P¢N` | `0x0D \| 0x10` | `PCN` |
| 22 | `P¢R` | `0x0E \| 0x10` | `PCR` |
| 23 | `al¢` | `0x0F \| 0x10` | `all` |

Implementation facts:
- **`MenuText.h`:**
  - Constants `MENU_CHAR_CENT "\\"` (0x5C), `MIDI_FILTER_CENT_FIRST 16`, `MIDI_FILTER_RX_ENTRIES 24` and `MIDI_FILTER_RX_CC38_SWAP 0x10`. They sit between `MENU_MIDI_FILTERING` (13) and `MENU_PPQ` (14); this is cosmetic, but it splits the menu-id list.
  - `midiFilterNames` rows shrank **from `[16]` to `[4]`**. The 8 ¢ rows were appended after `{"all"}`, with the **count byte left at `{16}`**, so `getMaxEntriesForMenu()` (pots, TxFilter) still sees 16.
  - `getMenuItemNameForValue()` copies 3 bytes from row `value+1`, so values up to 23 display without change. "Order is ABI": value `16+k` must keep bits `0x04 | (k&3) | ((k&4)<<1)`.
- **`menu_getDtypeMenuEntryCount()`** (`menu.c`) returns 24 for `PAR_MIDI_FILT_RX`.
  - It has only encoder callers: `menu_normalizeDtypeMenuEncoderValue()` and both encoder `DTYPE_MENU` clamps. Without the override, the normaliser would rescale any value ≥ 16 and the clamp would cap it at 15.
  - TxFilter is not overridden and stays 0..15.
- **Pot lock** (`menu_parseKnobValue()`): early return when `paramNr == PAR_MIDI_FILT_RX && value >= 16`. It sits after `editModeActive = 0`, so touching a pot still exits encoder edit mode.
- **Wire encoding** (`menu_parseGlobalParam()`, `case PAR_MIDI_FILT_RX`):
  - Values 0..15 are sent unchanged.
  - 16..23 are sent as the arithmetic mapping above `| 0x10`. A PROGMEM table was deliberately avoided to save flash.
  - Values ≥ 24 (corrupt) are repaired to 15 (`all`) in `parameter_values` and sent as 15.
  - This is the single AVR→STM send point for RxFilter: encoder, pot, global-file load via `menu_sendAllGlobals()`, and the `buttonHandler.c` armed-step revert.
- **Persistence:** globals are saved byte-for-byte, so 16..23 save and reload with no format change. Older firmware would show garbage for those values.
- **¢ glyph risk:** a standard HD44780 A00 ROM has `¥` at 0x5C. Fallback: define a CGRAM glyph with `lcd_generatechar()` (`Hardware/lcd.c:201`, currently unused) in **slot 1**, and set `MENU_CHAR_CENT "\x01"`. Slot 0 cannot be used because code 0 terminates strings.

**AVR flash constraint (discovered this session):** the AVR bootloader is linked at `-Ttext=0xE000` (`front/LxrAvr_bootloader/default/Makefile:184`), so the application must be **≤ 57,344 bytes**. At session start `LxrAvr.bin` was 56,682 bytes, leaving 662 bytes free. Shrinking the name rows saved 172 bytes net (17×16 = 272 → 25×4 = 100). Sizes through the session: 56,546 after P1, unchanged by P2, 56,570 after P3.

### 2.2 STM ¢ flag, CC38 swap, NRPN 96, RPN-null, roll NRPNs

- **`MIDI/MidiParser.h`:** new `MIDI_RX_FILTER_CC38_SWAP 0x10`; must equal the AVR `MIDI_FILTER_RX_CC38_SWAP`.
- **`midi_setFilter()`** RX branch:
  - The existing roll-hold release on note-bit-off stays first.
  - It then calls `globalMidiParser_setCc38Swap((value & 0x10) != 0)`.
  - The existing `(value & 0x0F)` store strips bit 4. The TX branch is unchanged.
- **`GlobalMidiParser.c`** new state:
  - `globalMidiParser_cc38Swap`: written only by `globalMidiParser_setCc38Swap()`; default 0 until the AVR sends globals.
  - `globalMidiParser_nrpnDataPending`: the RPN-null window.
- **`globalMidiParser_handleNrpnControl()`** rewritten:
  - CC98/CC99 select and close the window.
  - CC6: `pending = nrpnSelected`; if nothing is selected it is consumed and ignored. Otherwise `globalMidiParser_applySpecialNrpn()` handles 93-96, then the raw table handles 0..92.
  - CC38 (`DATA_MSB_LSB`) is consumed when the swap is on, and the window is left open because an Elektron sender emits CC38 between CC6 and any null. Otherwise the window closes and CC38 falls through to Drum 1 cutoff.
  - CC100/CC101 (`RPN_LSB`/`RPN_MSB`) with value 127 inside the window is consumed: `nrpnSelected = 0`, and the window stays open for the paired byte. Otherwise the window closes and they fall through to pan.
  - Any other CC closes the window.
  - Case values 6/38/98/99/100/101 are distinct. `NRPN_FINE`/`NRPN_COARSE`/`NRPN_DATA_ENTRY_COARSE` are LXR-enum names; `DATA_MSB_LSB`/`RPN_LSB`/`RPN_MSB` are external-CC enum names in `MidiMessages.h`.
  - Accepted cost: a deliberate Pan 4/5 = 127 sent *immediately* after an NRPN data entry is dropped.
- **`globalMidiParser_applySpecialNrpn()`:**
  - 93 → `seq_setRollRate(clamp 0..15)`, 94 → `seq_setRollNote()`, 95 → `seq_setRollVelocity()`. Each also sends a display report to the AVR.
  - 96 → `globalMidiParser_applyRawParameter(PAR_FILTER_FREQ_1, …)`, only when the swap is on. That is the exact store/apply/echo/record path Global CC38 uses. Otherwise it returns 0 and the table range check ignores it.
  - Roll NRPNs are not recorded as automation and not echoed to MIDI out.
  - Reports are sent on **every** data entry, with no change cache: the front panel also writes these values, so an STM-side cache could go stale and suppress a needed report.
- **Since P3**, `GLOBAL_MIDI_NRPN_ROLL_RATE/_NOTE/_VELOCITY/_FILTER_FREQ_1` (93-96) are defined in **`GlobalMidiParser.h`** so receive and transmit share them.

### 2.3 PERF roll display sync (STM → AVR)

- Without it, the PERF page shows stale values, and the next encoder/pot move resends the stale value and overwrites the MIDI one. Pre-S039 NRPN 93 had this gap too.
- **`PARAM_CC2` cannot be reused.** It reaches at most raw 255 (`PAR_ROLL_NOTE` = 270), and its AVR handler writes `parameters2[data1+128]`, but `parameters2` is only `END_OF_SOUND_PARAMETERS` long, so the write would overflow.
- New `SEQ_CC` sub-opcodes, **STM→AVR only**, with new names because protocol direction matters:

| STM name | AVR name | Value | data2 |
|---|---|---|---|
| `FRONT_SEQ_REPORT_ROLL_RATE` | `SEQ_REPORT_ROLL_RATE` | `0x71` | rate 0..15 (same index as `PAR_ROLL` / `SEQ_ROLL_RATE`) |
| `FRONT_SEQ_REPORT_ROLL_NOTE` | `SEQ_REPORT_ROLL_NOTE` | `0x72` | 0..127 |
| `FRONT_SEQ_REPORT_ROLL_VELOCITY` | `SEQ_REPORT_ROLL_VELOCITY` | `0x73` | 0..127 |

- **Sender:** `frontPanelSending_sendRollParamReport(subOpcode, value)` → `frontPanelSending_sendTriplet(FRONT_SEQ_CC, sub, value & 0x7f)`. This is the non-blocking queue, like `frontPanelSending_sendGlobalMorphRuntimeReport()`.
- **AVR** (`avrCommsReceivingProtocol.c`, `SEQ_CC` switch after `SEQ_REPORT_GLOBAL_MORPH_MSB`): writes `parameter_values[PAR_ROLL | PAR_ROLL_NOTE | PAR_ROLL_VELOCITY]` and calls `menu_repaint()`. There is **no echo** back. These are runtime-only parameters and never saved.
- Roll rate/note/velocity are runtime-only. They sit between `END_OF_SOUND_PARAMETERS` and `PAR_BEGINNING_OF_GLOBALS` and are never in files.

### 2.4 Imported-sample FM (`DSPAudio/Oscillator.c`)

**Root cause:** `calcUserSampleOscFmBlock()` contained `(void)modBuffer;` and delegated to `calcUserSampleOscBlock()`. The sample-import Phase 2 "FM user-sample compatibility path" was a stub.
- Affected carriers (all callers of `calcNextOscSampleFmBlock()`): Drum 1-3 main osc in FM mode (`DrumVoice.c` `calcDrumVoiceSyncBlock()`), Cymbal main osc and Hihat main osc.
- Snare has no FM path. Drum mix mode (`mixOscs`) intentionally has none.

**Fix — phase modulation of the read position.** All LXR "FM" offsets the read index; it does not change the increment.
- **Scaling derivation:**
  - `osc_calcUserSampleFreq()` uses `freq2PhaseIncr32767()`: 1024 × f / FS × 2^17. A user sample therefore advances **1024 frames per oscillator cycle** in a 17-bit fractional position.
  - The wavetable path offsets its 32-bit phase by `(mod*fmMod) << 19`, i.e. `mod*fmMod/8192` cycles = `mod*fmMod/8` frames = `(mod*fmMod) << 14` in the 17-bit domain.
  - The built-in CRASH path already uses `<< 14`. So `OSC_USER_SAMPLE_FM_SCALE 16384` gives an identical modulation index.
- **Clamp:** `OSC_USER_SAMPLE_FM_PRODUCT_MAX 131071` keeps `product × 16384` inside `int32_t` (2^31 − 16384), even if `fmMod` ever exceeds 1.0.
- **Loop structure** (a copy of `calcUserSampleOscBlock()` plus the offset):
  - The guards are identical (index range, size 0, offset below ROM, `!sampleActive`).
  - The read uses the **unmodulated** play head plus a signed offset. The offset is a multiply, never a left shift of a negative value. The CRASH path's `(uint32_t)(negative float)` cast is undefined behaviour and was not copied.
  - The offset is split as floor-frames (arithmetic `>> 17`) plus a positive 17-bit fraction.
  - **Loops:** wrap with one 32-bit `%` only when the read leaves the sample. This is O(1) even for tiny loops; a compare-and-subtract loop would be O(offset/size).
  - **One-shots:** clamp to `[0, size-1]`.
  - Interpolation uses the same `nextPos < sampleSize` rule as the non-FM path (no wrap), so `fmMod == 0` stays bit-identical.
  - The play-head advance and the `sampleActive` gate are identical to the non-FM path, so FM never changes one-shot length or end timing.
  - Also sets `osc->output`.
- Header comment updated in `Oscillator.h`.
- The implementer corrected a comment that contained `calcFm*/`, where `*/` would have closed the block comment early; it now reads `calcFm /`.
- **Dead code, harmless:** the single-sample dispatchers `calcNextOscSampleFm()` / `calcNextOscSample()` return 0 for imported samples, but nothing calls them (nor their scalar helpers `calcPeriodicInterpFm()` / `calcPeriodicInterp()`). All rendering, including waveform interpolation, goes through the block paths `calcNextOscSampleBlock()` / `calcNextOscSampleFmBlock()`. Correction: waveform interpolation is **not** limited to periodic waveforms. `osc_waveInterpActive()` allows blends up to the last imported sample, so CRASH→sample and sample→sample blends occur, and since this session they get real FM through `calcPeriodicInterpFmBlock()` → `calcUserSampleOscFmBlock()`. Blend mechanics: the modulator buffer is rendered once; each side is read at its own position plus the same FM offset and the two results are crossfaded, which for tables equals FM'ing a crossfaded table. Known artifact: in a sample→sample blend both copies share `samplePosition`/`sampleFraction`/`sampleActive`, and the further-along copy's position is kept; for two looped samples of different lengths, the shorter one can jump after the longer one wraps (a possible click). Fixing it would need per-copy positions.
- The planned host harness for FM (bit-identical output at `fmMod = 0`, one-shot timing under heavy FM, edge reads) **was not run**. Hardware listening is the verification.

### 2.5 Hihat modulator re-arm (`DSPAudio/HiHat.c`)

- `HiHat_trigger()` re-armed only `hatVoice.osc`. A one-shot imported sample on `modOsc`/`modOsc2` (`WAVE2_HH`/`WAVE3_HH`) played once, cleared `sampleActive` and output zeros forever, so hat FM vanished after the first hit.
- Added `osc_resetSamplePlayback(&hatVoice.modOsc)` and `(&hatVoice.modOsc2)` after the main-osc block. This matches Cymbal and Drum.
- Modulator **phase is deliberately not reset**, so periodic-waveform hats sound unchanged.

### 2.6 P1 review (independent, after implementation)

- All 21 P1 changes (D1-D3, M1-M8, P1-P3, A1-A7) matched the schedule.
- Binaries were current and embedded. Touched files recompiled with the real flags gave no new warnings: only the pre-existing `Oscillator.c:51` `__CLZ` inline warning and the intentional LED fall-through at `avrCommsReceivingProtocol.c:1022-1026`.
- P1 build: STM 246,508, AVR 56,546 (798 bytes free), `FIRMWARE.BIN` 303,872.
- Non-blocking findings at the time:
  - stale references to the plan's old file name (since fixed by commit `cdc5242` naming);
  - a placeholder commit hash in `MEMORY.md`;
  - the cosmetic `MenuText.h` constant placement.

---

## Part 3 — P2: RX Filter "C" (Control Change) Reception

### 3.1 Audit of the three MIDI settings against the intended behaviour

The user's specification:
- **mrt routing:** `U2M` USB in → MIDI out, `M2M` MIDI in → MIDI out, `A2M` both → MIDI out, `M2U` MIDI in → USB out. The firmware also has `off` and `M2A`.
- **txf / rxf:** letters P program change, C control change, R realtime start/stop/clock, N note on/off. A shown letter means transmitted/received; `off` = none, `all` = everything.

| Item | Code | Status before P2 |
|---|---|---|
| `mrt` | Forwarding runs first in `midiParser_parseMidiMessage()`, before any RX check. DIN realtime is forwarded in `midiParser_handleDinRealtime()`. USB realtime is re-parsed through `midiParser_parseMidiMessage()`. | matches |
| rxf N `0x01` | Note block gated, including the MIDI roll trigger | matches |
| rxf R `0x02` | Clock/start/stop/continue gated on DIN realtime and the USB/system path; MTC gated | matches |
| rxf P `0x08` | Program change gated | matches |
| rxf C `0x04` | **Not gated.** `midiParser_MIDIccHandler(msg,1)` was called unconditionally. `0x04` was tested only deep in the CC ladders (`ChannelMidiParser.c` ×7, `GlobalMidiParser.c` ×1), gating the front-panel display echo and automation recording | **defect** |
| txf N/R/P | `seq_sendMidiNoteOn()` 0x10, `seq_sendRealtime()` 0x20, `seq_sendProgChg()` 0x80. Note-offs (`seq_midiNoteOff()`) deliberately ungated so notes cannot hang | matches |
| txf C `0x40` | No CC transmitter existed | inert (fixed in P3) |

Clarification recorded for future sessions: "echo" in the CC ladders means the **front-panel (AVR) display update** via `channelMidiParser_sendParameterEcho()` → `frontPanelSending_sendParameterEcho()`. It never means MIDI output.

**Effect of the defect with C off:** CCs still changed the sound, were stored in the kit, could load a kit (CC0) or move morph (CC1), and drove NRPNs, while the display stopped following and nothing was recorded.

**Root cause:** the `--AS FILT` work attached C to the echo/record branch inside each destination instead of the entry point. `midiParser_MIDIccHandler()` has exactly one caller (the `MIDI_CC` branch). Internal automation and front-panel edits reach DSP via `frontParser_applyParameterCommand()`, never through this path.

### 3.2 Fix

- `MidiParser.c`, `MIDI_CC` branch of `midiParser_parseMidiMessage()`: `if(midiParser_txRxFilter & 0x04) midiParser_MIDIccHandler(msg,1);`, with the `RX CONTROLLER FILTER GATE` doc block.
  - Covers every channel and CC number, including CC0 bank/kit change, CC1 morph, NRPN select/data/null and both tables.
  - Routing still forwards, because it ran above.
- Doc block `EXTERNAL MIDI MESSAGE DISPATCH` on `midiParser_parseMidiMessage()`: routing first and unfiltered, then SysEx stop, system messages, then channel messages gated by their letter.
- The 8 inner `0x04` tests were **left in place**. They are now always true for external CCs, and removing them would be churn inside the large per-voice ladders.
- `MidiParser.h` comments rewritten (`MIDI TX/RX FILTER SETTER` bit layout; `MIDI FILTER STATE` writer/reader block).
- Interactions:
  - ¢ entries all include C.
  - NRPN selection state is untouched while C is off; a selector received before C went off stays selected. That is standard NRPN semantics.
  - The boot default `txRxFilter = 0xFF` is unchanged.
  - USB and DIN take the same path.
- **Behaviour change:** users with `rxf` lacking C lose all CC, bank-change, morph and NRPN control. This is intended.
- P2 build: STM 246,524 (+16), AVR unchanged. Review: applied exactly as scheduled, no new warnings.

---

## Part 4 — P3: TX Filter "C" — Menu Parameter MIDI Transmit

### 4.1 Deep-dive facts that shaped the design

- **The STM owns MIDI out.** `outputControl_sendMidi()` (`MidiOutputControl.c`) writes each message to `usb_sendMidi()` and `uart_sendMidi()`.
- **Menu sound-parameter edits travel as `MIDI_CC` (`0xb0`, raw id 0..127) and `CC_2` (`0xb6`, raw − 128).** The same opcodes are also sent by:
  - the armed-automation-step revert (`buttonHandler.c`);
  - per-voice preset loads and the audio-out send during voice load (`presetManager.c`).

  So the STM **cannot identify a menu edit by opcode**. That forced the new tagged opcodes.
- **The shift-encoder path's `MIDI_CC`/`CC_2` sends are unreachable** for sound parameters: in `menu_encoderChangeShiftParameter()`, `isMorphParam = (paramNr < END_OF_SOUND_PARAMETERS)`, so shift edits always go to the morph endpoint. They were left unchanged.
- **The encoder path re-sends target-selector edits** through `MIDI_CC`/`CC_2` with the modTargets **menu index** as the value (the `menu.c` "TODO this will also send MIDI_CC" comment).
- **PERF global morph** (`SEQ_SET_GLOBAL_MORPH_LSB/_MSB`) and **roll rate/note/velocity** (`SEQ_ROLL_RATE/_NOTE/_VELOCITY`) are sent only from `menu_parseGlobalParam()`.
  - Callers: encoder, knob, and the armed-step revert.
  - `menu_sendAllGlobals()` starts at `PAR_BEGINNING_OF_GLOBALS` and never sends them.
  - So those front opcodes are **menu-only**.
- **CC1 morph on receive:** `preset_morphAutomationValueToAmount()` maps `v*2`, and 127 → 255.
- **Armed step:** the STM tracks it in `seq_armedArmedAutomationStep/Track` (set and cleared by `seq_armAutomationStep()`). The AVR sends DISARM **before** the revert.
- **Front-panel framing** is a fixed 3 bytes for every non-SysEx status (`frontParser_parseUartData()`). **`0xca`/`0xcb` were unused** on both MCUs. No STM code filters front-panel traffic by `status == MIDI_CC/FRONT_CC_2` other than the dispatch cases (checked), so the new statuses are not missed by any gate.
- **Coverage:** the Global receive tables cover **216** sound parameters (123 CC, 93 NRPN).
  - Unmapped: envelope position 1-6 (voice-channel CC90 only), per-voice morph (voice-channel CC1 only), kit version and reserved slots.
  - `PAR_NONE` is 0, so unset table slots (CC0/1/6/98/99) are unmapped.
- **DIN MIDI out defect:** `fifo_midiTx` (256 bytes) was filled byte by byte with silent drop when full, so a message could be split.
- **USB MIDI out defect** (`usb_manager.c`, `usb_midi_core.c`): `usb_MidiInBuff` is two 64-byte ping-pong halves.
  - `usb_sendByte()` had **no bounds check**. More than 64 bytes between flushes wrote into the half being transmitted, and more than 128 wrapped, corrupting `usb_flushMidi()`'s `uint8_t` length.
  - `usb_flushMidi()` ran `DCD_EP_Flush()` + `DCD_EP_Tx()` on **every main-loop pass** without waiting for completion, so it could abort IN data still in the endpoint FIFO.
  - `usbd_midi_DataIn()` (the completion callback, compiled and registered in `MIDI_cb`) was an empty stub.
  - `usbd_core.c` only calls `DataIn` while the device is `USB_OTG_CONFIGURED`, and passes `epnum` without the direction bit (`MIDI_IN_EP` = `0x81`).

### 4.2 Design as implemented

**Tagged menu opcodes (AVR→STM)**

| AVR | STM | Value | Payload |
|---|---|---|---|
| `MIDI_CC_MENU` | `FRONT_CC_MENU` | `0xca` | raw id 0..127, value |
| `CC_2_MENU` | `FRONT_CC_2_MENU` | `0xcb` | raw id − 128, value |

- **AVR:** `menu_sendSoundParameter(paramNr, value)` is used by the encoder edit path and both knob branches (`DTYPE_AUTOM_TARGET` non-PERF and default). Loads, the armed-step revert and restores keep the plain opcodes. Traffic size is unchanged. AVR cost was +24 bytes.
- **STM:** `case FRONT_CC_MENU: case MIDI_CC:` and `case FRONT_CC_2_MENU: case FRONT_CC_2:` share the existing bodies, with an `isMenuEdit` flag.
  - The live message is normalised (`liveMsg.status = MIDI_CC`; `cc2Msg.status = FRONT_CC_2`), because `frontParser_applyParameterCommand()` treats any status other than `MIDI_CC` as CC2.
  - TX is queued **only in the applied-live branch**. Values only stored while temp playback is active are not transmitted.
  - Plain opcodes behave byte-identically to before.
- **PERF:** `FRONT_SEQ_ROLL_RATE/_NOTE/_VELOCITY` and `FRONT_SEQ_SET_GLOBAL_MORPH_MSB` call `outputControl_queueMenuPerfTx()` after applying.

**Gate** (in `MidiOutputControl.c`):
- `midiParser_txRxFilter & 0x40` (TX C), and Global channel `midi_MidiChannels[7]` in 1..16.
- `!seq_isAutomationStepArmed()`, a new accessor (`sequencer.c/.h`), checked at queue time.
- TX C and channel are re-checked at emit; if either is closed, everything queued is dropped.
- Side effect: a PERF morph/roll revert after DISARM transmits the restored value. That is harmless, because it equals what the DAW already holds.

**Address = inverse of the receive tables** (`globalMidiParser_getTxAddress()` in `GlobalMidiParser.c/.h`):
- Lazily built `uint8_t globalMidiParser_txAddress[END_OF_SOUND_PARAMETERS]`: 0 = none, 1..127 = CC n, 128 + n = NRPN n.
  - NRPN entries are written first, then CC 2..127 overwrite them (CC preferred: 3 bytes instead of 9).
  - Skips `PAR_NONE`, out-of-range ids and the **18 target selectors** (`PAR_VEL_DEST_1..6`, `PAR_VOICE_LFO1..6`, `PAR_TARGET_LFO1..6`; their receive value domain, raw NRPN value versus menu modTargets index, is unverified).
- Return kinds: `GLOBAL_MIDI_TX_NONE/_CC/_NRPN`.
- **¢ mirror:** `PAR_FILTER_FREQ_1` returns NRPN 96 while `globalMidiParser_cc38Swap`, else CC38.
- **Correction to the plan:** CC38, CC100 and CC101 are **real TX addresses** (Drum 1 cutoff, Snare/Cymbal pan), not protocol exclusions. Only the selectors are excluded.
- Result: **198 transmittable parameters** (123 CC + 75 NRPN = 216 − 18 selectors).

**PERF fixed addresses**

| Control | Address | Value |
|---|---|---|
| Roll rate | NRPN 93 | 0..15 selector |
| Roll note | NRPN 94 | 0..127 |
| Roll velocity | NRPN 95 | 0..127 |
| Global morph | CC1 (`MOD_WHEEL`) | `amount >> 1` (receive maps `v*2`, 127 → 255; round trip within ±1) |

**Wire format:**
- CC: `0xB0 | (ch−1), cc, value`.
- NRPN: `CC99 = 0`, `CC98 = n`, `CC6 = value`, every time.
- **Never CC38** after an NRPN (an LXR receiver outside ¢ mode would move Drum 1 cutoff); **no RPN-null tail**.
- **CC100/101 = 127 guard:** before emitting Pan 4/5 = 127, emit `CC99 = 0` first. It closes the receiver's P1 RPN-null window, so recorded hard-pan plays back.

**Coalescing paced queue** (`MidiOutputControl.c`):
- Slots = `END_OF_SOUND_PARAMETERS` + 4 PERF slots (`OUTPUT_MENU_TX_ROLL_RATE/_NOTE/_VELOCITY/_GLOBAL_MORPH` = 0..3).
- Storage: `value[]`, dirty bitmap, pending count, round-robin cursor. About 280 bytes of RAM.
- `outputControl_queueMenuParameterTx()` and `outputControl_queueMenuPerfTx()` implement latest-value-wins.
- `outputControl_serviceMenuTx()` runs in `main.c` **immediately before `usb_tick()`**:
  - O(1) when idle.
  - At most **one** message (or one drop of an unresolvable slot) per pass.
  - Emits only if **both** DIN and USB have room for the whole message: DIN 3 or 9 (+3 guard) bytes, USB 4 or 12 (+4) bytes.
  - Otherwise it retries the same slot next pass.
  - The address is resolved at emit time, so a ¢ toggle between queue and emit is honoured.
- Sequencer notes and clock keep their direct path.

**DIN whole-message rule (all output):**
- `fifo_freeSpace()` (`Hardware/FIFO.c/.h`): `(read − write − 1) & BUFFER_MASK`, conservative against the ISR consumer.
- `uart_sendMidi()` drops the entire message (`uart_midiTxDropCount++`) unless `1 + bits.length` bytes fit.
- `uart_midiTxFree()` for pacing.
- Single producer (main loop); the TXE ISR consumer only frees space.

**USB fix:**
- `usb_sendMidi()` drops a whole 4-byte event (`usb_midiTxDropCount++`) when `usb_midiTxFree()` < 4. `usb_midiTxFree()` returns the bytes left in the current half.
- `volatile usb_midiTxBusy` (defined in `usb_midi_core.c`, extern in `.h`):
  - set **before** `DCD_EP_Tx()` in `usb_flushMidi()`;
  - cleared in `usbd_midi_DataIn()` for `(epnum & 0x7F) == (MIDI_IN_EP & 0x7F)`.
- `usb_flushMidi()`:
  - **discards** pending bytes and clears busy when the device is not `USB_OTG_CONFIGURED` (unenumerated, addressed or suspended);
  - while busy, waits up to `USB_MIDI_TX_TIMEOUT_TICKS 20u` (5 ms of 250 µs `systick_ticks`), then abandons the half (`usb_midiTxTimeoutCount++`);
  - `DCD_EP_Flush()` now runs only after completion or timeout.
- `usb_manager.c` includes `globals.h` for `systick_ticks`.
- This also protects existing USB notes, clock and routing.

**Loops:** the receive path never transmits, so DAW playback into the LXR is applied once and not re-sent. `mrt` is independent.

**Scope**

| Source | TX? |
|---|---|
| Encoder/knob edit of a mapped sound parameter, gate open | yes |
| Same, during live record | yes |
| Same, automation step armed | no |
| Shift-edit (morph endpoint) | no |
| Stored but not applied live (temp playback) | no |
| PERF roll rate/note/velocity | yes, NRPN 93-95 |
| PERF global morph | yes, CC1 |
| PERF per-voice morph, envelope position | no (no Global mapping) |
| Target selectors | no |
| Loads, restores, background swap, automation, LFO, MIDI in, armed-step revert of sound parameters | never |

### 4.3 P3 verification

- **Implementer build:** STM 248,020 (+1,512), AVR 56,570 (+24), `FIRMWARE.BIN` 305,408.
- **Independent review:**
  - All scheduled changes (U1-U4, F1-F4, S1-S2, G1-G2, O1-O2, P1-P2, M1, A1-A2) were present and correct.
  - Both images were rebuilt from scratchpad copies (the tree had been cleaned at the time): rc 0 and identical sizes.
  - The 10 touched STM files and `menu.c` recompiled with the real flags: no new warnings.
- **Verbatim host harness**, using the `ParameterArray.h` enum, both receive tables, the G2 lookup block and the O2 queue block with stubs. **ALL PASS:**
  - every transmit address maps back to the same raw id through the receive tables; no protocol CC is ever produced; every non-selector receive-mapped parameter is transmittable; selectors return NONE;
  - Drum 1 cutoff gives CC38, or NRPN 96 with the swap; Pan 4 gives CC100;
  - TX C off, channel 0 and armed each send nothing;
  - coalescing keeps the latest value; one message per pass; channel nibble correct;
  - NRPN framing correct;
  - the guard fires only for 127;
  - DIN/USB room pacing holds and retries;
  - gate close drops the queue;
  - morph round-trips within ±1 for all 256 amounts;
  - roll is sent as NRPN 93/95.
- At closeout the tree was rebuilt: `FIRMWARE.BIN` is present and both binaries are verified embedded.

---

## Part 5 — Documentation and Manual Changes

- **`knowledge_files/comms_spec_reference/MIDI_TABLE.md`:**
  - CC38 / CC100 / CC101 notes, NRPN validity sentence, rows 93-96, `## RX Filter ¢ Modes` section, "RX filter and routing" bullets, and the full `## MIDI Transmit (TX filter C, Session 039 P3)` section;
  - updated again at closeout: date/status, stale "TX C reserved" line, ownership bullets, USB detail.
- **`COMMS_FLOW_SPEC.md`:** `SEQ_MIDI_FILT_RX` bit 4 and RX C bullets; `0xca`/`0xcb` bullet; `SEQ_REPORT_ROLL_*` in §2 plus the display-only sentence; status line; closeout additions in the MIDI Boundary and send-contract sections.
- **`BACKGROUND_LOAD_TEMPORARY.md`:** closeout guardrail that load traffic keeps the plain opcodes.
- **Manual (`manual/chapters/05_operation_modes.md`, `07_midi.md`, `manual/latex/midi_tables.tex`):**
  - Manual Roll MIDI bullets.
  - Menu Options rows (`rxf` ¢ note, new `rol` MIDI Roll Offset row, `txf` C text).
  - New subsections *MIDI Roll Offset* and *Receive MIDI Filter* (letters table, ¢ twins, encoder-only, routing pass-through), with *Loading and Saving Global Settings* moved below them.
  - MIDI chapter: roll-trigger note; new `## Transmitting Parameter Changes` section, which renumbers the MIDI CC List to **7.7** (Global table **7.7.2**); notes block before the tables.
  - LaTeX: CC38 † and CC100/101 ‡ markers with notes, NRPN 93-96 rows, the NRPN send-order note and the TX note. These use `\textcent`, which needs `textcomp` (kernel ≥ 2020).
  - The manual was rebuilt by the user (`manual/build/*` staged).
  - **User feedback:** this manual text is too verbose. It was left unchanged, and future manual entries must be terse.
- **`README.md`:** Version 1.03 changelog lines, including "Menu edits send Tx CC on Global channel when enabled."
- **`front/LxrAvr/main.c`:** `FIRMWARE_VERSION` "1.02" → **"1.03"**.

---

## Consolidated Hardware Verification Checklist (nothing yet tested)

**Session 038 (still pending):**
- Octatrack release with NOTE_ON velocity 0;
- a real `0x8n` sender;
- NRPN 93;
- stopped rolls and the stop/start handover;
- a single first hit when a roll is held through Play.

**P1:**
1. **¢ glyph:** RxFilter 16..23 shows `¢`, not `¥` or `\`. If not, use the CGRAM slot-1 fallback.
2. **¢ UI:**
   - The pot reaches only `off..all`.
   - Encoder edit walks `off..all, ¢..al¢`.
   - The 4-parameter view shows the ¢ name, and the RxFilter pot does nothing while ¢ is active.
   - The encoder can return to a normal entry.
   - TxFilter never shows ¢.
   - Save globals, power-cycle: ¢ is restored with the swap active.
3. **CC38 swap:**
   - In `¢RN`, an Elektron NRPN 93 changes roll rate and leaves Drum 1 cutoff alone.
   - NRPN 96 sets Drum 1 cutoff, and stored/echoed values match a CC38 move.
   - In `CRN`, CC38 moves the cutoff and NRPN 96 does nothing.
4. **RPN null:**
   - NRPN data, then CC101=127 and CC100=127: no pan change, and a following bare CC6 is ignored.
   - CC100/101 alone still pan. Check in `CRN` and `¢RN`.
5. **Roll NRPNs:**
   - 93/94/95 update the PERF page live during a sweep.
   - A held roll follows the rate at the quantise latch, and the next hit uses the new note/velocity.
   - The encoder/pot then start from the MIDI value.
6. **FM:**
   - Drum 1 with an imported-sample osc in FM mode: sweeping FM changes the timbre as with SIN, and FM 0 sounds identical to the previous build.
   - Repeat on Cymbal and Hihat, with one-shot and looped samples.
7. **Hihat modulator:** a one-shot sample on the hat `modOsc` gives FM on every hit. Periodic modulators sound unchanged.

**P2** (`mrt = M2M`, MIDI monitor on DIN out):
1. **`rxf = RN` (C off):**
   - Global CC89 gives no sound change but appears on DIN out.
   - Voice CC16 gives no change.
   - CC0 gives no kit load; CC1 no morph.
   - NRPN 93 gives no roll-rate change.
2. **`rxf = CRN`:** all of the above work, with display echo and recording.
3. **`rxf = ¢RN`:** same as `CRN` except CC38 is ignored and NRPN 96 sets the cutoff.
4. With C off, notes, clock and program changes still follow their own letters.

**P3** (MIDI monitor on DIN and USB, then a DAW):
1. A voice-page knob/encoder sends the correct CC or NRPN triplet on the Global channel.
2. Nothing is sent with TX C off, channel 0, or a step armed.
3. Drum 1 cutoff is sent as CC38, or NRPN 96 in ¢ mode.
4. PERF roll sends NRPN 93-95; global morph sends CC1.
5. File load, morph load, automation, LFO and MIDI-in playback send nothing.
6. A fast four-knob sweep gives valid streams on both outputs that end on the final values.
7. Record a sweep in a DAW and play it back with RX C on: the motion and the PERF/menu display follow.
8. **USB stress (highest hardware-only risk):** a high-rate sequencer plus routing gives no corrupted USB MIDI. USB recovers after the host stops reading (timeout), after cable unplug/replug, and after suspend/resume.

---

## Completed Changes

| File | Change |
|---|---|
| `front/LxrAvr/Menu/MenuText.h` | ¢ constants; `midiFilterNames` `[16]`→`[4]` plus 8 ¢ rows |
| `front/LxrAvr/Menu/menu.c` | RxFilter encoder count 24; pot lock; RX wire encoding/repair; `menu_sendSoundParameter()` plus 3 call sites |
| `front/LxrAvr/avrComms/avrCommsReceivingProtocol.h/.c` | `SEQ_REPORT_ROLL_*` 0x71-0x73 handlers; `MIDI_CC_MENU 0xca`, `CC_2_MENU 0xcb` |
| `front/LxrAvr/main.c` | `FIRMWARE_VERSION "1.03"` |
| `mainboard/LxrStm32/src/MIDI/MidiParser.c/.h` | ¢ flag decode in `midi_setFilter()`; `MIDI_RX_FILTER_CC38_SWAP`; RX C gate; dispatch and filter doc blocks |
| `mainboard/LxrStm32/src/MIDI/GlobalMidiParser.c/.h` | CC38 swap state/setter; RPN-null window; `applySpecialNrpn()` (93-96); NRPN handler rewrite; NRPN 93-96 defines moved to the header; TX reverse table and `getTxAddress()` |
| `mainboard/LxrStm32/src/MIDI/MidiOutputControl.c/.h` | Menu TX gate, coalescing queue, PERF slots, emitters, `outputControl_serviceMenuTx()` |
| `mainboard/LxrStm32/src/uARTFrontSYX/frontPanelReceivingProtocol.h/.c` | `FRONT_SEQ_REPORT_ROLL_*`; `FRONT_CC_MENU`/`FRONT_CC_2_MENU`; shared `_MENU` cases; PERF roll/morph TX queue calls |
| `mainboard/LxrStm32/src/uARTFrontSYX/frontPanelSendingProtocol.c/.h` | `frontPanelSending_sendRollParamReport()` |
| `mainboard/LxrStm32/src/uARTFrontSYX/Uart.c/.h` | `uart_sendMidi()` whole-message-or-drop; `uart_midiTxFree()`; drop counter |
| `mainboard/LxrStm32/src/Hardware/FIFO.c/.h` | `fifo_freeSpace()` |
| `mainboard/LxrStm32/src/Hardware/USB/usb_manager.c/.h`, `usb_midi_core.c/.h` | Bounds check, `usb_midiTxFree()`, busy flag via `DataIn`, 5 ms timeout, discard when unconfigured, counters |
| `mainboard/LxrStm32/src/Sequencer/sequencer.c/.h` | `seq_isAutomationStepArmed()` |
| `mainboard/LxrStm32/src/DSPAudio/Oscillator.c/.h` | Real `calcUserSampleOscFmBlock()` plus scaling constants |
| `mainboard/LxrStm32/src/DSPAudio/HiHat.c` | Modulator sample re-arm |
| `mainboard/LxrStm32/src/main.c` | Menu TX service call plus include |
| `firmware image/FIRMWARE.BIN` | Rebuilt (305,408 bytes) |
| `knowledge_files/comms_spec_reference/*` | See Part 5 |
| `manual/chapters/05_operation_modes.md`, `07_midi.md`, `manual/latex/midi_tables.tex`, `manual/build/*` | See Part 5 |
| `README.md` | 1.03 changelog |
| `MEMORY.md`, `000_SESSION_INDEX.md`, this log | Closeout |

---

## Known Issues Introduced

- **Stale code comments (doc debt, not behaviour):**
  - `MidiParser.h` `MIDI TX/RX FILTER SETTER` says TX C is "reserved, as the LXR transmits no CCs".
  - The `MIDI FILTER STATE` block says "0x40 has no transmitter".
  - Both were written in P2 and are false after P3. Update them on the next code pass.
- **Cosmetic:**
  - `usb_manager.c` keeps the original `usb_flushMidi()` comment header (with its TODO) directly above the new Session 039 header.
  - `FIFO.h` lines 47 and 80: the Latin-1 `ä` in two German comments was re-encoded by the editor (comment-only byte change).
  - `MenuText.h` ¢ constants split the menu-id define list.
- **Intended behaviour changes:**
  - `rxf` without C now blocks all CC/bank/morph/NRPN input.
  - `txf` with C now emits CC/NRPN on menu edits.
  - USB MIDI out now discards output while no host is configured.
  - DIN/USB out drop whole messages when full, instead of splitting them.
- **¢ glyph is unverified.**

## Known Issues Resolved

- Elektron/14-bit NRPN senders corrupting Drum 1 cutoff, via the opt-in ¢ modes.
- RPN-null CC101/100 = 127 hard-panning Snare/Cymbal.
- No MIDI access to roll note/velocity, and a stale PERF display after NRPN 93.
- No FM on imported-sample carriers.
- Hihat modulator one-shot samples silent after the first hit.
- RX filter C not gating CCs.
- TX filter C having no transmitter.
- USB MIDI out overflow/wrap and in-flight transfer abort.
- DIN MIDI out partial-message drops.

---

## Open Items / Next Decisions

1. **Hardware-verify Sessions 038 and 039** (checklists above). The USB flush timing change carries the most hardware-only risk.
2. **Commit P3**, which is staged. Fix the stale `MidiParser.h` TX comments, and optionally the cosmetic items.
3. **Pre-existing out-of-bounds writes in `seq_init()`**, re-confirmed by the compiler this session, not S039:
   - `midi_envPosition[i]` for `i < NUM_TRACKS` (7) into `uint8_t midi_envPosition[6]` (`sequencer.c` ~253);
   - `memset(seq_lastMasterStep, 0, NUM_TRACKS+1)` into `uint8_t seq_lastMasterStep[NUM_TRACKS]` (~266).

   Each writes one zero byte into the following global at init. Recommend a separate small fix.
4. **Target selectors over MIDI:** whether NRPN 21-26 / 33-44 receive a modTargets index or a raw parameter id is unverified. Resolve it before ever adding them to TX.
5. **Dead code:** `calcNextOscSampleFm()`/`calcNextOscSample()` return 0 for imported samples but have no callers (harmless). Separate real artifact: sample→sample waveform blends share one playback position; two looped samples of different lengths can jump or click after the longer loop wraps (fix needs per-copy positions; see Part 2.4).
6. **Manual terseness:** rewrite the Session 039 manual text tersely only if the user asks.
7. Carried from 037/036: front-panel step-editor row-0 sub-step addressing; the `LengthRotate` bitfield size.

---

## Repository State at Session End

- Branch `dev-roll-midi`. HEAD `cdc5242`, which contains P1, P2 and the six root S039 documents.
- **Staged, uncommitted:**
  - P3 code (AVR `menu.c`, `avrCommsReceivingProtocol.h`; STM `FIFO`, `usb_manager`, `usb_midi_core`, `GlobalMidiParser`, `MidiOutputControl`, `sequencer`, `main.c`, `Uart`, `frontPanelReceivingProtocol`);
  - AVR `main.c` version bump;
  - `FIRMWARE.BIN`;
  - `README.md`;
  - comms specs;
  - manual sources and build outputs;
  - `MEMORY.md`;
  - `S039_P3_IMPLEMENTATION.md`.
- **Closeout edits (unstaged):** `000_SESSION_INDEX.md`, this log, comms specs, `MEMORY.md`.
- The assistant performed **no git mutation**.

---

## End Of Session Block

```
DATE: 2026-10-16
SESSION GOAL: Investigate "NRPN 93 attenuates one track"; put PERF roll controls on
              NRPNs; fix missing FM on imported-sample oscillators. Grew into: ¢ RX-filter
              CC38 swap, RPN-null swallow, RX filter C gate, TX filter C menu transmit,
              USB/DIN MIDI-out robustness.

COMPLETED:
- P1: ¢ RxFilter twins (menu 16..23, encoder-only, pot-locked, wire bit 0x10) swap
  Global CC38 -> NRPN 96 (Drum 1 cutoff); RPN-null window; NRPN 94/95 roll note/velocity;
  STM->AVR SEQ_REPORT_ROLL_* 0x71-0x73 PERF display sync; real phase-modulated FM for
  imported samples (<<14, unmodulated play head); Hihat modulator sample re-arm.
- P2: RX filter C gates all incoming CCs at the MIDI_CC parser entry.
- P3: TX filter C transmits menu edits (0xca/0xcb tagged), PERF roll (NRPN 93-95) and
  global morph (CC1) as Global CC/NRPN via a coalescing paced queue; reverse address
  table from the receive tables; ¢ mirror; CC100/101=127 guard; DIN/USB
  whole-message-or-drop; USB DataIn busy flag + 5 ms timeout; discard when unconfigured.
- Manual, MIDI tables, comms specs, README 1.03, firmware version 1.03.

VERIFIED ON HARDWARE: NO. Builds clean (STM 248,020 / AVR 56,570 of 57,344 /
                      FIRMWARE.BIN 305,408); verbatim host harness for P3 all pass.

CHANGES THIS SESSION: see "Completed Changes".

KNOWN ISSUES INTRODUCED: stale "TX C reserved" comments in MidiParser.h; cosmetic
  usb_manager.c duplicate header, FIFO.h comment re-encoding; ¢ glyph unverified.
KNOWN ISSUES RESOLVED: Elektron CC38 cutoff corruption (opt-in), RPN-null pan,
  roll NRPN access + display sync, imported-sample FM, hat modulator re-arm, RX C,
  TX C, USB MIDI-out overflow/abort, DIN partial messages.

NEXT SESSION RECOMMENDED GOAL: hardware-verify Sessions 038 + 039 (USB stress first),
  then commit P3 with the MidiParser.h comment fix.

BLOCKERS: hardware testing; ¢ glyph on the real LCD.

CRITICAL REMINDERS FOR NEXT SESSION:
- AVR app must stay <= 57,344 bytes (bootloader 0xE000); 774 bytes free now.
- Load/revert/restore traffic must keep plain MIDI_CC/CC_2; only menu edits use
  0xca/0xcb, or loads would be transmitted as MIDI.
- RX filter C gates CCs at the parser entry; mrt routing always forwards first.
- TX address must stay the inverse of the receive tables (getTxAddress()); never
  emit CC38 after an NRPN; keep the CC100/101=127 guard.
- `make firmware` does not rebuild sources; build STM/AVR first.
- Manual entries must be terse.
```
