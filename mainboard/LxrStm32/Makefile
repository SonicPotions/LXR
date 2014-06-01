###############################################################################
#
# Makefile for LXR Cortex firmware
# Author: Patrick Dowling
#
# Requires either
# * arm compiler (arm-none-eabi-gcc et al) in path, or
# * ARM_TOOLKIT_ROOT set to installation directory
#
###############################################################################

###############################################################################
# OPTIONS

BINARY?=LxrStm32.bin

ifeq ($(DEBUG),1)
DEFINES += -DDEBUG
else
DEFINES += -DNDEBUG
endif

ifndef ARM_OPTIMIZE
ARM_OPTIMIZE=3
#ARM_OPTIMIZE=fast
endif

# if VERBOSE is defined, spam output
ifdef VERBOSE
AT :=
ECHO := @true
LDFLAGS += -v
else
AT := @
ECHO := @echo
endif

ifdef ARM_TOOLKIT_ROOT
BINPATH=$(ARM_TOOLKIT_ROOT)/bin/
else
BINPATH=""
endif

CC  =$(addprefix $(BINPATH),arm-none-eabi-gcc)
CXX =$(addprefix $(BINPATH),arm-none-eabi-c++)
LD  =$(addprefix $(BINPATH),arm-none-eabi-ld)
CP  =$(addprefix $(BINPATH),arm-none-eabi-objcopy)
OD  =$(addprefix $(BINPATH),arm-none-eabi-objdump)
AS  =$(addprefix $(BINPATH),arm-none-eabi-as)

###############################################################################
# SOURCE FILES
SRCDIR=.
CCSRCFILES  = $(shell find $(SRCDIR) -type f -name "*.c" | grep -v '/\.')
CXXSRCFILES = $(shell find $(SRCDIR) -type f -name "*.cpp" | grep -v '/\.')
ASSRCFILES  = $(shell find $(SRCDIR) -type f -name "*.S" | grep -v '/\.')

vpath %.c ./Libraries/STM32_USB_Device_Library/Core/src
vpath %.c ./Libraries/STM32_USB_OTG_Driver/src
vpath %.c ./Libraries/STM32F4xx_StdPeriph_Driver/src
vpath %.c ./src
vpath %.S ./src
vpath %.c ./src/AudioCodecManager
vpath %.c ./src/DSPAudio
vpath %.c ./src/Hardware
vpath %.c ./src/Hardware/SD_FAT
vpath %.c ./src/Hardware/USB
vpath %.c ./src/MIDI
vpath %.c ./src/SampleRom
vpath %.c ./src/Sequencer 

###############################################################################
# SETUP

OBJDIR=./build/

ELF=$(OBJDIR)/LxrStm32.elf

# Build object files from source...
OBJFILES = $(addprefix $(OBJDIR),$(notdir $(CCSRCFILES:.c=.o)))
OBJFILES+= $(addprefix $(OBJDIR),$(notdir $(CXXSRCFILES:.cpp=.o)))
OBJFILES+= $(addprefix $(OBJDIR),$(notdir $(ASSRCFILES:.S=.o)))

# Project defines
DEFINES += -DHSE_VALUE=8000000 -DSTM32F4XX -DUSE_STDPERIPH_DRIVER -DUSE_STM32F4_DISCOVERY

# Include directories
INCLUDES += -I"./Libraries/CMSIS/Include"
INCLUDES += -I"./Libraries/Device/STM32F4xx/Include"
INCLUDES += -I"./Libraries/STM32F4xx_StdPeriph_Driver/inc"
INCLUDES += -I"./Libraries/STM32_USB_Device_Library/Core/inc"
INCLUDES += -I"./Libraries/STM32_USB_OTG_Driver/inc"
INCLUDES += -I"./src"
INCLUDES += -I"./src/AudioCodecManager"
INCLUDES += -I"./src/DSPAudio"
INCLUDES += -I"./src/Hardware"
INCLUDES += -I"./src/Hardware/SD_FAT"
INCLUDES += -I"./src/Hardware/USB"
INCLUDES += -I"./src/MIDI"
INCLUDES += -I"./src/Sequencer" 

ARCHFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

CFLAGS += $(DEFINES) $(INCLUDES)
CFLAGS += $(ARCHFLAGS)
CFLAGS += -O$(ARM_OPTIMIZE) -ffunction-sections -fdata-sections -ffast-math -freciprocal-math -fsingle-precision-constant -fmessage-length=0
CFLAGS += -Wall -Wextra -c 
CFLAGS += -Wl,--gc-sections
ifeq ($(PEDANTIC),1)
CFLAGS += -Werror
endif
CXXFLAGS += $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS += -ahls $(ARCHFLAGS)
LDFLAGS += $(ARCHFLAGS) -Wl,--gc-sections --specs=nano.specs
LDFLAGS += -T./stm32_flash.ld

###############################################################################
# TARGETS

.PHONY: all
all:
	@echo "Valid targets are"
	@echo " clean : clean build directory"
	@echo " stm32: build LXR STM32 firmware"


.PHONY: clean
clean:
	@$(RM) $(BINARY)
	@$(RM) $(OBJDIR)*.o

.PHONY: printenv
printenv:
	@echo "AVR_TOOLKIT_ROOT='$(AVR_TOOLKIT_ROOT)'"
	@echo "CC  = $(CC)"
	@echo "CXX = $(CXX)"
	@echo "AS  = $(AS)"

.PHONY: printfiles
printfiles:
	@echo "** C FILES **"
	@echo "$(CCSRCFILES)"
	@echo ""

	@echo "** CXX FILES **"
	@echo "$(CXXSRCFILES)"
	@echo ""

	@echo "** S FILES **"
	@echo "$(ASSRCFILES)"
	@echo ""

.PHONY: stm32
stm32: $(BINARY)

$(ELF): $(OBJFILES)
	$(ECHO) "Linking $@..."
	$(AT)$(CXX) $(LDFLAGS) $^ -o $@

$(BINARY): $(ELF)
	$(ECHO) "Creating binary $@..."
	$(AT)$(CP) -O binary $(ELF) $(BINARY)

$(OBJFILES) : | $(OBJDIR)

###############################################################################
# BUILD RULES

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(OBJDIR)%.o: %.c
	$(ECHO) "Compiling $<..."
	$(AT)$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)%.o: %.cxx
	$(ECHO) "Compiling $<..."
	$(AT)$(CXX) $(CFLAGS) $< -o $@

$(OBJDIR)%.o: %.S
	$(ECHO) "Assembling $<..."
	$(AT)$(AS) $(ASFLAGS) $< -o $@ > $@.lst

# Automatic dependency generation
CFLAGS += -MMD
-include $(OBJFILES:.o=.d)
