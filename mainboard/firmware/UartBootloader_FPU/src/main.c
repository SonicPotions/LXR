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




/* Includes */

#include "stm32f4xx.h"
#include "flash_if.h"
#include "uart.h"
#include "globals.h"
#include "BootloaderCommands.h"


typedef  void (*pFunction)(void);
pFunction Jump_To_Application;
uint32_t JumpAddress;

//used to count the written bytes to the flash memory
volatile uint32_t addressCounter = 0;

//--------------------------------------------------------------
uint16_t crc_xmodem_update (uint16_t crc, uint8_t data)
{
	int i;

	crc = crc ^ ((uint16_t)data << 8);
	for (i=0; i<8; i++)
	{
		if (crc & 0x8000)
			crc = (crc << 1) ^ 0x1021;
		else
			crc <<= 1;
	}

	return crc;
}

//--------------------------------------------------------------

//stub function for newlib
void _exit(int status)
{
	_exit(status);
};

//----------------------------------------------------------------------
// holds the clock infos
RCC_ClocksTypeDef RCC_Clocks;


//----------------------------------------------------------------------
int main(void)
{

	/*
	GPIO_InitTypeDef  GPIO_InitStructure;
	// GPIOD Periph clock enable
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	// Configure PD12, PD13, PD14 and PD15 in output push-pull mode
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	// standard output pin
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_Write(GPIOD,0);	//initial state (all LEDs OFF)

*/

	//init systick timer
	systick_ticks = 0;
	/* get system clock info*/
	RCC_GetClocksFreq(&RCC_Clocks);
	/* set timebase systick to 1ms*/
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);

/*
	//blink some leds
	uint32_t ledBlinkTicks=0;
	uint8_t  blinkLedState=0;
	int i =0;
	while(i<4)
	{
		if(systick_ticks-ledBlinkTicks >= 100)
		{
			ledBlinkTicks = systick_ticks;
			blinkLedState = 1-blinkLedState;
			i++;
			GPIOD->ODR = GPIOD->ODR & 0x0fff;
			GPIOD->ODR |= (blinkLedState*0xf000);
		}
	}
	*/






	//init uart
	uart_init();

	//while(1)uart_tx(ACK);


	//listen for INIT_BOOTLOADER command

	//if INIT_BOOTLOADER cmd received
	if(uart_waitForInitCommand()==START_BOOTLOADER)
	{
		/* Unlock the Flash Program Erase controller */
		FLASH_If_Init();
		//disable write protection if enabled
		if(FLASH_If_GetWriteProtectionStatus())
		{
			FLASH_If_DisableWriteProtection();
		}
		//erase the application flash
		FLASH_If_Erase(APPLICATION_ADDRESS);

		//send ACK to confirm bootloader mode
		uart_tx(ACK);

		//--- start update ---

		//enter bootloader state machine
		uint8_t cmd=0;

		uint8_t state = STATE_WAIT_CMD;
		while(state != STATE_FINISHED)
		{
			switch(state)
			{
			case STATE_FINISHED:
				//should never get here
				break;

			case STATE_WAIT_CMD:
				//we are waiting for the next command
				//so we wait until UART data is received
				//and store the received command
				cmd = uart_rx();
				state = STATE_CMD_RECEIVED;
				break;

			case STATE_CMD_RECEIVED:
				//we know the command, so we have to get the corresponding data blocks
				switch(cmd)
				{

				case INIT_BOOTLOADER:
					//for stray INIT_BOOTLOADER messages that are still in the FIFO from wakeup
					// => ignore
					state = STATE_WAIT_CMD;
				break;

				case WRITE_ADDRESS:
				{
					//first we expect 4 byte address
					uint16_t crcRx; 	// the received crc value
					uint16_t crcCalc=0;	// the local calculated crc value
					uint8_t rxBuffer;	// a buffer to store received data

					//add the command to the crc
					crcCalc = crc_xmodem_update(crcCalc,WRITE_ADDRESS);

					//receive the address and add it to crc
					rxBuffer = uart_rx();
					crcCalc = crc_xmodem_update(crcCalc,rxBuffer);
					addressCounter = (rxBuffer<<24);

					rxBuffer = uart_rx();
					crcCalc = crc_xmodem_update(crcCalc,rxBuffer);
					addressCounter |= (rxBuffer<<16);

					rxBuffer = uart_rx();
					crcCalc = crc_xmodem_update(crcCalc,rxBuffer);
					addressCounter |= (rxBuffer<<8);

					rxBuffer = uart_rx();
					crcCalc = crc_xmodem_update(crcCalc,rxBuffer);
					addressCounter |= rxBuffer;

					//then receive 2 byte crc
					crcRx = (uart_rx()<<8);
					crcRx |= uart_rx();

					//compare CRCs
					if(crcRx == crcCalc)
					{
						//request next message
						uart_tx(ACK);
					}
					else
					{
						//crc check failed
						//request resend
						uart_tx(NACK);
					}

					state = STATE_WAIT_CMD;
				}
					break;
				case WRITE_DATA:
				{
					//first we expect 4 byte data
					uint32_t data;
					uint16_t crcRx;
					uint16_t crcCalc=0;
					uint8_t rxBuffer;

					//add the command to the crc
					crcCalc = crc_xmodem_update(crcCalc,WRITE_DATA);

					rxBuffer = uart_rx();
					crcCalc = crc_xmodem_update(crcCalc,rxBuffer);
					data = (rxBuffer<<24);

					rxBuffer = uart_rx();
					crcCalc = crc_xmodem_update(crcCalc,rxBuffer);
					data |= (rxBuffer<<16);

					rxBuffer = uart_rx();
					crcCalc = crc_xmodem_update(crcCalc,rxBuffer);
					data |= (rxBuffer<<8);

					rxBuffer = uart_rx();
					crcCalc = crc_xmodem_update(crcCalc,rxBuffer);
					data |= rxBuffer;

					//then receive 2 byte crc
					crcRx = (uart_rx()<<8);
					crcRx |= uart_rx();

					//compare CRCs
					if(crcRx == crcCalc)
					{
						//now we can write to the flash
						uint32_t add = addressCounter;
						FLASH_If_Write(&add,&data,1);
						uart_tx(ACK);
					}
					else
					{
						uart_tx(NACK);
					}

					state = STATE_WAIT_CMD;
				}
					break;

				case END_BOOTLOADER:
					state = STATE_FINISHED;
					uart_tx(ACK);
					break;

				default:
					//should never get here
					break;
				}
				break;

			default:
				break;
			}

		}

	}


	//start normal program
	{
	    /* Test if user code is programmed starting from address "APPLICATION_ADDRESS" */
	    if (((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x2FFE0000 ) == 0x20000000)
	    {
	    	uart_deinit();
	      /* Jump to user application */
	      JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
	      Jump_To_Application = (pFunction) JumpAddress;
	      /* Initialize user application's Stack Pointer */
	      __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
	      //align for the new vector table
	      NVIC_SetVectorTable(NVIC_VectTab_FLASH, APPLICATION_ADDRESS);

	      Jump_To_Application();
	    }
	}

	//never get here...
	return 0;
}

