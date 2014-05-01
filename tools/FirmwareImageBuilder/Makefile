###############################################################################
#
# Makefile for LXR FirmwareImageBuilder tool
# Author: Patrick Dowling
#
# Uses default CXX to compile
#
###############################################################################

###############################################################################
# OPTIONS
EXE ?= ../bin/FirmwareImageBuilder

# if VERBOSE is defined, spam output
ifdef VERBOSE
AT :=
ECHO := @true
else
AT := @
ECHO := @echo
endif

###############################################################################
# SOURCE FILES
SRCDIR=./FirmwareImageBuilder

###############################################################################
# SETUP

.PHONY: all
all:
	@echo "Valid targets are"
	@echo " clean : clean build directory"
	@echo " exe   : build FirmwareImageBuilder tool"

.PHONY: exe
exe: $(EXE)

$(EXE): $(SRCDIR)/FirmwareImageBuilder.cpp
	$(ECHO) "Compiling $@..."
	$(AT)$(CXX) $(CFLAGS) $(SRCDIR)/FirmwareImageBuilder.cpp -o $@

.PHONY: clean
clean:
	$(AT)$(RM) $(EXE)
