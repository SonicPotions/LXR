# S039 P3 — Implementation Schedule: MIDI Transmit of Menu Parameter Changes (TX "C")

Companion to `S039_P3_MIDI_TX_CC.md` (plan, all decisions settled). **No code has been changed by this document.**

- Base: `dev-roll-midi` working tree with S039 P1 and P2 applied (both reviewed clean). Line numbers are against that tree as of 2026-10-16.
- **Apply edits bottom-up within each file** so cited line numbers stay valid. Every change quotes an anchor.
- Comment blocks are written to be pasted as in-source documentation (`WHAT / WHY / INPUT / OUTPUT / ACCESSORS / AFFILIATES`).
- STM: USB, FIFO/UART, sequencer, Global MIDI parser, output control, front-panel parser, `main.c`. AVR: two opcodes and one helper. Docs and manual.

## 0. Deviations from the plan (found in the deep-dive)

1. **CC38, CC100 and CC101 are real TX addresses, not exclusions.**
   - Global CC38 = Drum 1 cutoff (sent as NRPN 96 in ¢ mode, decision 5).
   - CC100/101 = Snare/Cymbal pan.
   - Only the 18 target selectors are excluded. CC0/1/6/98/99 never come out of the reverse table because those table slots are `PAR_NONE`.
   - **New guard:** a pan value of 127 on CC100/101 would be taken by an LXR receiving the playback as an RPN-null if it followed an NRPN data entry (P1 rule). Before emitting CC100/101 = 127, TX first emits the harmless selector `CC99 = 0`, which closes the receiver's null window.
2. **The shift-encoder `MIDI_CC`/`CC_2` sends (`menu.c:3333-3346`) need no change.** In `menu_encoderChangeShiftParameter()`, `isMorphParam = (paramNr < END_OF_SOUND_PARAMETERS)` (`menu.c:3150`), so those `!isMorphParam` branches can never run for sound parameters. They always send the morph endpoint, which decision 2/plan §5 excludes from TX.
3. **No factored ingress helper.** Instead, the `_MENU` statuses share the existing `case` bodies via an extra `case` label, an `isMenuEdit` flag and a normalised local message. This is a smaller diff and gives byte-identical behaviour for the plain opcodes.
4. **USB fix: the flush discards when the device is not `USB_OTG_CONFIGURED`** (unenumerated, addressed or suspended) instead of only skipping `SUSPENDED`. The USB stack only delivers `DataIn` while configured (`usbd_core.c:311-315`), so this prevents 5 ms busy stalls and stale backlog when no host is reading.
5. **The special NRPN numbers move from `GlobalMidiParser.c` to `GlobalMidiParser.h`**, so the TX code uses the same constants as receive.

## Change index (apply order)

| ID | File | Lines | Action |
|---|---|---|---|
| U1 | `mainboard/LxrStm32/src/Hardware/USB/usb_midi_core.h` | after 64 | ADD |
| U2 | `mainboard/LxrStm32/src/Hardware/USB/usb_midi_core.c` | 396-401; after 140 | MODIFY; ADD |
| U3 | `mainboard/LxrStm32/src/Hardware/USB/usb_manager.h` | 49, 55 | ADD |
| U4 | `mainboard/LxrStm32/src/Hardware/USB/usb_manager.c` | 128-139; 111-126; after 104; after 42; after 35 | MODIFY / ADD |
| F1 | `mainboard/LxrStm32/src/Hardware/FIFO.h` | after 56 | ADD |
| F2 | `mainboard/LxrStm32/src/Hardware/FIFO.c` | after 64 | ADD |
| F3 | `mainboard/LxrStm32/src/uARTFrontSYX/Uart.h` | after 53 | ADD |
| F4 | `mainboard/LxrStm32/src/uARTFrontSYX/Uart.c` | after 235; 215-225; after 54 | ADD / MODIFY |
| S1 | `mainboard/LxrStm32/src/Sequencer/sequencer.h` | after 441 | ADD |
| S2 | `mainboard/LxrStm32/src/Sequencer/sequencer.c` | after 1220 | ADD |
| G1 | `mainboard/LxrStm32/src/MIDI/GlobalMidiParser.h` | after 35 | ADD |
| G2 | `mainboard/LxrStm32/src/MIDI/GlobalMidiParser.c` | after 711 (EOF); 36-49 | ADD; REPLACE |
| O1 | `mainboard/LxrStm32/src/MIDI/MidiOutputControl.h` | after 67 | ADD |
| O2 | `mainboard/LxrStm32/src/MIDI/MidiOutputControl.c` | after 78; after 50 | ADD |
| P1 | `mainboard/LxrStm32/src/uARTFrontSYX/frontPanelReceivingProtocol.h` | after 61 | ADD |
| P2 | `mainboard/LxrStm32/src/uARTFrontSYX/frontPanelReceivingProtocol.c` | 2741-2749; 2424-2431; 2020-2035; 1994-2017; after 50 | MODIFY / ADD |
| M1 | `mainboard/LxrStm32/src/main.c` | 289; after 65 | ADD |
| A1 | `front/LxrAvr/avrComms/avrCommsReceivingProtocol.h` | after 88 | ADD |
| A2 | `front/LxrAvr/Menu/menu.c` | 4409-4412; 4396-4399; 3104-3117; after 2879 | MODIFY / ADD |
| — | `front/LxrAvr/Menu/menu.c` 3333-3346; `buttonHandler.c` 284-298; `presetManager.c` 694, 2226-2230 | — | **NO CHANGE** (unreachable / non-menu traffic keeps plain opcodes) |
| K1-K6 | `MIDI_TABLE.md`, `COMMS_FLOW_SPEC.md`, `MEMORY.md`, manual `05`/`07`, `midi_tables.tex` | §9 | MODIFY / ADD |

---

## 1. USB MIDI out fix (decision 9)

### U1 — `Hardware/USB/usb_midi_core.h` after line 64: ADD

Anchor: `extern uint8_t* usb_MidiInRdPtr;`

```c

/* USB MIDI IN-ENDPOINT TRANSFER-IN-FLIGHT FLAG  (Session 039 P3)
   WHAT: 1 from the moment usb_flushMidi() hands a ping-pong half to
         DCD_EP_Tx() until the stack reports completion through
         usbd_midi_DataIn(), or until usb_flushMidi()'s timeout gives up.
   WHY:  usb_flushMidi() used to run DCD_EP_Flush() + DCD_EP_Tx() on every
         main-loop pass, aborting IN data still in the endpoint FIFO. The
         flag lets the flush wait for completion.
   WRITERS: usb_flushMidi() (set; clear on timeout or not configured),
            usbd_midi_DataIn() (clear; USB OTG interrupt context).
   READERS: usb_flushMidi().
   AFFILIATES: USB_MIDI_TX_TIMEOUT_TICKS (usb_manager.h). */
extern volatile uint8_t usb_midiTxBusy;
```

### U2 — `Hardware/USB/usb_midi_core.c`

**(a) Lines 396-401: MODIFY `usbd_midi_DataIn()`.** Replace the body

```c
static uint8_t  usbd_midi_DataIn (void *pdev, uint8_t epnum)
{
	UNUSED(epnum);
	UNUSED(pdev);
	return USBD_OK;
}
```

with

```c
static uint8_t  usbd_midi_DataIn (void *pdev, uint8_t epnum)
{
	/* MIDI IN TRANSFER COMPLETE  (Session 039 P3)
	   WHAT: clears usb_midiTxBusy when the MIDI IN endpoint finishes the
	         transfer started by usb_flushMidi().
	   WHY:  lets usb_flushMidi() start the next ping-pong half only after the
	         previous one has left the endpoint FIFO, instead of flushing it
	         mid-transfer.
	   INPUT:  epnum - endpoint number without the direction bit (usbd_core.c
	           only calls this while the device is USB_OTG_CONFIGURED).
	   OUTPUT: usb_midiTxBusy = 0 for the MIDI IN endpoint.
	   ACCESSORS: STM USB device core (OTG interrupt), via MIDI_cb.DataIn.
	   AFFILIATES: usb_flushMidi() (usb_manager.c), usb_midiTxBusy. */
	UNUSED(pdev);
	if((epnum & 0x7F) == (MIDI_IN_EP & 0x7F))
		usb_midiTxBusy = 0;
	return USBD_OK;
}
```

**(b) After line 140** (`uint8_t* usb_MidiInRdPtr = usb_MidiInBuff;`): **ADD**

```c
/* USB MIDI IN transfer-in-flight flag; see usb_midi_core.h (Session 039 P3). */
volatile uint8_t usb_midiTxBusy = 0;
```

### U3 — `Hardware/USB/usb_manager.h`

**(a) After line 55** (`void usb_flushMidi();`): **ADD**

```c
/* USB MIDI OUT FREE SPACE  (Session 039 P3)
   WHAT: bytes still free in the ping-pong half currently being filled
         (0..TOTAL_MIDI_BUF_SIZE). One USB-MIDI event is 4 bytes.
   WHY:  callers that pace output (outputControl_serviceMenuTx()) only emit
         whole messages that fit; usb_sendMidi() uses it as its bounds check.
   OUTPUT: free byte count.
   ACCESSORS: usb_sendMidi(), outputControl_serviceMenuTx().
   AFFILIATES: usb_flushMidi(), usb_MidiInWrPtr/RdPtr. */
uint8_t usb_midiTxFree(void);

/* USB MIDI busy timeout: 20 SysTicks x 250 us = 5 ms (Session 039 P3,
   user decision). After this, usb_flushMidi() assumes the host stopped
   polling, abandons the in-flight half and continues. */
#define USB_MIDI_TX_TIMEOUT_TICKS 20u
```

**(b) Line 49** (`void usb_sendMidi(MidiMsg msg);`): **ADD** a comment above it:

```c
/* Queue one 4-byte USB-MIDI event into the current ping-pong half. Session 039
   P3: whole-event-or-drop; never writes past the current half. */
```

### U4 — `Hardware/USB/usb_manager.c`

**(a) Lines 128-139: MODIFY `usb_sendMidi()`.** Insert as the first statements of the body:

```c
	/* WHOLE-EVENT BOUNDS CHECK  (Session 039 P3)
	   WHAT: drops the complete 4-byte event (and counts it) when it does not
	         fit in the ping-pong half being filled.
	   WHY:  usb_sendByte() has no bounds check: more than 64 bytes between
	         flushes ran into the half being transmitted, and more than 128
	         wrapped, corrupting usb_flushMidi()'s length. Whole-event drop
	         never produces a partial event.
	   INPUT:  msg; current half fill (usb_midiTxFree()).
	   OUTPUT: 4 bytes queued, or usb_midiTxDropCount++.
	   ACCESSORS: outputControl_sendMidi(), midiParser_parseMidiMessage()
	              routing, midiParser_handleDinRealtime() routing (main loop).
	   AFFILIATES: usb_flushMidi(), usb_midiTxFree(). */
	if(usb_midiTxFree() < 4u)
	{
		usb_midiTxDropCount++;
		return;
	}
```

The rest of the body (the four `usb_sendByte()` calls) is unchanged.

**(b) Lines 111-126: REPLACE `usb_flushMidi()`** (keep the comment block at 106-110 and append to it as below):

```c
/*
 * flush FIFO and send out next block of messages stored in the usb_MidiInBuff
 * called periodically by usb_tick()
 * Session 039 P3:
 *   WHAT: starts a DCD_EP_Tx() of the filled ping-pong half only when the
 *         previous transfer has completed (usb_midiTxBusy cleared by
 *         usbd_midi_DataIn()), or after USB_MIDI_TX_TIMEOUT_TICKS (5 ms).
 *         When the device is not USB_OTG_CONFIGURED, pending bytes are
 *         discarded.
 *   WHY:  the old code flushed and restarted the endpoint on every main-loop
 *         pass, aborting data still in flight. With no host reading, it kept
 *         accumulating into the unbounded ring.
 *   INPUT:  usb_MidiInWrPtr/RdPtr, usb_midiTxBusy, systick_ticks,
 *           USB_OTG_dev.dev.device_status.
 *   OUTPUT: one IN transfer started and halves swapped; or nothing; or the
 *           pending half discarded.
 *   ACCESSORS: usb_tick() (main loop).
 *   AFFILIATES: usb_sendMidi(), usb_midiTxFree(), usbd_midi_DataIn().
 */
void usb_flushMidi()
{
	if(USB_OTG_dev.dev.device_status != USB_OTG_CONFIGURED)
	{
		/* No host is reading: drop the backlog so the ring cannot fill. */
		usb_MidiInWrPtr = usb_MidiInRdPtr;
		usb_midiTxBusy = 0;
		return;
	}

	if(usb_midiTxBusy)
	{
		if((uint32_t)(systick_ticks - usb_midiTxStartTick) < USB_MIDI_TX_TIMEOUT_TICKS)
			return;
		/* Host stopped polling: abandon the in-flight half and continue. */
		usb_midiTxBusy = 0;
		usb_midiTxTimeoutCount++;
	}

	const uint8_t len = (uint8_t)(usb_MidiInWrPtr - usb_MidiInRdPtr);
	if(len != 0)
	{
		usb_midiTxStartTick = systick_ticks;
		usb_midiTxBusy = 1;	/* set before Tx: DataIn may fire immediately */
		DCD_EP_Flush (&USB_OTG_dev,MIDI_IN_EP);
		DCD_EP_Tx (&USB_OTG_dev, MIDI_IN_EP, usb_MidiInRdPtr, len);

		usb_MidiInRdPtr += TOTAL_MIDI_BUF_SIZE;
		if(usb_MidiInRdPtr >= (usb_MidiInBuff+(TOTAL_MIDI_BUF_SIZE * NUM_SUB_BUFFERS)))
		{
			usb_MidiInRdPtr = usb_MidiInBuff;
		}
		usb_MidiInWrPtr=usb_MidiInRdPtr;
	}
}
```

`DCD_EP_Flush()` is retained. It now runs only when the previous transfer completed or timed out, so it never aborts a live transfer.

**(c) After line 104** (closing `}` of `usb_sendByte()`): **ADD**

```c
//------------------------------------------------------------------------------
/* USB MIDI OUT FREE SPACE (Session 039 P3); see usb_manager.h. The write
   pointer always stays within [RdPtr, RdPtr + TOTAL_MIDI_BUF_SIZE] because
   usb_sendMidi() checks this before writing. */
uint8_t usb_midiTxFree(void)
{
	const uint8_t used = (uint8_t)(usb_MidiInWrPtr - usb_MidiInRdPtr);
	return (used >= TOTAL_MIDI_BUF_SIZE) ? 0u : (uint8_t)(TOTAL_MIDI_BUF_SIZE - used);
}
```

**(d) After line 42** (`USB_OTG_CORE_HANDLE           USB_OTG_dev;`): **ADD**

```c

/* USB MIDI OUT DIAGNOSTICS AND TIMING  (Session 039 P3)
   usb_midiTxStartTick: systick_ticks when the current IN transfer started;
     read only by usb_flushMidi() for the 5 ms timeout.
   usb_midiTxDropCount: whole USB-MIDI events dropped because the current
     ping-pong half was full (debugger-visible; wraps).
   usb_midiTxTimeoutCount: IN transfers abandoned after the busy timeout. */
static uint32_t usb_midiTxStartTick = 0;
static uint16_t usb_midiTxDropCount = 0;
static uint16_t usb_midiTxTimeoutCount = 0;
```

**(e) After line 35** (`#include <string.h>`): **ADD** `#include "globals.h"	/* systick_ticks (Session 039 P3 USB timeout) */`

---

## 2. DIN MIDI out whole-message rule (settled follow-up 1)

### F1 — `Hardware/FIFO.h` after line 56: ADD

Anchor: `uint8_t fifo_bufferOut(Fifo* fifo, uint8_t *pByte);`

```c

/* FIFO FREE SPACE  (Session 039 P3)
   WHAT: number of bytes that can still be written (0..BUFFER_SIZE-1; one slot
         is always kept empty to tell full from empty).
   WHY:  producers that must not split multi-byte messages
         (uart_sendMidi()) check for room before writing any byte.
   INPUT:  fifo. OUTPUT: free byte count.
   CONCURRENCY: exact for a single producer; a consumer in an ISR can only
                increase free space, so the result is conservative.
   ACCESSORS: uart_sendMidi(), uart_midiTxFree() (Uart.c).
   AFFILIATES: fifo_bufferIn(), fifo_bufferOut(). */
uint8_t fifo_freeSpace(Fifo* fifo);
```

### F2 — `Hardware/FIFO.c` after line 64: ADD

Anchor: the closing `}` of `fifo_bufferOut()`.

```c
//-------------------------------------------------------------------------
/* FIFO FREE SPACE (Session 039 P3); see FIFO.h. */
uint8_t fifo_freeSpace(Fifo* fifo)
{
  return (uint8_t)((fifo->read - fifo->write - 1) & BUFFER_MASK);
}
```

### F3 — `uARTFrontSYX/Uart.h` after line 53: ADD

Anchor: `void uart_sendMidiByte(uint8_t data);`

```c
/* DIN MIDI OUT FREE SPACE  (Session 039 P3)
   WHAT: bytes free in the DIN MIDI TX FIFO.
   WHY:  outputControl_serviceMenuTx() paces menu CC/NRPN output so a whole
         message (3 or 9 bytes, plus a 3-byte guard) always fits.
   OUTPUT: free byte count (0..255).
   ACCESSORS: outputControl_serviceMenuTx().
   AFFILIATES: uart_sendMidi(), fifo_freeSpace(). */
uint8_t uart_midiTxFree(void);
```

### F4 — `uARTFrontSYX/Uart.c`

**(a) After line 235** (closing `};` of `uart_sendMidiByte()`): **ADD**

```c
//-----------------------------------------------------------------------------
/* DIN MIDI OUT FREE SPACE (Session 039 P3); see Uart.h. */
uint8_t uart_midiTxFree(void)
{
	return fifo_freeSpace(&fifo_midiTx);
}
```

**(b) Lines 215-225: MODIFY `uart_sendMidi()`.** Insert after the existing first comment line (`/* Serialize a MIDI message onto the raw MIDI transport FIFO. */`):

```c
	/* WHOLE-MESSAGE-OR-DROP  (Session 039 P3, user decision)
	   WHAT: queues status + msg.bits.length data bytes only if all of them
	         fit; otherwise drops the whole message and counts it.
	   WHY:  fifo_bufferIn() drops single bytes when full, which could put a
	         partial message on the DIN wire (e.g. a status without its data,
	         or an NRPN triplet missing CC6), corrupting the receiver's
	         running status. Applies to every DIN output: sequencer notes,
	         clock, routing and menu CC/NRPN TX.
	   INPUT:  msg (status, data1, data2, bits.length 0..2).
	   OUTPUT: 1-3 bytes queued and TXE enabled, or uart_midiTxDropCount++.
	   ACCESSORS: outputControl_sendMidi(), midiParser_parseMidiMessage()
	              routing, midiParser_handleDinRealtime() routing (all main
	              loop; single producer).
	   AFFILIATES: fifo_freeSpace(), uart_sendMidiByte(), USART2 TXE ISR. */
	if(fifo_freeSpace(&fifo_midiTx) < (uint8_t)(1u + msg.bits.length))
	{
		uart_midiTxDropCount++;
		return;
	}
```

The remaining body is unchanged.

**(c) After line 54** (`static Fifo fifo_midiRx;`): **ADD**

```c
/* Whole DIN MIDI messages dropped by uart_sendMidi() because fifo_midiTx was
   full (Session 039 P3 diagnostics; debugger-visible; wraps). */
static uint16_t uart_midiTxDropCount = 0;
```

---

## 3. Armed-step accessor (decision 2)

### S1 — `Sequencer/sequencer.h` after line 441: ADD

Anchor: `void seq_armAutomationStep(uint8_t stepNr, uint8_t track,uint8_t isArmed);`

```c
/* AUTOMATION STEP ARMED QUERY  (Session 039 P3)
   WHAT: non-zero while a front-panel automation step is armed (step and
         track both set by seq_armAutomationStep()).
   WHY:  menu edits made while a step is armed are temporary step data that
         the AVR reverts on disarm; MIDI TX must not send them (user decision).
         The STM sees ARM/DISARM in order with the edits, so it is the single
         place to gate.
   OUTPUT: 1 armed, 0 not armed.
   ACCESSORS: outputControl_queueMenuParameterTx(),
              outputControl_queueMenuPerfTx() (MidiOutputControl.c).
   AFFILIATES: seq_armAutomationStep(), seq_recordAutomation(). */
uint8_t seq_isAutomationStepArmed(void);
```

### S2 — `Sequencer/sequencer.c` after line 1220: ADD

Anchor: the closing `}` of `seq_armAutomationStep()`.

```c
//------------------------------------------------------------------------------
/* AUTOMATION STEP ARMED QUERY (Session 039 P3); see sequencer.h. Uses the same
   test as seq_recordAutomation()'s armed-step branch. */
uint8_t seq_isAutomationStepArmed(void)
{
   return (uint8_t)((seq_armedArmedAutomationStep != -1)
                    && (seq_armedArmedAutomationTrack != -1));
}
```

---

## 4. TX address lookup (Global MIDI parser)

### G1 — `MIDI/GlobalMidiParser.h` after line 35: ADD

Anchor: `void globalMidiParser_setCc38Swap(uint8_t enabled);`

```c

/* GLOBAL SPECIAL NRPN NUMBERS  (Session 038 / 039; moved here in P3)
   WHAT: NRPN numbers outside globalMidiParser_nrpnToRawParam[] (0..92):
         93 roll rate (0..15), 94 roll note, 95 roll velocity (PERF),
         96 Drum 1 filter cutoff (¢ RX-filter mode only; replaces CC38).
   WHY:  shared by receive (globalMidiParser_applySpecialNrpn()) and menu TX
         (outputControl_serviceMenuTx(), globalMidiParser_getTxAddress()) so
         both directions use one definition.
   AFFILIATES: MIDI_TABLE.md, manual/latex/midi_tables.tex. */
#define GLOBAL_MIDI_NRPN_ROLL_RATE      93
#define GLOBAL_MIDI_NRPN_ROLL_NOTE      94
#define GLOBAL_MIDI_NRPN_ROLL_VELOCITY  95
#define GLOBAL_MIDI_NRPN_FILTER_FREQ_1  96

/* GLOBAL TX ADDRESS KINDS  (Session 039 P3)
   Return values of globalMidiParser_getTxAddress(). */
#define GLOBAL_MIDI_TX_NONE  0
#define GLOBAL_MIDI_TX_CC    1
#define GLOBAL_MIDI_TX_NRPN  2

/* GLOBAL TX ADDRESS LOOKUP  (Session 039 P3)
   WHAT: returns how a raw sound parameter is addressed on the Global MIDI
         channel for transmit: CC number, NRPN number, or none. It is the
         inverse of the receive tables globalMidiParser_ccToRawParam[] and
         globalMidiParser_nrpnToRawParam[]; the CC wins if both map a
         parameter. ¢ mirror: PAR_FILTER_FREQ_1 returns NRPN 96 while the RX
         filter is a ¢ entry, otherwise CC38.
   WHY:  menu TX must reproduce exactly what receive understands, so a DAW
         recording of the LXR's output plays back into the LXR. Deriving the
         inverse from the receive tables means the directions cannot diverge.
   EXCLUDED (returns NONE): parameters with no Global mapping (envelope
         position 1-6, per-voice morph, kit version, reserved) and the target
         selectors PAR_VEL_DEST_1..6, PAR_VOICE_LFO1..6, PAR_TARGET_LFO1..6
         (user decision: their receive value domain is unverified).
   INPUT:  rawParam - AVR/Preset raw id; number - out: CC or NRPN number.
   OUTPUT: GLOBAL_MIDI_TX_NONE / _CC / _NRPN.
   ACCESSORS: outputControl_queueMenuParameterTx(),
              outputControl_serviceMenuTx() (MidiOutputControl.c).
   AFFILIATES: globalMidiParser_setCc38Swap(), receive tables in
               GlobalMidiParser.c, MIDI_TABLE.md. */
uint8_t globalMidiParser_getTxAddress(uint16_t rawParam, uint8_t *number);
```

### G2 — `MIDI/GlobalMidiParser.c`

**(a) After line 711 (EOF, after `globalMidiParser_setCc38Swap()`): ADD**

```c

/* GLOBAL TX REVERSE TABLE  (Session 039 P3)
   WHAT: per raw sound parameter, the Global transmit address: 0 none,
         1..127 = CC number, 128 + n = NRPN n (n <= 92). Built once, lazily,
         by globalMidiParser_buildTxAddressTable().
   WHY:  O(1) lookup for menu TX without duplicating the receive tables.
   WRITER: globalMidiParser_buildTxAddressTable(). READER:
           globalMidiParser_getTxAddress(). */
static uint8_t globalMidiParser_txAddress[END_OF_SOUND_PARAMETERS];
static uint8_t globalMidiParser_txAddressBuilt = 0;

/* TX EXCLUSION TEST  (Session 039 P3)
   WHAT: true for the 18 modulation target selectors (user decision 4).
   WHY:  the menu stores a modTargets index for these and receive stores the
         NRPN value raw; the two domains are not verified to match.
   INPUT: rawParam. OUTPUT: 1 excluded, 0 allowed.
   ACCESSORS: globalMidiParser_buildTxAddressTable(). */
static uint8_t globalMidiParser_isTxExcluded(uint16_t rawParam)
{
   return (uint8_t)(((rawParam >= PAR_VEL_DEST_1) && (rawParam <= PAR_VEL_DEST_6))
                 || ((rawParam >= PAR_VOICE_LFO1) && (rawParam <= PAR_VOICE_LFO6))
                 || ((rawParam >= PAR_TARGET_LFO1) && (rawParam <= PAR_TARGET_LFO6)));
}

/* TX REVERSE TABLE BUILDER  (Session 039 P3)
   WHAT: inverts globalMidiParser_nrpnToRawParam[] first, then
         globalMidiParser_ccToRawParam[2..127] so a CC address overwrites an
         NRPN for the same parameter (CC preferred: 3 bytes instead of 9).
         Skips PAR_NONE slots (CC0/1/6/98/99 and unmapped), out-of-range ids
         and the excluded selectors.
   WHY:  one derivation from the receive tables (see getTxAddress()).
   OUTPUT: globalMidiParser_txAddress[] filled; built flag set.
   ACCESSORS: globalMidiParser_getTxAddress() (first call). */
static void globalMidiParser_buildTxAddressTable(void)
{
   uint16_t i;
   const uint16_t nrpnCount = (uint16_t)(sizeof(globalMidiParser_nrpnToRawParam)
                                       / sizeof(globalMidiParser_nrpnToRawParam[0]));

   for(i = 0; i < nrpnCount; i++)
   {
      const uint16_t raw = globalMidiParser_nrpnToRawParam[i];
      if(raw == PAR_NONE || raw >= END_OF_SOUND_PARAMETERS || globalMidiParser_isTxExcluded(raw))
         continue;
      globalMidiParser_txAddress[raw] = (uint8_t)(128u + i);
   }

   for(i = 2; i < 128; i++)
   {
      const uint16_t raw = globalMidiParser_ccToRawParam[i];
      if(raw == PAR_NONE || raw >= END_OF_SOUND_PARAMETERS || globalMidiParser_isTxExcluded(raw))
         continue;
      globalMidiParser_txAddress[raw] = (uint8_t)i;
   }

   globalMidiParser_txAddressBuilt = 1;
}

/* GLOBAL TX ADDRESS LOOKUP (Session 039 P3); full contract in
   GlobalMidiParser.h. */
uint8_t globalMidiParser_getTxAddress(uint16_t rawParam, uint8_t *number)
{
   uint8_t code;

   if(rawParam >= END_OF_SOUND_PARAMETERS)
      return GLOBAL_MIDI_TX_NONE;

   if(!globalMidiParser_txAddressBuilt)
      globalMidiParser_buildTxAddressTable();

   /* ¢ mirror (user decision 5): record what a ¢-mode receiver accepts. */
   if(rawParam == PAR_FILTER_FREQ_1 && globalMidiParser_cc38Swap)
   {
      *number = GLOBAL_MIDI_NRPN_FILTER_FREQ_1;
      return GLOBAL_MIDI_TX_NRPN;
   }

   code = globalMidiParser_txAddress[rawParam];
   if(code == 0)
      return GLOBAL_MIDI_TX_NONE;

   if(code < 128)
   {
      *number = code;
      return GLOBAL_MIDI_TX_CC;
   }

   *number = (uint8_t)(code - 128);
   return GLOBAL_MIDI_TX_NRPN;
}
```

**(b) Lines 36-49: REPLACE.** Remove the `GLOBAL SPECIAL NRPN NUMBERS` comment block and the four `#define GLOBAL_MIDI_NRPN_*` lines (36-49). They now live in `GlobalMidiParser.h` (G1), which this file already includes. Put in their place:

```c
/* Special NRPN numbers (93-96) are defined in GlobalMidiParser.h so receive
   and menu TX share them (Session 039 P3). */
```

Lines 50-53 (`GLOBAL_MIDI_ROLL_RATE_MAX`, the `GLOBAL_MIDI_RPN_NULL_VALUE` comment and define) stay in place.

---

## 5. Menu TX queue and pacing (output control)

### O1 — `MIDI/MidiOutputControl.h` after line 67: ADD

Anchor: `void outputControl_sendRealtime(uint8_t status);`

```c

/* MENU TX PERF SLOTS  (Session 039 P3)
   Identify PERF-page controls that have a Global MIDI address but are not raw
   sound parameters: roll rate/note/velocity (NRPN 93/94/95), global morph
   (CC1). Passed to outputControl_queueMenuPerfTx(). */
#define OUTPUT_MENU_TX_ROLL_RATE       0
#define OUTPUT_MENU_TX_ROLL_NOTE       1
#define OUTPUT_MENU_TX_ROLL_VELOCITY   2
#define OUTPUT_MENU_TX_GLOBAL_MORPH    3

/* QUEUE A MENU SOUND-PARAMETER CHANGE FOR MIDI TX  (Session 039 P3)
   WHAT: records the latest value of a raw sound parameter changed in the
         menu, to be sent on the Global channel as its CC/NRPN by
         outputControl_serviceMenuTx(). Repeated edits coalesce: the latest
         value wins.
   WHY:  "txf C" = menu parameter changes are transmitted (user spec), so knob
         motions can be recorded in a DAW and played back into the LXR.
   GATE: TX filter C (0x40) on, Global channel 1..16, no automation step
         armed; unmapped or excluded parameters are ignored.
   INPUT:  rawParam - AVR/Preset raw id; value - 7-bit value as applied.
   OUTPUT: slot marked dirty (or nothing when the gate is closed).
   ACCESSORS: frontParser_handleMidiMessage() FRONT_CC_MENU / FRONT_CC_2_MENU
              cases (frontPanelReceivingProtocol.c) only.
   AFFILIATES: globalMidiParser_getTxAddress(), seq_isAutomationStepArmed(),
               midiParser_txRxFilter, midi_MidiChannels[7]. */
void outputControl_queueMenuParameterTx(uint16_t rawParam, uint8_t value);

/* QUEUE A MENU PERF-CONTROL CHANGE FOR MIDI TX  (Session 039 P3)
   WHAT: as outputControl_queueMenuParameterTx() for the PERF slots above.
         Roll rate is the 0..15 selector; roll note/velocity 0..127; global
         morph 0..255, converted to CC1 as amount >> 1 (the inverse of
         receive preset_morphAutomationValueToAmount(): v*2, 127 -> 255).
   INPUT:  perfSlot - OUTPUT_MENU_TX_*; value - control value as applied.
   OUTPUT: slot marked dirty (or nothing when the gate is closed).
   ACCESSORS: frontParser_handleMidiMessage() FRONT_SEQ_ROLL_RATE/_NOTE/
              _VELOCITY and FRONT_SEQ_SET_GLOBAL_MORPH_MSB cases (menu-only
              senders on the AVR).
   AFFILIATES: GLOBAL_MIDI_NRPN_ROLL_* (GlobalMidiParser.h), MOD_WHEEL. */
void outputControl_queueMenuPerfTx(uint8_t perfSlot, uint8_t value);

/* MENU TX SERVICE  (Session 039 P3)
   WHAT: emits at most one queued menu change per call, round-robin, as a
         Global-channel CC (3 bytes) or NRPN (CC99 0, CC98 n, CC6 v; 9 bytes),
         only when both DIN and USB outputs can take the whole message.
         Drops everything queued if the gate has closed.
   WHY:  bounded, paced output: knob sweeps cannot saturate DIN (31,250 baud)
         or split messages, and sequencer notes/clock keep their direct path.
   OUTPUT: MIDI bytes via outputControl_sendMidi(); queue state updated.
   ACCESSORS: main.c main loop, immediately before usb_tick().
   AFFILIATES: uart_midiTxFree(), usb_midiTxFree(), outputControl_sendMidi(). */
void outputControl_serviceMenuTx(void);
```

### O2 — `MIDI/MidiOutputControl.c`

**(a) After line 78** (closing `}` of `outputControl_sendRealtime()`): **ADD**

```c

//----------------------------------------------------------------
/* MENU TX QUEUE STATE  (Session 039 P3)
   WHAT: one slot per raw sound parameter plus 4 PERF slots
         (OUTPUT_MENU_TX_*). value[] holds the latest 7-bit value; dirty[] is
         a bitmap of slots waiting to be sent; pending counts the dirty bits;
         cursor is the round-robin scan position.
   WHY:  coalescing (user decision 8): a knob sweep keeps only its latest
         value per parameter, and the service emits at a bounded rate.
   WRITERS: outputControl_menuTxMark(), outputControl_serviceMenuTx().
   READERS: outputControl_serviceMenuTx(). About 280 bytes of STM RAM. */
#define MENU_TX_PERF_SLOTS   4
#define MENU_TX_NUM_SLOTS    (END_OF_SOUND_PARAMETERS + MENU_TX_PERF_SLOTS)
#define MENU_TX_FILTER_BIT   0x40   /* TX nibble C (midi_setFilter() layout) */

static uint8_t  outputControl_menuTxValue[MENU_TX_NUM_SLOTS];
static uint8_t  outputControl_menuTxDirty[(MENU_TX_NUM_SLOTS + 7) / 8];
static uint16_t outputControl_menuTxPending = 0;
static uint16_t outputControl_menuTxCursor = 0;

/* MENU TX GATE  (Session 039 P3)
   WHAT: returns the Global MIDI channel (1..16) when menu TX is allowed, else 0.
   WHY:  user decisions: TX only with txf C on, and only on the Global channel,
         which must not be 0 ("off").
   INPUT:  midiParser_txRxFilter, midi_MidiChannels[7].
   OUTPUT: channel 1..16, or 0 = closed.
   ACCESSORS: queue functions and outputControl_serviceMenuTx(). */
static uint8_t outputControl_menuTxChannel(void)
{
   const uint8_t channel = midi_MidiChannels[7];

   if(!(midiParser_txRxFilter & MENU_TX_FILTER_BIT))
      return 0;
   if(channel == 0 || channel > 16)
      return 0;
   return channel;
}

/* MENU TX MARK  (Session 039 P3)
   WHAT: stores value in slot and sets its dirty bit, counting newly dirty
         slots. INPUT: slot < MENU_TX_NUM_SLOTS, value 0..127.
   ACCESSORS: outputControl_queueMenuParameterTx(), outputControl_queueMenuPerfTx(). */
static void outputControl_menuTxMark(uint16_t slot, uint8_t value)
{
   const uint8_t mask = (uint8_t)(1u << (slot & 7u));

   outputControl_menuTxValue[slot] = (uint8_t)(value & 0x7f);
   if(!(outputControl_menuTxDirty[slot >> 3] & mask))
   {
      outputControl_menuTxDirty[slot >> 3] |= mask;
      outputControl_menuTxPending++;
   }
}

/* MENU TX ADDRESS RESOLVE  (Session 039 P3)
   WHAT: maps a slot to its Global address at emit time (so a ¢ toggle
         between queue and emit is honoured). Sound slots use
         globalMidiParser_getTxAddress(); PERF slots are fixed: roll ->
         NRPN 93/94/95, global morph -> CC1 (MOD_WHEEL).
   INPUT:  slot; number - out. OUTPUT: GLOBAL_MIDI_TX_NONE/_CC/_NRPN.
   ACCESSORS: outputControl_serviceMenuTx(). */
static uint8_t outputControl_menuTxResolve(uint16_t slot, uint8_t *number)
{
   if(slot < END_OF_SOUND_PARAMETERS)
      return globalMidiParser_getTxAddress(slot, number);

   switch(slot - END_OF_SOUND_PARAMETERS)
   {
      case OUTPUT_MENU_TX_ROLL_RATE:     *number = GLOBAL_MIDI_NRPN_ROLL_RATE;     return GLOBAL_MIDI_TX_NRPN;
      case OUTPUT_MENU_TX_ROLL_NOTE:     *number = GLOBAL_MIDI_NRPN_ROLL_NOTE;     return GLOBAL_MIDI_TX_NRPN;
      case OUTPUT_MENU_TX_ROLL_VELOCITY: *number = GLOBAL_MIDI_NRPN_ROLL_VELOCITY; return GLOBAL_MIDI_TX_NRPN;
      case OUTPUT_MENU_TX_GLOBAL_MORPH:  *number = MOD_WHEEL;                      return GLOBAL_MIDI_TX_CC;
      default:                           return GLOBAL_MIDI_TX_NONE;
   }
}

/* MENU TX CC EMITTER  (Session 039 P3)
   WHAT: sends one control change on channel (1..16) to DIN and USB.
   ACCESSORS: outputControl_serviceMenuTx(). AFFILIATES: outputControl_sendMidi(). */
static void outputControl_emitCc(uint8_t channel, uint8_t cc, uint8_t value)
{
   MidiMsg msg = {0};

   msg.status = (uint8_t)(MIDI_CC | ((channel - 1u) & 0x0F));
   msg.data1 = (uint8_t)(cc & 0x7f);
   msg.data2 = (uint8_t)(value & 0x7f);
   msg.bits.length = 2;
   outputControl_sendMidi(msg);
}

//----------------------------------------------------------------
/* QUEUE A MENU SOUND-PARAMETER CHANGE (Session 039 P3); see MidiOutputControl.h. */
void outputControl_queueMenuParameterTx(uint16_t rawParam, uint8_t value)
{
   uint8_t number;

   if(rawParam >= END_OF_SOUND_PARAMETERS)
      return;
   if(!outputControl_menuTxChannel() || seq_isAutomationStepArmed())
      return;
   if(globalMidiParser_getTxAddress(rawParam, &number) == GLOBAL_MIDI_TX_NONE)
      return;

   outputControl_menuTxMark(rawParam, value);
}

//----------------------------------------------------------------
/* QUEUE A MENU PERF-CONTROL CHANGE (Session 039 P3); see MidiOutputControl.h. */
void outputControl_queueMenuPerfTx(uint8_t perfSlot, uint8_t value)
{
   if(perfSlot >= MENU_TX_PERF_SLOTS)
      return;
   if(!outputControl_menuTxChannel() || seq_isAutomationStepArmed())
      return;

   if(perfSlot == OUTPUT_MENU_TX_GLOBAL_MORPH)
      value = (uint8_t)(value >> 1);   /* 0..255 -> 0..127; receive maps 127 -> 255 */

   outputControl_menuTxMark((uint16_t)(END_OF_SOUND_PARAMETERS + perfSlot), value);
}

//----------------------------------------------------------------
/* MENU TX SERVICE (Session 039 P3); see MidiOutputControl.h.
   RPN-NULL GUARD: CC100/CC101 = 127 directly after an NRPN data entry is an
   NRPN deselect for an LXR receiver (Session 039 P1). Before emitting
   CC100/101 = 127 a harmless CC99 = 0 is sent, which closes the receiver's
   null window, so recorded Snare/Cymbal hard-pan plays back. */
void outputControl_serviceMenuTx(void)
{
   uint8_t channel;
   uint16_t scanned;

   if(outputControl_menuTxPending == 0)
      return;

   channel = outputControl_menuTxChannel();
   if(channel == 0)
   {
      /* Gate closed after queuing (txf C off or channel 0): drop all. */
      memset(outputControl_menuTxDirty, 0, sizeof(outputControl_menuTxDirty));
      outputControl_menuTxPending = 0;
      return;
   }

   for(scanned = 0; scanned < MENU_TX_NUM_SLOTS; scanned++)
   {
      const uint16_t slot = outputControl_menuTxCursor;
      const uint8_t mask = (uint8_t)(1u << (slot & 7u));

      if(outputControl_menuTxDirty[slot >> 3] & mask)
      {
         uint8_t number = 0;
         const uint8_t value = outputControl_menuTxValue[slot];
         const uint8_t kind = outputControl_menuTxResolve(slot, &number);

         if(kind != GLOBAL_MIDI_TX_NONE)
         {
            const uint8_t guard = (uint8_t)((kind == GLOBAL_MIDI_TX_CC)
                                  && (number == RPN_LSB || number == RPN_MSB)
                                  && (value == 127));
            const uint8_t dinNeed = (uint8_t)(((kind == GLOBAL_MIDI_TX_CC) ? 3u : 9u) + (guard ? 3u : 0u));
            const uint8_t usbNeed = (uint8_t)(((kind == GLOBAL_MIDI_TX_CC) ? 4u : 12u) + (guard ? 4u : 0u));

            if(uart_midiTxFree() < dinNeed || usb_midiTxFree() < usbNeed)
               return;   /* retry this slot on a later pass */

            if(kind == GLOBAL_MIDI_TX_NRPN)
            {
               outputControl_emitCc(channel, NRPN_COARSE, 0);
               outputControl_emitCc(channel, NRPN_FINE, number);
               outputControl_emitCc(channel, NRPN_DATA_ENTRY_COARSE, value);
            }
            else
            {
               if(guard)
                  outputControl_emitCc(channel, NRPN_COARSE, 0);
               outputControl_emitCc(channel, number, value);
            }
         }

         outputControl_menuTxDirty[slot >> 3] &= (uint8_t)~mask;
         outputControl_menuTxPending--;
         if(++outputControl_menuTxCursor >= MENU_TX_NUM_SLOTS)
            outputControl_menuTxCursor = 0;
         return;   /* at most one message (or one drop) per pass */
      }

      if(++outputControl_menuTxCursor >= MENU_TX_NUM_SLOTS)
         outputControl_menuTxCursor = 0;
   }
}
```

Notes:
- `NRPN_COARSE` (99), `NRPN_FINE` (98) and `NRPN_DATA_ENTRY_COARSE` (6) are the LXR-enum names the receive side uses. `RPN_LSB` (100) and `RPN_MSB` (101) are the external-CC enum names the P1 null logic uses. `MOD_WHEEL` (1) and `MIDI_CC` (0xb0) come from `MidiMessages.h`.
- `memset` needs `<string.h>`: add it in (b).

**(b) After line 50** (`#include "usb_manager.h"`): **ADD**

```c
#include "GlobalMidiParser.h"      /* globalMidiParser_getTxAddress(), GLOBAL_MIDI_NRPN_* (Session 039 P3) */
#include "Preset/ParameterArray.h" /* END_OF_SOUND_PARAMETERS (menu TX slots) */
#include <string.h>                /* memset (menu TX queue) */
```

---

## 6. Front-panel ingress (STM)

### P1 — `uARTFrontSYX/frontPanelReceivingProtocol.h` after line 61: ADD

Anchor: `#define PARAM_RESTORE_ACK             0xc9	// acknowledge a parameter restore request`

```c
/* MENU-ORIGINATED SOUND-PARAMETER EDITS, AVR -> STM  (Session 039 P3)
   WHAT: identical payload to MIDI_CC (raw id 0..127) / FRONT_CC_2 (raw id - 128)
         but sent only by AVR menu encoder/knob edits
         (menu_sendSoundParameter()).
   WHY:  the plain opcodes are also used by file/voice load and the
         armed-step revert, so the STM could not tell a menu edit apart.
         Only menu edits may be transmitted as MIDI CC/NRPN when txf C is on.
         Both are handled exactly like the plain opcodes, plus a TX queue call.
   ACCESSORS: frontParser_handleMidiMessage() MIDI_CC / FRONT_CC_2 cases.
   AFFILIATES: AVR MIDI_CC_MENU / CC_2_MENU (avrCommsReceivingProtocol.h,
               same values), outputControl_queueMenuParameterTx(). */
#define FRONT_CC_MENU                 0xca	// menu edit, parameter 0..127
#define FRONT_CC_2_MENU               0xcb	// menu edit, parameter 128+
```

### P2 — `uARTFrontSYX/frontPanelReceivingProtocol.c`

**(a) Lines 2741-2749: MODIFY** (roll cases). Replace

```c
      case FRONT_SEQ_ROLL_RATE:
         seq_setRollRate(frontParser_command.data2);
         break;
      case FRONT_SEQ_ROLL_NOTE:
         seq_setRollNote(frontParser_command.data2);
         break;
      case FRONT_SEQ_ROLL_VELOCITY:
         seq_setRollVelocity(frontParser_command.data2);
         break;
```

with

```c
      /* PERF ROLL CONTROLS + MENU TX  (Session 039 P3)
         WHAT: apply the roll setting, then queue it for MIDI TX as Global
               NRPN 93/94/95 (user decision 6).
         WHY:  these opcodes are sent only from AVR menu_parseGlobalParam()
               (encoder/knob edits; the armed-step revert after DISARM sends
               the restored value, which is harmless). Never from load or boot
               (menu_sendAllGlobals() starts at PAR_BEGINNING_OF_GLOBALS).
         INPUT:  data2 - rate 0..15 / note / velocity.
         OUTPUT: sequencer roll state; queued TX (gate in MidiOutputControl.c).
         AFFILIATES: outputControl_queueMenuPerfTx(), GlobalMidiParser NRPN
                     93-95 receive (same numbers). */
      case FRONT_SEQ_ROLL_RATE:
         seq_setRollRate(frontParser_command.data2);
         outputControl_queueMenuPerfTx(OUTPUT_MENU_TX_ROLL_RATE, frontParser_command.data2);
         break;
      case FRONT_SEQ_ROLL_NOTE:
         seq_setRollNote(frontParser_command.data2);
         outputControl_queueMenuPerfTx(OUTPUT_MENU_TX_ROLL_NOTE, frontParser_command.data2);
         break;
      case FRONT_SEQ_ROLL_VELOCITY:
         seq_setRollVelocity(frontParser_command.data2);
         outputControl_queueMenuPerfTx(OUTPUT_MENU_TX_ROLL_VELOCITY, frontParser_command.data2);
         break;
```

**(b) Lines 2424-2431: MODIFY** (global morph MSB). Replace

```c
      case FRONT_SEQ_SET_GLOBAL_MORPH_MSB:
      {
         uint8_t morphAmount =
            (uint8_t)(frontParser_globalMorphLsb
               | ((frontParser_command.data2 & 0x01) << 7));
         seq_setGlobalMorphAmount(morphAmount);
         break;
      }
```

with

```c
      /* GLOBAL MORPH (PERF MENU) + MENU TX  (Session 039 P3)
         WHAT: assemble the 8-bit amount, apply it, then queue it for MIDI TX
               as Global CC1 (user decision 6; converted to amount >> 1 in
               outputControl_queueMenuPerfTx()).
         WHY:  SEQ_SET_GLOBAL_MORPH_LSB/MSB are sent only by AVR
               menu_parseGlobalParam() PAR_MORPH. Global CC1 on receive drives
               global morph (v*2, 127 -> 255), so a DAW recording plays back.
         INPUT:  frontParser_globalMorphLsb, data2 bit 0.
         OUTPUT: global morph applied; queued TX.
         AFFILIATES: seq_setGlobalMorphAmount(), preset_morphAutomationValueToAmount(). */
      case FRONT_SEQ_SET_GLOBAL_MORPH_MSB:
      {
         uint8_t morphAmount =
            (uint8_t)(frontParser_globalMorphLsb
               | ((frontParser_command.data2 & 0x01) << 7));
         seq_setGlobalMorphAmount(morphAmount);
         outputControl_queueMenuPerfTx(OUTPUT_MENU_TX_GLOBAL_MORPH, morphAmount);
         break;
      }
```

**(c) Lines 2020-2035: MODIFY** (`FRONT_CC_2`). Replace

```c
      case FRONT_CC_2: // frontParser_command.status
         {
            if(preset_shouldApplyIngressToLive())
            {
               frontParser_applyParameterCommand(frontParser_command,1);
               //record automation if record is turned on
               seq_recordAutomation(frontParser_activeTrack, frontParser_command.data1+128, frontParser_command.data2);
            }
            else
            {
               /* FILE LOAD: Store normal-image params without changing the
                  temporary sound when the temporary pattern is active. */
               preset_storeParameterIngress(frontParser_command.data1+128, frontParser_command.data2);
            }
         }
         break;
```

with

```c
      /* SOUND PARAMETER 128+ (plain or menu-originated)
         WHAT: FRONT_CC_2 and FRONT_CC_2_MENU (Session 039 P3) share one body.
               The message is normalised to FRONT_CC_2 for the apply path; a
               menu edit that was applied live is also queued for MIDI TX.
         WHY:  identical sound/storage/record behaviour for both opcodes; only
               menu edits may be transmitted (loads/reverts use FRONT_CC_2).
               Values only stored (temp playback active) are not audible and
               are not transmitted.
         INPUT:  data1 = raw id - 128, data2 = value.
         OUTPUT: live apply + automation record, or normal-image store; TX queue.
         ACCESSORS: AVR avrComms_sendData(CC_2 / CC_2_MENU, ...).
         AFFILIATES: frontParser_applyParameterCommand(), seq_recordAutomation(),
                     outputControl_queueMenuParameterTx(). */
      case FRONT_CC_2_MENU:
      case FRONT_CC_2: // frontParser_command.status
         {
            const uint8_t isMenuEdit = (uint8_t)(frontParser_command.status == FRONT_CC_2_MENU);
            MidiMsg cc2Msg = frontParser_command;

            cc2Msg.status = FRONT_CC_2;

            if(preset_shouldApplyIngressToLive())
            {
               frontParser_applyParameterCommand(cc2Msg,1);
               //record automation if record is turned on
               seq_recordAutomation(frontParser_activeTrack, frontParser_command.data1+128, frontParser_command.data2);

               if(isMenuEdit)
                  outputControl_queueMenuParameterTx((uint16_t)(frontParser_command.data1 + 128),
                                                     frontParser_command.data2);
            }
            else
            {
               /* FILE LOAD: Store normal-image params without changing the
                  temporary sound when the temporary pattern is active. */
               preset_storeParameterIngress(frontParser_command.data1+128, frontParser_command.data2);
            }
         }
         break;
```

**(d) Lines 1994-2017: MODIFY** (`MIDI_CC`). Replace

```c
      case MIDI_CC: //frontParser_command.status
         // this is for parameters below 128
         {
            uint8_t rawParam = frontParser_command.data1;
            MidiMsg liveMsg = frontParser_command;

            liveMsg.data1 = (uint8_t)((rawParam + 1) & 0x7f);
```

(the rest of the case through line 2017 is kept as is, plus one addition in the live branch) with

```c
      /* SOUND PARAMETER 0..127 (plain or menu-originated)
         WHAT: MIDI_CC and FRONT_CC_MENU (Session 039 P3) share one body. The
               live message is forced to status MIDI_CC, because
               frontParser_applyParameterCommand() treats any other status as
               CC2. A menu edit that was applied live is also queued for MIDI TX.
         WHY:  identical behaviour for both opcodes; only menu edits may be
               transmitted. Stored-only values (temp playback) are not sent.
         INPUT:  data1 = raw id, data2 = value.
         OUTPUT: store + live apply (+1 apply id) + automation record, or
                 normal-image store; TX queue.
         ACCESSORS: AVR avrComms_sendData(MIDI_CC / MIDI_CC_MENU, ...).
         AFFILIATES: preset_storeParameterIngress(),
                     frontParser_applyParameterCommand(), seq_recordAutomation(),
                     outputControl_queueMenuParameterTx(). */
      case FRONT_CC_MENU:
      case MIDI_CC: //frontParser_command.status
         // this is for parameters below 128
         {
            const uint8_t isMenuEdit = (uint8_t)(frontParser_command.status == FRONT_CC_MENU);
            uint8_t rawParam = frontParser_command.data1;
            MidiMsg liveMsg = frontParser_command;

            liveMsg.status = MIDI_CC;
            liveMsg.data1 = (uint8_t)((rawParam + 1) & 0x7f);
```

In the same case's live branch, **after** the existing line `seq_recordAutomation(frontParser_activeTrack, rawParam, frontParser_command.data2);` (currently 2008), **ADD**

```c

               if(isMenuEdit)
                  outputControl_queueMenuParameterTx(rawParam, frontParser_command.data2);
```

The `else` (FILE LOAD) branch and `break;` are unchanged.

**(e) After line 50** (`#include "MidiParser.h"`): **ADD** `#include "MidiOutputControl.h"	/* menu MIDI TX queue (Session 039 P3) */`

---

## 7. Main loop

### M1 — `main.c`

**(a) Line 289: ADD before** `usb_tick();`:

```c
		/* MENU MIDI TX SERVICE  (Session 039 P3)
		   Emit at most one queued menu CC/NRPN per pass, before usb_tick() so
		   it is flushed to USB in the same pass. Bounded: one message (3-9
		   DIN bytes / 4-12 USB bytes) and an O(slots) scan only when work is
		   pending. See outputControl_serviceMenuTx(). */
		outputControl_serviceMenuTx();
```

**(b) After line 65** (`#include "MidiParser.h"`): **ADD** `#include "MidiOutputControl.h"	/* outputControl_serviceMenuTx() (Session 039 P3) */`

---

## 8. AVR

### A1 — `avrComms/avrCommsReceivingProtocol.h` after line 88: ADD

Anchor: `#define PARAM_RESTORE_ACK     0xc9	// acknowledge a parameter restore request`

```c
/* MENU-ORIGINATED SOUND-PARAMETER EDITS, AVR -> STM  (Session 039 P3)
   WHAT: same payload as MIDI_CC (raw id 0..127) / CC_2 (raw id - 128); sent
         only by menu encoder/knob edits via menu_sendSoundParameter().
   WHY:  lets the STM transmit menu changes as MIDI CC/NRPN (txf C) while
         loads, the armed-step revert and restores keep MIDI_CC/CC_2 and are
         never transmitted.
   AFFILIATES: STM FRONT_CC_MENU / FRONT_CC_2_MENU (frontPanelReceivingProtocol.h,
               same values). */
#define MIDI_CC_MENU          0xca	// menu edit, parameter 0..127
#define CC_2_MENU             0xcb	// menu edit, parameter 128+
```

### A2 — `Menu/menu.c`

**(a) Lines 4409-4412: MODIFY** (knob default branch). Replace

```c
		else if(paramNr<128) // => Sound Parameter below 128
			avrComms_sendData(MIDI_CC,(uint8_t)paramNr,dtypeValue);
		else if(paramNr>=128 && (paramNr < END_OF_SOUND_PARAMETERS)) // => Sound Parameter above 127
			avrComms_sendData(CC_2,(uint8_t)(paramNr-128),dtypeValue);
```

with

```c
		else if(paramNr < END_OF_SOUND_PARAMETERS) // => Sound Parameter: menu edit (Session 039 P3)
			menu_sendSoundParameter(paramNr, dtypeValue);
```

**(b) Lines 4396-4399: MODIFY** (knob `DTYPE_AUTOM_TARGET` non-PERF branch). Replace

```c
         if(paramNr<128) // => Sound Parameter below 128
   			avrComms_sendData(MIDI_CC,(uint8_t)paramNr,dtypeValue);
   		else if(paramNr>=128 && (paramNr < END_OF_SOUND_PARAMETERS)) // => Sound Parameter above 127
   			avrComms_sendData(CC_2,(uint8_t)(paramNr-128),dtypeValue);
```

with

```c
         if(paramNr < END_OF_SOUND_PARAMETERS) // => Sound Parameter: menu edit (Session 039 P3)
            menu_sendSoundParameter(paramNr, dtypeValue);
```

(The following `else menu_parseGlobalParam(...)` is unchanged.)

**(c) Lines 3104-3117: MODIFY** (encoder edit). Replace

```c
	else if(paramNr < 128) // => Sound Parameter
   {
      if (!isMorphParam)
      {
		   avrComms_sendData(MIDI_CC,(uint8_t)paramNr,*paramValue);
      }
   }
	else if(paramNr > 127 && (paramNr < END_OF_SOUND_PARAMETERS)) // => Sound Parameter above 127
   {
      if (!isMorphParam)
      {
		   avrComms_sendData(CC_2,(uint8_t)(paramNr-128),*paramValue);
      }
   }
```

with

```c
	else if(paramNr < END_OF_SOUND_PARAMETERS) // => Sound Parameter: menu edit (Session 039 P3)
   {
      if (!isMorphParam)
      {
         menu_sendSoundParameter(paramNr, *paramValue);
      }
   }
```

**(d) After line 2879** (closing `}` of `menu_sendVoiceMorphAmount()`): **ADD**

```c

//-----------------------------------------------------------------
/* MENU SOUND-PARAMETER SEND  (Session 039 P3)
   WHAT: sends a sound-parameter change made with the menu encoder or a knob
         to the STM as MIDI_CC_MENU (raw id 0..127) or CC_2_MENU (raw id
         128..END_OF_SOUND_PARAMETERS-1). Ignores non-sound ids.
   WHY:  the STM applies these exactly like MIDI_CC/CC_2 and, when txf C is on,
         also transmits them as Global CC/NRPN. Using distinct opcodes keeps
         load, revert and restore traffic (still MIDI_CC/CC_2) from ever being
         transmitted. The armed-step and gate checks are on the STM.
   INPUT:  paramNr - raw id; value - 7-bit parameter value.
   OUTPUT: one 3-byte packet to the STM (same size as before).
   ACCESSORS: menu_encoderChangeParameter(), menu_parseKnobValue() (two branches).
   AFFILIATES: STM FRONT_CC_MENU / FRONT_CC_2_MENU handling
               (frontPanelReceivingProtocol.c), outputControl_queueMenuParameterTx(). */
static void menu_sendSoundParameter(uint16_t paramNr, uint8_t value)
{
   if(paramNr < 128)
      avrComms_sendData(MIDI_CC_MENU, (uint8_t)paramNr, value);
   else if(paramNr < END_OF_SOUND_PARAMETERS)
      avrComms_sendData(CC_2_MENU, (uint8_t)(paramNr - 128), value);
}
```

It is defined before its first use (3104), so no prototype is needed. `MIDI_CC_MENU`/`CC_2_MENU` reach `menu.c` the same way `MIDI_CC` does: `menu.c:13` includes `avrCommsSendingProtocol.h`, which includes `avrCommsReceivingProtocol.h` (verified).

**No change:**
- `menu.c:3333-3346`: unreachable for sound parameters (§0.2).
- `buttonHandler.c:284-298`: armed-step revert, not a menu edit.
- `presetManager.c:694` and `:2226-2230`: loads.

---

## 9. Documentation

- **K1 — `knowledge_files/comms_spec_reference/MIDI_TABLE.md`, append at end (after line 434):** a `## MIDI Transmit (TX filter C, Session 039 P3)` section. Content: §1 of the plan; the gate (txf C, Global channel 1-16, not armed); the address derivation (inverse of the receive tables, CC preferred); exclusions (selectors, envelope position, per-voice morph); ¢ mirror (Drum 1 cutoff → NRPN 96); PERF (NRPN 93-95, CC1 = amount >> 1); NRPN framing (99 = 0, 98, 6; no CC38, no null); the CC100/101 = 127 guard; coalescing and pacing; whole-message DIN/USB; and that received MIDI is never re-transmitted.
- **K2 — `COMMS_FLOW_SPEC.md`, after line 196:** ADD a bullet for `` `MIDI_CC_MENU` / `FRONT_CC_MENU` (`0xca`) and `CC_2_MENU` / `FRONT_CC_2_MENU` (`0xcb`) - menu-originated sound-parameter edits; same payload and handling as `MIDI_CC`/`CC_2`, plus STM menu MIDI TX when txf C is on (Session 039 P3). Loads/reverts keep the plain opcodes. ``
- **K3 — `MEMORY.md`:**
  - **Line 459:** replace the trailing `TX C has no transmitter.` with `TX C (0x40) transmits menu edits only (FRONT_CC_MENU/FRONT_CC_2_MENU, PERF roll/morph) as Global-channel CC/NRPN via the coalescing queue in MidiOutputControl.c (Session 039 P3); DIN and USB MIDI out are whole-message-or-drop; USB IN waits for DataIn with a 5 ms timeout.`
  - Add a status paragraph for P3 at closeout.
- **K4 — `manual/chapters/05_operation_modes.md` line 361:** replace with `txf & Transmit MIDI Filter & Sets what MIDI commands are transmitted from the LXR. With C on and a global MIDI channel set, parameter changes made in the menu are sent as the CC or NRPN listed in the MIDI tables, on the global channel. \\`
- **K5 — `manual/chapters/07_midi.md` after line 52** (end of *Transmitting Sequencer Data to the MIDI Output*): ADD a paragraph headed **Transmitting parameter changes**. Content:
  - With `txf` including C and the global MIDI channel set (not 0), every parameter change made with the knobs or encoder is sent on the global channel as the CC or NRPN listed in the tables. This includes the Performance roll rate/note/velocity (NRPN 93-95) and global morph (CC1).
  - Use it to record knob motions in a DAW and play them back into the LXR (`rxf` must include C).
  - Not sent: loads, automation, LFOs, changes made while an automation step is armed, the LFO/velocity target selectors, envelope position and per-voice morph.
  - In a ¢ receive-filter entry, voice 1 filter frequency is sent as NRPN 96 instead of CC38.
- **K6 — `manual/latex/midi_tables.tex`, append at end of file:** a `{\small …}` note. Content: "With `txf` C on and a global channel set, menu changes are transmitted using the numbers in these tables on the global channel. NRPNs are sent as CC99 = 0, CC98, CC6. LFO/velocity target selectors are not transmitted."

---

## 10. Build and verification

1. **Builds:** `make -C mainboard/LxrStm32 -j8 stm32`, `make -C front/LxrAvr avr -j4`, then `make firmware`.
   - No new warnings in touched files.
   - **AVR binary ≤ 57,344 bytes.** It is 56,546 now; expect a net change of a few tens of bytes, since the four call sites shrink and the helper is added.
   - Both binaries embedded in `FIRMWARE.BIN` (AVR at 512, STM at 57,344).
2. **Host harness** (verbatim copies into the scratchpad):
   - `buildTxAddressTable` + `getTxAddress`: for every raw id, the TX address maps back to the same raw id through the receive tables. Selectors return NONE. Drum 1 cutoff gives CC38 normally and NRPN 96 with swap.
   - Coalescer and service: the latest value wins; never more than one message per call; never emits without DIN/USB room; the gate closing drops everything; the guard is emitted only for CC100/101 = 127.
   - `fifo_freeSpace`: correct at empty, full and wrap.
   - USB model: `usb_sendMidi()` never writes past the current half; the flush respects busy and timeout.
   - Morph: `amount >> 1` then receive `v*2`/`127 → 255` round-trips within 1 for 0-255.
3. **Hardware** (MIDI monitor on DIN and USB, then a DAW): the plan's §9 checklist, items 1-8.

## 11. Red-team

- **Ownership:** STM owns MIDI out, the gate and the mapping. The AVR only tags edit origin. Protocol direction: new AVR→STM opcodes only; no reuse.
- **Behaviour of existing traffic:**
  - plain `MIDI_CC`/`CC_2`: unchanged;
  - DIN output: whole-message-or-drop. Previously it could split a message; now it drops whole ones;
  - USB output: no longer aborts in-flight transfers; discards when unconfigured.
- **Timing:**
  - the service is O(1) when idle and at most one slot scan plus one message per pass otherwise;
  - `DataIn` runs in the OTG ISR and only clears a flag;
  - the 5 ms timeout bounds recovery.
- **Bounds:**
  - slots < `MENU_TX_NUM_SLOTS`;
  - reverse table indexed only below `END_OF_SOUND_PARAMETERS`;
  - NRPN code at most 128 + 92;
  - USB write pointer confined to the current half.
- **Concurrency:** DIN and USB producers are main-loop only (single producer). The ISR consumers only free space. `usb_midiTxBusy` is `volatile` and set before `DCD_EP_Tx()`.
- **AVR flash:** within the 798-byte headroom; build gate enforced.
- **Residual risks:**
  - USB timing changes need hardware validation under load (plan §9.8).
  - If `seq_armAutomationStep()` is ever called without a matching disarm, TX stays suppressed: an existing state, now visible to TX.
