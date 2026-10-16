# S039 P1 — Implementation Schedule

Companion to `S039_FM_BUGFIX.md` (the plan). This file is the line-cited code schedule. **No code has been changed yet.**

- Branch `dev-roll-midi`, HEAD `7a21412`. Every line number below is against that HEAD.
- **Apply edits bottom-up within each file** (highest line number first) so the cited line numbers stay valid while editing. Each change also quotes an anchor so it can be found if lines have drifted.
- Every comment block below is meant to be pasted **as the in-source documentation**. Blocks follow the existing Session 038 `WHAT / WHY / INPUT / OUTPUT / ACCESSORS / AFFILIATES` style.
- Settled decisions (user, Session 039):
  - The `all` twin is named `al¢`.
  - NRPN 96 is active only in ¢ modes.
  - ¢ entries follow `all` in encoder order.
  - RPN-null swallowing applies in all RX modes.
  - Include the Hihat modulator re-arm.
  - NRPN 93 stays roll rate. No existing CC or NRPN assignment changes.

## 0. Hard constraint: AVR flash

The AVR bootloader is linked at `-Ttext=0xE000` (`front/LxrAvr_bootloader/default/Makefile:184`), so the application must stay **≤ 57,344 bytes**. The current `front/LxrAvr/LxrAvr.bin` is **56,682 bytes, leaving 662 bytes of headroom**. This schedule therefore:

- shrinks `midiFilterNames` rows from `[16]` to `[4]`. The table goes from 17×16 = 272 bytes to 25×4 = 100 bytes, a net saving of 172 bytes even with the 8 new rows;
- computes the ¢→bits mapping arithmetically instead of storing a table.

The AVR build gate in §6 must confirm `LxrAvr.bin` ≤ 57,344 bytes.

## Change index

| ID | File | Lines @HEAD | Action |
|---|---|---|---|
| D1 | `mainboard/LxrStm32/src/DSPAudio/Oscillator.c` | 781-799 | MODIFY (replace FM stub, add constants) |
| D2 | `mainboard/LxrStm32/src/DSPAudio/Oscillator.h` | 142-145 | MODIFY (prototype comment) |
| D3 | `mainboard/LxrStm32/src/DSPAudio/HiHat.c` | after 119 | ADD (modulator sample re-arm) |
| M1 | `mainboard/LxrStm32/src/MIDI/MidiParser.h` | 78-79, 143-145 | MODIFY + ADD (`MIDI_RX_FILTER_CC38_SWAP`) |
| M2 | `mainboard/LxrStm32/src/MIDI/MidiParser.c` | 1075-1105 | MODIFY (`midi_setFilter()`) |
| M3 | `mainboard/LxrStm32/src/MIDI/GlobalMidiParser.h` | after 20 | ADD (`globalMidiParser_setCc38Swap()` prototype) |
| M4 | `mainboard/LxrStm32/src/MIDI/GlobalMidiParser.c` | after 530 (EOF) | ADD (`globalMidiParser_setCc38Swap()`) |
| M5 | `mainboard/LxrStm32/src/MIDI/GlobalMidiParser.c` | 510 | MODIFY (doc block on `globalMidiParser_MIDIccHandler()`) |
| M6 | `mainboard/LxrStm32/src/MIDI/GlobalMidiParser.c` | 348-393 | REPLACE (`globalMidiParser_handleNrpnControl()`) |
| M7 | `mainboard/LxrStm32/src/MIDI/GlobalMidiParser.c` | after 346 | ADD (`globalMidiParser_applySpecialNrpn()`) |
| M8 | `mainboard/LxrStm32/src/MIDI/GlobalMidiParser.c` | 36-39 | REPLACE (NRPN defines + new state) |
| P1 | `mainboard/LxrStm32/src/uARTFrontSYX/frontPanelReceivingProtocol.h` | after 207 | ADD (`FRONT_SEQ_REPORT_ROLL_*`) |
| P2 | `mainboard/LxrStm32/src/uARTFrontSYX/frontPanelSendingProtocol.h` | after 95 | ADD (prototype) |
| P3 | `mainboard/LxrStm32/src/uARTFrontSYX/frontPanelSendingProtocol.c` | after 384 | ADD (`frontPanelSending_sendRollParamReport()`) |
| A1 | `front/LxrAvr/Menu/MenuText.h` | 242-261 | MODIFY (row width, 8 ¢ rows) |
| A2 | `front/LxrAvr/Menu/MenuText.h` | after 26 | ADD (¢ constants) |
| A3 | `front/LxrAvr/Menu/menu.c` | after 4220 | ADD (pot guard in `menu_parseKnobValue()`) |
| A4 | `front/LxrAvr/Menu/menu.c` | 4014-4016 | MODIFY (RX wire conversion in `menu_parseGlobalParam()`) |
| A5 | `front/LxrAvr/Menu/menu.c` | 2811-2815 | MODIFY (`menu_getDtypeMenuEntryCount()`) |
| A6 | `front/LxrAvr/avrComms/avrCommsReceivingProtocol.c` | after 529 | ADD (three report cases) |
| A7 | `front/LxrAvr/avrComms/avrCommsReceivingProtocol.h` | after 261 | ADD (`SEQ_REPORT_ROLL_*`) |
| K1-K6 | docs / manual / `MEMORY.md` | see §5 | MODIFY |

Recommended order: D (DSP, independent), then M + P (STM MIDI and protocol), then A (AVR), then K (docs), then §6 (verify). STM-only and AVR-only builds are each valid on their own. Older AVR firmware never sends bit 4 and never receives `0x71-0x73`, and the STM side tolerates its absence.

---

## 1. DSP: FM on imported samples + Hihat re-arm

### D1 — `DSPAudio/Oscillator.c` lines 781-799: MODIFY

Anchor: `void calcUserSampleOscFmBlock(OscInfo* osc,int16_t* modBuffer, int16_t* buf, uint8_t size ,const float gain)`.

**Remove** lines 782-799 (the whole current body: `(void)modBuffer;`, the index guard, the "For Phase 2 …" comment and the delegating `calcUserSampleOscBlock(osc, buf, size, gain);`).

**Add** in their place. Keep the `//----` separator at 781 above the constants:

```c
/* USER-SAMPLE FM SCALING CONSTANTS  (Session 039)
   WHAT: fixed-point scale and clamp for the imported-sample phase-modulation
         offset computed in calcUserSampleOscFmBlock().
   WHY:  osc_calcUserSampleFreq() uses freq2PhaseIncr32767(), so an imported
         sample advances 1024 frames per oscillator cycle with a 17-bit
         fractional read position. The wavetable FM path (calcFm*/
         calcFmBlock) offsets its 32-bit phase by (mod*fmMod)<<19, i.e.
         mod*fmMod/8192 cycles = mod*fmMod/8 frames = (mod*fmMod)<<14 in the
         17-bit domain. Scaling by 2^14 therefore gives imported samples the
         same modulation index as SINE/SAW/TRI/REC; the built-in CRASH path
         (calcSampleOscFmBlock) already uses the same <<14.
         The product clamp keeps product*SCALE inside int32_t
         (131071*16384 = 2^31 - 16384) even if a modulation route ever pushes
         fmMod above 1.0.
   INPUT/OUTPUT: compile-time constants only.
   ACCESSORS: calcUserSampleOscFmBlock() (this file) only.
   AFFILIATES: freq2PhaseIncr32767(), osc_calcUserSampleFreq(),
               calcSampleOscFmBlock() (this file). */
#define OSC_USER_SAMPLE_FM_SCALE        16384     /* 2^14: (mod*fmMod) -> 17-bit fixed frames */
#define OSC_USER_SAMPLE_FM_PRODUCT_MAX  131071    /* |mod*fmMod| clamp so product*SCALE fits int32_t */

/* IMPORTED-SAMPLE FM BLOCK RENDERER  (Session 039; replaces the Phase 2 stub)
   WHAT: renders `size` frames of the imported sample selected by
         osc->waveform - OSC_SAMPLE_START, phase-modulated by modBuffer.
         Each output frame is read at (unmodulated play head + FM offset);
         the play head itself advances exactly as calcUserSampleOscBlock()
         advances it.
   WHY:  the sample-import Phase 2 stub discarded modBuffer and delegated to
         the non-FM reader, so selecting an imported sample on any FM carrier
         (Drum 1-3 in FM mode, Cymbal, Hihat) silently removed FM. All LXR
         "FM" is phase modulation of the read index, and imported samples now
         follow that same model. Keeping the play-head advance unmodulated
         means FM cannot change a one-shot's length, end detection, or
         sampleActive timing, and with fmMod == 0 the output is bit-identical
         to calcUserSampleOscBlock().
         Bounds: one-shots clamp the modulated read to [0, size-1]; loops wrap
         it with one 32-bit modulo (hardware divide on Cortex-M4) only when
         the read leaves the sample, so a tiny loop with a large offset still
         costs O(1). Interpolation uses the same "nextPos < sampleSize" rule as
         the non-FM path (no wrap) so fmMod == 0 stays bit-identical.
   INPUT:  osc       - carrier oscillator: waveform, samplePosition,
                       sampleFraction (17-bit), sampleActive, phaseInc, fmMod.
           modBuffer - `size` modulator samples, already scaled by the
                       voice's modulator gain (e.g. fmModAmount).
           buf       - output buffer of `size` int16 frames.
           size      - frames to render (OUTPUT_DMA_SIZE in practice).
           gain      - output gain applied per frame.
   OUTPUT: buf filled with PCM, or zeros for an invalid slot, zero-length or
           unmapped sample, or a finished one-shot. osc->samplePosition,
           sampleFraction and sampleActive advance as in the non-FM path;
           osc->output holds the last rendered frame.
   ACCESSORS: calcNextOscSampleFmBlock() default branch (this file), reached
              from DrumVoice.c calcDrumVoiceSyncBlock() (FM mode),
              CymbalVoice.c and HiHat.c HiHat_calcSyncBlock().
   AFFILIATES: calcUserSampleOscBlock() (non-FM twin; keep the two loops in
               step), osc_resetSamplePlayback() (trigger re-arm),
               sampleMemory_getSampleInfo()/getSampleSizeFrames()/
               isSampleLooped() (SampleRom/SampleMemory.c). */
void calcUserSampleOscFmBlock(OscInfo* osc,int16_t* modBuffer, int16_t* buf, uint8_t size ,const float gain)
{
   const uint8_t sampleIndex = (uint8_t)(osc->waveform - OSC_SAMPLE_START);
   const uint8_t sampleCount = sampleMemory_getNumSamples();

   /* Guard first so a stale AVR waveform value such as s1 after only s0 loaded
      produces silence rather than reading past the SampleInfo table. */
   if(sampleIndex >= sampleCount)
   {
      memset(buf, 0, size * sizeof(int16_t));
      return;
   }

   SampleInfo info = sampleMemory_getSampleInfo(sampleIndex);
   const uint32_t sampleSize = sampleMemory_getSampleSizeFrames(info);
   const uint8_t looped = sampleMemory_isSampleLooped(info);
   int16_t* sampleData = (int16_t*)((int8_t*)(info.offset));

   /* Same metadata/one-shot guard as calcUserSampleOscBlock(). */
   if(sampleSize == 0u || info.offset < SAMPLE_ROM_START_ADDRESS || !osc->sampleActive)
   {
      memset(buf, 0, size * sizeof(int16_t));
      return;
   }

   uint8_t i;
   for(i = 0; i < size; i++)
   {
      uint32_t pos = osc->samplePosition;
      int16_t oscOut;

      if(pos >= sampleSize)
      {
         if(looped)
         {
            pos %= sampleSize;
         }
         else
         {
            /* One-shot finished: identical gate behaviour to the non-FM path. */
            osc->sampleActive = 0u;
            osc->output = 0;
            buf[i] = 0;
            continue;
         }
      }

      /* Signed phase-modulation offset in 17-bit fixed frames. Multiply, never
         left-shift, a negative value; the CRASH path's (uint32_t)(negative
         float) cast is undefined behaviour and must not be copied. */
      int32_t fmProduct = (int32_t)(modBuffer[i] * osc->fmMod);
      if(fmProduct > OSC_USER_SAMPLE_FM_PRODUCT_MAX)
         fmProduct = OSC_USER_SAMPLE_FM_PRODUCT_MAX;
      else if(fmProduct < -OSC_USER_SAMPLE_FM_PRODUCT_MAX)
         fmProduct = -OSC_USER_SAMPLE_FM_PRODUCT_MAX;
      const int32_t fmOffset = fmProduct * OSC_USER_SAMPLE_FM_SCALE;

      /* Split into floor(frames) + positive 17-bit fraction, then add to the
         unmodulated head. GCC/ARM >> on int32_t is arithmetic (floor). */
      uint32_t readFrac = (osc->sampleFraction & 0x1ffffu) + ((uint32_t)fmOffset & 0x1ffffu);
      int32_t readPos = (int32_t)pos + (fmOffset >> 17) + (int32_t)(readFrac >> 17);
      readFrac &= 0x1ffffu;

      if(looped)
      {
         const int32_t size32 = (int32_t)sampleSize;
         if(readPos >= size32 || readPos < 0)
         {
            readPos %= size32;
            if(readPos < 0)
               readPos += size32;
         }
      }
      else if(readPos < 0)
      {
         readPos = 0;
         readFrac = 0u;
      }
      else if(readPos >= (int32_t)sampleSize)
      {
         readPos = (int32_t)sampleSize - 1;
         readFrac = 0u;
      }

#if INTERPOLATE_OSC
      oscOut = sampleData[readPos];
      {
         const uint32_t nextPos = (uint32_t)readPos + 1u;
         if(nextPos < sampleSize)
         {
            const float frac = readFrac * 0.00000762939453125f; /* 1/0x20000 */
            oscOut += frac * (sampleData[nextPos] - oscOut);
         }
      }
#else
      oscOut = sampleData[readPos];
#endif

      /* Unmodulated play-head advance: must match calcUserSampleOscBlock(). */
      osc->sampleFraction += osc->phaseInc;
      osc->samplePosition += (osc->sampleFraction >> 17);
      osc->sampleFraction &= 0x1ffffu;

      if(looped && osc->samplePosition >= sampleSize)
      {
         osc->samplePosition %= sampleSize;
      }

      osc->output = oscOut;
      buf[i] = oscOut * gain;
   }
}
```

Notes for the implementer:
- `pos` is at most 262,143 frames (flash sectors 8-11), and `|fmOffset >> 17|` is at most 16,383, so every `int32_t` intermediate is safe.
- With `fmMod == 0`: `fmOffset == 0`, `readPos == pos`, and `readFrac == sampleFraction & 0x1ffff`. The output then equals the non-FM path. The host harness in §6 checks this.

### D2 — `DSPAudio/Oscillator.h` lines 142-145: MODIFY

**Remove** lines 142-144 (the "FM user-sample compatibility path … delegates to the non-FM long-index reader …" comment). **Keep** line 145 (the prototype). **Add** above it:

```c
/* IMPORTED-SAMPLE FM BLOCK RENDERER  (Session 039)
   WHAT: renders `size` frames of the imported sample at
         osc->waveform - OSC_SAMPLE_START, phase-modulated by modBuffer
         (read offset = modBuffer[i] * osc->fmMod, scaled by 2^14 into the
         17-bit fixed-point frame domain, the same modulation index as the
         wavetable and CRASH FM paths).
   WHY:  FM must work whatever the carrier's waveform source. The previous
         stub ignored modBuffer.
   INPUT:  osc (play head, fmMod, sampleActive), modBuffer[size], buf[size],
           size, gain.
   OUTPUT: buf filled (zeros when the slot is invalid or a one-shot has
           finished); play head and one-shot gate advance exactly as
           calcUserSampleOscBlock(); osc->output = last frame.
   ACCESSORS: calcNextOscSampleFmBlock() default branch.
   AFFILIATES: calcUserSampleOscBlock(), osc_resetSamplePlayback(),
               calcSampleOscFmBlock() (CRASH precedent). */
```

### D3 — `DSPAudio/HiHat.c` after line 119: ADD

Anchor: the closing `}` of the `else { hatVoice.osc.phase = 0; … osc_resetSamplePlayback(&hatVoice.osc); }` block in `HiHat_trigger()` (lines 115-119), immediately before `osc_setBaseNote(&hatVoice.osc,note);` (line 121).

```c

	/* HIHAT MODULATOR SAMPLE RE-ARM  (Session 039)
	   WHAT: re-arms the imported-sample read heads of both hat modulator
	         oscillators on every trigger.
	   WHY:  modOsc/modOsc2 can select imported samples (WAVE2_HH/WAVE3_HH).
	         Without a reset, a one-shot modulator plays once, clears its
	         sampleActive gate and then outputs zeros forever, so hat FM
	         silently disappears after the first hit. CymbalVoice.c and
	         DrumVoice.c already re-arm their modulators. Only sample-read state
	         is touched; modulator phase is deliberately NOT reset, so
	         periodic-waveform hat modulators sound exactly as before.
	   INPUT:  hatVoice.modOsc, hatVoice.modOsc2.
	   OUTPUT: samplePosition = 0, sampleFraction = 0, sampleActive = 1 on both.
	   ACCESSORS: HiHat_trigger() only.
	   AFFILIATES: osc_resetSamplePlayback() (Oscillator.c),
	               HiHat_calcSyncBlock() (consumes the modulators),
	               CymbalVoice.c trigger (same pattern). */
	osc_resetSamplePlayback(&hatVoice.modOsc);
	osc_resetSamplePlayback(&hatVoice.modOsc2);
```

---

## 2. STM MIDI: ¢ flag, CC38 swap, RPN null, roll NRPNs 94/95, NRPN 96

### M1 — `MIDI/MidiParser.h`

**(a) Lines 78-79: MODIFY.** Replace

```c
/* Per-direction MIDI filter bits. */
void midi_setFilter(uint8_t is_tx, uint8_t value);
```

with

```c
/* MIDI TX/RX FILTER SETTER
   WHAT: installs the TX (high nibble) or RX (low nibble) MIDI filter of
         midiParser_txRxFilter. Bit layout per nibble: 0x01 notes, 0x02
         realtime/clock, 0x04 CC echo/record, 0x08 program change.
         RX only: bit 0x10 (MIDI_RX_FILTER_CC38_SWAP) is the Session 039 ¢ flag.
         It is stripped before the nibble is stored and forwarded to
         globalMidiParser_setCc38Swap().
   WHY:  one front-panel opcode (FRONT_SEQ_MIDI_RX_FILTER) carries both the RX
         nibble and the ¢ flag, so ¢ entries need no new protocol traffic.
   INPUT:  is_tx - 1 for TX, 0 for RX; value - filter bits (RX may carry 0x10).
   OUTPUT: midiParser_txRxFilter updated; RX also updates the CC38 swap state
           and may release MIDI roll holds (note bit turning off).
   ACCESSORS: frontPanelReceivingProtocol.c FRONT_SEQ_MIDI_TX/RX_FILTER case.
   AFFILIATES: globalMidiParser_setCc38Swap() (GlobalMidiParser.c),
               midiParser_clearMidiRollHolds() (MidiParser.c),
               AVR menu_parseGlobalParam() PAR_MIDI_FILT_RX (sender). */
void midi_setFilter(uint8_t is_tx, uint8_t value);

/* RX FILTER ¢ FLAG  (Session 039)
   WHAT: bit 4 of the RX filter byte sent by the AVR. Set only by the ¢ twins
         of RxFilter (AVR menu values 16..23).
   WHY:  Elektron devices append CC38 (data-entry LSB) to every NRPN, which
         lands on Global CC38 = Drum 1 filter cutoff. While this flag is set,
         Global CC38 is consumed and NRPN 96 controls Drum 1 cutoff instead.
   ACCESSORS: midi_setFilter() (decode). The AVR encodes the same value as
              MIDI_FILTER_RX_CC38_SWAP in Menu/MenuText.h; keep them equal. */
#define MIDI_RX_FILTER_CC38_SWAP 0x10
```

**(b) Lines 143-144: MODIFY.** Replace the comment

```c
/* High nibble is TX, low nibble is RX. See midi_setFilter() for the
   bit layout. */
```

with

```c
/* MIDI FILTER STATE
   High nibble is TX, low nibble is RX; see midi_setFilter() for the bit
   layout. The RX ¢ flag (MIDI_RX_FILTER_CC38_SWAP) is NOT stored here; it
   lives in GlobalMidiParser.c (globalMidiParser_cc38Swap). Note that the RX
   0x04 bit gates only CC echo/automation recording: incoming CCs are applied
   regardless (pre-existing behaviour, unchanged). */
```

### M2 — `MIDI/MidiParser.c` lines 1075-1105: MODIFY `midi_setFilter()`

**(a) Line 1075: MODIFY.** Replace `/* Configure the TX/RX MIDI filter bitfields. */` with
`/* Configure the TX/RX MIDI filter bitfields; RX also decodes the ¢ flag. See MidiParser.h for the full contract. */`

**(b) Line 1093: MODIFY** inside the existing block. Replace
`INPUT:  value, the new RX nibble (low 4 bits significant).`
with
`INPUT:  value, the new RX nibble (low 4 bits) plus the optional ¢ flag in bit 4 (MIDI_RX_FILTER_CC38_SWAP), decoded below.`

**(c) After line 1100** (`midiParser_clearMidiRollHolds();`) and before line 1102: **ADD**

```c

      /* RX ¢ FLAG DECODE  (Session 039)
         WHAT: forwards bit 4 of the RX value to the Global parser as the CC38
               swap state, then lets the existing line below store only the
               low nibble.
         WHY:  the ¢ RxFilter entries are ordinary RX filters plus "CC38 off,
               NRPN 96 = Drum 1 cutoff"; the flag must not leak into
               midiParser_txRxFilter, whose high nibble is TX.
         INPUT:  value bit MIDI_RX_FILTER_CC38_SWAP.
         OUTPUT: globalMidiParser_cc38Swap set or cleared.
         ACCESSORS: this function (RX branch) only.
         AFFILIATES: globalMidiParser_setCc38Swap(),
                     globalMidiParser_handleNrpnControl() and
                     globalMidiParser_applySpecialNrpn() (GlobalMidiParser.c). */
      globalMidiParser_setCc38Swap((uint8_t)((value & MIDI_RX_FILTER_CC38_SWAP) != 0));
```

Line 1102 (`midiParser_txRxFilter = (value & 0x0F) | …`) is unchanged; it already masks bit 4 away. `GlobalMidiParser.h` is already included (line 47).

### M3 — `MIDI/GlobalMidiParser.h` after line 20: ADD

```c

/* GLOBAL CC38 SWAP (¢ RX-FILTER MODE) SETTER  (Session 039)
   WHAT: enables or disables the ¢ behaviour: while enabled, Global-channel
         CC38 is consumed as an NRPN data-entry LSB and NRPN 96 controls
         Drum 1 filter cutoff (PAR_FILTER_FREQ_1); while disabled, CC38 keeps
         its published Drum 1 cutoff mapping and NRPN 96 is ignored.
   WHY:  Elektron senders append CC38 to every NRPN, which otherwise corrupts
         Drum 1 cutoff (and the stored kit) on every NRPN send. Opt-in keeps
         the published Global CC38 mapping for everyone else.
   INPUT:  enabled - nonzero to enable.
   OUTPUT: internal GlobalMidiParser.c state only.
   ACCESSORS: midi_setFilter() RX branch (MidiParser.c).
   AFFILIATES: MIDI_RX_FILTER_CC38_SWAP (MidiParser.h),
               globalMidiParser_MIDIccHandler(), AVR RxFilter ¢ entries. */
void globalMidiParser_setCc38Swap(uint8_t enabled);
```

### M4 — `MIDI/GlobalMidiParser.c` after line 530 (end of file): ADD

```c

/* GLOBAL CC38 SWAP SETTER  (Session 039)
   WHAT: stores the ¢ RX-filter flag decoded by midi_setFilter().
   WHY:  single writer for globalMidiParser_cc38Swap; see GlobalMidiParser.h.
   INPUT:  enabled - nonzero enables the swap.
   OUTPUT: globalMidiParser_cc38Swap = 0/1.
   ACCESSORS: midi_setFilter() (MidiParser.c).
   AFFILIATES: globalMidiParser_handleNrpnControl(),
               globalMidiParser_applySpecialNrpn() (readers). */
void globalMidiParser_setCc38Swap(uint8_t enabled)
{
   globalMidiParser_cc38Swap = enabled ? 1u : 0u;
}
```

### M5 — `MIDI/GlobalMidiParser.c` line 510: MODIFY (add doc block only)

Before `void globalMidiParser_MIDIccHandler(MidiMsg msg, uint8_t updateOriginalValue)`, **ADD**:

```c
/* GLOBAL-CHANNEL CC ROUTER
   WHAT: handles CC2-127 on the configured Global MIDI channel. NRPN protocol
         bytes (CC6/98/99, ¢-mode CC38, RPN-null CC100/101 = 127) are offered
         to globalMidiParser_handleNrpnControl() first; anything it does not
         consume is applied through the fixed legacy Global CC table.
   WHY:  NRPN protocol bytes must be consumed before the CC table, or they
         land on published sound parameters (CC38 = Drum 1 cutoff,
         CC100/101 = Snare/Cymbal pan).
   INPUT:  msg - MIDI_CC; updateOriginalValue - 1 to store/record/echo.
   OUTPUT: live DSP apply, Preset raw storage, optional echo/record.
   ACCESSORS: midiParser_MIDIccHandler() (MidiParser.c).
   AFFILIATES: globalMidiParser_handleNrpnControl(),
               globalMidiParser_applyRawParameter(), MIDI_TABLE.md. */
```

The function body is unchanged.

### M6 — `MIDI/GlobalMidiParser.c` lines 348-393: REPLACE `globalMidiParser_handleNrpnControl()`

**Remove** lines 348-393 entirely. **Add**:

```c
/* GLOBAL NRPN PROTOCOL HANDLER
   WHAT: consumes NRPN protocol CCs on the Global channel and applies the
         selected NRPN on data entry:
           CC99/CC98  select the NRPN number (MSB/LSB) and clear the
                      RPN-null window;
           CC6        applies the selected NRPN: special numbers first
                      (globalMidiParser_applySpecialNrpn(): 93-95 roll,
                      96 Drum 1 cutoff in ¢ mode), then the raw table 0..92;
                      opens the RPN-null window;
           CC38       consumed only in ¢ mode (data-entry LSB);
           CC100/101  value 127 inside the RPN-null window is the standard
                      NRPN deselect: consumed, clears nrpnSelected.
         Every other CC closes the RPN-null window and returns 0 so the
         caller applies it through the Global CC table.
   WHY:  without consumption, standard NRPN traffic writes published sound
         parameters: Elektron CC38 -> Drum 1 cutoff (¢ mode fixes it),
         RPN-null CC101/100 -> Cymbal/Snare hard pan (fixed in all modes).
         The window keeps deliberate CC100/101 pan moves working; only a
         127 sent immediately after an NRPN data entry is treated as a null.
   INPUT:  msg - Global-channel MIDI_CC; updateOriginalValue - forwarded to
           the apply helpers.
   OUTPUT: 1 if the CC was consumed here, 0 if the caller must apply it as
           an ordinary Global CC. Updates globalMidiParser_activeNrpnNumber,
           _nrpnSelected and _nrpnDataPending.
   ACCESSORS: globalMidiParser_MIDIccHandler() only.
   AFFILIATES: globalMidiParser_applySpecialNrpn(),
               globalMidiParser_applyRawParameter(),
               globalMidiParser_nrpnToRawParam[], globalMidiParser_cc38Swap,
               MIDI_TABLE.md "Global MIDI NRPN Table". */
static uint8_t globalMidiParser_handleNrpnControl(MidiMsg msg,
                                                  uint8_t updateOriginalValue)
{
   switch(msg.data1)
   {
      case NRPN_FINE:          /* CC98 */
         globalMidiParser_activeNrpnNumber &= (uint16_t)~0x7f;
         globalMidiParser_activeNrpnNumber |= (uint16_t)(msg.data2 & 0x7f);
         globalMidiParser_nrpnSelected = 1;
         globalMidiParser_nrpnDataPending = 0;
         return 1;

      case NRPN_COARSE:        /* CC99 */
         globalMidiParser_activeNrpnNumber &= 0x7f;
         globalMidiParser_activeNrpnNumber |= (uint16_t)(msg.data2 << 7);
         globalMidiParser_nrpnSelected = 1;
         globalMidiParser_nrpnDataPending = 0;
         return 1;

      case NRPN_DATA_ENTRY_COARSE:   /* CC6 */
         /* A data entry opens the RPN-null window only if it addressed a
            selected NRPN; CC6 before any selector stays ignored. */
         globalMidiParser_nrpnDataPending = globalMidiParser_nrpnSelected;

         if(!globalMidiParser_nrpnSelected)
            return 1;

         if(globalMidiParser_applySpecialNrpn(globalMidiParser_activeNrpnNumber,
                                              msg.data2,
                                              updateOriginalValue,
                                              msg.bits.source))
            return 1;

         if(globalMidiParser_activeNrpnNumber
               < (sizeof(globalMidiParser_nrpnToRawParam)
                  / sizeof(globalMidiParser_nrpnToRawParam[0])))
         {
            globalMidiParser_applyRawParameter(
               globalMidiParser_nrpnToRawParam[globalMidiParser_activeNrpnNumber],
               msg.data2,
               updateOriginalValue,
               msg.bits.source);
         }
         return 1;

      case DATA_MSB_LSB:       /* CC38: data-entry LSB */
         /* ¢ mode: consume. The RPN-null window is left as-is because an
            Elektron sender emits CC38 between CC6 and any null. */
         if(globalMidiParser_cc38Swap)
            return 1;
         globalMidiParser_nrpnDataPending = 0;
         return 0;            /* published Drum 1 cutoff mapping */

      case RPN_LSB:            /* CC100 */
      case RPN_MSB:            /* CC101 */
         if(globalMidiParser_nrpnDataPending
            && msg.data2 == GLOBAL_MIDI_RPN_NULL_VALUE)
         {
            /* Standard deselect: a later CC6 must not re-apply the NRPN.
               The window stays open so the paired null byte is consumed too. */
            globalMidiParser_nrpnSelected = 0;
            return 1;
         }
         globalMidiParser_nrpnDataPending = 0;
         return 0;            /* published Snare/Cymbal pan mapping */

      default:
         globalMidiParser_nrpnDataPending = 0;
         return 0;
   }
}
```

Name check: `DATA_MSB_LSB` (38), `RPN_LSB` (100) and `RPN_MSB` (101) are the external-CC enum constants in `MidiMessages.h:141/203/204`. `NRPN_FINE` (98), `NRPN_COARSE` (99) and `NRPN_DATA_ENTRY_COARSE` (6) are the existing LXR-enum labels. All six values are distinct, so no duplicate `case` can occur. No other definitions of these names exist in `src/` or `Libraries/`.

### M7 — `MIDI/GlobalMidiParser.c` after line 346: ADD `globalMidiParser_applySpecialNrpn()`

Anchor: the closing `}` of `globalMidiParser_applyRawParameter()` (line 346). The helper must sit after `globalMidiParser_applyRawParameter()` and before `globalMidiParser_handleNrpnControl()`.

```c

/* GLOBAL SPECIAL-NRPN APPLY  (Session 039)
   WHAT: applies the Global NRPN numbers that bypass
         globalMidiParser_nrpnToRawParam[]:
           93 roll rate      -> seq_setRollRate(clamp 0..15)  + AVR report
           94 roll note      -> seq_setRollNote(0..127)       + AVR report
           95 roll velocity  -> seq_setRollVelocity(0..127)   + AVR report
           96 Drum 1 cutoff  -> PAR_FILTER_FREQ_1, ¢ mode only (otherwise
                                not handled, so the caller's range check
                                ignores it as before)
   WHY:  roll rate/note/velocity are runtime PERF controls, not raw sound
         parameters, so they cannot live in the raw table; they are not
         recorded as automation or echoed (unchanged NRPN 93 behaviour).
         Each roll change is reported to the AVR so the PERF page shows the
         MIDI value; otherwise the next encoder/pot move would resend the
         stale menu value and overwrite it. Reports are sent on every data
         entry (no change cache): the front panel can also set these values,
         so an STM-side cache could go stale and suppress a needed report.
         NRPN 96 reuses globalMidiParser_applyRawParameter(), the exact
         store/apply/echo/record path Global CC38 uses, so ¢ mode only moves
         the transport, never the semantics.
   INPUT:  nrpn - selected NRPN number; value - CC6 data (0..127);
           updateOriginalValue, source - forwarded for NRPN 96.
   OUTPUT: 1 if handled (caller returns), 0 if not a special NRPN (or NRPN
           96 outside ¢ mode).
   ACCESSORS: globalMidiParser_handleNrpnControl() (CC6 case) only.
   AFFILIATES: seq_setRollRate()/seq_setRollNote()/seq_setRollVelocity()
               (Sequencer/sequencer.c), frontPanelSending_sendRollParamReport()
               (uARTFrontSYX/frontPanelSendingProtocol.c), AVR
               SEQ_REPORT_ROLL_* handlers (avrCommsReceivingProtocol.c),
               globalMidiParser_cc38Swap. */
static uint8_t globalMidiParser_applySpecialNrpn(uint16_t nrpn,
                                                 uint8_t value,
                                                 uint8_t updateOriginalValue,
                                                 enum MidiSource source)
{
   switch(nrpn)
   {
      case GLOBAL_MIDI_NRPN_ROLL_RATE:
         if(value > GLOBAL_MIDI_ROLL_RATE_MAX)
            value = GLOBAL_MIDI_ROLL_RATE_MAX;
         seq_setRollRate(value);
         frontPanelSending_sendRollParamReport(FRONT_SEQ_REPORT_ROLL_RATE, value);
         return 1;

      case GLOBAL_MIDI_NRPN_ROLL_NOTE:
         seq_setRollNote(value);
         frontPanelSending_sendRollParamReport(FRONT_SEQ_REPORT_ROLL_NOTE, value);
         return 1;

      case GLOBAL_MIDI_NRPN_ROLL_VELOCITY:
         seq_setRollVelocity(value);
         frontPanelSending_sendRollParamReport(FRONT_SEQ_REPORT_ROLL_VELOCITY, value);
         return 1;

      case GLOBAL_MIDI_NRPN_FILTER_FREQ_1:
         if(!globalMidiParser_cc38Swap)
            return 0;
         globalMidiParser_applyRawParameter(PAR_FILTER_FREQ_1,
                                            value,
                                            updateOriginalValue,
                                            source);
         return 1;

      default:
         return 0;
   }
}
```

`enum MidiSource` is the type of `msg.bits.source`, as already used by `globalMidiParser_applyRawParameter()`'s signature. `sequencer.h` and `frontPanelSendingProtocol.h` are already included (lines 17-19).

### M8 — `MIDI/GlobalMidiParser.c` lines 36-39: REPLACE

**Remove** lines 36-39:

```c
/* Global NRPN 93 is a performance roll-rate command. It intentionally bypasses
   the raw sound-parameter table and preserves every existing Global CC map. */
#define GLOBAL_MIDI_NRPN_ROLL_RATE 93
#define GLOBAL_MIDI_ROLL_RATE_MAX 15
```

**Add** in their place. Line 40 `#define GLOBAL_MIDI_RAW_UNMAPPED PAR_NONE` stays.

```c
/* GLOBAL SPECIAL NRPN NUMBERS  (Session 038 / Session 039)
   WHAT: NRPN numbers handled by globalMidiParser_applySpecialNrpn() instead
         of the raw table (which covers 0..92).
         93 roll rate (0..15), 94 roll note, 95 roll velocity: PERF controls.
         96 Drum 1 filter cutoff, active only in ¢ RX-filter mode, where it
            replaces Global CC38.
   WHY:  these are not raw sound-parameter slots; every existing Global CC
         and NRPN assignment is preserved.
   ACCESSORS: globalMidiParser_applySpecialNrpn().
   AFFILIATES: MIDI_TABLE.md, manual/latex/midi_tables.tex. */
#define GLOBAL_MIDI_NRPN_ROLL_RATE      93
#define GLOBAL_MIDI_NRPN_ROLL_NOTE      94
#define GLOBAL_MIDI_NRPN_ROLL_VELOCITY  95
#define GLOBAL_MIDI_NRPN_FILTER_FREQ_1  96
#define GLOBAL_MIDI_ROLL_RATE_MAX       15
/* Data value that, on CC100/CC101 right after an NRPN data entry, forms the
   standard "RPN null" deselect. */
#define GLOBAL_MIDI_RPN_NULL_VALUE      127

/* ¢ RX-FILTER STATE  (Session 039)
   WHAT: 1 while the AVR RxFilter is one of the ¢ entries.
   WHY:  gates CC38 consumption and NRPN 96 (see globalMidiParser_setCc38Swap()).
   WRITER: globalMidiParser_setCc38Swap() only (from midi_setFilter()).
   READERS: globalMidiParser_handleNrpnControl(),
            globalMidiParser_applySpecialNrpn(). Default 0 = published
            behaviour until the AVR sends its globals at boot. */
static uint8_t globalMidiParser_cc38Swap = 0;

/* RPN-NULL WINDOW  (Session 039)
   WHAT: 1 from an NRPN CC6 data entry (with a selected NRPN) until the next
         Global CC that is not ¢-consumed CC38 or a null byte.
   WHY:  lets CC100/101 = 127 be swallowed as an NRPN deselect only when it
         directly follows NRPN traffic, so deliberate Snare/Cymbal pan moves
         on CC100/101 keep working.
   WRITER/READER: globalMidiParser_handleNrpnControl() only. */
static uint8_t globalMidiParser_nrpnDataPending = 0;
```

---

## 3. STM → AVR roll report protocol

### P1 — `uARTFrontSYX/frontPanelReceivingProtocol.h` after line 207: ADD

Anchor: `#define FRONT_SEQ_OSC_WAVE_INTERPOLATION 0x70`.

```c
/* PERF ROLL DISPLAY REPORTS, STM -> AVR  (Session 039)
   WHAT: FRONT_SEQ_CC sub-opcodes reporting a roll value changed by external
         MIDI (Global NRPN 93/94/95). data2 = new value: rate 0..15 (the same
         selector index as PAR_ROLL / FRONT_SEQ_ROLL_RATE), note 0..127,
         velocity 0..127.
   WHY:  display-only sync so the AVR PERF page matches STM. Distinct names
         from FRONT_SEQ_ROLL_RATE/_NOTE/_VELOCITY because protocol direction
         matters; those are AVR -> STM. PARAM_CC2 cannot be used: it cannot
         address PAR_ROLL_NOTE (270) and its AVR handler writes parameters2[],
         which is only END_OF_SOUND_PARAMETERS long.
   ACCESSORS: frontPanelSending_sendRollParamReport() (sender).
   AFFILIATES: AVR SEQ_REPORT_ROLL_* (avrCommsReceivingProtocol.h, same values). */
#define FRONT_SEQ_REPORT_ROLL_RATE       0x71	// report roll rate (0..15) back to AVR
#define FRONT_SEQ_REPORT_ROLL_NOTE       0x72	// report roll note back to AVR
#define FRONT_SEQ_REPORT_ROLL_VELOCITY   0x73	// report roll velocity back to AVR
```

### P2 — `uARTFrontSYX/frontPanelSendingProtocol.h` after line 95: ADD

Anchor: `void frontPanelSending_sendGlobalMorphRuntimeReport(uint8_t amount);`.

```c
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
```

### P3 — `uARTFrontSYX/frontPanelSendingProtocol.c` after line 384: ADD

Anchor: the closing `}` of `frontPanelSending_sendGlobalMorphRuntimeReport()`.

```c

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
```

---

## 4. AVR: ¢ menu entries, encoder-only access, wire encoding, roll reports

### A1 — `Menu/MenuText.h` lines 242-261: MODIFY `midiFilterNames`

**Modify** line 242: `const char midiFilterNames[][16] PROGMEM =` becomes `const char midiFilterNames[][4] PROGMEM =`. Every existing name is at most 3 characters, and `getMenuItemNameForValue()` copies exactly 3 bytes (`memcpy_P(buf,p,3)`, `menu.c:2690`). `getMaxEntriesForMenu()` reads only `[0][0]`. No other code indexes this table (`menu.c:2606`, `2675` only).

**Modify** the line 244 comment `{16},	// number of entries` to `{16},	// pot/TX entry count; RX encoder also reaches the ¢ rows below (MIDI_FILTER_RX_ENTRIES)`. **The value stays 16.**

**Add** after line 260 (`{"all"},`), before `};`:

```c
	/* ¢ RX TWINS  (Session 039)
	   WHAT: values 16..23 of PAR_MIDI_FILT_RX. Each is identical to the
	         C-entry it shadows (C, CN, CR, CRN, PC, PCN, PCR, all) plus the
	         CC38 swap: Global CC38 ignored, NRPN 96 = Drum 1 filter cutoff.
	   WHY:  opt-in workaround for Elektron NRPN senders, which append CC38.
	         Rows sit after the count byte's range, so pots and TxFilter
	         (which use getMaxEntriesForMenu() = 16) can never reach them; only
	         the RX encoder edit path (menu_getDtypeMenuEntryCount()) can.
	   ORDER IS ABI: value 16+k must keep RX bits
	         0x04 | (k & 3) | ((k & 4) << 1), as encoded in menu_parseGlobalParam().
	   ACCESSORS: getMenuItemNameForValue() (display only). */
	{MENU_CHAR_CENT},
	{MENU_CHAR_CENT "N"},
	{MENU_CHAR_CENT "R"},
	{MENU_CHAR_CENT "RN"},
	{"P" MENU_CHAR_CENT},
	{"P" MENU_CHAR_CENT "N"},
	{"P" MENU_CHAR_CENT "R"},
	{"al" MENU_CHAR_CENT},
```

### A2 — `Menu/MenuText.h` after line 26: ADD

Anchor: `#define MENU_MIDI_FILTERING 13`. Place after it so the macros exist before `midiFilterNames` (line 242).

```c

/* ¢ RX-FILTER CONSTANTS  (Session 039)
   MENU_CHAR_CENT: the cent sign on this LCD's character ROM is code 0x5C
     (between '[' 0x5B and ']' 0x5D), written as the C escape "\\". If a
     display variant renders 0x5C as '¥' or '\', define a CGRAM glyph with
     lcd_generatechar() (Hardware/lcd.c) in slot 1 and change this to "\x01";
     slot 0 cannot be used in strings.
   MIDI_FILTER_CENT_FIRST: first ¢ value of PAR_MIDI_FILT_RX (16).
   MIDI_FILTER_RX_ENTRIES: RX encoder range, 16 normal + 8 ¢ = 24.
   MIDI_FILTER_RX_CC38_SWAP: wire bit 4 sent with SEQ_MIDI_FILT_RX for ¢
     values; must equal STM MIDI_RX_FILTER_CC38_SWAP (MidiParser.h).
   ACCESSORS: midiFilterNames[], menu_getDtypeMenuEntryCount(),
              menu_parseKnobValue(), menu_parseGlobalParam() (menu.c). */
#define MENU_CHAR_CENT            "\\"
#define MIDI_FILTER_CENT_FIRST    16
#define MIDI_FILTER_RX_ENTRIES    24
#define MIDI_FILTER_RX_CC38_SWAP  0x10
```

### A3 — `Menu/menu.c` after line 4220: ADD (pot guard in `menu_parseKnobValue()`)

Anchor: after `uint16_t paramNr = pgm_read_word(&menuPages[menu_activePage][activePage].bot1 + potNr + isOn2ndPage);` (lines 4219-4220), before the `//parameter fetch.` comment (line 4222).

```c

	/* ¢ RX-FILTER POT LOCK  (Session 039)
	   WHAT: ignores pot movement on RxFilter while a ¢ entry (value >= 16) is
	         selected.
	   WHY:  ¢ entries are encoder-only: they can be selected and left only in
	         encoder edit mode. Pots already cannot reach them (getDtypeValue()
	         scales over 16 entries); without this guard a pot nudge would
	         silently drop the CC38 workaround. The value stays visible in the
	         4-parameter view.
	   INPUT:  paramNr, parameter_values[PAR_MIDI_FILT_RX].
	   OUTPUT: early return (no value change, no STM traffic).
	   ACCESSORS: this function only.
	   AFFILIATES: menu_getDtypeMenuEntryCount() (encoder range),
	               menu_parseGlobalParam() PAR_MIDI_FILT_RX. */
	if(paramNr == PAR_MIDI_FILT_RX
	   && parameter_values[PAR_MIDI_FILT_RX] >= MIDI_FILTER_CENT_FIRST)
		return;
```

`editModeActive = 0;` at line 4214 still runs first, so touching any pot leaves encoder edit mode as before.

### A4 — `Menu/menu.c` lines 4014-4016: MODIFY (`menu_parseGlobalParam()`)

**Remove**:

```c
	case PAR_MIDI_FILT_RX:
		avrComms_sendData(SEQ_CC, SEQ_MIDI_FILT_RX, value);
		break;
```

**Add**:

```c
	/* RX FILTER WIRE ENCODING  (Session 039)
	   WHAT: converts the RxFilter menu index into the byte STM expects:
	         0..15  -> sent unchanged (RX nibble: N 0x01, R 0x02, C 0x04, P 0x08);
	         16..23 -> ¢ twin k = value-16: RX bits 0x04|(k&3)|((k&4)<<1)
	                   (C, CN, CR, CRN, PC, PCN, PCR, all) plus
	                   MIDI_FILTER_RX_CC38_SWAP;
	         >= 24  -> corrupt (e.g. global file): repaired to "all" (15) in
	                   parameter_values and sent as 15.
	   WHY:  this is the single AVR->STM send point for RxFilter (encoder,
	         pot, global-file load via menu_sendAllGlobals(), automation
	         restore in buttonHandler.c), so encoding here covers every path.
	         Arithmetic instead of a PROGMEM table to save AVR flash (0xE000
	         bootloader limit).
	   INPUT:  value - menu index 0..23 (or a corrupt value).
	   OUTPUT: SEQ_CC / SEQ_MIDI_FILT_RX / encoded byte (7-bit safe, <= 0x1F).
	   ACCESSORS: menu_encoderChangeParameter(), menu_encoderChangeShiftParameter(),
	              menu_parseKnobValue(), menu_sendAllGlobals(),
	              buttonHandler.c original-value restore.
	   AFFILIATES: STM midi_setFilter() / MIDI_RX_FILTER_CC38_SWAP (MidiParser),
	               midiFilterNames[] ¢ rows (MenuText.h). */
	case PAR_MIDI_FILT_RX:
	{
		uint8_t wire = value;

		if(value >= MIDI_FILTER_RX_ENTRIES)
		{
			wire = 0x0F;
			parameter_values[PAR_MIDI_FILT_RX] = wire;
		}
		else if(value >= MIDI_FILTER_CENT_FIRST)
		{
			const uint8_t k = (uint8_t)(value - MIDI_FILTER_CENT_FIRST);
			wire = (uint8_t)(0x04 | (k & 0x03) | ((k & 0x04) << 1)
			                 | MIDI_FILTER_RX_CC38_SWAP);
		}
		avrComms_sendData(SEQ_CC, SEQ_MIDI_FILT_RX, wire);
	}
		break;
```

The `case PAR_MIDI_FILT_TX` at 4011-4013 is unchanged.

### A5 — `Menu/menu.c` lines 2811-2815: MODIFY `menu_getDtypeMenuEntryCount()`

**Remove** lines 2811-2815. **Add**:

```c
/* ENCODER DTYPE_MENU ENTRY COUNT
   WHAT: number of selectable entries for a DTYPE_MENU parameter on the
         encoder edit path. Normally the menu table's count byte; RxFilter
         returns MIDI_FILTER_RX_ENTRIES (24) so the ¢ twins (16..23) are
         reachable (Session 039).
   WHY:  ¢ entries must be encoder-only. This helper is used ONLY by the
         encoder paths (menu_normalizeDtypeMenuEncoderValue() and the
         DTYPE_MENU clamps in menu_encoderChangeParameter() /
         menu_encoderChangeShiftParameter()); pots use getMaxEntriesForMenu()
         via getDtypeValue() and stay at 16. Without this override the
         normaliser would rescale any value >= 16 down and the clamp would
         cap it at 15. TxFilter shares the table but is not overridden, so it
         stays 0..15.
   INPUT:  paramNr - DTYPE_MENU parameter.
   OUTPUT: entry count (0 if the menu is unknown).
   ACCESSORS: menu_normalizeDtypeMenuEncoderValue() (2820),
              menu_encoderChangeParameter() (3058),
              menu_encoderChangeShiftParameter() (3291).
   AFFILIATES: getMaxEntriesForMenu(), midiFilterNames[] (MenuText.h). */
static uint8_t menu_getDtypeMenuEntryCount(uint16_t paramNr)
{
   const uint8_t menuId = pgm_read_byte(&parameter_dtypes[paramNr]) >> 4;

   if(paramNr == PAR_MIDI_FILT_RX)
      return MIDI_FILTER_RX_ENTRIES;

   return getMaxEntriesForMenu(menuId);
}
```

Encoder ordering result: `off … all` (0..15), then `¢ ¢N ¢R ¢RN P¢ P¢N P¢R al¢` (16..23). Display already works through `getMenuItemNameForValue()` (`menu.c:2674-2676`, index `curParmVal+1`) in both the edit line and the 4-parameter view.

### A6 — `avrComms/avrCommsReceivingProtocol.c` after line 529: ADD

Anchor: the closing `}` of `case SEQ_REPORT_GLOBAL_MORPH_MSB: { … break; }` (lines 519-529), inside `switch(avrCommsParser_command.data1)` under `status == SEQ_CC` (line 497-499). Insert before `case SEQ_SET_PAT_BEAT:` (line 531).

```c

                  /* PERF ROLL DISPLAY REPORTS, STM -> AVR  (Session 039)
                     WHAT: STM reports a roll value changed by external MIDI
                           (Global NRPN 93 rate 0..15, 94 note, 95 velocity);
                           store it in the PERF page slot and repaint.
                     WHY:  keeps PAR_ROLL/PAR_ROLL_NOTE/PAR_ROLL_VELOCITY
                           equal to the STM value, so a later encoder/pot move
                           starts from it instead of resending a stale value.
                           Display-only: no echo back to STM (STM already
                           holds the value), and these runtime-only params are
                           never saved, so no file/snapshot state is touched.
                     INPUT:  avrCommsParser_command.data2 - new value.
                     OUTPUT: parameter_values[PAR_ROLL*] updated; menu_repaint().
                     ACCESSORS: this parser only.
                     AFFILIATES: STM frontPanelSending_sendRollParamReport(),
                                 globalMidiParser_applySpecialNrpn(); menu.c
                                 PAR_ROLL/_NOTE/_VELOCITY send cases (AVR->STM). */
                  case SEQ_REPORT_ROLL_RATE:
                     parameter_values[PAR_ROLL] = avrCommsParser_command.data2;
                     menu_repaint();
                     break;

                  case SEQ_REPORT_ROLL_NOTE:
                     parameter_values[PAR_ROLL_NOTE] = avrCommsParser_command.data2;
                     menu_repaint();
                     break;

                  case SEQ_REPORT_ROLL_VELOCITY:
                     parameter_values[PAR_ROLL_VELOCITY] = avrCommsParser_command.data2;
                     menu_repaint();
                     break;
```

### A7 — `avrComms/avrCommsReceivingProtocol.h` after line 261: ADD

Anchor: `#define SEQ_OSC_WAVE_INTERPOLATION 0x70 // enable/disable waveform interpolation`.

```c
/* PERF ROLL DISPLAY REPORTS, STM -> AVR  (Session 039)
   WHAT: SEQ_CC sub-opcodes carrying a roll value changed by external MIDI
         (Global NRPN 93/94/95); data2 = rate 0..15 / note 0..127 /
         velocity 0..127.
   WHY:  display sync for the PERF page. Distinct from SEQ_ROLL_RATE (0x11),
         SEQ_ROLL_NOTE (0x40) and SEQ_ROLL_VELOCITY (0x41), which are AVR -> STM.
   ACCESSORS: avrCommsReceivingProtocol.c SEQ_CC switch.
   AFFILIATES: STM FRONT_SEQ_REPORT_ROLL_* (frontPanelReceivingProtocol.h,
               same values). */
#define SEQ_REPORT_ROLL_RATE       0x71	// STM reports roll rate (0..15)
#define SEQ_REPORT_ROLL_NOTE       0x72	// STM reports roll note
#define SEQ_REPORT_ROLL_VELOCITY   0x73	// STM reports roll velocity
```

---

## 5. Documentation

### K1 — `knowledge_files/comms_spec_reference/MIDI_TABLE.md`
- **Line 211** (CC 38 row): MODIFY the function cell to `Filter frequency (ignored in ¢ RX-filter modes; use NRPN 96)`.
- **Lines 273-274** (CC 100/101 rows): append `; value 127 directly after an NRPN data entry is consumed as RPN-null`.
- **Line 310**: MODIFY to: `Valid Global NRPN numbers are 0..95, plus 96 in ¢ RX-filter modes. CC6 before a selector is ignored. Out-of-range selected NRPN numbers are ignored. CC100/CC101 = 127 directly after a data entry deselects the NRPN (RPN null) and is not applied as pan. In ¢ modes CC38 (data-entry LSB) is consumed.`
- **After line 407**: ADD rows:
  ```
  | 94 | Roll note | Global performance | `seq_setRollNote(value)`; reported to AVR PERF page |
  | 95 | Roll velocity | Global performance | `seq_setRollVelocity(value)`; reported to AVR PERF page |
  | 96 | Filter frequency (¢ RX-filter modes only) | Drum 1 | `PAR_FILTER_FREQ_1` (replaces Global CC38) |
  ```
  Also append `; reported to AVR PERF page` to row 93.
- ADD a short `## RX Filter ¢ Modes` section at the end. It covers the value table (16..23 → bits, twin names), "encoder-only, pot-locked", and the wire bit `0x10`.

### K2 — `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md`
- **After line 195** (§1 bullets): ADD `` - `SEQ_CC` / `SEQ_MIDI_FILT_RX` (`0x31`) - RX filter nibble; bit 4 (`0x10`, Session 039) = ¢ mode (Global CC38 consumed, NRPN 96 = Drum 1 cutoff). STM strips it in `midi_setFilter()` and stores it in `GlobalMidiParser.c`. ``
- **After line 242** (§2 list): ADD `` - `SEQ_REPORT_ROLL_RATE` / `_NOTE` / `_VELOCITY` (`0x71-0x73`, STM -> AVR, Session 039) ``. After line 252, ADD one sentence: roll reports are display-only, sent on every Global NRPN 93-95 data entry, and the AVR does not echo them.
- **Line 4** (status line): prepend a Session 039 sentence summarising ¢ modes, NRPN 94-96, and the roll reports.

### K3 — `manual/latex/midi_tables.tex`
- **After line 242** (`92 & Audio output routing & 6 \\`): ADD
  ```
  93  & Roll rate                  & -- \\
  94  & Roll note                  & -- \\
  95  & Roll velocity              & -- \\
  96  & Filter frequency (rxf \textcent{} modes only) & 1 \\
  ```
  Confirm that `\textcent` builds with `manual/latex/preamble.tex`; it needs `textcomp`, which is in the LaTeX kernel since 2020. Otherwise write `cent modes`.
- **Line 55** (CC 38 row): MODIFY to `Filter frequency (off in rxf \textcent{} modes)`.

### K4 — `manual/chapters/05_operation_modes.md` line 354: MODIFY
`rxf & Receive MIDI Filter & Sets what MIDI commands are recognized by the LXR. Click the encoder to reach the ¢ variants (¢ … al¢): identical to the matching C entry, but CC38 is ignored and NRPN 96 sets voice 1 filter frequency (for Elektron NRPN senders). ¢ entries cannot be changed with the knob. \\`

### K5 — `MEMORY.md`
- **Line 457**: MODIFY the trailing `Global NRPN 93 = roll rate (0..15).` to: `Global NRPN 93 = roll rate (0..15), 94 = roll note, 95 = roll velocity, all reported to the AVR PERF page (STM->AVR SEQ_CC 0x71-0x73). NRPN 96 = Drum 1 cutoff in ¢ RxFilter modes only, where Global CC38 is consumed. CC100/101 = 127 directly after an NRPN data entry is an RPN-null deselect, not pan.`
- **ADD** a Sample Import reminder: `Imported-sample FM is read-offset phase modulation (calcUserSampleOscFmBlock(), <<14 into the 17-bit frame domain). The play head advances unmodulated, so FM never changes one-shot length; keep it in step with calcUserSampleOscBlock().`
- **ADD** to Toolchain: `The AVR app must stay <= 57344 bytes (bootloader at 0xE000). Check LxrAvr.bin size after every AVR change.`
- **ADD** a `Current status after Session 039` paragraph at closeout, and a High Priority line for hardware verification.

### K6 — Session record (at closeout)
`knowledge_files/log_archive/039_SESSION_HANDOFF_LOG.md` (new) and `000_SESSION_INDEX.md` (add a row).

---

## 6. Build and verification gates

1. **Builds.** `make firmware` alone does not rebuild changed sources:
   ```bash
   make -C mainboard/LxrStm32 -j8 stm32
   make -C front/LxrAvr avr -j4
   make firmware
   ```
   - No new warnings in the touched files.
   - **AVR gate**: `stat -f %z front/LxrAvr/LxrAvr.bin` ≤ **57344**. Expected about 56,600-56,700 after the −172-byte table saving and about +150 bytes of code. Also run `avr-size` on `build/LxrAvr.elf`; Data must not grow, because every addition is PROGMEM or code.
   - Confirm `FIRMWARE.BIN` embeds the new `LxrStm32.bin`.
2. **Host harness for D1 (before hardware).** Copy `calcUserSampleOscBlock()` and the new `calcUserSampleOscFmBlock()` **verbatim** into a scratch C harness. Stub `SampleInfo` and `sampleMemory_*`, and supply synthetic one-shot and looped samples (including a 7-frame loop). Assert:
   - (a) `fmMod = 0` gives byte-identical output and identical play-head state to the non-FM reader over 10k frames.
   - (b) Under ±32767 × fmMod 1.0, the one-shot `sampleActive` clears on the same frame as non-FM.
   - (c) There are no out-of-range reads near frame 0 or the last frame, for one-shot and loop. Instrument `sampleData[]` accesses.
   - (d) A tiny loop with maximum offset stays O(1) per frame.
3. **Hardware checklist.**
   1. **Glyph**: RxFilter values 16..23 show `¢` (not `¥` or `\`). If they don't, switch to the A2 CGRAM fallback.
   2. **¢ UI**:
      - The pot reaches only `off..all`.
      - Clicking the encoder and turning walks `off..all, ¢..al¢`.
      - The 4-parameter view shows the ¢ name, and the RxFilter pot does nothing while a ¢ entry is active.
      - The encoder can go back to a normal entry.
      - TxFilter never shows ¢ entries.
      - Save globals, power-cycle, and the ¢ entry is restored with the swap active.
   3. **CC38 swap**:
      - In `¢RN`, an Elektron NRPN 93 changes the roll rate and leaves Drum 1 cutoff alone.
      - NRPN 96 sets Drum 1 cutoff, and the stored and echoed values match a CC38 move.
      - In `CRN`, CC38 moves the cutoff and NRPN 96 does nothing.
   4. **RPN null**:
      - NRPN data, then CC101=127 and CC100=127: no pan change, and a following bare CC6 is ignored.
      - CC100/101 sent on their own still pan Snare and Cymbal.
      - Check in both `CRN` and `¢RN`.
   5. **Roll NRPNs**:
      - NRPN 93/94/95 update the PERF page live while a knob sweeps.
      - A held roll follows the rate at the next quantise boundary, and the next roll hit uses the new note and velocity.
      - The encoder and pot then start from the MIDI value.
   6. **FM**:
      - Drum 1 with an imported-sample osc in FM mode: sweeping the FM amount changes the timbre as it does with SIN, and FM 0 sounds the same as the previous build.
      - Repeat for the Cymbal and Hihat main osc, with one-shot and looped samples.
   7. **Hihat modulator**: with a one-shot imported sample on the hat `modOsc`, FM is present on every hit. Periodic-waveform hat modulators sound unchanged.
   8. **Regression**: the Session 038 roll checklist still passes.

## 7. Red-team notes

- **Ownership.** STM owns the CC38/NRPN semantics and the roll values. The AVR only encodes a menu index and mirrors display values. There is no new persistent state, and the global file format is unchanged.
- **Direction.** All new opcodes are new names in one direction only. No existing opcode is reused.
- **Timing.**
  - The roll report is a 3-byte non-blocking queue write per NRPN data entry, the same cost as the existing CC echo.
  - The FM loop adds one clamp, one multiply and two compares per frame. The loop `%` runs only when the modulated read leaves the sample.
- **Bounds.**
  - D1 reads stay within `[0, sampleSize-1]`.
  - The `midiFilterNames` rows are indexed only up to 23 via the encoder clamp.
  - Corrupt RxFilter values ≥ 24 are repaired in A4.
  - TxFilter is unchanged. Its pre-existing lack of a bounds check on corrupt values above 24 is out of scope.
- **Compatibility.**
  - New STM with old AVR: bit 4 is never sent, so behaviour is the published default, and AVR builds without the A6 cases silently ignore reports `0x71-0x73`.
  - Old STM with new AVR: bit 4 is masked by `& 0x0F`, so ¢ entries behave like their twins without the swap.
- **Residual risks.**
  - The `¢` glyph depends on this display's ROM; this is gate 3.1.
  - AVR flash headroom remains tight after this change.

---

## 8. Implementation progress (Session 039, 2026-10-16)

All code changes applied. Build verified.

| ID | Status | Notes |
|---|---|---|
| D1 | **done** | FM stub replaced with real phase-modulated sample reader. Fixed `calcFm*/` in comment (C compiler parsed `*/` as end of block comment). |
| D2 | **done** | Oscillator.h prototype comment updated. |
| D3 | **done** | Hihat modulator re-arm added after main osc block. |
| M1 | **done** | MidiParser.h: `MIDI_RX_FILTER_CC38_SWAP` define + updated comments. |
| M2 | **done** | MidiParser.c: ¢ flag decode in `midi_setFilter()` RX branch. |
| M3 | **done** | GlobalMidiParser.h: `globalMidiParser_setCc38Swap()` prototype. |
| M4 | **done** | GlobalMidiParser.c: setter at EOF. |
| M5 | **done** | GlobalMidiParser.c: doc block on `globalMidiParser_MIDIccHandler()`. |
| M6 | **done** | GlobalMidiParser.c: `globalMidiParser_handleNrpnControl()` rewritten with CC38/RPN-null/NRPN 94-96 handling. |
| M7 | **done** | GlobalMidiParser.c: `globalMidiParser_applySpecialNrpn()` added. |
| M8 | **done** | GlobalMidiParser.c: NRPN defines expanded, `cc38Swap` and `nrpnDataPending` state added. |
| P1 | **done** | frontPanelReceivingProtocol.h: `FRONT_SEQ_REPORT_ROLL_*` 0x71-0x73. |
| P2 | **done** | frontPanelSendingProtocol.h: prototype added. |
| P3 | **done** | frontPanelSendingProtocol.c: `frontPanelSending_sendRollParamReport()` added. |
| A1 | **done** | MenuText.h: row width `[16]` → `[4]`, 8 ¢ rows added. |
| A2 | **done** | MenuText.h: `MENU_CHAR_CENT`, `MIDI_FILTER_CENT_FIRST`, `MIDI_FILTER_RX_ENTRIES`, `MIDI_FILTER_RX_CC38_SWAP`. |
| A3 | **done** | menu.c: pot guard in `menu_parseKnobValue()`. |
| A4 | **done** | menu.c: RX wire encoding in `menu_parseGlobalParam()`. |
| A5 | **done** | menu.c: `menu_getDtypeMenuEntryCount()` returns 24 for `PAR_MIDI_FILT_RX`. |
| A6 | **done** | avrCommsReceivingProtocol.c: three `SEQ_REPORT_ROLL_*` cases. |
| A7 | **done** | avrCommsReceivingProtocol.h: `SEQ_REPORT_ROLL_*` 0x71-0x73. |
| K1 | **done** | MIDI_TABLE.md: CC38 note, CC100/101 RPN-null, NRPN range, rows 94-96, ¢ modes section. |
| K2 | **done** | COMMS_FLOW_SPEC.md: status line, `SEQ_MIDI_FILT_RX` bit 4, roll reports. |
| K3 | **done** | midi_tables.tex already had all rows (pre-populated). |
| K4 | **done** | 05_operation_modes.md already had ¢ documentation (pre-populated). |
| K5 | **done** | MEMORY.md: Session 039 status, NRPN/¢ reminders, FM/flash reminders, high priority. |

### Build results

- STM32: 246,508 bytes, no new warnings.
- AVR: 56,546 bytes (**798 bytes headroom**, 136 bytes smaller than pre-change 56,682 due to `midiFilterNames` table shrink from `[16]` to `[4]`).
- FIRMWARE.BIN: 303,872 bytes. STM at offset 57,344.

### Awaiting hardware verification

See `S039_FM_BUGFIX.md` §8 for the full checklist. Key gates:
1. ¢ glyph renders correctly (LCD ROM 0x5C).
2. ¢ UI: encoder reaches ¢ entries, pot does not, pot locked while ¢ active, globals save/restore.
3. CC38 swap works in ¢ modes, NRPN 96 sets Drum 1 cutoff.
4. RPN null swallows CC100/101 = 127 after NRPN, standalone pan still works.
5. Roll NRPNs 93/94/95 update PERF page live.
6. Imported-sample FM sounds correct on Drum 1/Cymbal/Hihat.
7. Hihat modulator re-arm: FM present on every hit with one-shot sample mod.
8. Session 038 roll checklist still passes.

---

## 9. Independent review of the implementation (2026-10-16)

Reviewed against §1-§5 of this schedule using `git diff` of the working tree on `dev-roll-midi` (HEAD `7a21412`, changes uncommitted).

**Verdict: all 21 code changes (D1-D3, M1-M8, P1-P3, A1-A7) are present and match the schedule. Logic, bounds and protocol direction all check out. No required code follow-ups.** P2 (`S039_P2_CC_RECEP_MIDI_FIX.md`) can proceed on this base.

### 9.1 Build integrity (re-checked, not taken from §8)

| Check | Result |
|---|---|
| `LxrStm32.bin` / `LxrAvr.bin` newer than every `.c`/`.h` in their trees | yes (no stale-binary trap) |
| AVR bytes embedded in `FIRMWARE.BIN` at offset 512 | byte-identical |
| STM bytes embedded in `FIRMWARE.BIN` at offset 57,344 | byte-identical |
| AVR app ≤ 57,344 (bootloader `0xE000`) | 56,546 bytes, 798 bytes headroom |
| Touched STM files recompiled into the scratchpad with the real flags (`GlobalMidiParser.c`, `MidiParser.c`, `Oscillator.c`, `HiHat.c`, `frontPanelSendingProtocol.c`) | no new warnings; only the pre-existing `Oscillator.c:51` `__CLZ` inline warning |
| Touched AVR files recompiled the same way (`menu.c`, `avrCommsReceivingProtocol.c`) | no new warnings; only the pre-existing intentional LED fall-through at `avrCommsReceivingProtocol.c:1022-1026` (present at HEAD) |

### 9.2 Logic review

- **M6/M7 NRPN handler.**
  - Case labels are distinct: 6, 38, 98, 99, 100, 101.
  - Non-¢ CC38 closes the RPN-null window and falls through to the published Drum 1 cutoff.
  - CC6 sets the window equal to `nrpnSelected`, so a CC6 with no NRPN selected is still consumed and ignored.
  - The null clears `nrpnSelected` and keeps the window open for the paired byte. The next CC98/99 or any other CC closes it.
  - NRPN 96 outside ¢ mode returns 0 and is then dropped by the `< 93` table range check.
  - NRPN 93 behaviour is unchanged apart from the new report.
- **M2 ¢ decode.** It sits after the roll-release check and before the nibble store, so bit 4 never reaches `midiParser_txRxFilter`. The TX branch is untouched.
- **P1-P3 / A6-A7.** Opcode values match on both sides (`0x71-0x73`). Reports use the non-blocking `frontPanelSending_sendTriplet()` path. The AVR writes only `parameter_values[PAR_ROLL*]` and repaints with no echo. `parameters2[]` is never touched.
- **D1 FM.** Verbatim match to the schedule.
  - With `fmMod == 0`, `readPos == pos` and `readFrac == sampleFraction`, so the output path is identical to `calcUserSampleOscBlock()`.
  - The clamp keeps `fmProduct * 16384` inside `int32_t`.
  - One-shot reads clamp to `[0, size-1]`. Loop reads wrap with a single `%`.
  - The play head advances unmodulated.
  - The original index guard was retained.
  - The implementer's `calcFm*/` → `calcFm /` comment fix is correct; `*/` would have closed the block early.
- **D3 Hihat.** Calls `osc_resetSamplePlayback()` only, with no phase reset, as specified.
- **A1/A4 AVR encoding.** Rows `[4]` × 25 = 100 bytes. The k → bits formula produces C 0x04, CN 0x05, CR 0x06, CRN 0x07, PC 0x0C, PCN 0x0D, PCR 0x0E, all 0x0F, each OR `0x10`, which is ≤ 0x1F and 7-bit safe. Corrupt values ≥ 24 are repaired to `all`. `getMenuItemNameForValue()` reads at most row 24.
- **A3/A5 UI gating.**
  - The pot guard runs after `editModeActive = 0`, so touching a pot still leaves edit mode, as before.
  - `menu_getDtypeMenuEntryCount()` has only encoder callers (`menu.c` normalise plus both encoder clamps), so pots and TxFilter keep 16 entries.

### 9.3 Non-blocking findings

1. **Stale document references.**
   - The §8 progress note and `MEMORY.md:514` cite `S039_FM_BUGFIX.md §8`. That plan now lives at `S039_P1_FM_BUGFIX.md`, which is untracked.
   - The index still has `S039_FM_BUGFIX.md` staged as added (`git status` shows `AD`).
   - At commit: fix the two references, `git add S039_P1_FM_BUGFIX.md`, and `git rm --cached S039_FM_BUGFIX.md`.
2. **`MEMORY.md:83` says "(HEAD after commit)"**, but nothing has been committed yet. Update it at commit.
3. **`MidiParser.h:80-81` and `:168-173` describe RX `0x04` as "CC echo/record" only.** That is accurate for this tree, but P2 changes the semantics; the comment edits are in the P2 schedule.
4. **Cosmetic:** the ¢ constants in `MenuText.h` sit between `MENU_MIDI_FILTERING` (13) and `MENU_PPQ` (14), which splits the 4-bit menu-ID list. Consider moving them below `MENU_FILE_LOAD_BACKGROUND` at the next touch.
5. **K6 session handoff log and index row are not written yet.** Write them at closeout.
6. **Hardware verification is outstanding.** The ¢ glyph (LCD ROM 0x5C) is the main unverified risk. The checklist is in §6.3 of this file.
