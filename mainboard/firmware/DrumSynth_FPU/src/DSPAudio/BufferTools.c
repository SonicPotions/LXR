/*
 * BufferTools.c
 *
 *  Created on: 04.01.2013
 *      Author: Julian
 */

#include "BufferTools.h"

//---------------------------------------------------
inline void bufferTool_addBuffers(int16_t* buf1, int16_t* buf2, const uint8_t size)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		buf1[i] += buf2[i];
	}
}
//---------------------------------------------------
inline void bufferTool_addBuffersSaturating(int16_t* buf1, int16_t* buf2, const uint8_t size)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		buf1[i] = (__QADD16(buf1[i],buf2[i]));
	}
}
//---------------------------------------------------
inline void bufferTool_addBuffersSaturatingWithGain(int16_t* buf1, int16_t* buf2, const float gain, const uint8_t size)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		buf1[i] = (__QADD16(buf1[i],buf2[i])) * gain;
	}
}
//---------------------------------------------------
inline void bufferTool_subBuffersSaturating(int16_t* buf1, int16_t* buf2, const uint8_t size)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		buf1[i] = (__QSUB16(buf1[i],buf2[i])) ;
	}
}
//---------------------------------------------------
inline void bufferTool_copyWithGain(int16_t* buf1, int16_t* buf2, float gain, const uint8_t size)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		buf1[i] = buf2[i]*gain;
	}
}
//---------------------------------------------------
inline void bufferTool_clearBuffer(int16_t* buf, const uint8_t size)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		buf[i] = 0;
	}
}
//---------------------------------------------------
inline void bufferTool_addGain(int16_t* buf, const float gain, const uint8_t size)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		buf[i] *= gain;
	}
}
//---------------------------------------------------
inline void bufferTool_addGainDithered(Dither* dither, int16_t* buf, const float gain, const uint8_t size)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		buf[i] = dither_process(dither,(buf[i]/32768.0f) * gain);
	}
}
//---------------------------------------------------
inline void bufferTool_addGainInterpolated(int16_t* buf, const float gain, const float lastGain, const uint8_t size)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		const float frac = i/(size-1.f);
		const float currentGain = lastGain + frac*(gain - lastGain);
		buf[i] = buf[i] * currentGain;
	}
}
//---------------------------------------------------
inline void bufferTool_mulInt(int16_t* buf, const int16_t gain, const uint8_t size)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		buf[i] *= gain;
	}
}
//---------------------------------------------------
inline void bufferTool_multiplyWithFloatBuffer(int16_t* buf, float* fltBuf, const uint8_t size)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		buf[i] *= fltBuf[i];
	}
}
//---------------------------------------------------
inline void bufferTool_multiplyWithFloatBufferDithered(Dither* dither, int16_t* buf, float* fltBuf, const uint8_t size)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		buf[i] = dither_process(dither,(buf[i]/32768.0f) * fltBuf[i]);

	}
}
//---------------------------------------------------
inline void bufferTool_moveBuffer(int16_t* dst, int16_t* src, const uint8_t size)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
		dst[i] = src[i];
	}
}
//---------------------------------------------------
