################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/DSPAudio/1PoleLp.c \
../src/DSPAudio/AD_EG.c \
../src/DSPAudio/Decay.c \
../src/DSPAudio/DrumVoice.c \
../src/DSPAudio/Oscillator.c \
../src/DSPAudio/PeakNotchEq.c \
../src/DSPAudio/ResonantFilter.c \
../src/DSPAudio/Snare.c \
../src/DSPAudio/distortion.c \
../src/DSPAudio/mixer.c \
../src/DSPAudio/random.c \
../src/DSPAudio/wavetable.c 

OBJS += \
./src/DSPAudio/1PoleLp.o \
./src/DSPAudio/AD_EG.o \
./src/DSPAudio/Decay.o \
./src/DSPAudio/DrumVoice.o \
./src/DSPAudio/Oscillator.o \
./src/DSPAudio/PeakNotchEq.o \
./src/DSPAudio/ResonantFilter.o \
./src/DSPAudio/Snare.o \
./src/DSPAudio/distortion.o \
./src/DSPAudio/mixer.o \
./src/DSPAudio/random.o \
./src/DSPAudio/wavetable.o 

C_DEPS += \
./src/DSPAudio/1PoleLp.d \
./src/DSPAudio/AD_EG.d \
./src/DSPAudio/Decay.d \
./src/DSPAudio/DrumVoice.d \
./src/DSPAudio/Oscillator.d \
./src/DSPAudio/PeakNotchEq.d \
./src/DSPAudio/ResonantFilter.d \
./src/DSPAudio/Snare.d \
./src/DSPAudio/distortion.d \
./src/DSPAudio/mixer.d \
./src/DSPAudio/random.d \
./src/DSPAudio/wavetable.d 


# Each subdirectory must supply rules for building sources it contributes
src/DSPAudio/%.o: ../src/DSPAudio/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Windows GCC C Compiler'
	arm-none-eabi-gcc -DSTM32F4XX -DHSE_VALUE=8000000 -DUSE_STDPERIPH_DRIVER -DUSE_STM32F4_DISCOVERY -I"C:\stm32DevTools\ArmGcc\lib\gcc\arm-none-eabi\4.6.2\include" -I"C:\stm32DevTools\ArmGcc\lib\gcc\arm-none-eabi\4.6.2\include-fixed" -I"C:\stm32DevTools\ArmGcc\arm-none-eabi\include" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\CMSIS\Include" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\Device\STM32F4xx\Include" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\STM32F4-Discovery" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\Libraries\STM32F4xx_StdPeriph_Driver\inc" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\src" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\src\AudioCodecManager" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\src\MIDI" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\src\DSPAudio" -I"E:\Eclipse_Drum_Synth_STM32\DrumSynth_Workspace\DrumSynth_FPU\src\Sequencer" -O3 -Wall -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m4 -mthumb -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


