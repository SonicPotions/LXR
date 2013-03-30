/*
 * Uart.h
 *
 * handles the uart init and communication
 * usart3 is front panel communication (PD8 TX, PD9 RX)
 *
 *  Created on: 11.07.2012
 *      Author: Julian Schmidt
 */

#ifndef UART_H_
#define UART_H_

#include "stm32f4xx.h"
#include "globals.h"
#include "Fifo.h"



void uart_init();
//wait indefinitely until new data is received
uint8_t uart_rx();
//wait for 'timeout' [ms] if data is received
//return data (uint8_t value) if data is received
//or -1 if a timeout occurs
int16_t uart_rxTimeout(uint16_t timeout);
//sends a byte to the frontpanel
void uart_tx(uint8_t data);
//wait for the front panel to send the INIT_BOOTLOADER message
//returns START_BOOTLOADER if INIT_BOOTLOADER is received
//returns START_PROGRAM if INIT_BOOTLOADER not received after INIT_TIMEOUT ms
uint8_t uart_waitForInitCommand();
#endif

