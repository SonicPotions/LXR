/*
 * wavetable.h
 *
 *  Created on: 01.04.2012
 *      Author: Julian
 */

#ifndef WAVETABLES_H_
#define WAVETABLES_H_

#include "stm32f4xx.h"
#include <stdio.h>

#define TABLESIZE 4096
#define TABLES 128

//#define FRAC_MAX (int16_t)32767
//#define FRAC_MIN (int16_t)-32768

//extern const int16_t sine_table[512];

extern const int16_t sine_table[TABLESIZE+1];

/*the midi note numbers for the 11 wavetables are
0 - 0	- 8.1758Hz
1 - 12	- 16.3516Hz
2 - 24	- 32.7032
3 - 36	- 65.4064
4 - 48	- 130.813
5 - 60	- 261.626
6 - 72	- 523.251
7 - 84	- 1046.5
8 - 96	- 2093
9 - 108	- 4186.01
10 - 120- 8372.02
 */

extern const int16_t recTable[11][1024];
extern const int16_t triTable[11][1024];
extern const int16_t sawTable[11][1024];





/** returns a sine wavetable taking index from 0 to TABLESIZE*/
//int16_t lutSineWavetable(int index);

/** a signed integer sine for math calculations - input [FRAC_MIN to FRAC_MAX]*/

//int16_t lutSineMath(uint16_t index);

#endif /* WAVETABLES_H_ */
