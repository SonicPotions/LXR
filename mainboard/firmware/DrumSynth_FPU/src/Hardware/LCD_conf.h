/*
 * LCD_conf.h
 *
 *  Created on: 20.04.2012
 *      Author: Julian
 */

#ifndef LCD_CONF_H_
#define LCD_CONF_H_

#define USE_LCD 0

#if USE_LCD

#include "stm32f4xx.h"

//TODO andere pins wählen.... so hängt er zusammen mit MEMS!

//the stm32f4 port the LCD is connected to
#define LCD_PORT	GPIOE
#define LCD_CLOCK	RCC_AHB1Periph_GPIOE

//the pins of the LCD_PORT which are connected to the LCD
#define LCD_RS 		GPIO_Pin_5
#define LCD_E		GPIO_Pin_4
#define LCD_DB4 	GPIO_Pin_0
#define LCD_DB5 	GPIO_Pin_1
#define LCD_DB6 	GPIO_Pin_2
#define LCD_DB7 	GPIO_Pin_3



#endif
#endif /* LCD_CONF_H_ */
