#ifndef GLOBAL_VARS_FILE
#define GLOBAL_VARS_FILE

#include "stm32f4xx.h"

// global vars
extern volatile uint32_t systick_ticks;
extern int16_t audioOutBuffer[2];
extern   uint8_t bCurrentSampleValid;
#define SAMPLE_VALID  0xff
#define FILTER_SHAPER -0.9f

#endif
