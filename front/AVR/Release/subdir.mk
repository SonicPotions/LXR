################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../buttonHandler.c \
../encoder.c \
../fifo.c \
../frontPanelParser.c \
../ledHandler.c \
../main.c 

OBJS += \
./buttonHandler.o \
./encoder.o \
./fifo.o \
./frontPanelParser.o \
./ledHandler.o \
./main.o 

C_DEPS += \
./buttonHandler.d \
./encoder.d \
./fifo.d \
./frontPanelParser.d \
./ledHandler.d \
./main.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	avr-gcc -DF_CPU=20000000UL -D__PROG_TYPES_COMPAT__ -Os -Wall -Wextra -c -funsigned-char -funsigned-bitfields -ffast-math -freciprocal-math -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -Wall -mmcu=atmega644 -std=gnu99 -MD -MP -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


