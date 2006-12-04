#
# Handheld Reverse Engineering Tool
#
# This program is free software distributed under the terms of the
# license that can be found in the COPYING file.
#
# This Makefile requires GNU Make.
#

# Program version
VERSION=0.4.4

# Output directory
OUT=out/

# Default compiler flags
CXXFLAGS = -MD -Wall -MD -O -march=armv4 -g -Iinclude -DVERSION=\"$(VERSION)\"
LDFLAGS = -Wl,--major-subsystem-version=2,--minor-subsystem-version=1
# LDFLAGS to debug invalid imports in exe
#LDFLAGS = -Wl,-M -Wl,--cref

LIBS = -lwinsock

vpath %.cpp src src/wince src/mach
vpath %.S src src/wince
vpath %.rc src/wince

.PHONY : all FORCE

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
	@echo "  Compiling $<"
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT)%.o: %.S
	@echo "  Assembling $<"
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT)%.o: %.rc
	@echo "  Building resource file from $<"
	$(Q)$(RC) $(RCFLAGS) -i $< -o $@

$(OUT)%.lib: src/wince/%.def
	@echo "  Building library $@"
	$(Q)$(DLLTOOL) $(DLLTOOLFLAGS) -d $< -l $@

$(OUT)%.exe: out/%-debug
	@echo "  Stripping $^ to make $@"
	$(Q)$(STRIP) $^ -o $@

################ Haret exe rules

MACHOBJS := machines.o arch-pxa.o arch-pxa27x.o arch-sa.o \
  mach-alpine.o mach-apache.o mach-beetles.o mach-blueangel.o \
  mach-himalya.o mach-magician.o mach-universal.o mach-h4000.o \
  mach-h4700.o mach-sable.o mach-jornada820.o

COREOBJS := $(MACHOBJS) haret-res.o \
  memory.o video.o asmstuff.o lateload.o output.o cpu.o linboot.o

HARETOBJS := $(COREOBJS) haret.o \
  s-cpu.o gpio.o uart.o wincmds.o irqchain.o getsetcp.o watch.o irq.o \
  script.o network.o terminal.o com_port.o tlhcmds.o

$(OUT)haret-debug: $(addprefix $(OUT),$(HARETOBJS)) src/haret.lds
	@echo "  Linking $@"
	$(Q)$(CXX) $(LDFLAGS) $^ $(LIBS) -o $@

####### Stripped down linux bootloading program.
LINLOADOBJS := $(COREOBJS) stubboot.o kernelfiles.o

INITRD := /dev/null
KERNEL := zImage
CMDLINE :=

$(OUT)kernelfiles.o: src/wince/kernelfiles.S FORCE
	@echo "  Building $@"
	$(Q)$(CXX) -c -DLIN_INITRD=\"$(INITRD)\" -DLIN_KERNEL=\"$(KERNEL)\" -DLIN_CMD='"$(CMDLINE)"' -o $@ $<

$(OUT)linload-debug: $(addprefix $(OUT), $(LINLOADOBJS)) src/haret.lds
	@echo "  Linking $@"
	$(Q)$(CXX) $(LDFLAGS) $^ $(LIBS) -o $@

linload: $(OUT)linload.exe

####### Generic rules
clean:
	rm -rf $(OUT)

$(OUT):
	mkdir $@

-include $(OUT)*.d
