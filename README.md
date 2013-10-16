Sonic Potions LXR Drumsynth Firmware
====================================

This is a fork of the LXR DrumSynth firmware. My intention for this fork is to add features and fix issues
that are important to me. I will be freely offering any changes back to the SonicPotions repository.

Here are a list of current changes (differences from main repos):
- Midi Off - Will now send midi note off messages before the next midi note on message and also when
  the sequencer is stopped. This allows better interaction with external synths.
- Add Eclipse CDT support for AVR. This allows all development of front and mainboard to be done in one 
  Eclipse workspace, and gets rid of the need for installing AVR studio
- Fix firmware image builder to accept command line parameters, and create new batch file

Please visit https://github.com/SonicPotions/LXR for the original firmware

- The 'front' folder contains the AVR code

- The 'mainboard' folder contains the STM32F4 code

- The 'tools' folder contains the firmware image builder tool, to combine AVR 
  and Cortex code into a single file, usable by the bootloader. This requires visual studio to compile right now.
