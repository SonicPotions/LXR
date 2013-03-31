/*
 * rnd.c
 *
 *  Created on: 07.04.2012
 *      Author: Julian
 */

#include "random.h"
#include "stm32f4xx.h"
//-------------------------------------------------------------
void initRng()
{
	/* Enable RNG clock source */
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);

	/* RNG Peripheral enable */
	RNG_Cmd(ENABLE);
}
//-------------------------------------------------------------
__inline uint32_t GetRngValue()
{
	return RNG_GetRandomNumber();
}
//-------------------------------------------------------------

