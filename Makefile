#
# Handheld Reverse Engineering Tool
#
# This program is free software distributed under the terms of the
# license that can be found in the COPYING file.
#
# This Makefile requires GNU Make.
#

# Program version
VERSION=pre-0.4.7-$(shell date +"%Y%m%d_%H%M")

# Output directory
OUT=out/

# Default compiler flags (note -march=armv4 is needed for 16 bit insns)
CXXFLAGS = -Wall -O -g -MD -march=armv4 -Iinclude
LDFLAGS = -Wl,--major-subsystem-version=2,--minor-subsystem-version=10
# LDFLAGS to debug invalid imports in exe
#LDFLAGS = -Wl,-M -Wl,--cref

LIBS = -lwinsock

all: $(OUT) $(OUT)haret.exe

# Run with "make V=1" to see the actual compile commands
ifdef V
Q=
else
Q=@
endif

.PHONY : all FORCE

vpath %.cpp src src/wince src/mach
vpath %.S src src/wince
vpath %.rc src/wince

################ cegcc settings

BASE ?= /opt/mingw32ce

RC = $(BASE)/bin/arm-wince-mingw32ce-windres
RCFLAGS = -r -l 0x409 -Iinclude

CXX = $(BASE)/bin/arm-wince-mingw32ce-g++
STRIP = $(BASE)/bin/arm-wince-mingw32ce-strip

DLLTOOL = $(BASE)/bin/arm-wince-mingw32ce-dlltool
DLLTOOLFLAGS =

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

$(OUT)%-debug:
	@echo "  Linking $@"
	$(Q)echo 'const char *VERSION = "$(VERSION)";' > $(OUT)version.cpp
	$(Q)$(CXX) $(LDFLAGS) $(OUT)version.cpp $^ $(LIBS) -o $@

$(OUT)%.exe: $(OUT)%-debug
	@echo "  Stripping $^ to make $@"
	$(Q)$(STRIP) $^ -o $@

################ Haret exe rules

# List of machines supported - note order is important - it determines
# which machines are checked first.
MACHOBJS := machines.o \
  arch-pxa27x.o arch-pxa.o arch-sa.o arch-omap.o arch-s3.o \
  mach-alpine.o \
  mach-apache.o \
  mach-aximx50.o \
  mach-beetles.o \
  mach-blueangel.o \
  mach-himalya.o \
  mach-magician.o \
  mach-universal.o \
  mach-h2200.o \
  mach-h3900.o \
  mach-h4000.o \
  mach-h5000.o \
  mach-hx4700.o \
  mach-sable.o \
  mach-jornada820.o \
  mach-wizard.o \
  mach-hermes.o \
  mach-g500.o \
  mach-artemis.o \
  mach-rx3715.o

COREOBJS := $(MACHOBJS) haret-res.o \
  script.o memory.o video.o asmstuff.o lateload.o output.o cpu.o linboot.o

HARETOBJS := $(COREOBJS) haret.o \
  s-cpu.o gpio.o uart.o wincmds.o irqchain.o getsetcp.o watch.o irq.o \
  network.o terminal.o com_port.o tlhcmds.o pxacmds.o

$(OUT)haret-debug: $(addprefix $(OUT),$(HARETOBJS)) src/haret.lds

####### Stripped down linux bootloading program.
LINLOADOBJS := $(COREOBJS) stubboot.o kernelfiles.o

INITRD := /dev/null
KERNEL := zImage
SCRIPT :=

$(OUT)kernelfiles.o: src/wince/kernelfiles.S FORCE
	@echo "  Building $@"
	$(Q)$(CXX) -c -DLIN_INITRD=\"$(INITRD)\" -DLIN_KERNEL=\"$(KERNEL)\" -DLIN_SCRIPT=\"$(SCRIPT)\" -o $@ $<

$(OUT)linload-debug: $(addprefix $(OUT), $(LINLOADOBJS)) src/haret.lds

linload: $(OUT)linload.exe

####### Generic rules
clean:
	rm -rf $(OUT)

$(OUT):
	mkdir $@

-include $(OUT)*.d
