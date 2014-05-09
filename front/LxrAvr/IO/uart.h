/*
 * uart.h
 *
 * Created: 26.04.2012 14:27:22
 *  Author: Julian
 */ 


#ifndef UART_H_
#define UART_H_


#include <avr/io.h>
#include <avr/interrupt.h> 

#include "../fifo.h"



//-------------------------------------------
#define ACK 1
#define NACK -1
//-------------------------------------------
#define ACK 1
#define NACK -1
//-------------------------------------------


void uart_init();
uint8_t uart_putc(unsigned char c);
void uart_puts (char *s);
uint8_t uart_getc(uint8_t *data);

//waits for a ACK message on the uart
uint8_t uart_waitAck();
//check if there is some data received an parse it
void uart_checkAndParse();


#endif /* UART_H_ */
