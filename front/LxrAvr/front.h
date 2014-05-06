/*
 * front.h
 *
 *  Created on: Nov 4, 2013
 *      Author: anshak
 */

#ifndef FRONT_H_
#define FRONT_H_

//js does not work with latest GCC
#define DISABLE_CONV_WARNING _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wconversion\"")
#define DISABLE_SIGN_WARNING _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"")
#define END_DISABLE_WARNING _Pragma("GCC diagnostic pop")
#define END_DISABLE_CONV_WARNING END_DISABLE_WARNING


#endif /* FRONT_H_ */
