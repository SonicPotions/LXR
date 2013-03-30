/*
 * AudioCodecDma.c
 *
 *  Created on: 15.04.2012
 *      Author: Julian
 */

#if 0
#include "AudioCodecDma.h"
#include "globals.h"

uint16_t audioFrequency = 44100;

DMA_InitTypeDef DMA_InitStructure;
uint32_t  CODECTimeout = CODEC_LONG_TIMEOUT;
uint8_t OutputDev=0; //stores the selected output device (headphone on, speaker off)




//-------------------------------------------------------------------------------------
uint8_t audioCodec_TIMEOUT()
{
	int error=1;
	return error;
}
//-------------------------------------------------------------------------------------

#if 0
/**
  * @brief  This function handles DMA interrupt.
  * @param  None
  * @retval 0 if correct communication, else wrong communication
  */


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
	       DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) &dma_buffer[32*(dmaPtr&0x1)];
	       DMA_InitStructure.DMA_BufferSize = 32;

	       /* Configure the DMA Stream with the new parameters */
	       DMA_Init(DMA1_Stream7, &DMA_InitStructure);

	       /* Enable the I2S DMA Stream*/
	       DMA_Cmd(DMA1_Stream7, ENABLE);

	       //now start next sample block calculation
			 bCurrentSampleValid = 1-(dmaPtr&0x1);



	     }


	  /* Do something*/
	 // dma_buffer[0] = 32756;
	//  audioCodec_start(dma_buffer,32);

    /* Clear the Interrupt flag */
   // DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_TCIF7);




  /* Half Transfer complete interrupt */
  if (DMA_GetFlagStatus(DMA1_Stream7, DMA_FLAG_HTIF7) != RESET)
  {
	  //do something

    /* Clear the Interrupt flag */
    DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_HTIF7);
  }


#if 1
/*
  if(DMA_GetFlagStatus(DMA1_Stream7, DMA_FLAG_TEIF7) != RESET)
  {

	  int error=12;
  }
  else if (DMA_GetFlagStatus(DMA1_Stream7, DMA_FLAG_FEIF7) != RESET)
  {
	  int error=12;
  }
  else if (DMA_GetFlagStatus(DMA1_Stream7, DMA_FLAG_DMEIF7) != RESET)
  {

	  	  int error=12;
  }
*/
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

#endif
//-------------------------------------------------------------------------------------
/**
  * @brief  Inserts a delay time (not accurate timing).
  * @param  nCount: specifies the delay time length.
  * @retval None
  */
static void Delay( __IO uint32_t nCount)
{
  for (; nCount != 0; nCount--);
}
//-------------------------------------------------------------------------------------
void audioCodec_initDma()
{
	NVIC_InitTypeDef NVIC_InitStructure;

	 /* Enable the DMA clock */
	    RCC_AHB1PeriphClockCmd(AUDIO_I2S_DMA_CLOCK, ENABLE);

	    /* Configure the DMA Stream */
	    DMA_Cmd(DMA1_Stream7, DISABLE);
	    DMA_DeInit(DMA1_Stream7);
	    /* Set the parameters to be configured */
	    DMA_InitStructure.DMA_Channel = DMA_Channel_0;
	    DMA_InitStructure.DMA_PeripheralBaseAddr = CODEC_I2S_ADDRESS;
	    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)0;      /* This field will be configured in play function */
	    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	    DMA_InitStructure.DMA_BufferSize = (uint32_t)0xFFFE;      /* This field will be configured in play function */
	    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	   // DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; //not needed when using double buffered mode
	    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;


	    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
	    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	    DMA_Init(DMA1_Stream7, &DMA_InitStructure);

	    // enable the dma interrupts transfer complete, half transfer complete and error
	    DMA_ITConfig(DMA1_Stream7, DMA_IT_TC, ENABLE);
	    DMA_ITConfig(DMA1_Stream7, DMA_IT_HT, ENABLE);
	    DMA_ITConfig(DMA1_Stream7, DMA_IT_TE | DMA_IT_FE | DMA_IT_DME, ENABLE);



	    /* I2S DMA IRQ Channel configuration */
	    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream7_IRQn;
	    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = EVAL_AUDIO_IRQ_PREPRIO;
	    NVIC_InitStructure.NVIC_IRQChannelSubPriority = EVAL_AUDIO_IRQ_SUBRIO;
	    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	    NVIC_Init(&NVIC_InitStructure);

	    /* Enable the I2S DMA request */
	     SPI_I2S_DMACmd(CODEC_I2S, SPI_I2S_DMAReq_Tx, ENABLE);

}
//-------------------------------------------------------------------------------------
/**
  * @brief  Initializes the Audio Codec audio interface (I2S)
  * @note   This function assumes that the I2S input clock (through PLL_R in
  *         Devices RevA/Z and through dedicated PLLI2S_R in Devices RevB/Y)
  *         is already configured and ready to be used.
  * @param  AudioFreq: Audio frequency to be configured for the I2S peripheral.
  * @retval None
  */
static void audioCodec_AudioInterface_Init(uint32_t AudioFreq)
{
  I2S_InitTypeDef I2S_InitStructure;


  /* Enable the CODEC_I2S peripheral clock */
  RCC_APB1PeriphClockCmd(CODEC_I2S_CLK, ENABLE);

  /* CODEC_I2S peripheral configuration */
  SPI_I2S_DeInit(CODEC_I2S);
  I2S_InitStructure.I2S_AudioFreq = AudioFreq;
  I2S_InitStructure.I2S_Standard = I2S_STANDARD;
  I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;
  I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;
  I2S_InitStructure.I2S_Mode = I2S_Mode_MasterTx;
  I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Enable;


  /* Initialize the I2S peripheral with the structure above */
  I2S_Init(CODEC_I2S, &I2S_InitStructure);


  /* The I2S peripheral will be enabled only in the EVAL_AUDIO_Play() function
       or by user functions if DMA mode not enabled */
}
//-------------------------------------------------------------------------------------

/**
  * @brief  Reads and returns the value of an audio codec register through the
  *         control interface (I2C).
  * @param  RegisterAddr: Address of the register to be read.
  * @retval Value of the register to be read or dummy value if the communication
  *         fails.
  */
static uint32_t audioCodec_ReadRegister(uint8_t RegisterAddr)
{
  uint32_t result = 0;

  /*!< While the bus is busy */
  CODECTimeout = CODEC_LONG_TIMEOUT;
  while(I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BUSY))
  {
    if((CODECTimeout--) == 0) return audioCodec_TIMEOUT();
  }

  /* Start the config sequence */
  I2C_GenerateSTART(CODEC_I2C, ENABLE);

  /* Test on EV5 and clear it */
  CODECTimeout = CODEC_FLAG_TIMEOUT;
  while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if((CODECTimeout--) == 0) return audioCodec_TIMEOUT();
  }

  /* Transmit the slave address and enable writing operation */
  I2C_Send7bitAddress(CODEC_I2C, CODEC_ADDRESS, I2C_Direction_Transmitter);

  /* Test on EV6 and clear it */
  CODECTimeout = CODEC_FLAG_TIMEOUT;
  while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    if((CODECTimeout--) == 0) return audioCodec_TIMEOUT();
  }

  /* Transmit the register address to be read */
  I2C_SendData(CODEC_I2C, RegisterAddr);

  /* Test on EV8 and clear it */
  CODECTimeout = CODEC_FLAG_TIMEOUT;
  while (I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BTF) == RESET)
  {
    if((CODECTimeout--) == 0) return audioCodec_TIMEOUT();
  }

  /*!< Send START condition a second time */
  I2C_GenerateSTART(CODEC_I2C, ENABLE);

  /*!< Test on EV5 and clear it (cleared by reading SR1 then writing to DR) */
  CODECTimeout = CODEC_FLAG_TIMEOUT;
  while(!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if((CODECTimeout--) == 0) return audioCodec_TIMEOUT();
  }

  /*!< Send Codec address for read */
  I2C_Send7bitAddress(CODEC_I2C, CODEC_ADDRESS, I2C_Direction_Receiver);

  /* Wait on ADDR flag to be set (ADDR is still not cleared at this level */
  CODECTimeout = CODEC_FLAG_TIMEOUT;
  while(I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_ADDR) == RESET)
  {
    if((CODECTimeout--) == 0) return audioCodec_TIMEOUT();
  }

  /*!< Disable Acknowledgment */
  I2C_AcknowledgeConfig(CODEC_I2C, DISABLE);

  /* Clear ADDR register by reading SR1 then SR2 register (SR1 has already been read) */
  (void)CODEC_I2C->SR2;

  /*!< Send STOP Condition */
  I2C_GenerateSTOP(CODEC_I2C, ENABLE);

  /* Wait for the byte to be received */
  CODECTimeout = CODEC_FLAG_TIMEOUT;
  while(I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_RXNE) == RESET)
  {
    if((CODECTimeout--) == 0) return audioCodec_TIMEOUT();
  }

  /*!< Read the byte received from the Codec */
  result = I2C_ReceiveData(CODEC_I2C);

  /* Wait to make sure that STOP flag has been cleared */
  CODECTimeout = CODEC_FLAG_TIMEOUT;
  while(CODEC_I2C->CR1 & I2C_CR1_STOP)
  {
    if((CODECTimeout--) == 0) return audioCodec_TIMEOUT();
  }

  /*!< Re-Enable Acknowledgment to be ready for another reception */
  I2C_AcknowledgeConfig(CODEC_I2C, ENABLE);

  /* Clear AF flag for next communication */
  I2C_ClearFlag(CODEC_I2C, I2C_FLAG_AF);

  /* Return the byte read from Codec */
  return result;
}

//-------------------------------------------------------------------------------------

/**
  * @brief  Writes a Byte to a given register into the audio codec through the
            control interface (I2C)
  * @param  RegisterAddr: The address (location) of the register to be written.
  * @param  RegisterValue: the Byte value to be written into destination register.
  * @retval 0 if correct communication, else wrong communication
  */
static uint32_t audioCodec_WriteRegister(uint8_t RegisterAddr, uint8_t RegisterValue)
{
  uint32_t result = 0;

  /*!< While the bus is busy */
  CODECTimeout = CODEC_LONG_TIMEOUT;
  while(I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BUSY))
  {
    if((CODECTimeout--) == 0) return audioCodec_TIMEOUT();
  }

  /* Start the config sequence */
  I2C_GenerateSTART(CODEC_I2C, ENABLE);

  /* Test on EV5 and clear it */
  CODECTimeout = CODEC_FLAG_TIMEOUT;
  while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if((CODECTimeout--) == 0) return audioCodec_TIMEOUT();
  }

  /* Transmit the slave address and enable writing operation */
  I2C_Send7bitAddress(CODEC_I2C, CODEC_ADDRESS, I2C_Direction_Transmitter);

  /* Test on EV6 and clear it */
  CODECTimeout = CODEC_FLAG_TIMEOUT;
  while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    if((CODECTimeout--) == 0) return audioCodec_TIMEOUT();
  }

  /* Transmit the first address for write operation */
  I2C_SendData(CODEC_I2C, RegisterAddr);

  /* Test on EV8 and clear it */
  CODECTimeout = CODEC_FLAG_TIMEOUT;
  while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
  {
    if((CODECTimeout--) == 0) return audioCodec_TIMEOUT();
  }

  /* Prepare the register value to be sent */
  I2C_SendData(CODEC_I2C, RegisterValue);

  /*!< Wait till all data have been physically transferred on the bus */
  CODECTimeout = CODEC_LONG_TIMEOUT;
  while(!I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BTF))
  {
    if((CODECTimeout--) == 0) audioCodec_TIMEOUT();
  }

  /* End the configuration sequence */
  I2C_GenerateSTOP(CODEC_I2C, ENABLE);

#ifdef VERIFY_WRITTENDATA
  /* Verify that the data has been correctly written */
  result = (audioCodec_ReadRegister(RegisterAddr) == RegisterValue)? 0:1;
#endif /* VERIFY_WRITTENDATA */

  /* Return the verifying value: 0 (Passed) or 1 (Failed) */
  return result;
}

//-------------------------------------------------------------------------------------
/**
  * @brief  Sets higher or lower the codec volume level.
  * @param  Volume: a byte value from 0 to 255 (refer to codec registers
  *         description for more details).
  * @retval 0 if correct communication, else wrong communication
  */
static uint32_t audioCodec_VolumeCtrl(uint8_t Volume)
{
	Volume = VOLUME_CONVERT(Volume);
  uint32_t counter = 0;

  if (Volume > 0xE6)
  {
    /* Set the Master volume */
    counter += audioCodec_WriteRegister(0x20, Volume - 0xE7);
    counter += audioCodec_WriteRegister(0x21, Volume - 0xE7);
  }
  else
  {
    /* Set the Master volume */
    counter += audioCodec_WriteRegister(0x20, Volume + 0x19);
    counter += audioCodec_WriteRegister(0x21, Volume + 0x19);
  }

  return counter;
}
//-------------------------------------------------------------------------------------
/**
  * @brief  Initializes the Audio Codec control interface (I2C).
  * @param  None
  * @retval None
  */
static void audioCodec_CtrlInterface_Init(void)
{
  I2C_InitTypeDef I2C_InitStructure;

  /* Enable the CODEC_I2C peripheral clock */
  RCC_APB1PeriphClockCmd(CODEC_I2C_CLK, ENABLE);

  /* CODEC_I2C peripheral configuration */
  I2C_DeInit(CODEC_I2C);
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 = 0x33;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = I2C_SPEED;
  /* Enable the I2C peripheral */
  I2C_Cmd(CODEC_I2C, ENABLE);
  I2C_Init(CODEC_I2C, &I2C_InitStructure);
}
//-------------------------------------------------------------------------------------
/**
  * @brief Initializes IOs used by the Audio Codec (on the control and audio
  *        interfaces).
  * @param  None
  * @retval None
  */
static void audioCodec_InitGPIO(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable Reset GPIO Clock */
  RCC_AHB1PeriphClockCmd(AUDIO_RESET_GPIO_CLK,ENABLE);

  /* Audio reset pin configuration -------------------------------------------------*/
  GPIO_InitStructure.GPIO_Pin = AUDIO_RESET_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(AUDIO_RESET_GPIO, &GPIO_InitStructure);

  /* Enable I2S and I2C GPIO clocks */
  RCC_AHB1PeriphClockCmd(CODEC_I2C_GPIO_CLOCK | CODEC_I2S_GPIO_CLOCK, ENABLE);

  /* CODEC_I2C SCL and SDA pins configuration -------------------------------------*/
  GPIO_InitStructure.GPIO_Pin = CODEC_I2C_SCL_PIN | CODEC_I2C_SDA_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(CODEC_I2C_GPIO, &GPIO_InitStructure);
  /* Connect pins to I2C peripheral */
  GPIO_PinAFConfig(CODEC_I2C_GPIO, CODEC_I2S_SCL_PINSRC, CODEC_I2C_GPIO_AF);
  GPIO_PinAFConfig(CODEC_I2C_GPIO, CODEC_I2S_SDA_PINSRC, CODEC_I2C_GPIO_AF);

  /* CODEC_I2S pins configuration: WS, SCK and SD pins -----------------------------*/
  GPIO_InitStructure.GPIO_Pin = CODEC_I2S_SCK_PIN | CODEC_I2S_SD_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(CODEC_I2S_GPIO, &GPIO_InitStructure);

  /* Connect pins to I2S peripheral  */
  GPIO_PinAFConfig(CODEC_I2S_WS_GPIO, CODEC_I2S_WS_PINSRC, CODEC_I2S_GPIO_AF);
  GPIO_PinAFConfig(CODEC_I2S_GPIO, CODEC_I2S_SCK_PINSRC, CODEC_I2S_GPIO_AF);


	GPIO_InitStructure.GPIO_Pin = CODEC_I2S_WS_PIN ;
	GPIO_Init(CODEC_I2S_WS_GPIO, &GPIO_InitStructure);
	GPIO_PinAFConfig(CODEC_I2S_GPIO, CODEC_I2S_SD_PINSRC, CODEC_I2S_GPIO_AF);


//Master Clock enable
  /* CODEC_I2S pins configuration: MCK pin */
  GPIO_InitStructure.GPIO_Pin = CODEC_I2S_MCK_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(CODEC_I2S_MCK_GPIO, &GPIO_InitStructure);
  /* Connect pins to I2S peripheral  */
  GPIO_PinAFConfig(CODEC_I2S_MCK_GPIO, CODEC_I2S_MCK_PINSRC, CODEC_I2S_GPIO_AF);

}
//---------------------------------------------------------------------------------------------
/**
  * @brief  Resets the audio codec. It restores the default configuration of the
  *         codec (this function shall be called before initializing the codec).
  * @note   This function calls an external driver function: The IO Expander driver.
  * @param  None
  * @retval None
  */
static void audioCodec_Reset(void)
{
  /* Power Down the codec */
  GPIO_WriteBit(AUDIO_RESET_GPIO, AUDIO_RESET_PIN, Bit_RESET);

  /* wait for a delay to insure registers erasing */
  Delay(CODEC_RESET_DELAY);

  /* Power on the codec */
  GPIO_WriteBit(AUDIO_RESET_GPIO, AUDIO_RESET_PIN, Bit_SET);
}
//---------------------------------------------------------------------------------------------
/** call on bootup to configure and start the audio codec via I2S  DMA
 */
 uint8_t audioCodec_init()
{
	 uint8_t error = 0;

	  /* Configure the Codec related IOs */
	 audioCodec_InitGPIO();

	  /* Reset the Codec Registers */
	  audioCodec_Reset();

	  /* Initialize the Control interface of the Audio Codec */
	  audioCodec_CtrlInterface_Init();


	  // Now configure the audio codec registers


	  /* Keep Codec powered OFF */
	  // Power Control 1
	  // -> 0000 0001 Powered Down - same as setting 1001 1111
	  // 1001 1110 Powered Up
	  // 1001 1111 Powered Down - same as setting 0000 0001
	  // 6
	  error += audioCodec_WriteRegister(0x02, 0x01);

	  /* SPK always OFF & HP always ON */
	  error += audioCodec_WriteRegister(0x04, 0xAF);
	    OutputDev = 0xAF;

      /* Clock configuration:*/
	  //testweise mal auto aus und manuell auf
	  // MCLK 11.2896 SR 44100 stellen
	  //7 auto			0
	  //6 speed1		0
	  //5 speed 0		1
	  //4 32k_group		0
	  //3 VIDEOCLK		0
	  //2 ratio1		0
	  //1 ratio0		0
	  //0 MCLKDIV2		0
	  error += audioCodec_WriteRegister(0x05, 0x20);

	  //disable limiter
	  error += audioCodec_WriteRegister(0x28, 0x00);

	  //disable limiter
	  audioCodec_WriteRegister(0x28, 0x00);

	  /* Set the Slave Mode and the audio Standard */
	  //0x04 = 00000100
	  //7 M/S		0 slave
	  //6 INC_SCLK	0 not inverted
	  //5 -
	  //4 DSP		0 no data packed interface
	  //3 DACDIF1	0
	  //2 DACDIF0	1 I2s format
	  //1 AWL1		0
	  //0 AWL0		0
	  error += audioCodec_WriteRegister(0x06, CODEC_STANDARD);

	  /* Set the Master volume */
	  audioCodec_VolumeCtrl(codec_volume);

	  /* Power on the Codec */
	  error += audioCodec_WriteRegister(0x02, 0x9E);



	  /* Disable the analog soft ramp */
	  error += audioCodec_WriteRegister(0x0A, 0x00);

	    /* enable the digital soft ramp and enable deemphasis*/
		  // 7 passthru b
		  // 6 passthru a
		  // 5 passb mute
		  // 4 passa mute
		  // 3 Freeze
		  // 2 de-emphasis
		  // 1 digital soft ramp
		  // 0 digital zero crossing

	  error += audioCodec_WriteRegister(0x0E, 0x07); // emphasis, soft ramp + zero crossing on

	  /* Disable the limiter attack level */
	  error += audioCodec_WriteRegister(0x27, 0x00);


	  /* Disable tone control JS*/
	  audioCodec_WriteRegister(0x1e, 0x38);
	  //00 beep off
	  //1 beep mix off
	  //11 treble auf 15kHz
	  //00 bass auf 50Hz
	  // 0 tone control aus

	  /* Adjust Bass and Treble levels */
	  //counter += Codec_WriteRegister(0x1F, 0x0F);
	  error += audioCodec_WriteRegister(0x1F, 0x8f);
	  /* Adjust PCM volume level */
	  error += audioCodec_WriteRegister(0x1A, 0x0A);
	  error += audioCodec_WriteRegister(0x1B, 0x0A);

	  /* Configure the I2S peripheral */
	  audioCodec_AudioInterface_Init(audioFrequency);

	  //now setup the DMA
	  audioCodec_initDma();

	  /* Return communication control value */
	  return error;
}
//------------------------------------------------------------------------------
void audioCodec_start(uint32_t Addr, uint32_t Size)
{
	/* Configure the buffer address and size */
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)Addr;
	DMA_InitStructure.DMA_BufferSize = (uint32_t)Size;

	/* Configure the DMA Stream with the new parameters */
	DMA_Init(DMA1_Stream7, &DMA_InitStructure);

	/* Enable the I2S DMA Stream*/
	DMA_Cmd(DMA1_Stream7, ENABLE);



  /* If the I2S peripheral is still not enabled, enable it */
  if ((CODEC_I2S->I2SCFGR & I2S_ENABLE_MASK) == 0)
  {
	I2S_Cmd(CODEC_I2S, ENABLE);
  }

}


//---------------------------------------------------------------------------------

//----------------- build in EQ ----------------------------
#if USE_LIMITER_EQ
//---------------------------------------------------------------------

//Beep & Tone Configuration
#define TONE_CONFIG_REF		0x1e

/**Treble Corner Frequency
Sets the corner frequency (-3 dB point) for the treble shelving filter
TREBCF[1:0] Treble Corner Frequency Setting
00 		5 kHz
01 		7 kHz
10 		10 kHz
11 		15 kHz
*/
#define TONE_TREB_CFG1		0x4
#define TONE_TREB_CFG0		0x3
/**Bass Corner Frequency
Sets the corner frequency (-3 dB point) for the bass shelving filter.
BASSCF[1:0] Bass Corner Frequency Setting
00 		50 Hz
01 		100 Hz
10 		200 Hz
11 		250 Hz
*/
#define TONE_BASS_CFG1		0x2
#define TONE_BASS_CFG0		0x1
/**Tone Control Enable
Configures the treble and bass activation.
0 disable
1 enable
*/
#define TONE_ENABLE			0x00

//Tone Control Register
#define TONE_CTRL_REG 		0x1f
/**Treble Gain
Sets the gain of the treble shelving filter.
TREB[3:0] Gain Setting
0000 	+12.0 dB
··· ···
0111 	+1.5 dB
1000 	0 dB
1001 	-1.5 dB
··· ···
1111 	-10.5 dB
Step Size: 1.5 dB
*/
#define TONE_TREB3			0x7
#define TONE_TREB2			0x6
#define TONE_TREB1			0x5
#define TONE_TREB0			0x4
/**Bass Gain
Sets the gain of the bass shelving filter.
Bass[3:0] Gain Setting
0000 	+12.0 dB
··· ···
0111 	+1.5 dB
1000 	0 dB
1001 	-1.5 dB
··· ···
1111 	-10.5 dB
Step Size: 1.5 dB
*/
#define TONE_BASS3			0x3
#define TONE_BASS2			0x2
#define TONE_BASS1			0x1
#define TONE_BASS0			0x0


//limiter control register 1
#define LIMITER_CONTROL1	0x27

/**Limiter Maximum Threshold
Sets the maximum level, below full scale, at which to limit and attenuate the output signal at the attack rate.
LMAX[2:0] Threshold Setting
000 	0 dB
001 	-3 dB
010 	-6 dB
011 	-9 dB
100 	-12 dB
101 	-18 dB
110 	-24 dB
111 	-30 dB
 */
#define LIMIT_LMAX2			0x07
#define LIMIT_LMAX1			0x06
#define LIMIT_LMAX0			0x05

/**Limiter Cushion Threshold
Sets the minimum level at which to disengage the Limiter’s attenuation at the release rate
CUSH[2:0] Threshold Setting
000 	0 dB
001 	-3 dB
010 	-6 dB
011 	-9 dB
100 	-12 dB
101 	-18 dB
110 	-24 dB
111 	-30 dB
*/
#define LIMIT_CUSH2			0x04
#define LIMIT_CUSH1			0x03
#define LIMIT_CUSH0			0x02

/**Limiter Soft Ramp Disable
 Configures an override of the digital soft ramp setting.
0 OFF; Limiter Attack Rate is dictated by the DIGSFT (“Digital Soft Ramp”) setting
1 ON; Limiter volume changes take effect in one step, regardless of the DIGSFT setting.
 */
#define LIMIT_SOFT_RAMP		0x01

/**Limiter Zero Cross Disable
Configures an override of the digital zero cross setting.
0 OFF; Limiter Attack Rate is dictated by the DIGZC (“Digital Zero Cross”) setting
1 ON; Limiter volume changes take effect in one step, regardless of the DIGZC setting.
*/
#define LIMIT_ZERO_X		0x00
//---------------------------------------------------------------------
//limiter control register 2
#define LIMITER_CONTROL2 	0x28

#define LIMIT_ON_OFF		0x7 /**<Configures the peak detect and limiter circuitry. 0:disabled, 1:enabled*/
#define LIMIT_ALL			0x6	/**<Sets how channels are attenuated when the limiter is enabled. 0:single channel, 1:both channels*/
#define LIMIT_RELEASE5		0x5 /**<Sets the rate at which the limiter releases the digital attenuation 00 0000:fastest, 111111:slowest*/
#define LIMIT_RELEASE4		0x4
#define LIMIT_RELEASE3		0x3
#define LIMIT_RELEASE2		0x2
#define LIMIT_RELEASE1		0x1
#define LIMIT_RELEASE0		0x0
//---------------------------------------------------------------------
//Limiter Attack Rate Register
#define LIMIT_ATTACK_REG	0x29
/** Limiter Attack Rate
 Sets the rate at which the limiter applies digital attenuation from levels above the MAX[2:0] threshold
 LIMARATE[5:0] Attack Time
00 0000 Fastest Attack
··· ···
11 1111 Slowest Attack
 */
#define LIMIT_ATTACK5		0x5
#define LIMIT_ATTACK4		0x4
#define LIMIT_ATTACK3		0x3
#define LIMIT_ATTACK2		0x2
#define LIMIT_ATTACK1		0x1
#define LIMIT_ATTACK0		0x0


void audioCodec__initEq()
{
	//init to:
	//disabled, trebF 5kHz, bassF 100Hz
	audioCodec_WriteRegister(TONE_CONFIG_REF, (1<<TONE_BASS_CFG0));
	//both gain 0dB
	audioCodec_WriteRegister(TONE_CTRL_REG, (1<<TONE_TREB3) | (1<<TONE_BASS3));
}
//---------------------------------------------------------------------
void audioCodec_setEqEnabled(uint8_t enable)
{
	uint8_t reg = audioCodec_ReadRegister(TONE_CONFIG_REF);

	if(enable)
	{
		reg |= (1<<TONE_ENABLE);
	}
	else
	{
		reg &= ~(1<<TONE_ENABLE);
	}

	audioCodec_WriteRegister(TONE_CONFIG_REF, reg);
}
//---------------------------------------------------------------------
/** 0 = 50Hz
 *  1 = 100Hz
 *  2 = 200Hz
 *  3 = 250Hz
 */
void audioCodec_setEqBassFreq(uint8_t f)
{
	uint8_t reg = audioCodec_ReadRegister(TONE_CONFIG_REF);

	//clear bass freq bits
	reg &= ~(  (1<<TONE_BASS_CFG1) |  (1<<TONE_BASS_CFG0)  );
	//set new freq (bits 1-2)
	reg |= ((f&0x3)<<TONE_BASS_CFG0);

	audioCodec_WriteRegister(TONE_CONFIG_REF, reg);
}
//---------------------------------------------------------------------
/** 0 = 5kHz
 *  1 = 7kHz
 *  2 = 10kHz
 *  3 = 15kHz
 */
void audioCodec_setEqTrebFreq(uint8_t f)
{
	uint8_t reg = audioCodec_ReadRegister(TONE_CONFIG_REF);

	//clear treb freq bits
	reg &= ~(  (1<<TONE_TREB_CFG1) |  (1<<TONE_TREB_CFG0)  );
	//set new freq (bits 3-4)
	reg |= ((f&0x3)<<TONE_TREB_CFG0);

	audioCodec_WriteRegister(TONE_CONFIG_REF, reg);
}

//---------------------------------------------------------------------
void audioCodec_setEqTrebGain(uint8_t g)
{
	uint8_t reg = audioCodec_ReadRegister(TONE_CTRL_REG);

	//clear bass gain bits
	reg &= ~(  (1<<TONE_TREB3) |  (1<<TONE_TREB2) |  (1<<TONE_TREB1) | (1<<TONE_TREB0)  );
	//set new gain (bits 4-7)
	reg |= ((g&0xf)<<TONE_TREB0);

	audioCodec_WriteRegister(TONE_CTRL_REG, reg);
}
//---------------------------------------------------------------------
void audioCodec_setEqBassGain(uint8_t g)
{
	uint8_t reg = audioCodec_ReadRegister(TONE_CTRL_REG);

	//clear bass gain bits
	reg &= ~(  (1<<TONE_BASS3) |  (1<<TONE_BASS2) |  (1<<TONE_BASS1) | (1<<TONE_BASS0)  );
	//set new gain (bits 0-3)
	reg |= (g&0xf);

	audioCodec_WriteRegister(TONE_CTRL_REG, reg);
}

//--------------------- build in limiter --------------------
void audioCodec_setLimiterInit()
{
	//initialize the limiter
	//inti state is
	//limiter off
	//shortest release time
	//shortest attack time
	//all channels
	audioCodec_WriteRegister(LIMITER_CONTROL2, (1<<LIMIT_ALL) );
	audioCodec_WriteRegister(LIMIT_ATTACK_REG, 0x00);
}
//---------------------------------------------------------------------
void audioCodec_setLimiterEnabled(uint8_t enable)
{
	uint8_t reg = audioCodec_ReadRegister(LIMITER_CONTROL2);

	if(enable)
	{
		reg |= (1<<LIMIT_ON_OFF);
	}
	else
	{
		reg &= ~(1<<LIMIT_ON_OFF);
	}

	audioCodec_WriteRegister(LIMITER_CONTROL2, reg);
}
//---------------------------------------------------------------------
void audioCodec_setLimiterAttack(uint8_t attack)
{
	audioCodec_WriteRegister(LIMIT_ATTACK_REG, attack&0x3f);
}
//---------------------------------------------------------------------
void audioCodec_setLimiterRelease(uint8_t release)
{
	uint8_t reg = audioCodec_ReadRegister(LIMITER_CONTROL2);

	//clear all release rate bits
	reg &= ~( (1<<LIMIT_RELEASE5) | (1<<LIMIT_RELEASE4) | (1<<LIMIT_RELEASE3) | (1<<LIMIT_RELEASE2) | (1<<LIMIT_RELEASE1) | (1<<LIMIT_RELEASE0) );

	//set the new value
	reg |= release&0x3f;

	audioCodec_WriteRegister(LIMITER_CONTROL2, reg);
}
//---------------------------------------------------------------------
void audioCodec_setLimiterMaxTresh(uint8_t max)
{
	uint8_t reg = audioCodec_ReadRegister(LIMITER_CONTROL1);

	//clear all MaxTresh bits
	reg &= ~( (1<<LIMIT_LMAX2) | (1<<LIMIT_LMAX1) | (1<<LIMIT_LMAX0) );

	//set the new value (bits 5,6,7)
	reg |= ((max&0x7)<<LIMIT_LMAX0);

	audioCodec_WriteRegister(LIMITER_CONTROL1, reg);

}
//---------------------------------------------------------------------
void audioCodec_setLimiterMinTresh(uint8_t min)
{
	uint8_t reg = audioCodec_ReadRegister(LIMITER_CONTROL1);

	//clear all MaxTresh bits
	reg &= ~( (1<<LIMIT_CUSH2) | (1<<LIMIT_CUSH1) | (1<<LIMIT_CUSH0) );

	//set the new value (bits 2,3,4)
	reg |= ((min&0x7)<<LIMIT_CUSH0);

	audioCodec_WriteRegister(LIMITER_CONTROL1, reg);
}
//---------------------------------------------------------------------

#endif
#endif
