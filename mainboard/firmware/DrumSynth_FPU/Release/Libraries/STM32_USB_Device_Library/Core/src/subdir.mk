################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libraries/STM32_USB_Device_Library/Core/src/usbd_core.c \
../Libraries/STM32_USB_Device_Library/Core/src/usbd_ioreq.c \
../Libraries/STM32_USB_Device_Library/Core/src/usbd_req.c 

OBJS += \
./Libraries/STM32_USB_Device_Library/Core/src/usbd_core.o \
./Libraries/STM32_USB_Device_Library/Core/src/usbd_ioreq.o \
./Libraries/STM32_USB_Device_Library/Core/src/usbd_req.o 

C_DEPS += \
./Libraries/STM32_USB_Device_Library/Core/src/usbd_core.d \
./Libraries/STM32_USB_Device_Library/Core/src/usbd_ioreq.d \
./Libraries/STM32_USB_Device_Library/Core/src/usbd_req.d 


# Each subdirectory must supply rules for building sources it contributes
Libraries/STM32_USB_Device_Library/Core/src/%.o: ../Libraries/STM32_USB_Device_Library/Core/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Windows GCC C Compiler (Tools For Embedded)'
	arm-none-eabi-gcc -DHSE_VALUE=8000000 -DSTM32F4XX -DUSE_DEVICE_MODE -DUSE_USB_OTG_FS -DUSE_STDPERIPH_DRIVER -DUSE_STM32F4_DISCOVERY -DNDEBUG -I"C:\codez\LXR\mainboard\firmware\DrumSynth_FPU\Libraries\CMSIS\Include" -I"C:\codez\LXR\mainboard\firmware\DrumSynth_FPU\Libraries\Device\STM32F4xx\Include" -I"C:\codez\LXR\mainboard\firmware\DrumSynth_FPU\Libraries\STM32F4xx_StdPeriph_Driver\inc" -I"C:\codez\LXR\mainboard\firmware\DrumSynth_FPU\Libraries\STM32_USB_Device_Library\Core\inc" -I"C:\codez\LXR\mainboard\firmware\DrumSynth_FPU\Libraries\STM32_USB_OTG_Driver\inc" -I"C:\codez\LXR\mainboard\firmware\DrumSynth_FPU\src" -I"C:\codez\LXR\mainboard\firmware\DrumSynth_FPU\src\AudioCodecManager" -I"C:\codez\LXR\mainboard\firmware\DrumSynth_FPU\src\DSPAudio" -I"C:\codez\LXR\mainboard\firmware\DrumSynth_FPU\src\Hardware" -I"C:\codez\LXR\mainboard\firmware\DrumSynth_FPU\src\Hardware\SD_FAT" -I"C:\codez\LXR\mainboard\firmware\DrumSynth_FPU\src\Hardware\USB" -I"C:\codez\LXR\mainboard\firmware\DrumSynth_FPU\src\MIDI" -I"C:\codez\LXR\mainboard\firmware\DrumSynth_FPU\src\Sequencer" -O3 -ffunction-sections -fdata-sections -ffast-math -freciprocal-math  -fsingle-precision-constant -Wall -Wextra -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


