#
# Handheld Reverse Engineering Tool
#
# This program is free software distributed under the terms of the
# license that can be found in the COPYING file.
#
# This Makefile requires GNU Make.
#

# Program version
VERSION=0.3.8-pre

# Output directory
OUT=out/

# Default compiler flags
CXXFLAGS = -MD -Wall -MD -O -march=armv5te -g -Iinclude -DVERSION=\"$(VERSION)\"
LDFLAGS =

LIBS = -lwinsock

vpath %.cpp src src/wince
vpath %.S src src/wince
vpath %.rc src/wince

all: $(OUT) $(OUT)haret.exe

################ cegcc settings

BASE ?= /opt/mingw32ce

RC = $(BASE)/bin/arm-wince-mingw32ce-windres
RCFLAGS = -r -l 0x409 -Iinclude

CXX = $(BASE)/bin/arm-wince-mingw32ce-g++
STRIP = $(BASE)/bin/arm-wince-mingw32ce-strip

DLLTOOL = $(BASE)/bin/arm-wince-mingw32ce-dlltool
DLLTOOLFLAGS =

# Run with "make V=1" to see the actual compile commands
ifdef V
Q=
else
Q=@
endif

$(OUT)%.o: %.cpp
	@echo "Compiling $<"
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT)%.o: %.S
	@echo "Assembling $<"
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT)%.o: %.rc
	@echo "Building resource file from $<"
	$(Q)$(RC) $(RCFLAGS) -i $< -o $@

$(OUT)%.lib: src/wince/%.def
	@echo "Building library $@"
	$(Q)$(DLLTOOL) $(DLLTOOLFLAGS) -d $< -l $@

################ Additional rules

HARETOBJS := haret.o haret-res.o \
  s-cpu.o s-util.o memory.o gpio.o uart.o video.o \
  asmstuff.o irqchain.o getsetcp.o irq.o cpu-pxa.o cpu-s3c24xx.o \
  util.o output.o script.o network.o cpu.o terminal.o linboot.o \
  com_port.o \
  toolhelp.lib

$(OUT)haret-debug: $(addprefix $(OUT),$(HARETOBJS))
	@echo "Linking $@"
	$(Q)$(CXX) $(LDFLAGS) $^ $(LIBS) -o $@

$(OUT)haret.exe: $(OUT)haret-debug
	@echo "Stripping $^ to make $@"
	$(Q)$(STRIP) $^ -o $@

clean:
	rm -rf $(OUT)

$(OUT):
	mkdir $@

-include $(OUT)*.d
