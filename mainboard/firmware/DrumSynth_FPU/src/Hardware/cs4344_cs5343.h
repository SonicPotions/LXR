/*
 * cs4344_cs5343.h
 *
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


#ifndef CS4344_CS5343_H_
#define CS4344_CS5343_H_

#include "stm32f4xx.h"
#include "config.h"

extern volatile int16_t dma_buffer[OUTPUT_DMA_SIZE*4]; 		// *4 because we need 16 stereo samples = *2
extern volatile int16_t dma_buffer2[OUTPUT_DMA_SIZE*4]; 	// *4 because we need 16 stereo samples = *2

/* Mask for the bit EN of the I2S CFGR register */
#define I2S_ENABLE_MASK                	 	0x0400
#define I2S_STANDARD                   		I2S_Standard_Phillips


/* Select the interrupt preemption priority and subpriority for the DMA interrupt */
#define EVAL_AUDIO_IRQ_PREPRIO           	0   /* Select the preemption priority level(0 is the highest) */
#define EVAL_AUDIO_IRQ_SUBRIO            	0   /* Select the sub-priority level (0 is the highest) */




//DAC1 Config defines
/* I2S3 peripheral configuration defines */
#define CODEC_DAC1_I2S                      SPI3
#define CODEC_DAC1_I2S_CLK                  RCC_APB1Periph_SPI3
#define CODEC_DAC1_I2S_ADDRESS              0x40003C0C	//SPI3_DR
#define CODEC_DAC1_I2S_GPIO_AF              GPIO_AF_SPI3
#define CODEC_DAC1_I2S_IRQ                  SPI3_IRQn
#define CODEC_DAC1_I2S_GPIO_CLOCK           (RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOA)

/* I2S DMA Stream definitions */
#define AUDIO_I2S_DMA_CLOCK            		RCC_AHB1Periph_DMA1
#define AUDIO_I2S_DMA_STREAM           		DMA1_Stream7
#define AUDIO_I2S_DMA_DREG             		CODEC_DAC1_I2S_ADDRESS
#define AUDIO_I2S_DMA_CHANNEL          		DMA_Channel_0
#define AUDIO_I2S_DMA_IRQ              		DMA1_Stream7_IRQn
#define AUDIO_I2S_DMA_FLAG_TC          		DMA_FLAG_TCIF7
#define AUDIO_I2S_DMA_FLAG_HT          		DMA_FLAG_HTIF7
#define AUDIO_I2S_DMA_FLAG_FE          		DMA_FLAG_FEIF7
#define AUDIO_I2S_DMA_FLAG_TE          		DMA_FLAG_TEIF7
#define AUDIO_I2S_DMA_FLAG_DME         		DMA_FLAG_DMEIF7


#define CODEC_DAC1_I2S_SCK_PIN 				GPIO_Pin_10 //port c
#define CODEC_DAC1_I2S_SD_PIN				GPIO_Pin_12 //port c
#define CODEC_DAC1_I2S_WS_PIN               GPIO_Pin_15 //Port a
#define CODEC_DAC1_I2S_MCK_PIN              GPIO_Pin_7  //port c

#define CODEC_DAC1_I2S_WS_PINSRC            GPIO_PinSource15
#define CODEC_DAC1_I2S_SCK_PINSRC           GPIO_PinSource10
#define CODEC_DAC1_I2S_SD_PINSRC            GPIO_PinSource12
#define CODEC_DAC1_I2S_MCK_PINSRC           GPIO_PinSource7

#define CODEC_DAC1_I2S_GPIO                 GPIOC
#define CODEC_DAC1_I2S_WS_GPIO              GPIOA


#if USE_DAC2
//DAC2 config defines
/* I2S3 peripheral configuration defines */
#define CODEC_DAC2_I2S                  SPI2
#define CODEC_DAC2_I2S_CLK              RCC_APB1Periph_SPI2
#define CODEC_DAC2_I2S_ADDRESS          0x4000380C	//SPI2_DR (reference manual: 0x4000 3800 - 0x4000 3BFF SPI2 / I2S2
#define CODEC_DAC2_I2S_GPIO_AF          GPIO_AF_SPI2
#define CODEC_DAC2_I2S_IRQ              SPI2_IRQn
#define CODEC_DAC2_I2S_GPIO_CLOCK       (RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC)


#define CODEC_DAC2_I2S_SCK_PIN 			GPIO_Pin_13 //port b
#define CODEC_DAC2_I2S_SD_PIN			GPIO_Pin_15 //port b
#define CODEC_DAC2_I2S_WS_PIN           GPIO_Pin_12 //Port b
#define CODEC_DAC2_I2S_MCK_PIN          GPIO_Pin_6  //port c

#define CODEC_DAC2_I2S_SCK_PINSRC       GPIO_PinSource13
#define CODEC_DAC2_I2S_SD_PINSRC        GPIO_PinSource15
#define CODEC_DAC2_I2S_WS_PINSRC        GPIO_PinSource12
#define CODEC_DAC2_I2S_MCK_PINSRC       GPIO_PinSource6

#define CODEC_DAC2_I2S_GPIO             GPIOB
#define CODEC_DAC2_I2S_MCK_GPIO         GPIOC

/* I2S2 DMA Stream definitions */
#define CODEC_I2S2_DMA_CLOCK            		RCC_AHB1Periph_DMA1
#define CODEC_I2S2_DMA_STREAM           		DMA1_Stream4
#define CODEC_I2S2_DMA_DREG             		CODEC_DAC2_I2S_ADDRESS
#define CODEC_I2S2_DMA_CHANNEL          		DMA_Channel_0
#define CODEC_I2S2_DMA_IRQ              		DMA1_Stream4_IRQn
#define CODEC_I2S2_DMA_FLAG_TC          		DMA_FLAG_TCIF4
#define CODEC_I2S2_DMA_FLAG_HT          		DMA_FLAG_HTIF4
#define CODEC_I2S2_DMA_FLAG_FE          		DMA_FLAG_FEIF4
#define CODEC_I2S2_DMA_FLAG_TE          		DMA_FLAG_TEIF4
#define CODEC_I2S2_DMA_FLAG_DME         		DMA_FLAG_DMEIF4



#else
//ADC config Defines
#define CODEC_ADC_I2S                  SPI2
#define CODEC_ADC_I2S_CLK              RCC_APB1Periph_SPI2
#define CODEC_ADC_I2S_ADDRESS          0x4000380C	//SPI2_DR (reference manual: 0x4000 3800 - 0x4000 3BFF SPI2 / I2S2
#define CODEC_ADC_I2S_GPIO_AF          GPIO_AF_SPI2
#define CODEC_ADC_I2S_IRQ              SPI2_IRQn
#define CODEC_ADC_I2S_GPIO_CLOCK       (RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC)

#define CODEC_ADC_I2S_SCK_PIN 		   GPIO_Pin_13 //port b
#define CODEC_ADC_I2S_SD_PIN		   GPIO_Pin_15 //port b
#define CODEC_ADC_I2S_WS_PIN           GPIO_Pin_12 //Port b
#define CODEC_ADC_I2S_MCK_PIN          GPIO_Pin_6  //port c

#define CODEC_ADC_I2S_SCK_PINSRC       GPIO_PinSource13
#define CODEC_ADC_I2S_SD_PINSRC        GPIO_PinSource15
#define CODEC_ADC_I2S_WS_PINSRC        GPIO_PinSource12
#define CODEC_ADC_I2S_MCK_PINSRC       GPIO_PinSource6

#define CODEC_ADC_I2S_GPIO             GPIOB
#define CODEC_ADC_I2S_MCK_GPIO         GPIOC

/* I2S2 DMA Stream definitions */
#define CODEC_I2S2_DMA_CLOCK            		RCC_AHB1Periph_DMA1
#define CODEC_I2S2_DMA_STREAM           		DMA1_Stream3
#define CODEC_I2S2_DMA_DREG             		CODEC_ADC_I2S_ADDRESS
#define CODEC_I2S2_DMA_CHANNEL          		DMA_Channel_0
#define CODEC_I2S2_DMA_IRQ              		DMA1_Stream3_IRQn
#define CODEC_I2S2_DMA_FLAG_TC          		DMA_FLAG_TCIF3
#define CODEC_I2S2_DMA_FLAG_HT          		DMA_FLAG_HTIF3
#define CODEC_I2S2_DMA_FLAG_FE          		DMA_FLAG_FEIF3
#define CODEC_I2S2_DMA_FLAG_TE          		DMA_FLAG_TEIF3
#define CODEC_I2S2_DMA_FLAG_DME         		DMA_FLAG_DMEIF3
#endif


void codec_initCsCodec(uint32_t Addr1, uint32_t Size1,uint32_t Addr2, uint32_t Size2);

#endif /* CS4344_CS5343_H_ */
