/*
 * SD_Manager.h
 *
 *  Created on: 24.10.2012
 *      Author: Julian
 *
 *      handles all high level SD functionality. load/save preset/pattern etc
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

#ifndef SD_MANAGER_H_
#define SD_MANAGER_H_
#include "config.h"
#if USE_SD_CARD
#include "stm32f4xx.h"
#include "MidiMessages.h"
#include "MidiParser.h"
#include "stdio.h"
#include "ff.h"
#include "FIFO.h"

#define NUM_PARAMS 185
#define PAR_BEGINNING_OF_GLOBALS 128

#define SD_STATE_IDLE 			0
#define SD_STATE_LOAD_SOUND		1
#define SD_STATE_SAVE_SOUND		2
#define SD_STATE_LOAD_PATTERN	3

void sdManager_init();
uint8_t sdManager_loadDrumset(uint8_t presetNr, uint8_t isMorph);
void sdManager_loadPattern(uint8_t patternNr);
void sdManager_tick(); //call periodically to ensure front panel is updated (needed because of fifo overruns when sending bulk data)



#endif /* SD_MANAGER_H_ */
#endif
