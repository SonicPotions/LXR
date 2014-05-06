/*
 * screensaver.h
 *
 * Created: 25.01.2013 17:07:52
 *  Author: Julian
 */ 


#ifndef SCREENSAVER_H_
#define SCREENSAVER_H_


void screensaver_touch();
void screensaver_check();

extern volatile uint16_t screensaver_timer;

#endif /* SCREENSAVER_H_ */