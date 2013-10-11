################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/Sequencer/subdir.mk
-include src/MIDI/subdir.mk
-include src/Hardware/USB/subdir.mk
-include src/Hardware/SD_FAT/subdir.mk
-include src/Hardware/subdir.mk
-include src/DSPAudio/subdir.mk
-include src/AudioCodecManager/subdir.mk
-include src/subdir.mk
-include Libraries/STM32_USB_OTG_Driver/src/subdir.mk
-include Libraries/STM32_USB_Device_Library/Core/src/subdir.mk
-include Libraries/STM32F4xx_StdPeriph_Driver/src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
DrumSynth_FPU.bin \

SECONDARY_LIST += \
DrumSynth_FPU.lst \

SECONDARY_SIZE += \
DrumSynth_FPU.siz \


# All Target
all: DrumSynth_FPU.elf secondary-outputs

# Tool invocations
DrumSynth_FPU.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM Windows GCC C Linker (Tools For Embedded)'
	arm-none-eabi-gcc -T"C:/codez/LXR/mainboard/firmware/DrumSynth_FPU\stm32_flash.ld" -Xlinker --gc-sections -Wl,-Map,"DrumSynth_FPU.map" -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -o "DrumSynth_FPU.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

DrumSynth_FPU.bin: DrumSynth_FPU.elf
	@echo 'Invoking: ARM Windows GNU Create Flash Image (Tools For Embedded)'
	arm-none-eabi-objcopy -O binary "DrumSynth_FPU.elf"  "DrumSynth_FPU.bin"
	@echo 'Finished building: $@'
	@echo ' '

DrumSynth_FPU.lst: DrumSynth_FPU.elf
	@echo 'Invoking: ARM Windows GNU Create Listing (Tools For Embedded)'
	arm-none-eabi-objdump -h -S "DrumSynth_FPU.elf" > "DrumSynth_FPU.lst"
	@echo 'Finished building: $@'
	@echo ' '

DrumSynth_FPU.siz: DrumSynth_FPU.elf
	@echo 'Invoking: ARM Windows GNU Print Size (Tools For Embedded)'
	arm-none-eabi-size  --format=berkeley "DrumSynth_FPU.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(SECONDARY_SIZE)$(OBJS)$(C_DEPS)$(ASM_DEPS)$(SECONDARY_FLASH)$(EXECUTABLES)$(SECONDARY_LIST)$(S_UPPER_DEPS) DrumSynth_FPU.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
