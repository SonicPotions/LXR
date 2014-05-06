/*
 * cs4344_cs5343.c
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


#include "cs4344_cs5343.h"
#include "globals.h"

DMA_InitTypeDef DMA_InitStructure;
DMA_InitTypeDef DMA_InitStructure2;


uint8_t dmaPtr=0;
uint8_t dmaPtr2=0;
//################################ DAC 1 ############################################################
void DMA1_Stream7_IRQHandler(void)
{


  /* Transfer complete interrupt */
  if (DMA_GetFlagStatus(DMA1_Stream7, DMA_FLAG_TCIF7) != RESET)
  {

	  dmaPtr++;

	  /* Wait the DMA Stream to be effectively disabled */
	   while (DMA_GetCmdStatus(DMA1_Stream7) != DISABLE)
	   {}

	   /* Clear the Interrupt flag */
	   DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_TCIF7);

	   /* Re-Configure the buffer address and size */
	   DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) &dma_buffer[(OUTPUT_DMA_SIZE*2)*(dmaPtr&0x1)];
	   DMA_InitStructure.DMA_BufferSize = OUTPUT_DMA_SIZE*2;

	   /* Configure the DMA Stream with the new parameters */
	   DMA_Init(DMA1_Stream7, &DMA_InitStructure);

	   /* Enable the I2S DMA Stream*/
	   DMA_Cmd(DMA1_Stream7, ENABLE);

	   //now start next sample block calculation
		 bCurrentSampleValid = 1-(dmaPtr&0x1);
	}

  /* Half Transfer complete interrupt */
  if (DMA_GetFlagStatus(DMA1_Stream7, DMA_FLAG_HTIF7) != RESET)
  {
	  //do something

    /* Clear the Interrupt flag */
    DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_HTIF7);
  }


#if 1
  /* FIFO Error interrupt */
  if ((DMA_GetFlagStatus(DMA1_Stream7, DMA_FLAG_TEIF7) != RESET) || \
     (DMA_GetFlagStatus(DMA1_Stream7, DMA_FLAG_FEIF7) != RESET) || \
     (DMA_GetFlagStatus(DMA1_Stream7, DMA_FLAG_DMEIF7) != RESET))

  {
    /* Clear the Interrupt flag */
    DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_TEIF7 | DMA_FLAG_FEIF7 | \
    		DMA_FLAG_DMEIF7);
  }
#endif //error flags
}
//----------------------------------------------------------------------------------
void codec_initDma_DAC()
{
		NVIC_InitTypeDef NVIC_InitStructure;

	 /* Enable the DMA clock */
	    RCC_AHB1PeriphClockCmd(AUDIO_I2S_DMA_CLOCK, ENABLE);

	    /* Configure the DMA Stream */
	    DMA_Cmd(DMA1_Stream7, DISABLE);
	    DMA_DeInit(DMA1_Stream7);
	    /* Set the parameters to be configured */
	    DMA_InitStructure.DMA_Channel 				= DMA_Channel_0;
	    DMA_InitStructure.DMA_PeripheralBaseAddr 	= CODEC_DAC1_I2S_ADDRESS;
	    DMA_InitStructure.DMA_Memory0BaseAddr 		= (uint32_t)0;      /* This field will be configured in play function */
	    DMA_InitStructure.DMA_DIR 					= DMA_DIR_MemoryToPeripheral;
	    DMA_InitStructure.DMA_BufferSize 			= (uint32_t)0xFFFE;      /* This field will be configured in play function */
	    DMA_InitStructure.DMA_PeripheralInc 		= DMA_PeripheralInc_Disable;
	    DMA_InitStructure.DMA_MemoryInc 			= DMA_MemoryInc_Enable;
	    DMA_InitStructure.DMA_PeripheralDataSize 	= DMA_PeripheralDataSize_HalfWord;
	    DMA_InitStructure.DMA_MemoryDataSize 		= DMA_MemoryDataSize_HalfWord;
	  //DMA_InitStructure.DMA_Mode 					= DMA_Mode_Circular; //not needed when using double buffered mode
	    DMA_InitStructure.DMA_Mode 					= DMA_Mode_Normal;

	    DMA_InitStructure.DMA_Priority 				= DMA_Priority_High;
	    DMA_InitStructure.DMA_FIFOMode 				= DMA_FIFOMode_Disable;
	    DMA_InitStructure.DMA_FIFOThreshold 		= DMA_FIFOThreshold_1QuarterFull;
	    DMA_InitStructure.DMA_MemoryBurst 			= DMA_MemoryBurst_Single;
	    DMA_InitStructure.DMA_PeripheralBurst 		= DMA_PeripheralBurst_Single;
	    DMA_Init(DMA1_Stream7, &DMA_InitStructure);

	    // enable the dma interrupts transfer complete, half transfer complete and error
	    DMA_ITConfig(DMA1_Stream7, DMA_IT_TC, ENABLE);
	    DMA_ITConfig(DMA1_Stream7, DMA_IT_HT, ENABLE);
	    DMA_ITConfig(DMA1_Stream7, DMA_IT_TE | DMA_IT_FE | DMA_IT_DME, ENABLE);

	    /* I2S DMA IRQ Channel configuration */
	    NVIC_InitStructure.NVIC_IRQChannel 				= DMA1_Stream7_IRQn;
	    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = EVAL_AUDIO_IRQ_PREPRIO;
	    NVIC_InitStructure.NVIC_IRQChannelSubPriority	= EVAL_AUDIO_IRQ_SUBRIO;
	    NVIC_InitStructure.NVIC_IRQChannelCmd 			= ENABLE;
	    NVIC_Init(&NVIC_InitStructure);

	    /* Enable the I2S DMA request */
	     SPI_I2S_DMACmd(CODEC_DAC1_I2S, SPI_I2S_DMAReq_Tx, ENABLE);

}
//----------------------------------------------------------------------------------
static void codec_AudioInterface_Init_DAC()
{
  I2S_InitTypeDef I2S_InitStructure;

  /* Enable the CODEC_I2S peripheral clock */
  RCC_APB1PeriphClockCmd(CODEC_DAC1_I2S_CLK, ENABLE);

  /* CODEC_I2S peripheral configuration */
  SPI_I2S_DeInit(CODEC_DAC1_I2S);
  I2S_InitStructure.I2S_AudioFreq 	= I2S_AudioFreq_44k;
  I2S_InitStructure.I2S_Standard 	= I2S_STANDARD;
  I2S_InitStructure.I2S_DataFormat 	= I2S_DataFormat_16b;
  I2S_InitStructure.I2S_CPOL 		= I2S_CPOL_Low;
  I2S_InitStructure.I2S_Mode 		= I2S_Mode_MasterTx;
  I2S_InitStructure.I2S_MCLKOutput 	= I2S_MCLKOutput_Enable;

  /* Initialize the I2S peripheral with the structure above */
  I2S_Init(CODEC_DAC1_I2S, &I2S_InitStructure);


  /* The I2S peripheral will be enabled only in the EVAL_AUDIO_Play() function
       or by user functions if DMA mode not enabled */
}
//----------------------------------------------------------------------------------
void codec_start_dac1(uint32_t Addr, uint32_t Size)
{
	/* Configure the buffer address and size */
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)Addr;
	DMA_InitStructure.DMA_BufferSize = (uint32_t)Size;

	/* Configure the DMA Stream with the new parameters */
	DMA_Init(DMA1_Stream7, &DMA_InitStructure);

	/* Enable the I2S DMA Stream*/
	DMA_Cmd(DMA1_Stream7, ENABLE);

	/* If the I2S peripheral is still not enabled, enable it */
	if ((CODEC_DAC1_I2S->I2SCFGR & I2S_ENABLE_MASK) == 0)
	{
		I2S_Cmd(CODEC_DAC1_I2S, ENABLE);
	}
}
//----------------------------------------------------------------------------------
void codec_InitGPIO_DAC1(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

		//DAC1

		/* Enable I2S and I2C GPIO clocks */
		RCC_AHB1PeriphClockCmd( CODEC_DAC1_I2S_GPIO_CLOCK, ENABLE);

		/* CODEC_I2S pins configuration: WS, SCK and SD pins -----------------------------*/
		GPIO_InitStructure.GPIO_Pin 	= CODEC_DAC1_I2S_SCK_PIN | CODEC_DAC1_I2S_SD_PIN;
		GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_OType 	= GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd 	= GPIO_PuPd_NOPULL;
		GPIO_Init(CODEC_DAC1_I2S_GPIO, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = CODEC_DAC1_I2S_WS_PIN ;
		GPIO_Init(CODEC_DAC1_I2S_WS_GPIO, &GPIO_InitStructure);

		/* Connect pins to I2S peripheral  */
		GPIO_PinAFConfig(CODEC_DAC1_I2S_WS_GPIO, 	CODEC_DAC1_I2S_WS_PINSRC, 	CODEC_DAC1_I2S_GPIO_AF);
		GPIO_PinAFConfig(CODEC_DAC1_I2S_GPIO, 		CODEC_DAC1_I2S_SCK_PINSRC, 	CODEC_DAC1_I2S_GPIO_AF);
		GPIO_PinAFConfig(CODEC_DAC1_I2S_GPIO, 		CODEC_DAC1_I2S_SD_PINSRC, 	CODEC_DAC1_I2S_GPIO_AF);


		//Master Clock enable
		/* CODEC_I2S pins configuration: MCK pin */
		GPIO_InitStructure.GPIO_Pin 	= CODEC_DAC1_I2S_MCK_PIN;
		GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_OType 	= GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd 	= GPIO_PuPd_NOPULL;
		GPIO_Init(CODEC_DAC1_I2S_GPIO, &GPIO_InitStructure);
		/* Connect pins to I2S peripheral  */
		GPIO_PinAFConfig(CODEC_DAC1_I2S_GPIO, 		CODEC_DAC1_I2S_MCK_PINSRC, CODEC_DAC1_I2S_GPIO_AF);

}

//----------------------------------------------------------------------------------------------------
//################################ DAC 2 ##############################################################
#if USE_DAC2
//-----------------------------------------------------------------------------------------------------
void DMA1_Stream4_IRQHandler(void)
{

	  /* Transfer complete interrupt */
	  if (DMA_GetFlagStatus(CODEC_I2S2_DMA_STREAM, CODEC_I2S2_DMA_FLAG_TC) != RESET)
	  {

		  dmaPtr2++;

		  /* Wait the DMA Stream to be effectively disabled */
		   while (DMA_GetCmdStatus(CODEC_I2S2_DMA_STREAM) != DISABLE)
		   {}

		   /* Clear the Interrupt flag */
		   DMA_ClearFlag(CODEC_I2S2_DMA_STREAM, CODEC_I2S2_DMA_FLAG_TC);

		   /* Re-Configure the buffer address and size */
		   DMA_InitStructure2.DMA_Memory0BaseAddr = (uint32_t) &dma_buffer2[(OUTPUT_DMA_SIZE*2)*(dmaPtr2&0x1)];
		   DMA_InitStructure2.DMA_BufferSize = OUTPUT_DMA_SIZE*2;

		   /* Configure the DMA Stream with the new parameters */
		   DMA_Init(CODEC_I2S2_DMA_STREAM, &DMA_InitStructure2);

		   /* Enable the I2S DMA Stream*/
		   DMA_Cmd(CODEC_I2S2_DMA_STREAM, ENABLE);

		}



	  /* Half Transfer complete interrupt */
	  if (DMA_GetFlagStatus(CODEC_I2S2_DMA_STREAM, CODEC_I2S2_DMA_FLAG_HT) != RESET)
	  {
		  //do something

	    /* Clear the Interrupt flag */
	    DMA_ClearFlag(CODEC_I2S2_DMA_STREAM, CODEC_I2S2_DMA_FLAG_HT);
	  }


	#if 1

	  /* FIFO Error interrupt */
	  if ((DMA_GetFlagStatus(CODEC_I2S2_DMA_STREAM, CODEC_I2S2_DMA_FLAG_TE) != RESET) || \
	     (DMA_GetFlagStatus(CODEC_I2S2_DMA_STREAM, CODEC_I2S2_DMA_FLAG_FE) != RESET) || \
	     (DMA_GetFlagStatus(CODEC_I2S2_DMA_STREAM, CODEC_I2S2_DMA_FLAG_DME) != RESET))

	  {
	    /* Clear the Interrupt flag */
	    DMA_ClearFlag(CODEC_I2S2_DMA_STREAM, CODEC_I2S2_DMA_FLAG_TE | CODEC_I2S2_DMA_FLAG_FE | \
	    		CODEC_I2S2_DMA_FLAG_DME);
	  }
	#endif //error flags

}
//-----------------------------------------------------------------------------------------------------
void codec_initDma_DAC2()
{
		NVIC_InitTypeDef NVIC_InitStructure;

	 /* Enable the DMA clock */
	    RCC_AHB1PeriphClockCmd(CODEC_I2S2_DMA_CLOCK, ENABLE);

	    /* Configure the DMA Stream */
	    DMA_Cmd(CODEC_I2S2_DMA_STREAM, DISABLE);
	    DMA_DeInit(CODEC_I2S2_DMA_STREAM);
	    /* Set the parameters to be configured */
	    DMA_InitStructure2.DMA_Channel 				= DMA_Channel_0;
	    DMA_InitStructure2.DMA_PeripheralBaseAddr 	= CODEC_DAC2_I2S_ADDRESS;
	    DMA_InitStructure2.DMA_Memory0BaseAddr 		= (uint32_t)0;      /* This field will be configured in play function */
	    DMA_InitStructure2.DMA_DIR 					= DMA_DIR_MemoryToPeripheral;
	    DMA_InitStructure2.DMA_BufferSize 			= (uint32_t)0xFFFE;      /* This field will be configured in play function */
	    DMA_InitStructure2.DMA_PeripheralInc 		= DMA_PeripheralInc_Disable;
	    DMA_InitStructure2.DMA_MemoryInc 			= DMA_MemoryInc_Enable;
	    DMA_InitStructure2.DMA_PeripheralDataSize 	= DMA_PeripheralDataSize_HalfWord;
	    DMA_InitStructure2.DMA_MemoryDataSize 		= DMA_MemoryDataSize_HalfWord;
	   // DMA_InitStructure.DMA_Mode 				= DMA_Mode_Circular; //not needed when using double buffered mode
	    DMA_InitStructure2.DMA_Mode 				= DMA_Mode_Normal;


	    DMA_InitStructure2.DMA_Priority 			= DMA_Priority_High;
	    DMA_InitStructure2.DMA_FIFOMode 			= DMA_FIFOMode_Disable;
	    DMA_InitStructure2.DMA_FIFOThreshold 		= DMA_FIFOThreshold_1QuarterFull;
	    DMA_InitStructure2.DMA_MemoryBurst 			= DMA_MemoryBurst_Single;
	    DMA_InitStructure2.DMA_PeripheralBurst 		= DMA_PeripheralBurst_Single;
	    DMA_Init(CODEC_I2S2_DMA_STREAM, &DMA_InitStructure2);

	    // enable the dma interrupts transfer complete, half transfer complete and error
	    DMA_ITConfig(CODEC_I2S2_DMA_STREAM, DMA_IT_TC, ENABLE);
	    DMA_ITConfig(CODEC_I2S2_DMA_STREAM, DMA_IT_HT, ENABLE);
	    DMA_ITConfig(CODEC_I2S2_DMA_STREAM, DMA_IT_TE | DMA_IT_FE | DMA_IT_DME, ENABLE);

	    /* I2S DMA IRQ Channel configuration */
	    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream4_IRQn;
	    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = EVAL_AUDIO_IRQ_PREPRIO;
	    NVIC_InitStructure.NVIC_IRQChannelSubPriority = EVAL_AUDIO_IRQ_SUBRIO;
	    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	    NVIC_Init(&NVIC_InitStructure);

	    /* Enable the I2S DMA request */
	     SPI_I2S_DMACmd(CODEC_DAC2_I2S, SPI_I2S_DMAReq_Tx, ENABLE);

}
//-----------------------------------------------------------------------------------------------------
void codec_InitGPIO_DAC2(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

		//DAC1

		/* Enable I2S and I2C GPIO clocks */
		RCC_AHB1PeriphClockCmd( CODEC_DAC2_I2S_GPIO_CLOCK, ENABLE);

		/* CODEC_I2S pins configuration: WS, SCK and SD pins -----------------------------*/
		GPIO_InitStructure.GPIO_Pin 	= CODEC_DAC2_I2S_WS_PIN | CODEC_DAC2_I2S_SCK_PIN | CODEC_DAC2_I2S_SD_PIN;
		GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_OType 	= GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd 	= GPIO_PuPd_NOPULL;
		GPIO_Init(CODEC_DAC2_I2S_GPIO, &GPIO_InitStructure);

		/* Connect pins to I2S peripheral  */
		GPIO_PinAFConfig(CODEC_DAC2_I2S_GPIO, 		CODEC_DAC2_I2S_WS_PINSRC, 	CODEC_DAC2_I2S_GPIO_AF);
		GPIO_PinAFConfig(CODEC_DAC2_I2S_GPIO, 		CODEC_DAC2_I2S_SCK_PINSRC, 	CODEC_DAC2_I2S_GPIO_AF);
		GPIO_PinAFConfig(CODEC_DAC2_I2S_GPIO, 		CODEC_DAC2_I2S_SD_PINSRC, 	CODEC_DAC2_I2S_GPIO_AF);


		//Master Clock enable
		/* CODEC_I2S pins configuration: MCK pin */
		GPIO_InitStructure.GPIO_Pin 	= CODEC_DAC2_I2S_MCK_PIN;
		GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_OType 	= GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd 	= GPIO_PuPd_NOPULL;
		GPIO_Init(CODEC_DAC2_I2S_MCK_GPIO, &GPIO_InitStructure);
		/* Connect pins to I2S peripheral  */
		GPIO_PinAFConfig(CODEC_DAC2_I2S_MCK_GPIO, 		CODEC_DAC2_I2S_MCK_PINSRC, CODEC_DAC2_I2S_GPIO_AF);

}
//-----------------------------------------------------------------------------------------------------
void codec_start_dac2(uint32_t Addr, uint32_t Size)
{
	/* Configure the buffer address and size */
	DMA_InitStructure2.DMA_Memory0BaseAddr = (uint32_t)Addr;
	DMA_InitStructure2.DMA_BufferSize = (uint32_t)Size;

	/* Configure the DMA Stream with the new parameters */
	DMA_Init(CODEC_I2S2_DMA_STREAM, &DMA_InitStructure2);

	/* Enable the I2S DMA Stream*/
	DMA_Cmd(CODEC_I2S2_DMA_STREAM, ENABLE);

	/* If the I2S peripheral is still not enabled, enable it */
	if ((CODEC_DAC2_I2S->I2SCFGR & I2S_ENABLE_MASK) == 0)
	{
		I2S_Cmd(CODEC_DAC2_I2S, ENABLE);
	}
}
//-----------------------------------------------------------------------------------------------------
static void codec_AudioInterface_Init_DAC2()
{
  I2S_InitTypeDef I2S_InitStructure;


  /* Enable the CODEC_I2S peripheral clock */
  RCC_APB1PeriphClockCmd(CODEC_DAC2_I2S_CLK, ENABLE);

  /* CODEC_I2S peripheral configuration */
  SPI_I2S_DeInit(CODEC_DAC2_I2S);
  I2S_InitStructure.I2S_AudioFreq 	= I2S_AudioFreq_44k;
  I2S_InitStructure.I2S_Standard 	= I2S_STANDARD;
  I2S_InitStructure.I2S_DataFormat 	= I2S_DataFormat_16b;
  I2S_InitStructure.I2S_CPOL 		= I2S_CPOL_Low;
  I2S_InitStructure.I2S_Mode 		= I2S_Mode_MasterTx;
  I2S_InitStructure.I2S_MCLKOutput 	= I2S_MCLKOutput_Enable;


  /* Initialize the I2S peripheral with the structure above */
  I2S_Init(CODEC_DAC2_I2S, &I2S_InitStructure);


  /* The I2S peripheral will be enabled only in the EVAL_AUDIO_Play() function
       or by user functions if DMA mode not enabled */
}
//-----------------------------------------------------------------------------------------------------
#else
//################################ ADC ################################################################
//-----------------------------------------------------------------------------------------------------
void DMA1_Stream3_IRQHandler(void)
{

	  /* Transfer complete interrupt */
	  if (DMA_GetFlagStatus(CODEC_I2S2_DMA_STREAM, CODEC_I2S2_DMA_FLAG_TC) != RESET)
	  {

		  dmaPtr2++;

		  /* Wait the DMA Stream to be effectively disabled */
		   while (DMA_GetCmdStatus(CODEC_I2S2_DMA_STREAM) != DISABLE)
		   {}

		   /* Clear the Interrupt flag */
		   DMA_ClearFlag(CODEC_I2S2_DMA_STREAM, CODEC_I2S2_DMA_FLAG_TC);

		   /* Re-Configure the buffer address and size */
		   DMA_InitStructure2.DMA_Memory0BaseAddr = (uint32_t) &dma_buffer2[(OUTPUT_DMA_SIZE*2)*(dmaPtr2&0x1)];
		   DMA_InitStructure2.DMA_BufferSize = OUTPUT_DMA_SIZE*2;

		   /* Configure the DMA Stream with the new parameters */
		   DMA_Init(CODEC_I2S2_DMA_STREAM, &DMA_InitStructure2);

		   /* Enable the I2S DMA Stream*/
		   DMA_Cmd(CODEC_I2S2_DMA_STREAM, ENABLE);

		   //TODO eigene DMA buffer einbauen
		   //now start next sample block calculation
			 //bCurrentSampleValid = 1-(dmaPtr&0x1);
		}



	  /* Half Transfer complete interrupt */
	  if (DMA_GetFlagStatus(CODEC_I2S2_DMA_STREAM, CODEC_I2S2_DMA_FLAG_HT) != RESET)
	  {
		  //do something

	    /* Clear the Interrupt flag */
	    DMA_ClearFlag(CODEC_I2S2_DMA_STREAM, CODEC_I2S2_DMA_FLAG_HT);
	  }


	#if 1

	  /* FIFO Error interrupt */
	  if ((DMA_GetFlagStatus(CODEC_I2S2_DMA_STREAM, CODEC_I2S2_DMA_FLAG_TE) != RESET) || \
	     (DMA_GetFlagStatus(CODEC_I2S2_DMA_STREAM, CODEC_I2S2_DMA_FLAG_FE) != RESET) || \
	     (DMA_GetFlagStatus(CODEC_I2S2_DMA_STREAM, CODEC_I2S2_DMA_FLAG_DME) != RESET))

	  {


	    /* Clear the Interrupt flag */
	    DMA_ClearFlag(CODEC_I2S2_DMA_STREAM, CODEC_I2S2_DMA_FLAG_TE | CODEC_I2S2_DMA_FLAG_FE | \
	    		CODEC_I2S2_DMA_FLAG_DME);
	  }
	#endif //error flags

}
//-----------------------------------------------------------------------------------------------------
void codec_initDma_ADC()
{
		NVIC_InitTypeDef NVIC_InitStructure;

	 /* Enable the DMA clock */
	    RCC_AHB1PeriphClockCmd(CODEC_I2S2_DMA_CLOCK, ENABLE);

	    /* Configure the DMA Stream */
	    DMA_Cmd(CODEC_I2S2_DMA_STREAM, DISABLE);
	    DMA_DeInit(CODEC_I2S2_DMA_STREAM);
	    /* Set the parameters to be configured */
	    DMA_InitStructure2.DMA_Channel = DMA_Channel_0;
	    DMA_InitStructure2.DMA_PeripheralBaseAddr = CODEC_ADC_I2S_ADDRESS;
	    DMA_InitStructure2.DMA_Memory0BaseAddr = (uint32_t)0;      /* This field will be configured in play function */
	    DMA_InitStructure2.DMA_DIR = DMA_DIR_PeripheralToMemory;
	    DMA_InitStructure2.DMA_BufferSize = (uint32_t)0xFFFE;      /* This field will be configured in play function */
	    DMA_InitStructure2.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	    DMA_InitStructure2.DMA_MemoryInc = DMA_MemoryInc_Enable;
	    DMA_InitStructure2.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	    DMA_InitStructure2.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	   // DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; //not needed when using double buffered mode
	    DMA_InitStructure2.DMA_Mode = DMA_Mode_Normal;


	    DMA_InitStructure2.DMA_Priority = DMA_Priority_High;
	    DMA_InitStructure2.DMA_FIFOMode = DMA_FIFOMode_Disable;
	    DMA_InitStructure2.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
	    DMA_InitStructure2.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	    DMA_InitStructure2.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	    DMA_Init(CODEC_I2S2_DMA_STREAM, &DMA_InitStructure2);

	    // enable the dma interrupts transfer complete, half transfer complete and error
	    DMA_ITConfig(CODEC_I2S2_DMA_STREAM, DMA_IT_TC, ENABLE);
	    DMA_ITConfig(CODEC_I2S2_DMA_STREAM, DMA_IT_HT, ENABLE);
	    DMA_ITConfig(CODEC_I2S2_DMA_STREAM, DMA_IT_TE | DMA_IT_FE | DMA_IT_DME, ENABLE);

	    /* I2S DMA IRQ Channel configuration */
	    NVIC_InitStructure.NVIC_IRQChannel = CODEC_I2S2_DMA_IRQ;
	    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = EVAL_AUDIO_IRQ_PREPRIO;
	    NVIC_InitStructure.NVIC_IRQChannelSubPriority = EVAL_AUDIO_IRQ_SUBRIO;
	    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	    NVIC_Init(&NVIC_InitStructure);

	    /* Enable the I2S DMA request */
	     SPI_I2S_DMACmd(CODEC_ADC_I2S, SPI_I2S_DMAReq_Rx, ENABLE);

}
//-----------------------------------------------------------------------------------------------------
void codec_InitGPIO_ADC(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

		//DAC1

		/* Enable I2S and I2C GPIO clocks */
		RCC_AHB1PeriphClockCmd( CODEC_ADC_I2S_GPIO_CLOCK, ENABLE);

		/* CODEC_I2S pins configuration: WS, SCK and SD pins -----------------------------*/
		GPIO_InitStructure.GPIO_Pin = CODEC_ADC_I2S_WS_PIN | CODEC_ADC_I2S_SCK_PIN | CODEC_ADC_I2S_SD_PIN;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(CODEC_ADC_I2S_GPIO, &GPIO_InitStructure);

		/* Connect pins to I2S peripheral  */
		GPIO_PinAFConfig(CODEC_ADC_I2S_GPIO, 		CODEC_ADC_I2S_WS_PINSRC, 	CODEC_ADC_I2S_GPIO_AF);
		GPIO_PinAFConfig(CODEC_ADC_I2S_GPIO, 		CODEC_ADC_I2S_SCK_PINSRC, 	CODEC_ADC_I2S_GPIO_AF);
		GPIO_PinAFConfig(CODEC_ADC_I2S_GPIO, 		CODEC_ADC_I2S_SD_PINSRC, 	CODEC_ADC_I2S_GPIO_AF);


		//Master Clock enable
		/* CODEC_I2S pins configuration: MCK pin */
		GPIO_InitStructure.GPIO_Pin = CODEC_ADC_I2S_MCK_PIN;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(CODEC_ADC_I2S_MCK_GPIO, &GPIO_InitStructure);
		/* Connect pins to I2S peripheral  */
		GPIO_PinAFConfig(CODEC_ADC_I2S_MCK_GPIO, 		CODEC_ADC_I2S_MCK_PINSRC, CODEC_ADC_I2S_GPIO_AF);

}
//-----------------------------------------------------------------------------------------------------
void codec_start_adc(uint32_t Addr, uint32_t Size)
{
	/* Configure the buffer address and size */
	DMA_InitStructure2.DMA_Memory0BaseAddr = (uint32_t)Addr;
	DMA_InitStructure2.DMA_BufferSize = (uint32_t)Size;

	/* Configure the DMA Stream with the new parameters */
	DMA_Init(CODEC_I2S2_DMA_STREAM, &DMA_InitStructure2);

	/* Enable the I2S DMA Stream*/
	DMA_Cmd(CODEC_I2S2_DMA_STREAM, ENABLE);

	/* If the I2S peripheral is still not enabled, enable it */
	if ((CODEC_ADC_I2S->I2SCFGR & I2S_ENABLE_MASK) == 0)
	{
		I2S_Cmd(CODEC_ADC_I2S, ENABLE);
	}
}
//-----------------------------------------------------------------------------------------------------
static void codec_AudioInterface_Init_ADC(uint32_t AudioFreq)
{
  I2S_InitTypeDef I2S_InitStructure;


  /* Enable the CODEC_I2S peripheral clock */
  RCC_APB1PeriphClockCmd(CODEC_ADC_I2S_CLK, ENABLE);

  /* CODEC_I2S peripheral configuration */
  SPI_I2S_DeInit(CODEC_ADC_I2S);
  I2S_InitStructure.I2S_AudioFreq = I2S_AudioFreq_44k;
  I2S_InitStructure.I2S_Standard = I2S_STANDARD;
  I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;
  I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;
  I2S_InitStructure.I2S_Mode = I2S_Mode_MasterRx;
  I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Enable;


  /* Initialize the I2S peripheral with the structure above */
  I2S_Init(CODEC_ADC_I2S, &I2S_InitStructure);


  /* The I2S peripheral will be enabled only in the EVAL_AUDIO_Play() function
       or by user functions if DMA mode not enabled */
}
//-----------------------------------------------------------------------------------------------------
#endif
//################################ Interface Functions ################################################
static void codec_InitGPIO(void)
{
	codec_InitGPIO_DAC1();
#if USE_DAC2
	codec_InitGPIO_DAC2();
#else
	codec_InitGPIO_ADC();
#endif

}
//-----------------------------------------------------------------------------------------------------
void codec_start(uint32_t Addr1, uint32_t Size1, uint32_t Addr2, uint32_t Size2)
{
	codec_start_dac1(Addr1,Size1);
#if USE_DAC2
	codec_start_dac2(Addr2,Size2);
#else
	codec_start_adc(Addr2,Size2);
#endif
}

//-----------------------------------------------------------------------------------------------------
static void codec_AudioInterface_Init(uint32_t AudioFreq)
{
	codec_AudioInterface_Init_DAC(AudioFreq);

#if USE_DAC2
	codec_AudioInterface_Init_DAC2(AudioFreq);
#else
	codec_AudioInterface_Init_ADC(AudioFreq);
#endif
}

//----------------------------------------------------------------------------------
void codec_initDma()
{

		codec_initDma_DAC();
#if USE_DAC2
		codec_initDma_DAC2();
#else
		codec_initDma_ADC();
#endif


}
//----------------------------------------------------------------------------------
void codec_initCsCodec(uint32_t Addr1, uint32_t Size1,uint32_t Addr2, uint32_t Size2)
{
	codec_InitGPIO();
	codec_AudioInterface_Init(44100);
	codec_initDma();
	codec_start(Addr1,Size1,Addr2,Size2);
};
//-----------------------------------------------------------------------------------------------------
