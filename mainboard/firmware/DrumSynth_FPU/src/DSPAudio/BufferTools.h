/*
 * BufferTools.h
 *
 *  Created on: 04.01.2013
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


#ifndef BUFFERTOOLS_H_
#define BUFFERTOOLS_H_

#include "stm32f4xx.h"
#include "dither.h"
//---------------------------------------------------
inline void bufferTool_addBuffers(int16_t* buf1, int16_t* buf2, const uint8_t size);
//---------------------------------------------------
inline void bufferTool_addBuffersSaturating(int16_t* buf1, int16_t* buf2, const uint8_t size);
//---------------------------------------------------
inline void bufferTool_addBuffersSaturatingWithGain(int16_t* buf1, int16_t* buf2, const float gain, const uint8_t size);
//---------------------------------------------------
inline void bufferTool_subBuffersSaturating(int16_t* buf1, int16_t* buf2, const uint8_t size);
//---------------------------------------------------
inline void bufferTool_copyWithGain(int16_t* buf1, int16_t* buf2, float gain, const uint8_t size);
//---------------------------------------------------
inline void bufferTool_clearBuffer(int16_t* buf, const uint8_t size);
//---------------------------------------------------
inline void bufferTool_addGain(int16_t* buf, const float gain, const uint8_t size);
//---------------------------------------------------
inline void bufferTool_addGainDithered(Dither* dither, int16_t* buf, const float gain, const uint8_t size);
//---------------------------------------------------
inline void bufferTool_addGainInterpolated(int16_t* buf, const float gain, const float lastGain, const uint8_t size);
//---------------------------------------------------
inline void bufferTool_mulInt(int16_t* buf, const int16_t gain, const uint8_t size);
//---------------------------------------------------
inline void bufferTool_multiplyWithFloatBuffer(int16_t* buf, float* fltBuf, const uint8_t size);
//---------------------------------------------------
inline void bufferTool_multiplyWithFloatBufferDithered(Dither* dither, int16_t* buf, float* fltBuf, const uint8_t size);
//---------------------------------------------------
inline void bufferTool_moveBuffer(int16_t* dst, int16_t* src, const uint8_t size);
//---------------------------------------------------
#endif /* BUFFERTOOLS_H_ */
