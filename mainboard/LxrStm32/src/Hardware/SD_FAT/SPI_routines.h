/*
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


#ifndef _SPI_ROUTINES_H_
#define _SPI_ROUTINES_H_
#include "config.h"
#if USE_SD_CARD
#include "stm32f4xx.h"


#ifdef __GNUC__
#define VARIABLE_IS_NOT_USED __attribute__ ((unused))
#else
#define VARIABLE_IS_NOT_USED
#endif


static SPI_InitTypeDef VARIABLE_IS_NOT_USED SPI_InitStructure;


#define SPI_SD			SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;SPI_Init(SPI1, &SPI_InitStructure);
#define SPI_HIGH_SPEED 	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;SPI_Init(SPI1, &SPI_InitStructure);


#define SPI_MISO_MOSI_PORT	GPIOA
#define SPI_CS_SCK_PORT		GPIOB
#define SPI_MISO_PIN		GPIO_Pin_6	//a
#define SPI_MOSI_PIN		GPIO_Pin_7	//a
#define SPI_SCK_PIN			GPIO_Pin_3  //b
#define SPI_CS_PIN			GPIO_Pin_0  //b

#define SPI_MISO_PIN_SRC	GPIO_PinSource6	//a
#define SPI_MOSI_PIN_SRC	GPIO_PinSource7	//a
#define SPI_SCK_PIN_SRC		GPIO_PinSource3  //b
#define SPI_CS_PIN_SRC		GPIO_PinSource0  //b


void spi_init(void);
void spi_deInit();
unsigned char SPI_transmit(unsigned char);
unsigned char SPI_receive(void);

#endif
#endif
