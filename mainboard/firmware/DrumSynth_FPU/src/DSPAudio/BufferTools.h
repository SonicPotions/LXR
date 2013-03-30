/*
 * BufferTools.h
 *
 *  Created on: 04.01.2013
 *      Author: Julian
 */

#ifndef BUFFERTOOLS_H_
#define BUFFERTOOLS_H_

#include "stm32f4xx.h"
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
inline void bufferTool_mulInt(int16_t* buf, const int16_t gain, const uint8_t size);
//---------------------------------------------------
inline void bufferTool_multiplyWithFloatBuffer(int16_t* buf, float* fltBuf, const uint8_t size);
//---------------------------------------------------
inline void bufferTool_moveBuffer(int16_t* dst, int16_t* src, const uint8_t size);
//---------------------------------------------------
#endif /* BUFFERTOOLS_H_ */
