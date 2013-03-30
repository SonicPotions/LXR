/*
 * LCD_driver.h
 *
 *  Created on: 20.04.2012
 *      Author: Julian
 */

#ifndef LCD_DRIVER_H_
#define LCD_DRIVER_H_

#include "LCD_conf.h"
#include "FIFO.h"

#if USE_LCD

#define LCD_CLEAR_DISPLAY       0x01
#define LCD_CURSOR_HOME         0x02

// Set DD RAM Address --------- 0b1xxxxxxx  (Display Data RAM)
#define LCD_SET_DDADR           0x80

#define LCD_DDADR_LINE1         0x00
#define LCD_DDADR_LINE2         0x40
#define LCD_DDADR_LINE3         0x10
#define LCD_DDADR_LINE4         0x50

// Set Entry Mode ------------- 0b000001xx
#define LCD_SET_ENTRY           0x04

#define LCD_ENTRY_DECREASE      0x00
#define LCD_ENTRY_INCREASE      0x02
#define LCD_ENTRY_NOSHIFT       0x00
#define LCD_ENTRY_SHIFT         0x01

// Set Function --------------- 0b001xxxxx
#define LCD_SET_FUNCTION        0x20

#define LCD_FUNCTION_4BIT       0x00
#define LCD_FUNCTION_8BIT       0x10
#define LCD_FUNCTION_1LINE      0x00
#define LCD_FUNCTION_2LINE      0x08
#define LCD_FUNCTION_5X7        0x00
#define LCD_FUNCTION_5X10       0x04

// Set Display ---------------- 0b00001xxx
#define LCD_SET_DISPLAY         0x08

#define LCD_DISPLAY_OFF         0x00
#define LCD_DISPLAY_ON          0x04
#define LCD_CURSOR_OFF          0x00
#define LCD_CURSOR_ON           0x02
#define LCD_BLINKING_OFF        0x00
#define LCD_BLINKING_ON         0x01

void lcd_init();

void sendByte(uint8_t data);

void lcd_string( const char *data );

#endif
#endif /* LCD_DRIVER_H_ */
