/*
 * Uart.h
 * ------------------------------------------------------------------------------------------------------------------------
 *  Copyright 2013 Julian Schmidt
 *  Julian@sonic-potions.com
 * ------------------------------------------------------------------------------------------------------------------------
 *  This file is part of the Sonic Potions LXR drumsynth firmware.
 * ------------------------------------------------------------------------------------------------------------------------
 *  Redistribution and use of the LXR code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *       - The code may not be sold, nor may it be used in a commercial product or activity.
 *
 *       - Redistributions that are modified from the original source must include the complete
 *         source code, including the source code for all components used by a binary built
 *         from the modified sources. However, as a special exception, the source code distributed
 *         need not include anything that is normally distributed (in either source or binary form)
 *         with the major components (compiler, kernel, and so on) of the operating system on which
 *         the executable runs, unless that component itself accompanies the executable.
 *
 *       - Redistributions must reproduce the above copyright notice, this list of conditions and the
 *         following disclaimer in the documentation and/or other materials provided with the distribution.
 * ------------------------------------------------------------------------------------------------------------------------
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *   USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ------------------------------------------------------------------------------------------------------------------------
 */


#ifndef MIDIUART_H_
#define MIDIUART_H_

#include "MidiMessages.h"

#define ACK 1
#define NACK -1

/* Audio DMA remains the first pending IRQ (priority 0/subpriority 0). USART2
   shares its preemption level but has lower subpriority, so it can preempt USB
   capture while its bounded timestamp-and-queue ISR cannot preempt audio DMA. */
#define MIDI_UART_IRQ_PREPRIO 0
#define MIDI_UART_IRQ_SUBRIO  1

/* Raw MIDI/UART transport ownership. These functions move bytes and service
   the transport FIFOs; protocol composition lives in the parser/send modules. */
void initMidiUart();
void uart_sendMidi(MidiMsg msg);
void uart_sendMidiByte(uint8_t data);
/* DIN MIDI OUT FREE SPACE  (Session 039 P3)
   WHAT: bytes free in the DIN MIDI TX FIFO.
   WHY:  outputControl_serviceMenuTx() paces menu CC/NRPN output so a whole
         message (3 or 9 bytes, plus a 3-byte guard) always fits.
   OUTPUT: free byte count (0..255).
   ACCESSORS: outputControl_serviceMenuTx().
   AFFILIATES: uart_sendMidi(), fifo_freeSpace(). */
uint8_t uart_midiTxFree(void);
void uart_processMidi();
/* Drain DIN system-realtime bytes captured by USART2 RX. This main-loop API
   dispatches only the bounded realtime queue, preserving ordinary MIDI FIFO
   work for uart_processMidi() and keeping sequencer/routing work out of the
   USART ISR. */
void uart_serviceMidiRealtime();

/* Front-panel transport ownership. These are the lowest-level send/process
   entry points used by the front-panel protocol layer. */
void initFrontpanelUart();
void uart_processFront();
void uart_sendFrontpanelByte(uint8_t data);
void uart_sendFrontpanelPriorityByte(uint8_t data);
void uart_sendFrontpanelPriorityByteWait(uint8_t data);
void uart_sendFrontpanelSysExByte(uint8_t data);

void uart_clearFrontFifo();
#endif /* MIDIUART_H_ */
