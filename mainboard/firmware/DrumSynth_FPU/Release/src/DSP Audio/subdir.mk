################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/DSP\ Audio/DrumVoice.c \
../src/DSP\ Audio/Oscillator.c \
../src/DSP\ Audio/ResonantFilter.c \
../src/DSP\ Audio/wavetable.c 

OBJS += \
./src/DSP\ Audio/DrumVoice.o \
./src/DSP\ Audio/Oscillator.o \
./src/DSP\ Audio/ResonantFilter.o \
./src/DSP\ Audio/wavetable.o 

C_DEPS += \
./src/DSP\ Audio/DrumVoice.d \
./src/DSP\ Audio/Oscillator.d \
./src/DSP\ Audio/ResonantFilter.d \
./src/DSP\ Audio/wavetable.d 


# Each subdirectory must supply rules for building sources it contributes
src/DSP\ Audio/DrumVoice.o: ../src/DSP\ Audio/DrumVoice.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Windows GCC C Compiler'
	arm-none-eabi-gcc -DSTM32F4XX -DHSE_VALUE=8000000 -DUSE_STDPERIPH_DRIVER -DUSE_STM32F4_DISCOVERY -I"C:\stm32DevTools\ArmGcc\lib\gcc\arm-none-eabi\4.6.2\include" -I"C:\stm32DevTools\ArmGcc\lib\gcc\arm-none-eabi\4.6.2\include-fixed" -I"C:\stm32DevTools\ArmGcc\arm-none-eabi\include" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\CMSIS\Include" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\Device\STM32F4xx\Include" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\STM32F4-Discovery" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\STM32F4xx_StdPeriph_Driver\inc" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\src" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\src\AudioCodecManager" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\src\DSP Audio" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\src\MIDI" -Os -Wall -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -MMD -MP -MF"src/DSP Audio/DrumVoice.d" -MT"src/DSP\ Audio/DrumVoice.d" -mcpu=cortex-m4 -mthumb -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/DSP\ Audio/Oscillator.o: ../src/DSP\ Audio/Oscillator.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Windows GCC C Compiler'
	arm-none-eabi-gcc -DSTM32F4XX -DHSE_VALUE=8000000 -DUSE_STDPERIPH_DRIVER -DUSE_STM32F4_DISCOVERY -I"C:\stm32DevTools\ArmGcc\lib\gcc\arm-none-eabi\4.6.2\include" -I"C:\stm32DevTools\ArmGcc\lib\gcc\arm-none-eabi\4.6.2\include-fixed" -I"C:\stm32DevTools\ArmGcc\arm-none-eabi\include" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\CMSIS\Include" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\Device\STM32F4xx\Include" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\STM32F4-Discovery" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\STM32F4xx_StdPeriph_Driver\inc" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\src" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\src\AudioCodecManager" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\src\DSP Audio" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\src\MIDI" -Os -Wall -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -MMD -MP -MF"src/DSP Audio/Oscillator.d" -MT"src/DSP\ Audio/Oscillator.d" -mcpu=cortex-m4 -mthumb -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/DSP\ Audio/ResonantFilter.o: ../src/DSP\ Audio/ResonantFilter.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Windows GCC C Compiler'
	arm-none-eabi-gcc -DSTM32F4XX -DHSE_VALUE=8000000 -DUSE_STDPERIPH_DRIVER -DUSE_STM32F4_DISCOVERY -I"C:\stm32DevTools\ArmGcc\lib\gcc\arm-none-eabi\4.6.2\include" -I"C:\stm32DevTools\ArmGcc\lib\gcc\arm-none-eabi\4.6.2\include-fixed" -I"C:\stm32DevTools\ArmGcc\arm-none-eabi\include" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\CMSIS\Include" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\Device\STM32F4xx\Include" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\STM32F4-Discovery" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\STM32F4xx_StdPeriph_Driver\inc" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\src" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\src\AudioCodecManager" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\src\DSP Audio" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\src\MIDI" -Os -Wall -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -MMD -MP -MF"src/DSP Audio/ResonantFilter.d" -MT"src/DSP\ Audio/ResonantFilter.d" -mcpu=cortex-m4 -mthumb -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/DSP\ Audio/wavetable.o: ../src/DSP\ Audio/wavetable.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Windows GCC C Compiler'
	arm-none-eabi-gcc -DSTM32F4XX -DHSE_VALUE=8000000 -DUSE_STDPERIPH_DRIVER -DUSE_STM32F4_DISCOVERY -I"C:\stm32DevTools\ArmGcc\lib\gcc\arm-none-eabi\4.6.2\include" -I"C:\stm32DevTools\ArmGcc\lib\gcc\arm-none-eabi\4.6.2\include-fixed" -I"C:\stm32DevTools\ArmGcc\arm-none-eabi\include" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\CMSIS\Include" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\Device\STM32F4xx\Include" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\STM32F4-Discovery" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\STM32F4xx_StdPeriph_Driver\inc" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\src" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\src\AudioCodecManager" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\src\DSP Audio" -I"E:\Eclipse Drum Synth STM32\DrumSynth_Workspace\DrumSynth_FPU\src\MIDI" -Os -Wall -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -MMD -MP -MF"src/DSP Audio/wavetable.d" -MT"src/DSP\ Audio/wavetable.d" -mcpu=cortex-m4 -mthumb -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


