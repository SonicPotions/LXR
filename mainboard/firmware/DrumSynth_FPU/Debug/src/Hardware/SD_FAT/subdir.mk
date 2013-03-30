################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Hardware/SD_FAT/SD_Manager.c \
../src/Hardware/SD_FAT/SD_routines.c \
../src/Hardware/SD_FAT/SPI_routines.c \
../src/Hardware/SD_FAT/diskIo.c \
../src/Hardware/SD_FAT/ff.c 

OBJS += \
./src/Hardware/SD_FAT/SD_Manager.o \
./src/Hardware/SD_FAT/SD_routines.o \
./src/Hardware/SD_FAT/SPI_routines.o \
./src/Hardware/SD_FAT/diskIo.o \
./src/Hardware/SD_FAT/ff.o 

C_DEPS += \
./src/Hardware/SD_FAT/SD_Manager.d \
./src/Hardware/SD_FAT/SD_routines.d \
./src/Hardware/SD_FAT/SPI_routines.d \
./src/Hardware/SD_FAT/diskIo.d \
./src/Hardware/SD_FAT/ff.d 


# Each subdirectory must supply rules for building sources it contributes
src/Hardware/SD_FAT/%.o: ../src/Hardware/SD_FAT/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Windows GCC C Compiler'
	arm-none-eabi-gcc -DSTM32F4XX -DUSE_DEVICE_MODE -DUSE_USB_OTG_FS -DNDEBUG -DHSE_VALUE=8000000 -DUSE_STDPERIPH_DRIVER -DUSE_STM32F4_DISCOVERY -I"C:\stm32DevTools\ArmGcc\lib\gcc\arm-none-eabi\4.6.2\include" -I"C:\stm32DevTools\ArmGcc\lib\gcc\arm-none-eabi\4.6.2\include-fixed" -I"C:\stm32DevTools\ArmGcc\arm-none-eabi\include" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\CMSIS\Include" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\Device\STM32F4xx\Include" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\STM32F4-Discovery" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\STM32F4xx_StdPeriph_Driver\inc" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\src" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\src\AudioCodecManager" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\src\MIDI" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\src\DSPAudio" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\src\Sequencer" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\src\Hardware" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\src\Hardware\SD_FAT" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\src\Hardware\USB" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\STM32_USB_OTG_Driver" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\STM32_USB_OTG_Driver\inc" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\STM32_USB_Device_Library" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\STM32_USB_Device_Library\Core\inc" -O3 -ffunction-sections -fdata-sections -ffast-math -freciprocal-math -fsingle-precision-constant -Wall -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m4 -mthumb -mthumb-interwork -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


