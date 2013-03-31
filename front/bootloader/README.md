Drumsynth Frontpanel Bootloader
===============================

SD-Card bootloader for Atmega644.
Copyright Julian Schmidt 2013

A firmware image is loaded from the SD-Card.
First the AVR is programmed, then the attached STM32 on the mainboard.
Firmware images need to be build with the supplied firmware image builder app.

The sourcecode is provided as AVR Studio 5.0 project.

The SD, FAT32 and SPI routines are based on a demo project from 
CC Dharmani, Chennai (India)
www.dharmanitech.com
http://www.dharmanitech.com/2009/01/sd-card-interfacing-with-atmega8-fat32.html
thanks a lot!

