/*
 * AudioCodecDma.h
 *
 *  Created on: 15.04.2012
 *      Author: Julian
 */

#ifndef AUDIOCODECDMA_H_
#define AUDIOCODECDMA_H_

#include "stm32f4xx.h"

#if 0

#define USE_LIMITER_EQ 1
#if 0
#define OUTPUT_DMA_SIZE 16

extern volatile int16_t dma_buffer[OUTPUT_DMA_SIZE*4]; 	//*4 because we need 16 stereo samples = *2
#endif

#define VOLUME_CONVERT(x)    ((Volume > 100)? 100:((uint8_t)((Volume * 255) / 100)))

//#################### OPTION FLAGS ######################
#define VERIFY_WRITTENDATA	///* Verify that the data send to the codec via I2C has been correctly written. only ctrl commands, no audio stream */
extern __IO uint8_t codec_volume ;



//#################### DEFINES ###########################

/* Select the interrupt preemption priority and subpriority for the DMA interrupt */
#define EVAL_AUDIO_IRQ_PREPRIO           0   /* Select the preemption priority level(0 is the highest) */
#define EVAL_AUDIO_IRQ_SUBRIO            0   /* Select the sub-priority level (0 is the highest) */

/* Mask for the bit EN of the I2S CFGR register */
#define I2S_ENABLE_MASK                 0x0400

// codec audio standard = Phillips
#define  CODEC_STANDARD                0x04
#define I2S_STANDARD                   I2S_Standard_Phillips

/* The 7 bits Codec address (sent through I2C interface) */
#define CODEC_ADDRESS                   0x94  /* b00100111 */

/* Maximum Timeout values for flags and events waiting loops. These timeouts are
   not based on accurate values, they just guarantee that the application will
   not remain stuck if the I2C communication is corrupted.
   You may modify these timeout values depending on CPU frequency and application
   conditions (interrupts routines ...). */
#define CODEC_FLAG_TIMEOUT             ((uint32_t)0x1000)
#define CODEC_LONG_TIMEOUT             ((uint32_t)(300 * CODEC_FLAG_TIMEOUT))

/* I2C clock speed configuration (in Hz)
  WARNING:
   Make sure that this define is not already declared in other files (ie.
  stm322xg_eval.h file). It can be used in parallel by other modules. */
 #define I2C_SPEED                        100000


/* Delay for the Codec to be correctly reset */
#define CODEC_RESET_DELAY               0x4FFF

/* Audio Reset Pin definition */
#define AUDIO_RESET_GPIO_CLK           RCC_AHB1Periph_GPIOD
#define AUDIO_RESET_PIN                GPIO_Pin_4
#define AUDIO_RESET_GPIO               GPIOD

/* I2C peripheral configuration defines (control interface of the audio codec) */
#define CODEC_I2C                      I2C1
#define CODEC_I2C_CLK                  RCC_APB1Periph_I2C1
#define CODEC_I2C_GPIO_CLOCK           RCC_AHB1Periph_GPIOB
#define CODEC_I2C_GPIO_AF              GPIO_AF_I2C1
#define CODEC_I2C_GPIO                 GPIOB
#define CODEC_I2C_SCL_PIN              GPIO_Pin_6
#define CODEC_I2C_SDA_PIN              GPIO_Pin_9
#define CODEC_I2S_SCL_PINSRC           GPIO_PinSource6
#define CODEC_I2S_SDA_PINSRC           GPIO_PinSource9

/* I2S DMA Stream definitions */
#define AUDIO_I2S_DMA_CLOCK            RCC_AHB1Periph_DMA1
#define AUDIO_I2S_DMA_STREAM           DMA1_Stream7
#define AUDIO_I2S_DMA_DREG             CODEC_I2S_ADDRESS
#define AUDIO_I2S_DMA_CHANNEL          DMA_Channel_0
#define AUDIO_I2S_DMA_IRQ              DMA1_Stream7_IRQn
#define AUDIO_I2S_DMA_FLAG_TC          DMA_FLAG_TCIF7
#define AUDIO_I2S_DMA_FLAG_HT          DMA_FLAG_HTIF7
#define AUDIO_I2S_DMA_FLAG_FE          DMA_FLAG_FEIF7
#define AUDIO_I2S_DMA_FLAG_TE          DMA_FLAG_TEIF7
#define AUDIO_I2S_DMA_FLAG_DME         DMA_FLAG_DMEIF7

/* I2S peripheral configuration defines */
#define CODEC_I2S                      SPI3
#define CODEC_I2S_CLK                  RCC_APB1Periph_SPI3
#define CODEC_I2S_ADDRESS              0x40003C0C
#define CODEC_I2S_GPIO_AF              GPIO_AF_SPI3
#define CODEC_I2S_IRQ                  SPI3_IRQn
#define CODEC_I2S_GPIO_CLOCK           (RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOA)
#define CODEC_I2S_WS_PIN               GPIO_Pin_4
#define CODEC_I2S_SCK_PIN              GPIO_Pin_10
#define CODEC_I2S_SD_PIN               GPIO_Pin_12
#define CODEC_I2S_MCK_PIN              GPIO_Pin_7
#define CODEC_I2S_WS_PINSRC            GPIO_PinSource4
#define CODEC_I2S_SCK_PINSRC           GPIO_PinSource10
#define CODEC_I2S_SD_PINSRC            GPIO_PinSource12
#define CODEC_I2S_MCK_PINSRC           GPIO_PinSource7
#define CODEC_I2S_GPIO                 GPIOC
#define CODEC_I2S_WS_GPIO              GPIOA
#define CODEC_I2S_MCK_GPIO             GPIOC
#define Audio_I2S_IRQHandler           SPI3_IRQHandler


uint8_t audioCodec_init();
void audioCodec_start(uint32_t Addr, uint32_t Size);


//----------------- build in EQ ----------------------------
#if USE_LIMITER_EQ

void audioCodec__initEq();

void audioCodec_setEqEnabled(uint8_t enable);

/** 0 = 50Hz
 *  1 = 100Hz
 *  2 = 200Hz
 *  3 = 250Hz
 */
void audioCodec_setEqBassFreq(uint8_t f);

/** 0 = 5kHz
 *  1 = 7kHz
 *  2 = 10kHz
 *  3 = 15kHz
 */
void audioCodec_setEqTrebFreq(uint8_t f);


void audioCodec_setEqTrebGain(uint8_t g);

void audioCodec_setEqBassGain(uint8_t g);

//--------------------- build in limiter --------------------
void audioCodec_setLimiterInit();
void audioCodec_setLimiterEnabled(uint8_t enable);
/** set the release rate 0 is fastest, 63 is slowest*/
void audioCodec_setLimiterAttack(uint8_t attack);

/** set the release rate 0 is fastest, 63 is slowest*/
void audioCodec_setLimiterRelease(uint8_t release);
void audioCodec_setLimiterMaxTresh(uint8_t max);
void audioCodec_setLimiterMinTresh(uint8_t min);


#endif



#endif


#endif /* AUDIOCODECDMA_H_ */
