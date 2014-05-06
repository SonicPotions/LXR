################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL := cmd.exe
RM := rm -rf

USER_OBJS :=

LIBS := 
PROJ := 

O_SRCS := 
C_SRCS := 
S_SRCS := 
S_UPPER_SRCS := 
OBJ_SRCS := 
ASM_SRCS := 
PREPROCESSING_SRCS := 
OBJS := 
OBJS_AS_ARGS := 
C_DEPS := 
C_DEPS_AS_ARGS := 
EXECUTABLES := 
OUTPUT_FILE_PATH :=
OUTPUT_FILE_PATH_AS_ARGS :=
AVR_APP_PATH :=$$$AVR_APP_PATH$$$
QUOTE := "
ADDITIONAL_DEPENDENCIES:=
OUTPUT_FILE_DEP:=

# Every subdirectory with source files must be described here
SUBDIRS :=  \
../lcd \
../Bootloader \
../elmChan


# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS +=  \
../dout.c \
../elmChan/diskIo.c \
../elmChan/ff.c \
../FileParser.c \
../lcd/lcd.c \
../SD_main.c \
../SD_routines.c \
../SPI_routines.c \
../UART_routines.c \
../userInterface.c


PREPROCESSING_SRCS += 


ASM_SRCS += 


OBJS +=  \
dout.o \
elmChan/diskIo.o \
elmChan/ff.o \
FileParser.o \
lcd/lcd.o \
SD_main.o \
SD_routines.o \
SPI_routines.o \
UART_routines.o \
userInterface.o


OBJS_AS_ARGS +=  \
dout.o \
elmChan/diskIo.o \
elmChan/ff.o \
FileParser.o \
lcd/lcd.o \
SD_main.o \
SD_routines.o \
SPI_routines.o \
UART_routines.o \
userInterface.o


C_DEPS +=  \
dout.d \
elmChan/diskIo.d \
elmChan/ff.d \
FileParser.d \
lcd/lcd.d \
SD_main.d \
SD_routines.d \
SPI_routines.d \
UART_routines.d \
userInterface.d


C_DEPS_AS_ARGS +=  \
dout.d \
elmChan/diskIo.d \
elmChan/ff.d \
FileParser.d \
lcd/lcd.d \
SD_main.d \
SD_routines.d \
SPI_routines.d \
UART_routines.d \
userInterface.d


OUTPUT_FILE_PATH +=SD_Card.elf

OUTPUT_FILE_PATH_AS_ARGS +=SD_Card.elf

ADDITIONAL_DEPENDENCIES:=

OUTPUT_FILE_DEP:= ./makedep.mk

# AVR32/GNU C Compiler





















./%.o: .././%.c
	@echo Building file: $<
	@echo Invoking: AVR/GNU C Compiler
	$(QUOTE)C:\Program Files (x86)\Atmel\AVR Studio 5.1\extensions\Atmel\AVRGCC\3.3.1.27\AVRToolchain\bin\avr-gcc.exe$(QUOTE)  -funsigned-char -funsigned-bitfields -DF_CPU=20000000UL  -Os -mtiny-stack -fno-unit-at-a-time -fno-inline-small-functions  -mcall-prologues -fno-jump-tables -fno-move-loop-invariants -ffunction-sections -fpack-struct -fshort-enums -Wall -pedantic -c -gdwarf-2 -std=gnu99  -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"  -mmcu=atmega644  -o"$@" "$<" 
	@echo Finished building: $<

elmChan/%.o: ../elmChan/%.c
	@echo Building file: $<
	@echo Invoking: AVR/GNU C Compiler
	$(QUOTE)C:\Program Files (x86)\Atmel\AVR Studio 5.1\extensions\Atmel\AVRGCC\3.3.1.27\AVRToolchain\bin\avr-gcc.exe$(QUOTE)  -funsigned-char -funsigned-bitfields -DF_CPU=20000000UL  -Os -mtiny-stack -fno-unit-at-a-time -fno-inline-small-functions  -mcall-prologues -fno-jump-tables -fno-move-loop-invariants -ffunction-sections -fpack-struct -fshort-enums -Wall -pedantic -c -gdwarf-2 -std=gnu99  -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"  -mmcu=atmega644  -o"$@" "$<" 
	@echo Finished building: $<

lcd/%.o: ../lcd/%.c
	@echo Building file: $<
	@echo Invoking: AVR/GNU C Compiler
	$(QUOTE)C:\Program Files (x86)\Atmel\AVR Studio 5.1\extensions\Atmel\AVRGCC\3.3.1.27\AVRToolchain\bin\avr-gcc.exe$(QUOTE)  -funsigned-char -funsigned-bitfields -DF_CPU=20000000UL  -Os -mtiny-stack -fno-unit-at-a-time -fno-inline-small-functions  -mcall-prologues -fno-jump-tables -fno-move-loop-invariants -ffunction-sections -fpack-struct -fshort-enums -Wall -pedantic -c -gdwarf-2 -std=gnu99  -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"  -mmcu=atmega644  -o"$@" "$<" 
	@echo Finished building: $<



# AVR32/GNU Preprocessing Assembler



# AVR32/GNU Assembler




ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: $(OUTPUT_FILE_PATH) $(ADDITIONAL_DEPENDENCIES)

$(OUTPUT_FILE_PATH): $(OBJS) $(USER_OBJS) $(OUTPUT_FILE_DEP)
	@echo Building target: $@
	@echo Invoking: AVR/GNU C Linker
	$(QUOTE)C:\Program Files (x86)\Atmel\AVR Studio 5.1\extensions\Atmel\AVRGCC\3.3.1.27\AVRToolchain\bin\avr-gcc.exe$(QUOTE) -o$(OUTPUT_FILE_PATH_AS_ARGS) $(OBJS_AS_ARGS) $(USER_OBJS) $(LIBS) -Wl,-Map="SD_Card.map" -Wl,--gc-sections -mrelax -Ttext=0xE000   -mmcu=atmega644  
	@echo Finished building target: $@
	"C:\Program Files (x86)\Atmel\AVR Studio 5.1\extensions\Atmel\AVRGCC\3.3.1.27\AVRToolchain\bin\avr-objcopy.exe" -O ihex -R .eeprom -R .fuse -R .lock -R .signature  "SD_Card.elf" "SD_Card.hex"
	"C:\Program Files (x86)\Atmel\AVR Studio 5.1\extensions\Atmel\AVRGCC\3.3.1.27\AVRToolchain\bin\avr-objcopy.exe" -j .eeprom  --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0  --no-change-warnings -O ihex "SD_Card.elf" "SD_Card.eep" || exit 0
	"C:\Program Files (x86)\Atmel\AVR Studio 5.1\extensions\Atmel\AVRGCC\3.3.1.27\AVRToolchain\bin\avr-objdump.exe" -h -S "SD_Card.elf" > "SD_Card.lss"
	"C:\Program Files (x86)\Atmel\AVR Studio 5.1\extensions\Atmel\AVRGCC\3.3.1.27\AVRToolchain\bin\avr-size.exe" -C --mcu=atmega644  "SD_Card.elf"
	




# Other Targets
clean:
	-$(RM) $(OBJS_AS_ARGS)$(C_DEPS_AS_ARGS) $(EXECUTABLES) 
	rm -rf "SD_Card.hex" "SD_Card.lss" "SD_Card.eep" "SD_Card.map"
	