Sonic Potions LXR Drumsynth Firmware
====================================

The LXR is a digital drum synthesizer based on the 32-bit Cortex-M4 processor and an Atmega644 8-bit CPU.


- The 'front' folder contains the AVR code

- The 'mainboard' folder contains the STM32F4 code

- The 'tools' folder contains the firmware image builder tool, to combine AVR 
  and Cortex code into a single file, usable by the bootloader.


Please note that there are libraries from ST and ARM used in the mainboard code of this project.
They are all located in the Libraries subfolder of the project.
Those come with their own license. The libraries are:
- ARM CMSIS library
- ST STM32_USB_Device_Library
- ST STM32_USB_OTG_Driver
- ST STM32F4xx_StdPeriph_Driver