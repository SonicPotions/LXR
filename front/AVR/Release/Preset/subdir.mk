################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Preset/presetManager.c 

OBJS += \
./Preset/presetManager.o 

C_DEPS += \
./Preset/presetManager.d 


# Each subdirectory must supply rules for building sources it contributes
Preset/%.o: ../Preset/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	avr-gcc -DF_CPU=20000000UL -D__PROG_TYPES_COMPAT__ -Os -Wall -Wextra -c -funsigned-char -funsigned-bitfields -ffast-math -freciprocal-math -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -Wall -mmcu=atmega644 -std=gnu99 -MD -MP -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


