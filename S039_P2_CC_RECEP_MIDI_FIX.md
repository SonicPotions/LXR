# S039 P2 — MIDI "C" (Control Change) Reception Fix

Branch `dev-roll-midi`, on top of the S039 P1 working tree (reviewed clean, see `S039_P1_IMPLEMENTATION.md` §9). Status: **plan only.** The line-cited schedule is `S039_P2_IMPLEMENTATION.md`.

## 1. Intended behaviour (user specification)

| Setting | Intended behaviour |
|---|---|
| `mrt` MIDI Routing | `U2M` USB in → DIN out; `M2M` DIN in → DIN out; `A2M` USB + DIN in → DIN out; `M2U` DIN in → USB out. The firmware also has `off` and `M2A` (DIN in → DIN + USB out). |
| `txf` Transmit filter | P program change, C control change, R realtime (start/stop/clock), N note on/off. A shown letter means that category is transmitted; `off` sends nothing; `all` sends everything. |
| `rxf` Receive filter | Same letters. A shown letter means that category is received; `off` receives nothing; `all` receives everything. |

Routing is independent of the receive filter. A message that `rxf` filters out must still be forwarded if `mrt` routes it; it just must not affect the LXR.

## 2. What the code does today (audited)

| Item | Code | Status |
|---|---|---|
| `mrt` | Forwarding runs first in `midiParser_parseMidiMessage()` (`MidiParser.c:545-562`), before any RX check. DIN realtime bytes are forwarded separately in `midiParser_handleDinRealtime()` (`:923-931`). USB realtime is re-parsed through `midiParser_parseMidiMessage()` and forwarded there. | **matches** |
| `rxf` N (`0x01`) | Note block gated (`MidiParser.c:588`), including the MIDI roll trigger. | **matches** |
| `rxf` R (`0x02`) | Clock/start/stop/continue gated on the DIN realtime path (`:905-916`) and the USB/system path (`GlobalMidiParser.c` `handleSystemMessage`). MTC is gated too. | **matches** |
| `rxf` P (`0x08`) | Program change gated (`MidiParser.c:802`). | **matches** |
| `rxf` C (`0x04`) | **Not gated.** `MidiParser.c:826-830` calls `midiParser_MIDIccHandler(msg,1)` unconditionally. `0x04` is only tested deep inside the CC ladders (`ChannelMidiParser.c` ×7, `GlobalMidiParser.c` ×1), where it gates only the front-panel display echo and automation recording. | **defect** |
| `txf` N / R / P | Gated at `seq_sendMidiNoteOn()`, `seq_sendRealtime()` and `seq_sendProgChg()` (`sequencer.c:2546/2533/2580`). Note-offs are deliberately ungated so notes cannot hang. | **matches** |
| `txf` C (`0x40`) | No code path transmits CCs, so the bit has nothing to gate. | **inert** (see §6) |

Effect of the RX C defect with C off: incoming CCs still change the sound, are stored into the kit (`preset_storeParameterIngress()`), can load a kit (CC0) or move morph (CC1), and drive NRPNs. Meanwhile the front-panel display is not updated and nothing is recorded. The setting therefore does the opposite of its description and leaves the display out of step with the sound.

## 3. Root cause

When filtering was added (the `--AS FILT` comments), the C bit was attached to the "echo/record" branch inside each CC destination instead of the CC entry point. `midiParser_parseMidiMessage()` is the single external ingress for CCs: `midiParser_MIDIccHandler()` has exactly one caller (`MidiParser.c:828`), and internal automation reaches DSP through `frontParser_applyParameterCommand()`, never through this path.

## 4. Fix

Gate the `MIDI_CC` branch of `midiParser_parseMidiMessage()` on `midiParser_txRxFilter & 0x04`, using the same idiom as N/P:

```c
else if(msgonly==MIDI_CC){
   if(midiParser_txRxFilter & 0x04)
      midiParser_MIDIccHandler(msg,1);
}
```

Consequences:
- **All CCs are covered:** every channel (Global and voice), every CC number, CC0 bank/kit change, CC1 morph, NRPN select/data/RPN-null, and both CC tables. With C off none of them affects the LXR.
- **Routing still forwards CCs** (it already ran above), so "routed but does not affect the sound" holds.
- **The inner `& 0x04` checks** (`ChannelMidiParser.c` ×7, `GlobalMidiParser.c` ×1) become always-true for external CCs. Leave them in place: removing eight conditions inside the large per-voice ladders is churn with zero behavioural gain. The new gate's comment records that they are redundant.
- **Comments:** update the `MidiParser.h` filter comments that currently say "0x04 = CC echo/record".

## 5. Interactions checked

| Area | Effect |
|---|---|
| S039 P1 ¢ entries | All ¢ entries include C, so behaviour is unchanged. With C off, CC38/NRPN 96 are moot because everything is blocked. |
| NRPN state | Not touched while C is off. A selector received before C was switched off stays selected. That is standard NRPN semantics and harmless. No reset is needed in `midi_setFilter()`. |
| MIDI roll trigger, notes | Unaffected (N bit). |
| Live record | With C on, incoming CCs are recorded as before. With C off, nothing is recorded, which is consistent. |
| Boot | STM default `midiParser_txRxFilter = 0xFF` (C on) until the AVR sends its globals, so there is no change to boot behaviour. |
| Front-panel parameter edits, sequencer automation, LFOs | Do not enter `midiParser_parseMidiMessage()`, so they are unaffected. |
| USB vs DIN | Same function and same gate. |
| Bank change while C off | No kit load. This is a deliberate consequence: CC0 is a control change. Document it. |

**Behaviour change users can notice:** anyone running `rxf` without C (e.g. `RN`, `PRN`) who relied on CCs still working will lose CC control. That is the intended fix. Release notes and `MEMORY.md` should say so.

## 6. Out of scope: TX C

The LXR never transmits CC messages, so `txf` C has nothing to gate. Fixing that would mean a new CC/NRPN *transmit* feature: front-panel and automation parameter changes sent as MIDI CC. That is a separate design decision. Until then, the recommendation is to describe TX C in the manual as "reserved: the LXR does not currently transmit controller messages". **User decision pending; not part of P2.**

## 7. Changes (summary; full detail in `S039_P2_IMPLEMENTATION.md`)

| ID | File | Change |
|---|---|---|
| C1 | `mainboard/LxrStm32/src/MIDI/MidiParser.c` | Gate the `MIDI_CC` branch on RX C, with a doc block |
| C2 | `mainboard/LxrStm32/src/MIDI/MidiParser.c` | Doc block on `midiParser_parseMidiMessage()` stating the route-then-filter order |
| C3 | `mainboard/LxrStm32/src/MIDI/MidiParser.h` | `midi_setFilter()` bit-layout comment |
| C4 | `mainboard/LxrStm32/src/MIDI/MidiParser.h` | `midiParser_txRxFilter` state comment |
| K1-K4 | `MIDI_TABLE.md`, `COMMS_FLOW_SPEC.md`, `MEMORY.md`, `manual/chapters/05_operation_modes.md` | Document RX gating, routing pass-through and the P1 housekeeping fixes |

STM only. No AVR change, no protocol change, no flash-size impact on the AVR.

## 8. Verification

1. `make -C mainboard/LxrStm32 -j8 stm32`, then `make firmware`. No new warnings in `MidiParser.c`. Confirm the STM binary is embedded at offset 57,344.
2. Hardware, `mrt = M2M`, with a MIDI monitor on DIN out:
   1. **`rxf = RN` (C off):**
      - Global CC89 (Drum 1 volume): no sound change, but the CC appears on DIN out.
      - Voice-channel CC16 (filter): no change.
      - CC0 bank: no kit load.
      - CC1: no morph.
      - NRPN 93: no roll-rate change, and the PERF page is unchanged.
   2. **`rxf = CRN`:** every item in 2.1 works as before, with display echo, and is recorded when record is on.
   3. **`rxf = ¢RN`:** same as `CRN` except CC38 is ignored and NRPN 96 sets the Drum 1 cutoff (P1 behaviour retained).
   4. **Notes and clock:** with C off, notes, clock and program changes still respond according to their own letters.
3. **Regression:** run the S039 P1 checklist and the Session 038 roll checklist.
