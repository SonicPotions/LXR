# Makefile for LXR ARM firmware
#

###############################################################################
# OPTIONS
ifeq ($(DEBUG),1)
DEFINES += -DDEBUG
else
DEFINES += -DNDEBUG
endif

ifndef OPTIMIZE
OPTIMIZE=3
endif

# if VERBOSE is defined, spam output
ifdef VERBOSE
AT :=
ECHO := @true
else
AT := @
ECHO := @echo
endif

CC  = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD  = arm-none-eabi-ld -v
CP  = arm-none-eabi-objcopy
OD  = arm-none-eabi-objdump
AS  = arm-none-eabi-as

###############################################################################
# SOURCE FILES
SRCDIR=.

CCSRCFILES  = $(shell find . -type f -name *.c)
CXXSRCFILES = $(shell find . -type f -name *.cpp)
ASSRCFILES  = $(shell find . -type f -name *.S)

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

FIRMWARE=DrumSynth_FPU

# Build object files from source...
OBJDIR=./build/
OBJFILES = $(addprefix $(OBJDIR),$(notdir $(CCSRCFILES:.c=.o)))
OBJFILES+= $(addprefix $(OBJDIR),$(notdir $(CXXSRCFILES:.cpp=.o)))
OBJFILES+= $(addprefix $(OBJDIR),$(notdir $(ASSRCFILES:.S=.o)))

# Project defines
DEFINES += -DHSE_VALUE=8000000 -DSTM32F4XX -DUSE_DEVICE_MODE -DUSE_USB_OTG_FS -DUSE_STDPERIPH_DRIVER -DUSE_STM32F4_DISCOVERY

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

CFLAGS += $(DEFINES) $(INCLUDES)
CFLAGS += -O$(OPTIMIZE) -ffunction-sections -fdata-sections -ffast-math -freciprocal-math -fsingle-precision-constant -fmessage-length=0 -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS += -Wall -Wextra -c 
CFLAGS += -Wl,--gc-sections
CXXFLAGS += $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS += -ahls -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
LDFLAGS += -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -gc-sections --specs=nano.specs
LDFLAGS += -T./stm32_flash.ld

###############################################################################
# Targets

all:
	@echo "Valid targets are"
	@echo " clean : clean build directory"
	@echo " lxr   : build LXR cortex firmware"


# Clean up temporary files
.PHONY: clean
clean:
	@$(RM) $(FIRMWARE)
	@$(RM) $(OBJDIR)/*.o

.PHONY: printfiles
printfiles:
	@echo "$(CCSRCFILES)"
	@echo "$(CXXSRCFILES)"
	@echo "$(ASSRCFILES)"

.PHONY: lxr
lxr: $(FIRMWARE)

$(FIRMWARE): $(OBJFILES)
	$(ECHO) "Linking $@..."
	$(AT)$(CXX) $(LDFLAGS) $^ -o $@

$(OBJFILES) : | $(OBJDIR)

###############################################################################
# BUILD RULES
$(OBJDIR):
	mkdir -p $(OBJDIR)

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
