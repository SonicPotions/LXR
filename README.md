Sonic Potions LXR Drumsynth Firmware
====================================
The LXR is a digital drum synthesizer based on the 32-bit Cortex-M4 processor and an Atmega644 8-bit CPU. Developed by Julian Schmidt.

    The 'front' folder contains the AVR code

    The 'mainboard' folder contains the STM32F4 code

    The 'tools' folder contains the firmware image builder tool, to combine AVR and Cortex code into a single file, usable by the bootloader.

Please note that there are libraries from ST and ARM used in the mainboard code of this project. They are all located in the Libraries subfolder of the project. Those come with their own license. The libraries are:

    ARM CMSIS library
    ST STM32_USB_Device_Library
    ST STM32_USB_OTG_Driver
    ST STM32F4xx_StdPeriph_Driver

	

Many Thanks to user Rudeog who contributet a lot of bugfixes and features for version 0.26




Instructions for building:
--------------------------

1.  Install Eclipse Juno CDT (You could install a later version, but this is the version I have working)

2.  Install the Eclipse GNU ARM plugin. Go into the help menu, Install new Software, add a site: http://gnuarmeclipse.sourceforge.net/updates. Then check the box to install that plugin.

3.  Download and install the GCC ARM toolchain https://launchpad.net/gcc-arm-embedded/+download

4.  Download and install gnu make: http://gnuwin32.sourceforge.net/packages/make.htm

5.  Download and install Atmel AVR toolchain from http://www.atmel.com/tools/ATMELAVRTOOLCHAINFORWINDOWS.aspx (you don't need the headers package)

6.  Ensure that the bin directory from 3, 4, and 5 are on the path. I made a batch file that adds these 3 bin directories to my path and launches eclipse.

7.  Fetch the LXR sources from github. You can either install git and do it the git way, or download a zip and unzip it.

8.  In Eclipse, create a workspace in root folder of whole tree that you unzipped (or git'd).

9.  Add two project dirs mainboard\firmware\DrumSynth_FPU and front\AVR to the workspace. To do this, use File/Import/General/Existing projects into workspace. Then select root directory and browse to these dirs. Do this step once for each of these two dirs. You will end up with two projects in your workspace.

10.  These should build. Eclipse is a bit squirrely, so you might need to do a make clean first to create the first makefiles, or rebuild indexes.

11.  I've built the firmwareimagebuilder.exe in the \tools\bin folder. I've also put a batch file that launches it and copies the binaries from the respective output directories to create FIRMWARE.BIN in that same dir. If you don't trust the .EXE I built, you will need to build it from tools\FirmwareImageBuilder\FirmwareImageBuilder. As is you will need visual studio. If you don't have it, you can try to install the free version, mingw, etc and compile the one file FirmwareImageBuilder.cpp (I've fixed it so it should build with any tool) and make your own exe and copy it to that dir.

12.  Thats it, after running the batch file you will have your firmware file. 


