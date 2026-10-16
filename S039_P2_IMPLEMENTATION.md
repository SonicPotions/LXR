# S039 P2 — Implementation Schedule: RX "C" Control-Change Reception Gate

Companion to `S039_P2_CC_RECEP_MIDI_FIX.md`. **All code and doc changes applied 2026-10-16.**

- Base: `dev-roll-midi` working tree with S039 P1 applied (reviewed clean, `S039_P1_IMPLEMENTATION.md` §9). Line numbers are against that tree as of 2026-10-16.
- Apply edits bottom-up within each file so cited line numbers stay valid. Each change quotes an anchor.
- Comment blocks are written to be pasted as in-source documentation (`WHAT / WHY / INPUT / OUTPUT / ACCESSORS / AFFILIATES`).
- STM only. No AVR, protocol or file-format change.

## Change index

| ID | File | Lines | Action |
|---|---|---|---|
| C4 | `mainboard/LxrStm32/src/MIDI/MidiParser.h` | 168-173 | MODIFY (state comment) |
| C3 | `mainboard/LxrStm32/src/MIDI/MidiParser.h` | 79-81 | MODIFY (bit-layout text) |
| C1 | `mainboard/LxrStm32/src/MIDI/MidiParser.c` | 826-830 | MODIFY (RX C gate) |
| C2 | `mainboard/LxrStm32/src/MIDI/MidiParser.c` | 540 | MODIFY (function doc block) |
| — | `ChannelMidiParser.c` (468, 715, 963, 1212, 1462, 1707, 1952), `GlobalMidiParser.c` (372) | — | **NO CHANGE** (see note under C1) |
| K1 | `knowledge_files/comms_spec_reference/MIDI_TABLE.md` | after 23 | ADD |
| K2 | `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md` | 196 | MODIFY |
| K3 | `MEMORY.md` | 459, 514, 83 | MODIFY |
| K4 | `manual/chapters/05_operation_modes.md` | after 435 | ADD |

---

## C1 — `MIDI/MidiParser.c` lines 826-830: MODIFY

Anchor (current):

```c
      else if(msgonly==MIDI_CC){
      // respond to CC message. 
         midiParser_MIDIccHandler(msg,1); // send with 1 to record value to either 
                                       // automation or kit param, 0 for DSP only
      } 
```

**Replace with:**

```c
      else if(msgonly==MIDI_CC){
         /* RX CONTROLLER FILTER GATE  (Session 039 P2)
            WHAT: passes an incoming Control Change to the CC router only
                  when the RX filter's C bit (0x04) is set. This covers every
                  channel (Global and voice) and every CC number: CC0 bank/kit
                  change, CC1 morph, NRPN select/data/RPN-null, and both CC
                  parameter tables.
            WHY:  rxf "C" means "controller messages are received". Before this
                  gate the CC router ran unconditionally, and 0x04 was only
                  tested deep inside the CC ladders, where it suppressed the
                  front-panel display echo and automation recording. With C off,
                  CCs still changed the sound and the stored kit while the
                  display silently desynchronised.
                  Routing (mrt) is deliberately unaffected: forwarding to the
                  DIN/USB outputs already happened at the top of this function,
                  before any RX filtering, so a filtered CC is still passed
                  through but does not affect the LXR.
            INPUT:  msg - external MIDI_CC (DIN or USB, any channel);
                    midiParser_txRxFilter RX nibble bit 0x04.
            OUTPUT: midiParser_MIDIccHandler(msg, 1) when C is on (1 = store
                    to kit / record automation; 0 would be DSP-only); nothing
                    when C is off. NRPN selection state is left untouched.
            ACCESSORS: this function only. It is the single external CC
                       ingress; internal automation and front-panel edits
                       reach DSP via frontParser_applyParameterCommand() and
                       never pass here.
            AFFILIATES: midiParser_MIDIccHandler() (this file),
                        channelMidiParser_MIDIccHandler() (ChannelMidiParser.c),
                        globalMidiParser_MIDIccHandler() and its NRPN handler
                        (GlobalMidiParser.c), midi_setFilter() (RX nibble
                        writer), AVR PAR_MIDI_FILT_RX (C and ¢ entries).
                        The inner (midiParser_txRxFilter & 0x04) tests in
                        ChannelMidiParser.c and GlobalMidiParser.c are now
                        always true for external CCs. They are kept as
                        harmless guards; do not read them as the RX C
                        implementation. */
         if(midiParser_txRxFilter & 0x04)
            midiParser_MIDIccHandler(msg,1);
      } 
```

Note on the eight inner `0x04` tests (`ChannelMidiParser.c:468, 715, 963, 1212, 1462, 1707, 1952`; `GlobalMidiParser.c:372`): **no change.** After C1 they can only be reached with C set. Removing them would touch the large per-voice ladders for no behavioural gain. The C1 block documents the redundancy.

## C2 — `MIDI/MidiParser.c` line 540: MODIFY

Anchor: `/* Parse incoming MIDI messages and route them to the shared helpers. */` immediately above `void midiParser_parseMidiMessage(MidiMsg msg)` (line 541).

**Replace that one-line comment with:**

```c
/* EXTERNAL MIDI MESSAGE DISPATCH
   WHAT: entry point for every complete external MIDI message (DIN via
         midiParser_parseUartData(), USB via usb_midi_core.c). Order:
           1. mrt routing: forward to DIN/USB outputs per midiParser_routing,
              UNFILTERED;
           2. SysEx bytes: stop here (pass-through only);
           3. system messages -> globalMidiParser_handleSystemMessage()
              (RX R bit gates clock/start/stop/MTC there);
           4. channel messages, each gated by its RX letter:
              notes N (0x01), program change P (0x08), control change C (0x04).
   WHY:  the receive filter (rxf) decides only what the LXR itself responds
         to; routing (mrt) decides what is passed through. Keeping routing
         first preserves that separation.
   INPUT:  msg - parsed message with bits.source (DIN/USB).
   OUTPUT: forwarded bytes; voice/sequencer/parameter side effects for
           accepted messages.
   ACCESSORS: midiParser_parseUartData() (this file), main.c USB dispatch,
              usb_midi_core.c.
   AFFILIATES: midi_setRouting(), midi_setFilter(), midiParser_handleDinRealtime()
               (DIN realtime takes its own route + filter path). */
```

Code is unchanged.

## C3 — `MIDI/MidiParser.h` lines 79-81: MODIFY

Anchor (inside the `MIDI TX/RX FILTER SETTER` block):

```c
   WHAT: installs the TX (high nibble) or RX (low nibble) MIDI filter of
         midiParser_txRxFilter. Bit layout per nibble: 0x01 notes, 0x02
         realtime/clock, 0x04 CC echo/record, 0x08 program change.
```

**Replace with:**

```c
   WHAT: installs the TX (high nibble) or RX (low nibble) MIDI filter of
         midiParser_txRxFilter. Bit layout per nibble: 0x01 notes, 0x02
         realtime/clock, 0x04 control change (RX: gates all incoming
         CC/NRPN at the midiParser_parseMidiMessage() entry, Session 039 P2;
         TX: reserved, as the LXR transmits no CCs), 0x08 program change.
         The RX filter never affects mrt routing, which forwards first.
```

The rest of the block is unchanged.

## C4 — `MIDI/MidiParser.h` lines 168-173: MODIFY

Anchor:

```c
/* MIDI FILTER STATE
   High nibble is TX, low nibble is RX; see midi_setFilter() for the bit
   layout. The RX ¢ flag (MIDI_RX_FILTER_CC38_SWAP) is NOT stored here; it
   lives in GlobalMidiParser.c (globalMidiParser_cc38Swap). Note that the RX
   0x04 bit gates only CC echo/automation recording: incoming CCs are applied
   regardless (pre-existing behaviour, unchanged). */
```

**Replace with:**

```c
/* MIDI FILTER STATE
   WHAT: high nibble TX, low nibble RX; see midi_setFilter() for the bit
         layout. The RX ¢ flag (MIDI_RX_FILTER_CC38_SWAP) is NOT stored here;
         it lives in GlobalMidiParser.c (globalMidiParser_cc38Swap).
   WHY:  one byte read on every MIDI message; nibble split keeps TX and RX
         independent.
   WRITER: midi_setFilter() only. Default 0xFF (everything on) until the AVR
           sends its globals at boot.
   READERS: RX - midiParser_parseMidiMessage() (N 0x01, P 0x08, C 0x04 gate
            at entry since Session 039 P2), midiParser_handleDinRealtime()
            and globalMidiParser_handleSystemMessage() (R 0x02), plus legacy
            0x04 echo/record guards in the CC ladders (now always true for
            external CCs). TX - seq_sendMidiNoteOn() 0x10, seq_sendRealtime()
            0x20, seq_sendProgChg() 0x80; 0x40 has no transmitter.
   AFFILIATES: midi_setRouting() (routing is independent of these bits). */
```

---

## K1 — `knowledge_files/comms_spec_reference/MIDI_TABLE.md` after line 23: ADD

After the last "Global-channel CC pre-emption" bullet (`- Non-global CC messages continue to use the existing ChannelMidiParser.c implementation.`), add:

```markdown

RX filter and routing (Session 039 P2):

- `mrt` routing runs first in `midiParser_parseMidiMessage()` and is never affected by the RX filter. A filtered message is still forwarded if routed.
- RX letters gate what the LXR responds to: N `0x01` notes (including MIDI rolls), R `0x02` clock/start/stop/continue/MTC, C `0x04` **all** incoming CC traffic (every channel; CC0 bank, CC1 morph, NRPN, both CC tables), P `0x08` program change.
- C is gated once at the `MIDI_CC` entry. The older `0x04` tests inside the CC ladders are now redundant guards.
- TX C (`0x40`) is reserved: the LXR transmits no CCs.
```

## K2 — `knowledge_files/comms_spec_reference/COMMS_FLOW_SPEC.md` line 196: MODIFY

Append to the end of the existing `SEQ_MIDI_FILT_RX` bullet:

`. RX bit 2 (0x04, C) gates all incoming CC/NRPN at the STM MIDI_CC entry (Session 039 P2); routing (mrt) is unaffected.`

## K3 — `MEMORY.md`: MODIFY

- **Line 459** (MIDI reminders bullet ending `…an RPN-null deselect, not pan.`): append ` RX filter C (0x04) gates all incoming CCs at the midiParser_parseMidiMessage() MIDI_CC entry (Session 039 P2); before P2 it only gated front-panel echo/recording. mrt routing always forwards before RX filtering. TX C has no transmitter.`
- **Line 514** (Session 039 hardware-verification bullet): replace `Full checklist in S039_FM_BUGFIX.md §8.` with `Checklists in S039_P1_IMPLEMENTATION.md §6.3 and S039_P2_CC_RECEP_MIDI_FIX.md §8.` (P1 review finding 1).
- **Line 83** (Session 039 status paragraph): replace `(HEAD after commit)` with the actual commit hash at commit time (P1 review finding 2). Append: `P2 gates RX filter C (control change) at the MIDI_CC entry; previously CCs were applied with C off.`

## K4 — `manual/chapters/05_operation_modes.md` after line 435: ADD

After `` `PCN`, `PCR`, and `all`. `` (end of the value list in *Receive MIDI Filter*), add:

```markdown

The receive filter only controls what the LXR itself responds to. A message that is filtered
out is still passed on to the MIDI outputs if MIDI Routing (\param{mrt}) forwards it. With C
turned off, no controller message has any effect: this includes bank change (CC0), the mod
wheel morph (CC1) and NRPNs.
```

(`txf` C wording in the Menu Options table, line 361, is left as is pending the TX C decision in the plan, §6.)

---

## Build and verification

1. `make -C mainboard/LxrStm32 -j8 stm32`, then `make firmware`. `make firmware` alone does not rebuild. There must be no new warnings in `MidiParser.c`. Confirm `LxrStm32.bin` is embedded at offset 57,344 of `FIRMWARE.BIN`. No AVR rebuild is needed.
2. Hardware checks are in `S039_P2_CC_RECEP_MIDI_FIX.md` §8. In short:
   - C off: no CC/NRPN/bank/morph effect, but CCs are still routed.
   - C on and ¢ modes: unchanged from P1.
   - N/R/P: unaffected.

## Red-team

- **Scope:** one executable line changed, plus comments and docs.
- **Ownership:** STM MIDI parser only.
- **Timing:** one AND and one branch per CC.
- **Bounds:** none involved.
- **Compatibility:** no protocol or file change. Old and new AVRs both send the same RX nibble.
- **Behaviour change (intended):** users with C off lose CC control. Call this out in release notes.
- **Not covered:** TX C (no transmitter), which needs a separate decision.

## Implementation log

All changes applied 2026-10-16 in the order C4, C3, C1, C2, K1, K2, K4, K3.

- **Build:** `make -C mainboard/LxrStm32 -j8 stm32` clean (no new warnings in `MidiParser.c`). `make firmware` produces `FIRMWARE.BIN` with STM binary (246,524 bytes) at offset 57,344. No AVR rebuild needed.
- **C1:** `MIDI_CC` branch gated on `midiParser_txRxFilter & 0x04` with full doc block. Anchor matched exactly.
- **C2:** Function comment replaced with `EXTERNAL MIDI MESSAGE DISPATCH` doc block. Anchor matched exactly.
- **C3:** Bit-layout text updated: `0x04 CC echo/record` → `0x04 control change (RX: gates all incoming CC/NRPN…)`. Anchor matched exactly.
- **C4:** State comment replaced with `WHAT/WHY/WRITER/READERS/AFFILIATES` block. Anchor matched exactly.
- **K1:** RX filter and routing section appended to MIDI_TABLE.md after Global CC pre-emption bullets.
- **K2:** RX bit 2 sentence appended to COMMS_FLOW_SPEC.md `SEQ_MIDI_FILT_RX` bullet.
- **K3:** MEMORY.md line 459 (MIDI reminders), line 514 (verification checklist ref), and line 83 (session status) updated.
- **K4:** Receive-filter routing paragraph inserted in 05_operation_modes.md between the value list and the ¢ entries section.
- **Not yet hardware-tested.** Verification checklist is in `S039_P2_CC_RECEP_MIDI_FIX.md` §8.

---

## Independent review of the implementation (2026-10-16)

Reviewed against this schedule using `git diff` of the working tree (`dev-roll-midi`, base `7a21412`, uncommitted).

**Verdict: C1-C4 and K1-K4 are applied exactly as scheduled. The gate is correct, the build is current, and there are no new warnings. No required follow-ups.**

### Code

| ID | Check | Result |
|---|---|---|
| C1 | `MidiParser.c:845-883`: `else if(msgonly==MIDI_CC){ … if(midiParser_txRxFilter & 0x04) midiParser_MIDIccHandler(msg,1); }`. The brace closes before `else { // anything else` (884) and the channel-message `if/else` chain is intact. The doc block matches verbatim. | correct |
| C1 coverage | `midiParser_MIDIccHandler()` still has exactly one caller (`MidiParser.c:882`), so every external CC on DIN and USB, every channel, CC0/CC1, NRPN and both tables goes through the gate. Routing (`:564-581` region) still runs before it. | correct |
| C1 note | Inner `0x04` tests left untouched: 7 in `ChannelMidiParser.c` and 1 in `GlobalMidiParser.c` (counted). | as scheduled |
| C2 | The `EXTERNAL MIDI MESSAGE DISPATCH` block replaces the one-line comment above `midiParser_parseMidiMessage()`. Comment only. | correct |
| C3 | `MidiParser.h` bit-layout text updated inside the existing `MIDI TX/RX FILTER SETTER` block. The ¢-flag lines that follow are preserved. | correct |
| C4 | `MidiParser.h` `MIDI FILTER STATE` block replaced. Its reader/writer list matches the code (N 588, P 802+, C gate, R in `handleDinRealtime()` and `handleSystemMessage()`; TX 0x10/0x20/0x80 in `sequencer.c`; 0x40 has no transmitter). | correct |
| scope | Only `MidiParser.c`/`.h` changed in code for P2. No AVR or protocol file was touched. | correct |

### Build integrity (re-checked)

- `LxrStm32.bin` (246,524 bytes, +16 over P1) is newer than every STM source, and its bytes are identical to `FIRMWARE.BIN` at offset 57,344.
- `LxrAvr.bin` (56,546 bytes) is unchanged since P1, newer than every AVR source, and identical at offset 512. The AVR did not need a rebuild.
- `MidiParser.c` recompiled into the scratchpad with the real flags: no warnings.

### Docs

| ID | Result |
|---|---|
| K1 | `MIDI_TABLE.md` "RX filter and routing (Session 039 P2)" block added after the Global CC pre-emption bullets. Text matches. |
| K2 | `COMMS_FLOW_SPEC.md:196` sentence appended. |
| K3 | `MEMORY.md:459` append and the `:514` checklist reference are correct. **`:83` now reads "(HEAD `7a21412`)"**, which is the pre-change base, not a commit containing S039. The schedule asked for the real commit hash at commit time. Fix when committing (non-blocking). |
| K4 | `manual/chapters/05_operation_modes.md:437-440` routing paragraph is in place between the value list and the ¢ section. |

### Open items carried forward (non-blocking)

1. **P1 housekeeping is still open.**
   - `git status` still shows `AD S039_FM_BUGFIX.md`, and `S039_P1_FM_BUGFIX.md` is untracked.
   - `S039_P1_IMPLEMENTATION.md:3` and `:1135` still cite `S039_FM_BUGFIX.md`.
   - Resolve at commit: `git rm --cached S039_FM_BUGFIX.md`, add `S039_P1_FM_BUGFIX.md` and the two P2 docs, and fix the references.
2. **`MEMORY.md:83` commit hash** (see K3).
3. **TX C decision.** `txf` C still has no transmitter. The manual Menu Options row (`05_operation_modes.md:361`) is unchanged, pending a user decision.
4. **Release note.** Users running `rxf` without C now lose CC, bank-change, morph and NRPN control. This is intended.
5. **Hardware verification** is outstanding for P1 (§6.3 of the P1 schedule) and P2 (`S039_P2_CC_RECEP_MIDI_FIX.md` §8). Run P2 check 2.1 with a MIDI monitor on the routed output to confirm the "routed but no effect" behaviour.
6. **K6 session handoff log** (`039_SESSION_HANDOFF_LOG.md` plus an index row) is still to be written at closeout.
