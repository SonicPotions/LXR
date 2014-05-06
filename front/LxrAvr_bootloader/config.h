/*
 * config.h
 *
 * Created: 07.07.2012 11:38:44
 *  Author: Julian
 */ 


#ifndef CONFIG_H_
#define CONFIG_H_

#include "./lcd/lcd.h"

#include "./elmChan/ff.h"
extern FIL firmwareFile;		/* place to hold 1 file*/

#define CORTEX_RESET_PIN	PD7
#define CORTEX_RESET_PORT	PORTD
#define CORTEX_RESET_DDR	DDRD

#endif /* CONFIG_H_ */