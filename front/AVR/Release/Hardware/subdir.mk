################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Hardware/lcd.c \
../Hardware/timebase.c 

OBJS += \
./Hardware/lcd.o \
./Hardware/timebase.o 

C_DEPS += \
./Hardware/lcd.d \
./Hardware/timebase.d 


# Each subdirectory must supply rules for building sources it contributes
Hardware/%.o: ../Hardware/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	avr-gcc -DF_CPU=20000000UL -D__PROG_TYPES_COMPAT__ -Os -Wall -Wextra -c -funsigned-char -funsigned-bitfields -ffast-math -freciprocal-math -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -Wall -mmcu=atmega644 -std=gnu99 -MD -MP -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


