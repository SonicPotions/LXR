# S039 — ¢ RX-Filter Modes, Roll NRPNs, Sample-Oscillator FM Fix

Branch: `dev-roll-midi` (HEAD `7a21412`). Status: **plan only, no code changed.**
Prerequisite: the Session 038 MIDI roll fix still needs hardware verification (see `MEMORY.md`, High Priority). Session 039 builds on that tree. **No existing CC or NRPN assignment changes**; NRPN 93 stays roll rate.

Scope:
1. New "¢" MIDI RX-filter entries that turn off Global CC38 (Elektron data-entry LSB) and move its target to NRPN 96.
2. Swallow the RPN-null deselect (CC101/CC100 = 127) after an NRPN.
3. Roll note and roll velocity on NRPN 94 and 95, plus PERF-page display sync for NRPN 93-95.
4. FM on imported-sample oscillators.
5. Hihat modulator sample re-arm on trigger.

---

## 1. Background: why Elektron NRPNs damage Drum 1

Elektron devices send every NRPN as CC99, CC98, CC6, **CC38** (data-entry LSB). `globalMidiParser_handleNrpnControl()` (`GlobalMidiParser.c`) consumes only CC6/98/99. CC38 falls through to `globalMidiParser_ccToRawParam[38]` = `PAR_FILTER_FREQ_1` and is applied **and stored** (`preset_storeParameterIngress()`). The LSB is normally 0, so every Elektron NRPN slams Drum 1's filter cutoff shut. NRPN 93 itself only sets roll rate.

Global CC38 is a published mapping, so the fix is opt-in through new RX-filter modes rather than a change to the table.

Existing quirk, not changed here: the RX `C` bit (`0x04`) gates only CC echo/automation recording. Incoming CCs are applied whatever its state (`MidiParser.c`, `MIDI_CC` branch).

---

## 2. ¢ RX-filter entries (AVR menu + STM flag)

### 2.1 Behaviour

`PAR_MIDI_FILT_RX` ("RxFilter", global page, `menuPages.h:376`) is a `DTYPE_MENU` over `midiFilterNames` (`MenuText.h`). The value is the RX bit nibble: N = `0x01`, R = `0x02`, C = `0x04`, P = `0x08`.

Every entry that receives CC gets a twin that shows `¢` in place of `C`:

| Value | Name | RX bits sent | Same as |
|---:|---|---|---|
| 16 | `¢` | `0x04` | `C` (4) |
| 17 | `¢N` | `0x05` | `CN` (5) |
| 18 | `¢R` | `0x06` | `CR` (6) |
| 19 | `¢RN` | `0x07` | `CRN` (7) |
| 20 | `P¢` | `0x0C` | `PC` (12) |
| 21 | `P¢N` | `0x0D` | `PCN` (13) |
| 22 | `P¢R` | `0x0E` | `PCR` (14) |
| 23 | `al¢` | `0x0F` | `all` (15) |

`all` has no literal `C`, so its 3-character twin is `al¢` (**confirm the name**).

A ¢ entry behaves exactly like its twin, with two differences:
- Global-channel **CC38 is ignored**.
- **NRPN 96 → `PAR_FILTER_FREQ_1`** (Drum 1 filter cutoff), so the parameter stays reachable over MIDI.

In non-¢ modes NRPN 96 stays out of range and is ignored, as it is today. Making NRPN 96 unconditional is a one-line change if wanted.

UI rules:
- **Pots (4-parameter view)**: they keep the existing 16-entry range (0..15), so they can never select a ¢ entry. While a ¢ entry is active, the pot for this parameter is **ignored**, so it cannot change the value. The 4-parameter view still shows the ¢ name.
- **Encoder edit mode (after clicking the encoder)**: steps through all 24 entries. ¢ entries follow `all` in value order.
- **TX filter** (`PAR_MIDI_FILT_TX`) shares `midiFilterNames`, but its range must stay 0..15 for both pots and encoder.

### 2.2 The ¢ character

The user reports that this LCD's ROM has `¢` at **0x5C** (between `[` 0x5B and `]` 0x5D). Standard HD44780 A00 ROM has `¥` there, so this is the first hardware check. In C, 0x5C is the backslash literal:

```c
#define MENU_CHAR_CENT "\\"   /* LCD ROM 0x5C renders as the cent sign on this display */
...
{MENU_CHAR_CENT},
{MENU_CHAR_CENT "N"},
{MENU_CHAR_CENT "R"},
{MENU_CHAR_CENT "RN"},
{"P" MENU_CHAR_CENT},
{"P" MENU_CHAR_CENT "N"},
{"P" MENU_CHAR_CENT "R"},
{"al" MENU_CHAR_CENT},
```

Fallback if 0x5C does not render as `¢`: define the glyph in CGRAM with `lcd_generatechar()` (`Hardware/lcd.c:201`, currently unused). Point `MENU_CHAR_CENT` at that slot. Slot 0 cannot be used, because code 0 terminates strings, so use slot 1 (`"\x01"`).

### 2.3 AVR changes (`front/LxrAvr`)

- `Menu/MenuText.h`: append the 8 rows above to `midiFilterNames` after `{"all"}`. **Leave the count byte at `{16}`**. `getMaxEntriesForMenu(MENU_MIDI_FILTERING)` keeps returning 16, so pots and the TX filter stay unchanged. `getMenuItemNameForValue()` indexes `[value + 1]` and copies 3 bytes, so values 16..23 display without further change. Add `#define MIDI_FILTER_CENT_FIRST 16` and `#define MIDI_FILTER_RX_ENTRIES 24` beside `MENU_MIDI_FILTERING`.
- `Menu/menu.c`, encoder entry count: add
  ```c
  /* RX filter exposes the ¢ (CC38-swap) twins to the encoder only. */
  static uint8_t menu_getEncoderMenuEntryCount(uint16_t paramNr)
  {
     if(paramNr == PAR_MIDI_FILT_RX)
        return MIDI_FILTER_RX_ENTRIES;
     return menu_getDtypeMenuEntryCount(paramNr);
  }
  ```
  Use it in place of `menu_getDtypeMenuEntryCount()` at every encoder-path call: `menu_normalizeDtypeMenuEncoderValue()`, the `DTYPE_MENU` clamp in `menu_encoderChangeParameter()` (~line 3058), and the equivalent `DTYPE_MENU` handling in `menu_encoderChangeShiftParameter()`. Without this, `menu_normalizeDtypeMenuEncoderValue()` would rescale any value ≥ 16 back down, and the clamp would cap it at 15.
- `Menu/menu.c`, `menu_parseKnobValue()`: before `getDtypeValue()`, add
  ```c
  /* ¢ RX-filter entries are encoder-only; a pot must not leave them. */
  if(paramNr == PAR_MIDI_FILT_RX && parameter_values[paramNr] >= MIDI_FILTER_CENT_FIRST)
     return;
  ```
- `Menu/menu.c`, `menu_parseGlobalParam()`, `case PAR_MIDI_FILT_RX`: this is the single point that sends to STM. It covers encoder edits, pot edits, global-file load and `menu_sendAllGlobals()`. Convert the menu index to the wire byte:
  ```c
  static const uint8_t menu_midiFilterCentBits[8] PROGMEM =
     {0x04, 0x05, 0x06, 0x07, 0x0C, 0x0D, 0x0E, 0x0F};
  ...
  if(value >= MIDI_FILTER_RX_ENTRIES)
     value = 0x0F;                                   /* corrupt file value -> "all" */
  wire = (value >= MIDI_FILTER_CENT_FIRST)
       ? (pgm_read_byte(&menu_midiFilterCentBits[value - MIDI_FILTER_CENT_FIRST]) | 0x10)
       : value;
  avrComms_sendData(SEQ_CC, SEQ_MIDI_FILT_RX, wire);
  ```
  Write any clamp back into `parameter_values[PAR_MIDI_FILT_RX]`. The TX case is unchanged.
- Persistence: globals are saved byte-for-byte (`presetManager.c`, `PAR_BEGINNING_OF_GLOBALS..NUM_PARAMS`), so values 16..23 save and reload with no format change. Older firmware loading such a file would show garbage for RxFilter. That is acceptable, and the change log will say so.

### 2.4 STM changes (`mainboard/LxrStm32`)

- `MIDI/MidiParser.c`, `midi_setFilter()`, RX branch: keep the existing roll-release logic on `value & 0x01`. Store `value & 0x0F` in the nibble as now, and add `globalMidiParser_setCc38Swap((value & 0x10) != 0);`. Bit 4 never reaches `midiParser_txRxFilter`'s TX nibble. TX values are always ≤ 15, so the TX branch is unchanged.
- `MIDI/GlobalMidiParser.c/.h`:
  - `static uint8_t globalMidiParser_cc38Swap = 0;` plus the exported setter `globalMidiParser_setCc38Swap()`, with the required comment.
  - In `globalMidiParser_MIDIccHandler()`, after the NRPN handler: `if(msg.data1 == 38 && globalMidiParser_cc38Swap) return;`. This consumes it as the data-entry LSB.
  - `#define GLOBAL_MIDI_NRPN_FILTER_FREQ_1 96`. On CC6 with NRPN 96 selected and swap on, call `globalMidiParser_applyRawParameter(PAR_FILTER_FREQ_1, value, updateOriginalValue, source)`. This is the same helper and the same store/echo/record path that CC38 uses today, so behaviour matches exactly. With swap off, NRPN 96 is ignored.

---

## 3. RPN-null deselect (CC101/CC100 = 127)

Many senders close an NRPN with CC101 = 127, CC100 = 127. Today those land on `PAR_PAN5` (Cymbal) and `PAR_PAN4` (Snare) and hard-pan them. This applies in **all** RX modes.

`GlobalMidiParser.c`:
- Add `static uint8_t globalMidiParser_nrpnDataPending = 0;`. CC6 sets it whenever an NRPN is selected (applied or ignored). CC38 leaves it set while swap is on (that CC38 is consumed). Any other Global CC clears it.
- CC101 or CC100 with value 127 while `nrpnDataPending` is set: consume it, clear `nrpnSelected`, and keep `nrpnDataPending` set until both null bytes have passed.
- Any other CC100/101 value, or one not following an NRPN data entry, keeps its published pan mapping.

Cost: a deliberate PAN4/PAN5 = 127 sent *immediately* after an NRPN data entry is dropped. Accepted.

---

## 4. Roll NRPNs 93-95 and PERF display sync

PERF page (`menuPages.h:428`) has `PAR_ROLL` (rate, 0..15), `PAR_ROLL_NOTE` (0..127) and `PAR_ROLL_VELOCITY` (0..127). STM setters: `seq_setRollRate()`, `seq_setRollNote()`, `seq_setRollVelocity()`. These are runtime-only values and are not saved.

| NRPN | Target | Range |
|---:|---|---|
| 93 | roll rate (existing, unchanged) | clamp 0..15 |
| 94 | roll note | 0..127 |
| 95 | roll velocity | 0..127 |

### 4.1 STM ingress

`GlobalMidiParser.c`: replace the single NRPN 93 `if` with a small switch on the selected NRPN for 93/94/95 (and 96 from §2.4), before the table lookup. Each case calls its setter, sends the display report (§4.2), and returns 1. Nothing is recorded as automation or echoed to MIDI out, matching current NRPN 93 behaviour.

### 4.2 AVR display sync

This is required. Without it the PERF page shows stale values, and the next encoder or pot move sends the stale value and overwrites the MIDI one. NRPN 93 has this gap today too.

- `PARAM_CC2` **cannot** be reused. It reaches at most raw 255, so `PAR_ROLL_NOTE` (270) is out of reach. The AVR handler also writes `parameters2[data1+128]`, and that array is only `END_OF_SOUND_PARAMETERS` long, so the write would overflow.
- Add new STM→AVR `SEQ_CC` sub-opcodes. They are free on both MCUs, and governance forbids reusing `FRONT_SEQ_ROLL_*` in the opposite direction:
  - `FRONT_SEQ_REPORT_ROLL_RATE 0x71` / AVR `SEQ_REPORT_ROLL_RATE`
  - `FRONT_SEQ_REPORT_ROLL_NOTE 0x72` / AVR `SEQ_REPORT_ROLL_NOTE`
  - `FRONT_SEQ_REPORT_ROLL_VELOCITY 0x73` / AVR `SEQ_REPORT_ROLL_VELOCITY`
- STM: `frontPanelSending_sendRollParamReport(uint8_t subOpcode, uint8_t value)`. It is non-blocking `frontPanelSending_sendByte()`, like `frontPanelSending_sendGlobalMorphRuntimeReport()`, because it is called from the MIDI parse path. Send only on change.
- AVR (`avrCommsReceivingProtocol.c`, `SEQ_CC` switch next to `SEQ_REPORT_GLOBAL_MORPH_*`): write `parameter_values[PAR_ROLL | PAR_ROLL_NOTE | PAR_ROLL_VELOCITY]` and call `menu_repaint()`. No echo back to STM.

---

## 5. Bug: no FM when the main oscillator is an imported sample

### 5.1 Root cause (confirmed)

`DSPAudio/Oscillator.c`, `calcUserSampleOscFmBlock()`:

```c
(void)modBuffer;
...
/* For Phase 2, route FM user samples through the non-FM long-index reader. */
calcUserSampleOscBlock(osc, buf, size, gain);
```

The sample-import Phase 2 work left FM as a stub. Sine, saw, tri and rec, plus the built-in `CRASH` sample, all apply `modBuffer[i] * osc->fmMod`. Imported samples (`waveform >= OSC_SAMPLE_START`) do not.

Affected: Drum 1-3 main osc in FM mode (`DrumVoice.c:272`), Cymbal main osc (`CymbalVoice.c:196`), and Hihat main osc (`HiHat.c:197`). Snare has no FM path. Drum mix mode (`mixOscs`) intentionally has no FM.

### 5.2 Fix: phase-modulate the sample read position

LXR "FM" is phase modulation: the modulator offsets the read index and does not change the increment. Imported samples get the same treatment.

Scaling: `osc_calcUserSampleFreq()` uses `freq2PhaseIncr32767()`, so a user sample advances **1024 frames per oscillator cycle** with a 17-bit fractional position. The wavetable offset of `mod*fmMod/8192` cycles equals `mod*fmMod/8` frames, which is `(mod*fmMod) << 14` in 17-bit fixed point. That is the `CRASH` path's shift, so `<< 14` keeps the modulation index identical to the periodic waveforms.

`calcUserSampleOscFmBlock()` becomes a copy of `calcUserSampleOscBlock()`'s loop with these changes:

1. Guards unchanged: sample index range, `sampleSize == 0`, `info.offset < SAMPLE_ROM_START_ADDRESS`, `!osc->sampleActive`.
2. Per frame, the read position comes from the **unmodulated** play head:
   ```c
   const int32_t fmOffset = (int32_t)(modBuffer[i] * osc->fmMod) << 14;   /* signed, 17-bit fixed */
   int64_t readFix = ((int64_t)pos << 17) + osc->sampleFraction + fmOffset;
   ```
   - Looped samples: wrap `readFix` into `[0, sampleSize << 17)`, handling negative values. Use compare-and-add/subtract, since `|fmOffset|` ≤ ~4096 frames.
   - One-shots: clamp to `[0, (sampleSize-1) << 17]`.
   - `readPos = readFix >> 17`, `frac = readFix & 0x1ffff`. Interpolate to `readPos+1` with the same `nextPos < sampleSize` guard, wrapping to 0 for loops.
   - Cast to **signed** `int32_t`. The `CRASH` path's `(uint32_t)(negative float)` is undefined behaviour; do not copy it.
3. Advance `samplePosition`/`sampleFraction` and the one-shot `sampleActive` gate exactly as the non-FM path does. FM must not change a one-shot's length or end detection.
4. Also set `osc->output = oscOut`.

With `fmMod == 0`, the output must be bit-identical to `calcUserSampleOscBlock()`.

Out of scope (latent): `calcNextOscSampleFm()` / `calcNextOscSample()` return 0 for imported samples. They are only reached through the waveform-interpolation helpers, which blend periodic waveforms only.

---

## 6. Hihat modulator sample re-arm

`HiHat.c` trigger re-arms only `hatVoice.osc`. A one-shot imported sample on `modOsc` or `modOsc2` plays once and then stays silent, so FM disappears after the first hit. `CymbalVoice.c:124-126` and `DrumVoice.c:178` already handle this.

Add after the main-osc block in the hat trigger:
```c
// Mod oscillators can also select imported samples; re-arm their read heads.
osc_resetSamplePlayback(&hatVoice.modOsc);
osc_resetSamplePlayback(&hatVoice.modOsc2);
```
Do **not** add the cymbal's modulator `phase = 0` resets. That would change the sound of existing periodic-waveform hats. `osc_resetSamplePlayback()` touches only sample-read state, so non-sample modulators are unaffected.

---

## 7. Files touched

| File | Change |
|---|---|
| `front/LxrAvr/Menu/MenuText.h` | 8 ¢ rows, `MENU_CHAR_CENT`, `MIDI_FILTER_CENT_FIRST`, `MIDI_FILTER_RX_ENTRIES` |
| `front/LxrAvr/Menu/menu.c` | encoder entry-count helper, pot guard, RX wire conversion in `menu_parseGlobalParam()` |
| `front/LxrAvr/avrComms/avrCommsReceivingProtocol.h/.c` | `SEQ_REPORT_ROLL_*` handlers |
| `mainboard/LxrStm32/src/MIDI/MidiParser.c` | RX bit 4 → `globalMidiParser_setCc38Swap()` |
| `mainboard/LxrStm32/src/MIDI/GlobalMidiParser.c/.h` | CC38 swap, NRPN 94/95/96, RPN-null swallow |
| `mainboard/LxrStm32/src/uARTFrontSYX/frontPanelReceivingProtocol.h` | `FRONT_SEQ_REPORT_ROLL_*` 0x71-0x73 |
| `mainboard/LxrStm32/src/uARTFrontSYX/frontPanelSendingProtocol.c/.h` | `frontPanelSending_sendRollParamReport()` |
| `mainboard/LxrStm32/src/DSPAudio/Oscillator.c` | real `calcUserSampleOscFmBlock()` |
| `mainboard/LxrStm32/src/DSPAudio/HiHat.c` | modulator sample re-arm |
| `knowledge_files/comms_spec_reference/MIDI_TABLE.md` | NRPN range `0..96`; rows 94-96; ¢ modes; CC38 and RPN-null notes |
| `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md` | RX-filter bit 4 on `SEQ_MIDI_FILT_RX`; three report sub-opcodes |
| `MEMORY.md` | ¢ modes, NRPN 94-96, FM-on-samples convention, session status |
| `manual/` (if RxFilter or NRPNs are documented there) | ¢ entries and NRPN 94-96 |
| `knowledge_files/log_archive/039_SESSION_HANDOFF_LOG.md`, `000_SESSION_INDEX.md` | session record |

## 8. Verification

Build. `make firmware` alone does not rebuild:
```bash
make -C mainboard/LxrStm32 -j8 stm32
make -C front/LxrAvr avr -j4
make firmware
```
Confirm `LxrStm32.bin` is embedded in `FIRMWARE.BIN`. Check AVR flash/RAM headroom: the table grows by 8×16 bytes of PROGMEM.

Host-side (cheap): copy the new `calcUserSampleOscFmBlock()` verbatim into a harness with a synthetic sample. Check that `fmMod = 0` is bit-identical to the non-FM reader, that one-shot end frame and `sampleActive` timing are unchanged under heavy FM, and that negative offsets near frame 0 clamp (one-shot) or wrap (loop) with no out-of-range reads.

Hardware:
1. **¢ glyph**: the RxFilter value shows `¢` (not `¥` or `\`). If it does not, switch to the CGRAM fallback.
2. **¢ UI**: pots on RxFilter only reach `off..all`. Click the encoder, and turning reaches `¢..al¢`. Click out, and the 4-parameter view keeps the ¢ name. Turning that pot does nothing while a ¢ entry is active. Re-enter encoder edit to return to a normal entry. TxFilter never shows ¢ entries. Save globals, power-cycle, and the ¢ entry is restored with CC38 still swapped.
3. **CC38 swap**: in `¢RN` (for example), an Elektron NRPN 93 changes roll rate and does **not** move Drum 1 cutoff. NRPN 96 moves Drum 1 cutoff. In `CRN`, behaviour is identical to today (CC38 moves cutoff, NRPN 96 is ignored).
4. **RPN null**: an NRPN followed by CC101/100 = 127 leaves Snare/Cymbal pan untouched. CC100/101 sent on their own still pan.
5. **Roll NRPNs**: NRPN 93/94/95 update the PERF page live. A held roll follows the rate (quantised latch) and the next roll hit uses the new note and velocity. A following encoder or pot move starts from the MIDI value.
6. **FM**: Drum 1 with osc = imported sample, FM mode, sweep FM amount. The timbre changes as with SIN. FM amount 0 sounds identical to the previous build. Repeat on Cymbal and Hihat main osc, with one-shot and looped samples.
7. **Hihat modulator**: hat modulator set to a one-shot sample. FM is present on every hit, not just the first. Periodic-waveform hat modulators sound unchanged.
8. Run the Session 038 checklist alongside (unchanged: NRPN 93 = roll rate).

## 9. Remaining confirmations

1. Name of the `all` twin: `al¢`?
2. Should NRPN 96 work only in ¢ modes (planned) or always?
3. Order of entries in encoder edit: after `all` (planned), or interleaved (`C`, `¢`, `CN`, `¢N`, …)? Interleaving needs a display-order map; the stored values stay the same.
