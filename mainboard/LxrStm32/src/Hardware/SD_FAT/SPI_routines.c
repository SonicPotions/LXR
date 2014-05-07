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

#include "config.h"
#if USE_SD_CARD
#include "SPI_routines.h"

//-------------------------------------------------------------------------
void spi_initInterface()
{
	/* Enable the SPI peripheral clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE); //APB2 Prescaler 2 = 168000000/2 = 84000000 = 84mHz

	/* SPI1 peripheral configuration */
	SPI_I2S_DeInit(SPI1);

	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256; //84000000/256 = 328125 Hz = 328kHz
	SPI_InitStructure.SPI_CPHA				= SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_CPOL				= SPI_CPOL_High;
	SPI_InitStructure.SPI_CRCPolynomial		= 7;
	SPI_InitStructure.SPI_DataSize			= SPI_DataSize_8b;
	SPI_InitStructure.SPI_Direction			= SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_FirstBit			= SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_Mode				= SPI_Mode_Master;
	SPI_InitStructure.SPI_NSS				= SPI_NSS_Soft;

	/* Initialize the SPI1 peripheral with the structure above */
	SPI_Init(SPI1, &SPI_InitStructure);

	SPI_Cmd(SPI1,ENABLE);
}

//-------------------------------------------------------------------------
void spi_gpioInit()
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable spi port clock */
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB, ENABLE);

	// init the pins
	//MISO and MOSI
	GPIO_InitStructure.GPIO_Pin = SPI_MISO_PIN | SPI_MOSI_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(SPI_MISO_MOSI_PORT, &GPIO_InitStructure);
	//SCK pins
	GPIO_InitStructure.GPIO_Pin = SPI_SCK_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(SPI_CS_SCK_PORT, &GPIO_InitStructure);
	//CS
	GPIO_InitStructure.GPIO_Pin = SPI_CS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(SPI_CS_SCK_PORT, &GPIO_InitStructure);

	//configure pins to alternate function
	GPIO_PinAFConfig(SPI_MISO_MOSI_PORT, 	SPI_MISO_PIN_SRC, 	GPIO_AF_SPI1);
	GPIO_PinAFConfig(SPI_MISO_MOSI_PORT, 	SPI_MOSI_PIN_SRC, 	GPIO_AF_SPI1);
	GPIO_PinAFConfig(SPI_CS_SCK_PORT, 		SPI_SCK_PIN_SRC, 	GPIO_AF_SPI1);
}
//-------------------------------------------------------------------------
//SPI initialize for SD card
void spi_init(void)
{
	
	//init port
	spi_gpioInit();
	
	spi_initInterface();
}

void spi_deInit()
{
	SPI_Cmd(SPI1,DISABLE);

	GPIO_InitTypeDef GPIO_InitStructure;


	// init the pins
	//MISO and MOSI
	GPIO_InitStructure.GPIO_Pin = SPI_MISO_PIN | SPI_MOSI_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(SPI_MISO_MOSI_PORT, &GPIO_InitStructure);
	//SCK pins
	GPIO_InitStructure.GPIO_Pin = SPI_SCK_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(SPI_CS_SCK_PORT, &GPIO_InitStructure);
	//CS
	GPIO_InitStructure.GPIO_Pin = SPI_CS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(SPI_CS_SCK_PORT, &GPIO_InitStructure);


}

unsigned char SPI_transmit(unsigned char data)
{


	/*!< Loop while DR register in not empty */
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);

	/*!< Send byte through the SPI1 peripheral */
	SPI_I2S_SendData(SPI1, data);

	/*!< Wait to receive a byte */
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);

	/*!< Return the byte read from the SPI bus */
	return SPI_I2S_ReceiveData(SPI1);
}

unsigned char SPI_receive(void)
{
	return (SPI_transmit(0xff));
}
#endif

