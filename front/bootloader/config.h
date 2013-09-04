/*
 * config.h
 *
 * Created: 07.07.2012 11:38:44
 *  Author: Julian
 */ 


#ifndef CONFIG_H_
#define CONFIG_H_

#include "./lcd/lcd.h"

#define USE_ELM_CHAN_FS 1

#if USE_ELM_CHAN_FS
	#include "./elmChan/ff.h"
	extern FIL firmwareFile;		/* place to hold 1 file*/
#endif

#define SMALL 1

#define CORTEX_RESET_PIN	PD7
#define CORTEX_RESET_PORT	PORTD
#define CORTEX_RESET_DDR	DDRD

#endif /* CONFIG_H_ */