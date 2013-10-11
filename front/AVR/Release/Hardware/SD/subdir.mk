################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Hardware/SD/SD_routines.c \
../Hardware/SD/SPI_routines.c \
../Hardware/SD/diskIo.c \
../Hardware/SD/ff.c 

OBJS += \
./Hardware/SD/SD_routines.o \
./Hardware/SD/SPI_routines.o \
./Hardware/SD/diskIo.o \
./Hardware/SD/ff.o 

C_DEPS += \
./Hardware/SD/SD_routines.d \
./Hardware/SD/SPI_routines.d \
./Hardware/SD/diskIo.d \
./Hardware/SD/ff.d 


# Each subdirectory must supply rules for building sources it contributes
Hardware/SD/%.o: ../Hardware/SD/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	avr-gcc -DF_CPU=20000000UL -D__PROG_TYPES_COMPAT__ -Os -Wall -Wextra -c -funsigned-char -funsigned-bitfields -ffast-math -freciprocal-math -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -Wall -mmcu=atmega644 -std=gnu99 -MD -MP -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


